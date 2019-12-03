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

#ifndef LIB_FS_H__
#define LIB_FS_H__

/* fs_mksock --
 *   Creates a domain socket and chowns it to 777. See listen(2) for
 *   backlog. Returns fd on success, -1 on error. Sets errno. */
int fs_mksock(const char *path, int backlog);

/* fs_mkdir_all --
 *   Creates a directory. If path consists of non-existant intermediate
 *   directories those will be created as well. The mode of the created
 *   directories will be that of 0777, subject to umask(2).
 *   Returns 0 on success, -1 on error. Sets errno. */
int fs_mkdir_all(const char *path);

/* fs_remove_all --
 *   Removes a file or directory. If path is a directory, the content of
 *   the directory will be removed recursively. Changes current working
 *   directory during directory traversal.
 *   Returns 0 on success, -1 on error. Sets errno. */
int fs_remove_all(const char *path);

#endif
