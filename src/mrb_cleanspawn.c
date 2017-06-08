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

#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mrb_cleanspawn.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

extern char **environ;

static mrb_value mrb_do_cleanspawn(mrb_state *mrb, mrb_value self) {
  char *program;
  mrb_value *rest, ret;
  mrb_int argc;
  pid_t pid;
  char **argv;
  int i, status;
  mrb_get_args(mrb, "z*", &program, &rest, &argc);

  argv = mrb_malloc(mrb, sizeof(char *) * (argc + 2));

  argv[0] = program;
  for (i = 0; i < argc; i++) {
    argv[i + 1] = mrb_string_value_cstr(mrb, &rest[i]);
  }
  argv[argc + 1] = NULL;

  if (posix_spawn(&pid, program, NULL, NULL, argv, environ) != 0) {
    mrb_sys_fail(mrb, "posix_spawn");
  }

  if (waitpid(pid, &status, 0) < 0) {
    mrb_sys_fail(mrb, "waitpid");
  }
  mrb_free(mrb, argv);

  if (WIFEXITED(status)) {
    ret = WEXITSTATUS(status) == 0 ? mrb_true_value() : mrb_false_value();
  } else {
    ret = mrb_false_value();
  }

  return ret;
}

void mrb_mruby_clean_spawn_gem_init(mrb_state *mrb) {
  struct RClass *kern;
  kern = mrb->kernel_module;
  mrb_define_method(mrb, kern, "spawn", mrb_do_cleanspawn, MRB_ARGS_ANY());
  DONE;
}

void mrb_mruby_clean_spawn_gem_final(mrb_state *mrb) {}
