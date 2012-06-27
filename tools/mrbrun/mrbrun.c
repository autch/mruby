#include "mruby.h"
#include "mruby/proc.h"
#include "mruby/array.h"
#include "mruby/string.h"
#include "mruby/dump.h"
#include <stdio.h>
#include <string.h>

struct _args {
  FILE *rfp;
  int argc;
  char** argv;
};

static void
usage(const char *name)
{
  printf("Usage: %s programfile.mrb\n", name);
}

static int
parse_args(mrb_state *mrb, int argc, char **argv, struct _args *args)
{
  char **origargv = argv;

  memset(args, 0, sizeof(*args));

  argv++;
  argc--;

  if (args->rfp == NULL) {
    if (*argv == NULL) args->rfp = stdin;
    else if ((args->rfp = fopen(*argv, "rb")) == NULL) {
      printf("%s: Cannot open program file. (%s)\n", *origargv, *argv);
      return 0;
    }
  }
  args->argv = mrb_realloc(mrb, args->argv, sizeof(char*) * (argc + 1));
  memcpy(args->argv, argv, (argc+1) * sizeof(char*));
  args->argc = argc;

  return 0;
}

static void
cleanup(mrb_state *mrb, struct _args *args)
{
  if (args->rfp && args->rfp != stdin)
    fclose(args->rfp);
  if (args->argv)
    mrb_free(mrb, args->argv);
  mrb_close(mrb);
}

int
main(int argc, char **argv)
{
  mrb_state *mrb = mrb_open();
  int n = -1;
  int i;
  struct _args args;
  struct mrb_parser_state *p;

  if (mrb == NULL) {
    fprintf(stderr, "Invalid mrb_state, exiting mruby");
    return EXIT_FAILURE;
  }

  n = parse_args(mrb, argc, argv, &args);
  if (n < 0 || args.rfp == NULL) {
    cleanup(mrb, &args);
    usage(argv[0]);
    return n;
  }

  n = mrb_load_irep(mrb, args.rfp);
  if (n >= 0) {
    mrb_value ARGV = mrb_ary_new(mrb);
    for (i = 0; i < args.argc; i++) {
      mrb_ary_push(mrb, ARGV, mrb_str_new(mrb, args.argv[i], strlen(args.argv[i])));
    }
    mrb_define_global_const(mrb, "ARGV", ARGV);

    mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_top_self(mrb));
    if (mrb->exc) {
      mrb_p(mrb, mrb_obj_value(mrb->exc));
    }
  }

  cleanup(mrb, &args);

  return n > 0 ? 0 : 1;
}
