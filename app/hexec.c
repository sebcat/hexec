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

#include <sys/socket.h> /* SOMAXCONN */
#include <unistd.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#include "lib/fs.h"
#include "lib/iomux.h"

static struct opts {
  const char *listen;
  int backlog;
} opts_ = {
  .backlog = SOMAXCONN,
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

static void on_accept(struct iomux_ctx *ctx, struct iomux_handler *h) {
  int ret;

  for (;;) {
    ret = accept4(h->fd, NULL, 0, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (ret < 0) {
      if (errno == ECONNABORTED || errno == EINTR) {
        continue;
      } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
        break;
      } else {
        perror("accept4");
        exit(EXIT_FAILURE); /* TODO: Handle better. Exit the event-loop */
      }
    }

    close(ret); /* TODO: Do something else */
  }
}

int main(int argc, char *argv[]) {
  int ret;
  int lfd;
  int status = EXIT_FAILURE;
  struct iomux_ctx iomux;
  struct iomux_handler listener = {
    .source_func = on_accept,
  };

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
      fprintf(stderr,
          "usage: %s [opts]\n"
          "opts:\n"
          "  -l,--listen  <path>    Path to listening socket\n"
          "  -b,--backlog    <n>    Max number of pending connections\n"
          "  -h,--help              This text\n"
          , argv[0]);
      return EXIT_FAILURE;
    }
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

  ret = iomux_init(&iomux);
  if (ret != 0) {
    perror("iomux_init");
    goto close_lfd;
  }

  listener.fd = lfd;
  ret = iomux_add_source(&iomux, &listener);
  if (ret < 0) {
    perror("iomux_add_source");
    goto iomux_cleanup;
  }

  ret = iomux_run(&iomux);
  if (ret < 0) {
    perror("iomux_run");
    goto iomux_cleanup;
  }

  status = EXIT_SUCCESS;
iomux_cleanup:
  iomux_cleanup(&iomux);
close_lfd:
  close(lfd);
done:
  return status;
}
