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
#include <sys/wait.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "app/hexec_sync.h"

static volatile sig_atomic_t got_sigchld_ = 0;

static int mask_sigchld(int how) {
  sigset_t sigmask;

  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGCHLD);
  return sigprocmask(how, &sigmask, NULL);
}

static void on_accept(struct hexec_sync_opts *opts, int fd) {
  int ret;
  pid_t pid;

  for (;;) {
    ret = accept(fd, NULL, 0);
    if (ret < 0) {
      if (errno == ECONNABORTED || errno == EINTR) {
        continue; /* possibly more connections in queue - try again */
      } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
        break;
      } else {
        perror("accept");
        break;
      }
    }

    pid = fork();
    if (pid < 0) {
      perror("fork");
      close(ret);
      continue;
    } else if (pid == 0) {
      mask_sigchld(SIG_UNBLOCK);
      signal(SIGCHLD, SIG_DFL);
      signal(SIGINT, SIG_DFL);
      signal(SIGHUP, SIG_DFL);
      signal(SIGTERM, SIG_DFL);
      dup2(ret, STDIN_FILENO);
      dup2(ret, STDOUT_FILENO);
      dup2(ret, STDERR_FILENO);
      close(ret);
      close(fd);

      if (opts->timeout > 0) {
        alarm(opts->timeout);
      }

      execv(opts->argv[0], opts->argv);
      perror(opts->argv[0]);
      _exit(EXIT_FAILURE);
    } else {
      close(ret);
    }
  }

  return;
}

static void on_sigchld(int sig) {
  got_sigchld_ = 1;
}

static void reap_children(void) {
  pid_t pid;
  int status;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    /* TODO: Implement */
  }
}

int hexec_sync_run(struct hexec_sync_opts *opts, int fd) {
  sigset_t sigmask;
  struct sigaction sa = {0};
  fd_set readfds;
  int ret;
  int status = EXIT_FAILURE;

  /* block SIGCHLD from delivery - should only be delivered to pselect(2) */
  ret = mask_sigchld(SIG_BLOCK);
  if (ret < 0) {
    goto done;
  }

  /* set up SIGCHLD handler */
  sa.sa_flags = 0;
  sa.sa_handler = on_sigchld;
  sigemptyset(&sa.sa_mask);
  ret = sigaction(SIGCHLD, &sa, NULL);
  if (ret < 0) {
    goto unblock_sigchld;
  }

  sigemptyset(&sigmask); /* mask within pselect */
  for (;;) {
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    ret = pselect(fd + 1, &readfds, NULL, NULL, NULL, &sigmask);
    if (ret < 0 && errno != EINTR) {
      perror("pselect");
      goto default_sigchld;
    }

    if (got_sigchld_) {
      got_sigchld_ = 0;
      reap_children();
    }

    if (FD_ISSET(fd, &readfds)) {
      on_accept(opts, fd);
    }
  }

  status = EXIT_SUCCESS;
default_sigchld:
  signal(SIGCHLD, SIG_DFL);
unblock_sigchld:
  mask_sigchld(SIG_UNBLOCK);
done:
  return status;
}

