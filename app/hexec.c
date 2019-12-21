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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/macros.h"
#include "app/hexec_sync.h"

int main(int argc, char *argv[]) {
  char new_argv0[64];
  const char *argv0 = argv[0];
  int status = EXIT_FAILURE;
  size_t i;
  static const struct {
    const char *name;
    int (*func)(int, char **);
  } subcmds[] = {
    {"sync", hexec_sync_main},
  };

  if (argc < 2) {
    goto usage;
  }

  snprintf(new_argv0, sizeof(new_argv0), "%s-%s", argv[0], argv[1]);

  for (i = 0; i < ARRAY_SIZE(subcmds); i++) {
    if (strcmp(subcmds[i].name, argv[1]) == 0) {
      argv[1] = new_argv0;
      status = subcmds[i].func(argc - 1, &argv[1]);
      break;
    }
  }

  if (i == ARRAY_SIZE(subcmds)) {
    goto usage;
  }

  return status;
usage:
  fprintf(stderr,
      "usage: %s <sub-command> [args]\n"
      "sub-commands:\n"
      "  sync - evaluate SCGI requests in sync mode\n"
      , argv0);
  return EXIT_FAILURE;
}
