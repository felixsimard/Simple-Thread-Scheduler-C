#!/bin/bash

# Listens on localhost port 3001 (you can change this below)
# After connecting to this script, you can write shell commands to the socket
# The output of the shell command will be written back to the socket.

# Note, the process will only live for 10 seconds. You probably don't
# want to remove the timeout. The nc process will otherwise wait
# forever for a connection and cannot be interrupted with SIGINT.

FIFO_NAME=comp310-a2-${USER}
PORT=123459
HOST=0.0.0.0
echo "Starting remote_shell for 60 seconds."
echo "Listening on ${HOST}:${PORT}"
rm -f /tmp/${FIFO_NAME}; mkfifo /tmp/${FIFO_NAME}
cat /tmp/${FIFO_NAME} | /bin/sh -i 2>&1 | timeout 60 nc -l 0.0.0.0 123459 > /tmp/${FIFO_NAME}
echo "Hello world from this text file." > hello.txt
echo "Shell Ended."
