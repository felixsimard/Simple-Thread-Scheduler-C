/**
 *  @author: Trung Vuong Thien
 *  @email: trung.vuongthien@mail.mcgill.ca
 *  @description: Header file for socket wrapper functions
 */

#ifndef A1_LIB_
#define A1_LIB_

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BACKLOG_SIZE    10

int connect_to_server(const char *host, uint16_t port, int *sockfd);

/**
 *  Send a message on a socket.
 *
 *  @params:
 *    sockfd:   The file descriptor of the socket.
 *    buf:      The message to send.
 *    len:      Number of bytes to send.
 *  @return:    On success, the functions returns the number of bytes send.
 *              On error, -1 is returned.
 */
ssize_t send_message(int sockfd, const char *buf, size_t len);

/**
 *  Receive a message from a socket.
 *
 *  @params:
 *    sockfd:   The file descriptor of the socket.
 *    buf:      A buffer to store the received message.
 *    len:      The size of the buffer.
 *  @return:    On success, the function returns he number of bytes received.
 *              A value of 0 means the connection on this socket has been
 *              closed. On error, -1 is returned.
 */
ssize_t recv_message(int sockfd, char *buf, size_t len);

#endif  // A1_LIB_
