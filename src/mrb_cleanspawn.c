/*
** mrb_cleanspawn.c - CleanSpawn class
**
** Copyright (c) Uchio Kondo 2017
**
** See Copyright Notice in LICENSE
*/

#include <mruby.h>
#include <mruby/data.h>
#include <mruby/error.h>
#include <mruby/string.h>

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <pthread.h>

#include "mrb_cleanspawn.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

extern char **environ;

static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
#define DO_LOCK()                                                                                  \
  do {                                                                                             \
    if (pthread_mutex_lock(&mut) != 0)                                                             \
      mrb_sys_fail(mrb, "pthread_mutex_lock");                                                     \
  } while (0)
#define DO_UNLOCK()                                                                                \
  do {                                                                                             \
    if (pthread_mutex_unlock(&mut) != 0)                                                           \
      mrb_sys_fail(mrb, "pthread_mutex_unlock");                                                   \
  } while (0)

static mrb_value mrb_do_cleanspawn(mrb_state *mrb, mrb_value self)
{
  char *program;
  mrb_value *rest, ret;
  mrb_int argc;
  pid_t pid;
  char **argv;
  int i, status;
  struct sigaction sa, sach, intr, quit, chld;
  sigset_t omask;

  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  sach.sa_handler = SIG_DFL;
  sach.sa_flags = 0;
  sigemptyset(&sach.sa_mask);

  mrb_get_args(mrb, "z*", &program, &rest, &argc);

  argv = mrb_malloc(mrb, sizeof(char *) * (argc + 2));

  argv[0] = program;
  for (i = 0; i < argc; i++) {
    argv[i + 1] = mrb_string_value_cstr(mrb, &rest[i]);
  }
  argv[argc + 1] = NULL;

  DO_LOCK();
  if (sigaction(SIGCHLD, &sach, &chld) < 0)
    goto out;
  if (sigaction(SIGINT, &sa, &intr) < 0)
    goto out_restore_sigchld;
  if (sigaction(SIGQUIT, &sa, &quit) < 0)
    goto out_restore_sigint;

  DO_UNLOCK();

  sigaddset(&sa.sa_mask, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &sa.sa_mask, &omask) < 0) {
    DO_LOCK();
    (void)sigaction(SIGQUIT, &quit, (struct sigaction *)NULL);
  out_restore_sigint:
    (void)sigaction(SIGINT, &intr, (struct sigaction *)NULL);
  out_restore_sigchld:
    (void)sigaction(SIGCHLD, &chld, (struct sigaction *)NULL);
  out:
    DO_UNLOCK();
    mrb_sys_fail(mrb, "sigaction");
    return mrb_nil_value();
  }

  switch (pid = fork()) {
  case -1:
    mrb_sys_fail(mrb, "fork");
    break;
  case 0: {
    int fileno;
    DIR *d;
    struct dirent *dp;

    (void)sigaction(SIGQUIT, &quit, (struct sigaction *)NULL);
    (void)sigaction(SIGINT, &intr, (struct sigaction *)NULL);
    (void)sigaction(SIGCHLD, &chld, (struct sigaction *)NULL);
    (void)sigprocmask(SIG_SETMASK, &omask, (sigset_t *)NULL);

    (void)setsid();
    (void)setpgid(0, 0);

    d = opendir("/proc/self/fd");
    if (!d) {
      mrb_sys_fail(mrb, "opendir: /proc/self/fd");
    }

    while ((dp = readdir(d)) != NULL) {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
        /* Skip */
      } else {
        fileno = (int)strtol(dp->d_name, NULL, 0);
        if (fileno > 2) {
          int flags = fcntl(fileno, F_GETFD);
          if (flags < 0) {
            mrb_sys_fail(mrb, "fcntl (get fd flags)");
          }
          if (fcntl(fileno, F_SETFD, flags | FD_CLOEXEC) < 0) {
            mrb_sys_fail(mrb, "fcntl (set fd FD_CLOEXEC)");
          }
        }
      }
    }
    closedir(d);
    execve(program, argv, environ);

    mrb_sys_fail(mrb, "execve");
    _exit(127);
  } break;
  default:
    break;
  }
  mrb_free(mrb, argv);

  if (waitpid(pid, &status, 0) < 0) {
    perror("waitpid");
    status = -1;
  }
  DO_LOCK();
  if ((sigaction(SIGINT, &quit, (struct sigaction *)NULL) |
       sigaction(SIGQUIT, &intr, (struct sigaction *)NULL)) != 0 ||
      sigprocmask(SIG_SETMASK, &omask, (sigset_t *)NULL) != 0) {
    (void)sigaction(SIGCHLD, &chld, (struct sigaction *)NULL);
    mrb_sys_fail(mrb, "signal restoration");
  }
  if (sigaction(SIGCHLD, &chld, (struct sigaction *)NULL) != 0) {
    mrb_sys_fail(mrb, "sigchld restoration");
  }
  DO_UNLOCK();

  if (WIFEXITED(status)) {
    ret = WEXITSTATUS(status) == 0 ? mrb_true_value() : mrb_false_value();
  } else if (WIFSIGNALED(status)) {
    mrb_warn(mrb, "may be signaled: %d", WTERMSIG(status));
    ret = mrb_false_value();
  } else {
    mrb_raisef(mrb, E_RUNTIME_ERROR, "Something is wrong. status: %S", mrb_fixnum_value(status));
  }

  return ret;
}

#ifdef MRB_DEBUG
static mrb_value mrb__test_fd_leak(mrb_state *mrb, mrb_value self)
{
  (void)open("/dev/null", O_RDWR);
  (void)open("/dev/null", O_RDWR);
  (void)open("/dev/null", O_RDWR);
  return mrb_nil_value();
}
#endif

void mrb_mruby_clean_spawn_gem_init(mrb_state *mrb)
{
  struct RClass *kern;
  kern = mrb->kernel_module;
  mrb_define_method(mrb, kern, "clean_spawn", mrb_do_cleanspawn, MRB_ARGS_ANY());

#ifdef MRB_DEBUG
  struct RClass *cleanspawn;
  cleanspawn = mrb_define_module(mrb, "CleanSpawn");
  mrb_define_module_function(mrb, cleanspawn, "_test_fd_leak", mrb__test_fd_leak, MRB_ARGS_NONE());
#endif
  DONE;
}

void mrb_mruby_clean_spawn_gem_final(mrb_state *mrb)
{
}
