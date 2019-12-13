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
#ifndef LIB_IOMUX_H__
#define LIB_IOMUX_H__

#define IOMUX_NEVS 16

/* struct iomux_ctx flags */
#define IOMUXF_RUNNING   (1 << 0) /* event loop is running */

struct iomux_ctx;

struct iomux_handler {
  void (*source_func)(struct iomux_ctx *ctx, struct iomux_handler *h);
  int fd;
};

struct iomux_ctx {
  int flags;
  int status;
  int qfd;
  int nhandlers;
  struct iomux_handler *evs_to_close[IOMUX_NEVS];
  int nevs_to_close;
};

/* iomux_init --
 *   Initialize an iomux context. Returns -1 on error, 0 on success.  */
int iomux_init(struct iomux_ctx *ctx);

/* iomux_cleanup --
 *   Release resources associated with an iomux context. Returns -1 on
 *   error, 0 on success */
int iomux_cleanup(struct iomux_ctx *ctx);

/* iomux_add_source --
 *   Add a source handler for a file descriptor to the iomux context.
 *   If/when the file descriptor becomes readable, the source_func of
 *   the handler will be called. Returns -1 on error, 0 on success. */
int iomux_add_source(struct iomux_ctx *ctx, struct iomux_handler *h);

/* iomux_run --
 *   Run the multiplexer. Returns 0 on success, -1 on failure */
int iomux_run(struct iomux_ctx *ctx);

/* iomux_break --
 *   Break out of the multiplexer from an event handler. */
static inline void iomux_break(struct iomux_ctx *ctx) {
  ctx->flags &= ~IOMUXF_RUNNING;
}

/* iomux_err --
 * Signal an error to the multiplexer, causing iomux_run to fail */
static inline void iomux_err(struct iomux_ctx *ctx) {
  ctx->status = -1;
  iomux_break(ctx);
}
#endif
