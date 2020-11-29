/*

COMP 310 / ECSE 427 - Operating Systems
Assignment 2
Felix Simard (260865674)

*/

#define _XOPEN_SOURCE

#ifndef __SUT_H__
#define __SUT_H__

#include <stdbool.h>

#include <ucontext.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <sys/wait.h>

#include "a1_lib.h"
#include "helper.h"
#include "queue.h"

// CONSTANTS
#define MAX_THREADS                        32
#define THREAD_STACK_SIZE                  1024*64

#define RR                                 1   // round robin
#define FCFS                               2   // first come first served

#define RR_QUANTUM                         2   // in seconds

#define SPLIT_TOKEN "|TOK|"

#define HOST "0.0.0.0"
#define PORT 123459

#define BUFSIZE 4096

// Task definition
typedef void (*sut_task_f)();

// Thread descriptor structure
typedef struct __threaddesc {
  int threadid;
  char* threadstack;
  sut_task_f* threadfunc; // void* threadfunc;
  ucontext_t threadcontext;
} threaddesc;


extern int numthreads;

/*--- Kernel level threads ---*/
void *cexec_scheduler();
void *iexec_scheduler();

/*--- SUT library ---*/
void sut_init();
bool sut_create(sut_task_f fn);
void sut_yield();
void sut_exit();
void sut_open(char *dest, int port);
void sut_write(char *buf, int size);
void sut_close();
char *sut_read();
void sut_shutdown();


/*--- Remote process functions ---*/

/*--- Close connection to remote process ---*/
int RPC_Close(int sockfd);

/*--- Connect to remote process ---*/
int RPC_Connect(const char *host, uint16_t port, int *sockfd);

/*--- Send message ---*/
ssize_t RPC_Call(int sockfd, const char *buf, size_t len);

/*--- Receive message ---*/
ssize_t RPC_Receive(int sockfd, char *buf, size_t len);


#endif
