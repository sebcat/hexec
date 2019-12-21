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
#include <getopt.h>
#include <limits.h>

#include "lib/fs.h"
#include "app/hexec_sync.h"

#define DEFAULT_BACKLOG        SOMAXCONN
#define DEFAULT_SYNC_TIMEOUT   10
#define DEFAULT_NCONCURRENT    64

struct hexec_sync_opts {
  char **argv;
  int argc;
  int timeout;
  int nconcurrent;
};

static struct opts {
  const char *listen;
  int backlog;
  int timeout;
  int nconcurrent;
} opts_ = {
  .backlog      = DEFAULT_BACKLOG,
  .timeout      = DEFAULT_SYNC_TIMEOUT,
  .nconcurrent  = DEFAULT_NCONCURRENT,
};

static const char *optstr_ = "l:b:t:n:h";

static struct option options_[] = {
  {"listen",       required_argument, NULL, 'l'},
  {"backlog",      required_argument, NULL, 'b'},
  {"timeout",      required_argument, NULL, 't'},
  {"nconcurrent",  required_argument, NULL, 'n'},
  {"help",         no_argument,       NULL, 'h'},
  {NULL,           0,                 NULL, 0},
};

static volatile sig_atomic_t got_sigchld_;
static int nchildren_;

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
      nchildren_++;
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
    nchildren_--;
  }
}

static int hexec_sync_run(struct hexec_sync_opts *opts, int fd) {
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

    if (nchildren_ < opts->nconcurrent) {
      FD_SET(fd, &readfds);
    }

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

int hexec_sync_main(int argc, char *argv[]) {
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
    case 't':
      opts_.timeout = int_or_die("timeout", optarg);
      if (opts_.timeout < 0) {
        fprintf(stderr, "timeout: invalid value\n");
        goto usage;
      }
      break;
    case 'n':
      opts_.nconcurrent = int_or_die("nconcurrent", optarg);
      if (opts_.nconcurrent <= 0) {
        fprintf(stderr, "nconcurrent: invalid value\n");
        goto usage;
      }
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
  sync_opts.timeout = opts_.timeout;
  sync_opts.nconcurrent = opts_.nconcurrent;
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
      "  -t, --timeout    <n>    Execution timeout, in seconds\n"
      "  -h, --help              This text\n"
      , argv0);
  return EXIT_FAILURE;
}

