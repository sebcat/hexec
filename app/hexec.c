/* Copyright (c) 2019 Sebastian Cato
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

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
