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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/event.h>

#include "lib/macros.h"
#include "lib/iomux.h"

int iomux_init(struct iomux_ctx *ctx) {
  int qfd;

  memset(ctx, 0, sizeof(*ctx));

  qfd = kqueue();
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
  struct kevent ev = {0};
  int ret;

  /* Room for improvement: buffer EV_ADD to a chunk and add the chunk with
   * one call to kevent, while also getting any outstanding events */
  EV_SET(&ev, h->fd, EVFILT_READ, EV_ADD, 0, 0, h);
  ret = kevent(ctx->qfd, &ev, 1, NULL, 0, NULL);
  if (ret < 0) {
    return -1;
  }

  ctx->nhandlers++;
  return 0;
}

static void queue_close(struct iomux_ctx *ctx, struct iomux_handler *h) {
  size_t i;

  assert(ctx->nevs_to_close < ARRAY_SIZE(ctx->evs_to_close));
  for (i = 0; i < ctx->nevs_to_close; i++) {
    if (ctx->evs_to_close[i] == h) {
      return; /* already queued for closing - do not enqueue again */
    }
  }

  ctx->evs_to_close[ctx->nevs_to_close++] = h;
}

static void handle_events(struct iomux_ctx *ctx, struct kevent *evs,
    size_t nevs) {
  size_t i;
  struct iomux_handler *h;
  int n;

  ctx->nevs_to_close = 0; /* clear # of handlers to close */

  for (i = 0; i < nevs; i++) {
    h = evs[i].udata;

    if ((evs[i].flags & EV_ERROR) != 0 &&
        (evs[i].filter == EVFILT_READ || evs[i].filter == EVFILT_WRITE)) {
      queue_close(ctx, h);
    } else if (evs[i].filter == EVFILT_READ) {
      if (evs[i].data > 0 && h->source_func != NULL) {
        h->source_func(ctx, h);
      }

      if (evs[i].flags & EV_EOF) {
        queue_close(ctx, h);
      }
    }
  }

  /* close file descriptors queued for closing */
  for (n = 0; n < ctx->nevs_to_close; n++) {
    close(h->fd);
    ctx->nhandlers--;
  }
}

int iomux_run(struct iomux_ctx *ctx) {
  struct kevent evs[IOMUX_NEVS];
  int ret = 0;

  ctx->status = 0;
  ctx->flags |= IOMUXF_RUNNING;
  while (ctx->nhandlers > 0) {
    /* Room for improvement: add new events here instead of just
     * waiting for them */
    ret = kevent(ctx->qfd, NULL, 0, evs, ARRAY_SIZE(evs), NULL);
    if (ret > 0) {
      handle_events(ctx, evs, ret);
    } else if (ret < 0 && errno != EINTR) {
      iomux_err(ctx);
    }

    if (!(ctx->flags & IOMUXF_RUNNING)) {
      break;
    }
  }

  ctx->flags &= ~IOMUXF_RUNNING;
  return ctx->status;
}
