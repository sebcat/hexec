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

UNAME_S != uname -s

# conditional compilation for platform dependent source code
lib_iomux_SRC_FreeBSD = lib/iomux_kqueue.c
lib_iomux_SRC := ${lib_iomux_SRC_${UNAME_S}}
lib_iomux_OBJ := ${lib_iomux_SRC:.c=.o}

CFLAGS += -I.
SRCS    = lib/fs.c lib/fs_test.c ${lib_iomux_SRC} lib/iomux_test.c
OBJS    = $(SRCS:.c=.o)
TESTS   = lib/fs_test lib/iomux_test

RM ?= rm -f

.PHONY: clean all check

all: check

${lib_iomux_OBJ}: ${lib_iomux_SRC} lib/iomux.h lib/macros.h
lib/iomux_test.o: lib/iomux_test.c lib/iomux.h lib/test.h
lib_iomux_test_DEPS = lib/iomux_test.o ${lib_iomux_OBJ}
lib/iomux_test: ${lib_iomux_test_DEPS}
	$(CC) $(CFLAGS) -o $@ $(lib_iomux_test_DEPS) $(LDFLAGS)

lib/fs.o: lib/fs.c lib/fs.h
lib/fs_test.o: lib/fs_test.c lib/fs.h lib/test.h
lib_fs_test_DEPS = lib/fs_test.o lib/fs.o
lib/fs_test: $(lib_fs_test_DEPS)
	$(CC) $(CFLAGS) -o $@ $(lib_fs_test_DEPS) $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(TESTS)

check: $(TESTS)
	@for T in $(TESTS); do \
		./$$T; \
	done
