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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>

#include "lib/fs.h"
#include "lib/iomux.h"
#include "app/hexec_sync.h"

#define DEFAULT_BACKLOG    SOMAXCONN

static struct opts {
  const char *listen;
  int backlog;
} opts_ = {
  .backlog  = DEFAULT_BACKLOG,
};

static const char *optstr_ = "l:b:h";

static struct option options_[] = {
  {"listen",  required_argument, NULL, 'l'},
  {"backlog", required_argument, NULL, 'b'},
  {"help",    no_argument,       NULL, 'h'},
  {NULL,      0,                 NULL, 0},
};

static int int_or_die(const char *name, const char *s) {
  long val;
  char *end;

  val = strtol(s, &end, 10);
  if (val < INT_MIN || val > INT_MAX || *end != '\0') {
    fprintf(stderr, "%s: invalid integer\n", name);
    exit(EXIT_FAILURE);
  }

  return (int)val;
}

int main(int argc, char *argv[]) {
  int ret;
  int lfd;
  int status = EXIT_FAILURE;
  const char *argv0 = argv[0];
  struct hexec_sync_opts sync_opts = {0};

  while ((ret = getopt_long(argc, argv, optstr_, options_, NULL)) != -1) {
    switch (ret) {
    case 'l':
      opts_.listen = optarg;
      break;
    case 'b':
      opts_.backlog = int_or_die("backlog", optarg);
      break;
    case 'h':
    default:
      goto usage;
    }
  }

  argv += optind;
  argc -= optind;
  if (argc <= 0) {
    goto usage;
  }

  if (access(argv[0], F_OK|X_OK) != 0) {
    perror(argv[0]);
    goto done;
  }

  if (opts_.listen == NULL) {
    fprintf(stderr, "listen: missing path\n");
    goto done;
  }

  lfd = fs_mksock(opts_.listen, opts_.backlog);
  if (lfd < 0) {
    perror(opts_.listen);
    goto done;
  }

  sync_opts.argv = argv;
  sync_opts.argc = argc;
  status = hexec_sync_run(&sync_opts, lfd);
/* close_lfd: */
  close(lfd);
done:
  return status;
usage:
  fprintf(stderr,
      "usage: %s [opts] <path>\n"
      "opts:\n"
      "  -l, --listen  <path>    Path to listening socket\n"
      "  -b, --backlog    <n>    Max number of pending connections\n"
      "  -h, --help              This text\n"
      , argv0);
  return EXIT_FAILURE;
}
