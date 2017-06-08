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
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mrb_cleanspawn.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

extern char **environ;

static mrb_value mrb_do_cleanspawn(mrb_state *mrb, mrb_value self)
{
  char *program;
  mrb_value *rest, ret;
  mrb_int argc;
  pid_t pid;
  char **argv;
  int i, status;
  posix_spawn_file_actions_t file_actions;

  mrb_get_args(mrb, "z*", &program, &rest, &argc);

  argv = mrb_malloc(mrb, sizeof(char *) * (argc + 2));

  argv[0] = program;
  for (i = 0; i < argc; i++) {
    argv[i + 1] = mrb_string_value_cstr(mrb, &rest[i]);
  }
  argv[argc + 1] = NULL;

  if (posix_spawn_file_actions_init(&file_actions) != 0) {
    mrb_sys_fail(mrb, "posix_spawn_file_actions_init");
  }

  int fd;
  DIR *d = opendir("/proc/self/fd");
  if (!d) {
    mrb_sys_fail(mrb, "opendir: /proc/self/fd");
  }

  struct dirent *dp;
  while ((dp = readdir(d)) != NULL) {
    if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
      /* Skip */
    } else {
      fd = (int)strtol(dp->d_name, NULL, 0);
      if (fd > 2) {
        if (posix_spawn_file_actions_addclose(&file_actions, fd) != 0) {
          mrb_sys_fail(mrb, "posix_spawn_file_actions_addclose");
        }
      }
    }
  }
  closedir(d);

  if (posix_spawn(&pid, program, &file_actions, NULL, argv, environ) != 0) {
    mrb_sys_fail(mrb, "posix_spawn");
  }

  if (waitpid(pid, &status, 0) < 0) {
    mrb_sys_fail(mrb, "waitpid");
  }
  mrb_free(mrb, argv);

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
  int fd1 = open("/dev/null", O_RDWR);
  int fd2 = open("/dev/null", O_RDWR);
  int fd3 = open("/dev/null", O_RDWR);
  return mrb_nil_value();
}
#endif

void mrb_mruby_clean_spawn_gem_init(mrb_state *mrb)
{
  struct RClass *kern, *cleanspawn;
  kern = mrb->kernel_module;
  mrb_define_method(mrb, kern, "clean_spawn", mrb_do_cleanspawn, MRB_ARGS_ANY());

#ifdef MRB_DEBUG
  cleanspawn = mrb_define_module(mrb, "CleanSpawn");
  mrb_define_module_function(mrb, cleanspawn, "_test_fd_leak", mrb__test_fd_leak, MRB_ARGS_NONE());
#endif
  DONE;
}

void mrb_mruby_clean_spawn_gem_final(mrb_state *mrb)
{
}
