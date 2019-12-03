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

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fts.h>

#include "lib/fs.h"

int fs_mksock(const char *path, int backlog) {
  int fd;
  int ret;
  struct sockaddr_un sun = {0};
  struct stat sb = {0};

  sun.sun_family = AF_UNIX;
  ret = snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", path);
  if (ret <= 0) {
    errno = EINVAL;
    goto fail;
  } else if (ret >= sizeof(sun.sun_path)) {
    errno = ENAMETOOLONG;
    goto fail;
  }

  /* if 'path' exists and is a socket, unlink it */
  ret = stat(path, &sb);
  if (ret == 0) {
    if (S_ISSOCK(sb.st_mode)) {
      ret = unlink(path);
      if (ret != 0) {
        goto fail;
      }
    }
  } else if (errno != ENOENT) {
    goto fail;
  }

  fd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (fd < 0) {
    goto fail;
  }

  ret = bind(fd, (struct sockaddr *)&sun, sizeof(sun));
  if (ret < 0) {
    goto close_fd;
  }

  ret = listen(fd, backlog);
  if (ret < 0) {
    goto close_fd;
  }

  /* fchmod on socket in POSIX is undefined behavior. chmod on path is
   * TOCTOU... forking a child, setting umask and passing fd from child
   * to parent is portable, but has its own issues. Let's chmod for now */
  ret = chmod(sun.sun_path, 0777);
  if (ret < 0) {
    goto close_fd;
  }

  return fd;
close_fd:
  close(fd);
fail:
  return -1;
}

int fs_mkdir_all(const char *path) {
  char buf[2048];
  char *curr;
  size_t pathlen;
  int ret;
  int mode = S_IRWXU | S_IRWXG | S_IRWXO; /* count on umask(2) */

  if (path == NULL || *path == '\0') {
    return 0; /* creating nothing successfully */
  }

  pathlen = strlen(path);
  if (pathlen >= sizeof(buf)) {
    errno = ENAMETOOLONG;
    return -1;
  }

  strncpy(buf, path, pathlen+1);

  /* trim trailing slashes */
  for(curr = buf+pathlen-1; curr > buf && *curr == '/'; curr--) {
    *curr = '\0';
  }

  if (curr == buf) {
    return 0; /* nothing but slashes */
  }

  /* create intermediate directories */
  for(curr = buf+1; *curr != '\0'; curr++) {
    if (*curr == '/') {
      *curr = '\0';
      ret = mkdir(buf, mode);
      if (ret < 0 && errno != EEXIST && errno != EISDIR) {
        return -1;
      }
      *curr = '/';
      while (curr[1] == '/') {
        curr++;
      }
    }
  }

  ret = mkdir(buf, mode);
  if ( ret < 0 && errno != EEXIST && errno != EISDIR) {
    return -1;
  }

  return 0;
}

int fs_remove_all(const char *path) {
  char * const paths[] = {(char*)path, NULL};
  FTSENT *ent;
  FTS *fts = NULL;

  fts = fts_open(paths, FTS_PHYSICAL, NULL);
  if (fts == NULL) {
    goto fail;
  }

  while ((ent = fts_read(fts)) != NULL) {
    if (ent->fts_info == FTS_DP) {
      if (rmdir(ent->fts_name) != 0) {
        goto fts_close;
      }
    } else if (ent->fts_info == FTS_F ||
        ent->fts_info == FTS_SL ||
        ent->fts_info == FTS_SLNONE ||
        ent->fts_info == FTS_DEFAULT) {
      if (unlink(ent->fts_name) != 0) {
        goto fts_close;
      }
    }
  }

  if (fts_close(fts) != 0) {
    goto fail;
  }

  return 0;

fts_close:
  fts_close(fts);
fail:
  return -1;
}
