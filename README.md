# hexec

## Status

In progress

## Goals

- (A)synchronous execution of processes over HTTP
- SCGI interface (can be used by nginx, apache, ...)
- bounded parallelism - most N processes at a time

## Usage

- make
- ./app/hexec --listen foo.sock ./test.sh
- socat stdio unix-connect:foo.sock
