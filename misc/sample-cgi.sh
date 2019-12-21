#!/bin/sh
printf 'Status: 200 OK\r\nContent-Type: text/plain\r\n\r\nO Hai\n'
exec 0>&- >&- 2>&-
