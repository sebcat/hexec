#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static struct opts {
  int async;
} opts_;

static const char *optstr_ = "ah";

static struct option options_[] = {
  {"async", no_argument, NULL, 'a'},
  {NULL,    0,           NULL, 0},
};

static void usage(const char *argv0) {
  fprintf(stderr,
      "usage: %s [opts]\n",
      argv0);
}

int main(int argc, char *argv[]) {
  int ch;

  while ((ch = getopt_long(argc, argv, optstr_, options_, NULL)) != -1) {
    switch (ch) {
    case 'a':
      opts_.async = 1;
      break;
    case 'h':
    default:
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  return 0;
}
