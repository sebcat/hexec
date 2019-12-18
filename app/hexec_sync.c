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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "lib/iomux.h"
#include "app/hexec_sync.h"

#define SYNC_LISTENER(x) ((struct sync_listener *)(x))

struct sync_listener {
  struct iomux_handler handler; /* must be first */
  struct hexec_sync_opts opts;
};

static void on_accept(struct iomux_ctx *iomux, struct iomux_handler *h) {
  int ret;
  pid_t pid;
  struct sync_listener *listener = SYNC_LISTENER(h);

  for (;;) {
    ret = accept(h->fd, NULL, 0);
    if (ret < 0) {
      if (errno == ECONNABORTED || errno == EINTR) {
        continue; /* possibly more connections in queue - try again */
      } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
        break;
      } else {
        perror("accept");
        goto fail;
      }
    }

    pid = fork();
    if (pid < 0) {
      perror("fork");
      close(ret);
      goto fail;
    } else if (pid == 0) {
      signal(SIGINT, SIG_DFL);
      signal(SIGHUP, SIG_DFL);
      signal(SIGTERM, SIG_DFL);
      signal(SIGCHLD, SIG_DFL);
      dup2(ret, STDIN_FILENO);
      dup2(ret, STDOUT_FILENO);
      dup2(ret, STDERR_FILENO);
      close(ret);
      iomux_cleanup(iomux);
      close(h->fd);
      execv(listener->opts.argv[0], listener->opts.argv);
      perror(listener->opts.argv[0]);
      _exit(EXIT_FAILURE);
    } else {
      close(ret);
    }
  }

  return;
fail:
  iomux_err(iomux);
}

int hexec_sync_run(struct hexec_sync_opts *opts, int fd) {
  int ret;
  struct iomux_ctx iomux;
  int status = EXIT_FAILURE;
  struct sync_listener listener = {
    .handler = {
      .fd          = fd,
      .source_func = on_accept,
    },
    .opts = *opts,
  };

  ret = iomux_init(&iomux);
  if (ret != 0) {
    perror("iomux_init");
    goto done;
  }

  ret = iomux_add_source(&iomux, IOMUX_HANDLER(&listener));
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
done:
  return status;
}

