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

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "lib/iomux.h"
#include "lib/test.h"

/* fork a child that does some writing on a socketpair(2), then exits */
static int spawn_source(void) {
  int sv[2];
  int ret;
  pid_t pid;

  ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  if (ret != 0) {
    return -1;
  }

  pid = fork();
  if (pid < 0) {
    goto close_sv;
  } else if (pid == 0) {
    close(sv[0]);
    dup2(sv[1], STDIN_FILENO);
    dup2(sv[1], STDOUT_FILENO);
    dup2(sv[1], STDERR_FILENO);
    close(sv[1]);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("oh\n");
    sleep(1);
    printf("hello\n");
    sleep(1);
    _exit(0);
  }

  close(sv[1]);
  return sv[0];

close_sv:
  close(sv[0]);
  close(sv[1]);
  return -1;
}

static int test_run_empty(void) {
  int ret;
  struct iomux_ctx ctx;
  int status = TEST_FAIL;

  ret = iomux_init(&ctx);
  if (ret != 0) {
    TEST_LOGF("iomux_init return: %d", ret);
    goto done;
  }

  ret = iomux_run(&ctx);
  if (ret != 0) {
    TEST_LOGF("iomux_run return: %d", ret);
    goto iomux_cleanup;
  }

  status = TEST_OK;
iomux_cleanup:
  ret = iomux_cleanup(&ctx);
  if (ret != 0) {
    TEST_LOGF("iomux_cleanup return: %d", ret);
    status = TEST_FAIL;
  }
done:
  return status;
}

static void single_handler_func(struct iomux_ctx *ctx,
    struct iomux_handler *h) {
  char buf[128];
  ssize_t n;

  n = read(h->fd, buf, sizeof(buf));
  if (n > 0) {
    write(STDOUT_FILENO, buf, n);
  }
}

static int test_run_single(void) {
  int status = TEST_FAIL;
  struct iomux_ctx ctx;
  int ret;
  int fd;
  struct iomux_handler h = {0};

  signal(SIGCHLD, SIG_IGN);

  ret = iomux_init(&ctx);
  if (ret != 0) {
    TEST_LOGF("iomux_init return: %d", ret);
    goto done;
  }

  fd = spawn_source();
  if (fd < 0) {
    TEST_LOGF("spawn_source: %s", strerror(errno));
    goto iomux_cleanup;
  }

  h.fd = fd;
  h.source_func = &single_handler_func;
  ret = iomux_add_source(&ctx, &h);
  if (ret != 0) {
    TEST_LOGF("iomux_add_source: %s", strerror(errno));
    close(fd);
    goto iomux_cleanup;
  }

  ret = iomux_run(&ctx);
  if (ret != 0) {
    TEST_LOGF("iomux_run: %s", strerror(errno));
    goto iomux_cleanup;
  }

  status = TEST_OK;
iomux_cleanup:
  ret = iomux_cleanup(&ctx);
  if (ret != 0) {
    TEST_LOGF("iomux_cleanup return: %d", ret);
    status = TEST_FAIL;
  }
done:
  signal(SIGCHLD, SIG_DFL);
  return status;
}

TEST_ENTRY(
  {"run_empty", test_run_empty},
  {"run_single", test_run_single},
);
