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
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lib/fs.h"
#include "lib/test.h"

#define TESTDIR     ".fs_test"
#define TESTDATADIR TESTDIR "/testdata"
#define TESTSOCK    TESTDATADIR "/some.sock"

static int test_remove_all(void) {
  int ret;

  ret = fs_remove_all(TESTDIR);
  if (ret != 0) {
    TEST_LOGF("%s: %s", TESTDIR, strerror(errno));
    return TEST_FAIL;
  }

  return TEST_OK;
}

static int test_mkdir_all(void) {
  int ret;

  ret = fs_mkdir_all(TESTDATADIR);
  if (ret != 0) {
    TEST_LOGF("%s: %s", TESTDATADIR, strerror(errno));
    return TEST_FAIL;
  }

  return TEST_OK;
}

static int test_mksock(void) {
  int ret;

  ret = fs_mksock(TESTSOCK, SOMAXCONN);
  if (ret < 0) {
    TEST_LOGF("%s: %s", TESTSOCK, strerror(errno));
    return TEST_FAIL;
  }

  ret = close(ret);
  if (ret != 0) {
    TEST_LOGF("close: %s", strerror(errno));
    return TEST_FAIL;
  }

  return TEST_OK;
}

static int test_verify(void) {
  struct stat sb;
  int ret;

  ret = stat(TESTDIR, &sb);
  if (ret != -1 || errno != ENOENT) {
    TEST_LOGF("stat: expected ENOENT, got ret:%d errno:%d", ret, errno);
    return TEST_FAIL;
  }

  return TEST_OK;
}

TEST_ENTRY(
  {"remove_all", test_remove_all},
  {"mkdir_all", test_mkdir_all},
  {"mksock", test_mksock},
  {"remove_all (2nd)", test_remove_all},
  {"verify", test_verify},
);
