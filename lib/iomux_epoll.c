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

#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "lib/iomux.h"
#include "lib/macros.h"

int iomux_init(struct iomux_ctx *ctx) {
  int qfd;

  memset(ctx, 0, sizeof(*ctx));

  qfd = epoll_create1(EPOLL_CLOEXEC);
  if (qfd < 0) {
    return -1;
  }

  ctx->qfd = qfd;
  return 0;
}

int iomux_cleanup(struct iomux_ctx *ctx) {
  int ret;

  ret = close(ctx->qfd);
  if (ret != 0) {
    return -1;
  }

  return 0;
}

int iomux_add_source(struct iomux_ctx *ctx, struct iomux_handler *h) {
  struct epoll_event ev;
  int ret;

  ev.events = EPOLLIN;
  ev.data.ptr = h;
  ret = epoll_ctl(ctx->qfd, EPOLL_CTL_ADD, h->fd, &ev);
  if (ret < 0) {
    return -1;
  }

  ctx->nhandlers++;
  return 0;
}

int iomux_close_source(struct iomux_ctx *ctx, struct iomux_handler *h) {
  struct epoll_event ev;
  int ret;

  /* pre 2.6.9 kernels required event to be set even though its ignored */
  ev.events = EPOLLIN;
  ev.data.ptr = h;
  ret = epoll_ctl(ctx->qfd, EPOLL_CTL_DEL, h->fd, &ev);
  if (ret < 0) {
    return -1;
  }

  ret = close(h->fd);
  ctx->nhandlers--;
  if (ret < 0) {
    return -1;
  }

  return 0;
}

static void handle_events(struct iomux_ctx *ctx, struct epoll_event *evs,
    size_t nevs) {
  struct iomux_handler *h;
  size_t i;

  for (i = 0; i < nevs; i++) {
    h = evs[i].data.ptr;

    if ((evs[i].events & EPOLLIN) == EPOLLIN) {
      h->source_func(ctx, h);
    }
  }
}

int iomux_run(struct iomux_ctx *ctx) {
  struct epoll_event evs[IOMUX_NEVS];
  int ret = 0;

  ctx->status = 0;
  ctx->flags |= IOMUXF_RUNNING;
  while (ctx->nhandlers > 0) {
    ret = epoll_wait(ctx->qfd, evs, ARRAY_SIZE(evs), -1);
    if (ret > 0) {
      handle_events(ctx, evs, ret);
    } else if (ret < 0 && errno != EINTR) {
      iomux_err(ctx);
      break;
    }

    if (!(ctx->flags & IOMUXF_RUNNING)) {
      break;
    }
  }

  ctx->flags &= ~IOMUXF_RUNNING;
  return ctx->status;
}

