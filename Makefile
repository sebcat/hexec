# Copyright (c) 2019 Sebastian Cato
# 
#  Permission is hereby granted, free of charge, to any person obtaining
#  a copy of this software and associated documentation files (the
#  "Software"), to deal in the Software without restriction, including
#  without limitation the rights to use, copy, modify, merge, publish,
#  distribute, sublicense, and/or sell copies of the Software, and to
#  permit persons to whom the Software is furnished to do so, subject to
#  the following conditions:
# 
#  The above copyright notice and this permission notice shall be
#  included in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
#  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
#  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
#  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

CFLAGS += -I.
SRCS    = lib/fs.c lib/fs_test.c
OBJS    = $(SRCS:.c=.o)
TESTS   = lib/fs_test

RM ?= rm -f

.PHONY: clean all check

all: check

lib/fs.o: lib/fs.c lib/fs.h
lib/fs_test.o: lib/fs_test.c lib/fs.h lib/test.h
lib_fs_test_DEPS = lib/fs_test.o lib/fs.o
lib/fs_test: $(lib_fs_test_DEPS)
	$(CC) $(CFLAGS) -o $@ $(lib_fs_test_DEPS)

clean:
	$(RM) $(OBJS) $(TESTS)

check: $(TESTS)
	@for T in $(TESTS); do \
		./$$T; \
	done
