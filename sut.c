/*

COMP 310 / ECSE 427 - Operating Systems
Assignment 2
Felix Simard (260865674)

*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "sut.h"
#include "a1_lib.h"
#include "helper.h"
#include "queue.h"

// C-EXEC kernel thread
pthread_t cexec;

// I-EXEC kernel thread
pthread_t iexec;

// C-EXEC Mutex
pthread_mutex_t cexec_mutex = PTHREAD_MUTEX_INITIALIZER;

// I/O Mutex
pthread_mutex_t io_mutex = PTHREAD_MUTEX_INITIALIZER;

// C-EXEC 'main' user thread fo others to come back to when yielding or done
ucontext_t main_cexec;

// Global socket descriptor (only one remote socket opened at all time)
int sockfd;

// Just to assign thread IDs
int numthreads;

// TASK READY QUEUE
struct queue taskreadyqueue;

// WAIT QUEUE (I/O)
struct queue waitqueue;

// To-IO queue
struct queue toio;

// From-IO queue
struct queue fromio;

// Ready queue empty flag
bool ready_queue_empty = false;

// Wait queue empty flag
bool wait_queue_empty = false;

// Kernel threads flag
bool first_task_done = false;


/*--- sut_init ---*/
void sut_init() {

  // Number of threads counter
  numthreads = 0;

  // Initialize necessary queues
  taskreadyqueue = queue_create();
  queue_init(&taskreadyqueue);

  waitqueue = queue_create();
  queue_init(&waitqueue);

  toio = queue_create();
  queue_init(&toio);

  fromio = queue_create();
  queue_init(&fromio);

  // Capture context of main
  getcontext(&main_cexec);

  // Create C-EXEC kernel thread, and its mutex
  pthread_create(&cexec, NULL, cexec_scheduler, NULL);

  // Create I-EXEC kernel thread
  pthread_create(&iexec, NULL, iexec_scheduler, NULL);

}

/*--- C-EXEC scheduler ---*/
void *cexec_scheduler() {

  struct queue_entry *next_task;

  while(!ready_queue_empty || !wait_queue_empty) {
    // Peek front of taskreadyqueue, check if not null
    pthread_mutex_lock(&cexec_mutex);
    next_task = queue_peek_front(&taskreadyqueue);
    pthread_mutex_unlock(&cexec_mutex);

    if(next_task) {
      // Pop head of taskreadyqueue (atomic) as next task to be executed
      pthread_mutex_lock(&cexec_mutex);
      queue_pop_head(&taskreadyqueue);
      pthread_mutex_unlock(&cexec_mutex);

      ucontext_t context = *(ucontext_t *)next_task->data;
      // swap context from current task to next task
      swapcontext(&main_cexec, &context);

      first_task_done = true;

    } else if(first_task_done){

      ready_queue_empty = true;

    }
    nanosleep((const struct timespec[]){{0, 1000000000L}}, NULL);
    //usleep(100000);
  }
  return 0;
}

/*--- I-EXEC scheduler ---*/
void *iexec_scheduler() {

  struct queue_entry *toio_peek;
  struct queue_entry *wait_entry;
  char msg[BUFSIZE];
  char tmp_msg[BUFSIZE];
  char received_msg[BUFSIZE];

  while(!ready_queue_empty || !wait_queue_empty) {

    // PEEK
    pthread_mutex_lock(&io_mutex);
    toio_peek = queue_peek_front(&toio);
    pthread_mutex_unlock(&io_mutex);

    if(toio_peek) {

      sprintf(tmp_msg, "%s", (char*)toio_peek->data);
      strcpy(msg, tmp_msg);

      printf("MSG: %s\n", msg);

      // POP
      pthread_mutex_lock(&io_mutex);
      queue_pop_head(&toio);
      pthread_mutex_unlock(&io_mutex);

      char *action = strtok(msg, SPLIT_TOKEN);

      if(strcmp(action, "open") == 0) { // OPEN

        printf("\nOPEN\n");
        char* dest = strtok(NULL, SPLIT_TOKEN);
        int port = atoi(strtok(NULL, SPLIT_TOKEN));

        printf("Opened connection to %s:%d\n", dest, port);

        RPC_Connect(dest, port, &sockfd);

      } else if(strcmp(action, "read") == 0) { // READ

        printf("\nREAD\n");

        // Receive message from client, add it to from-io queue
        RPC_Receive(sockfd, received_msg, sizeof(received_msg));

        struct queue_entry *fromio_msg_entry = queue_new_node(received_msg);
        pthread_mutex_lock(&cexec_mutex);
        queue_insert_tail(&fromio, fromio_msg_entry);
        pthread_mutex_unlock(&cexec_mutex);

        pthread_mutex_lock(&cexec_mutex);
        struct queue_entry *ready = queue_peek_front(&waitqueue);
        if(ready) {
          queue_insert_tail(&taskreadyqueue, ready);
          queue_pop_head(&waitqueue);
        }
        pthread_mutex_unlock(&cexec_mutex);

      } else if(strcmp(action, "write") == 0) { // WRITE

        printf("\nWRITE\n");

        char* param1 = strtok(NULL, SPLIT_TOKEN);
        char* param2 = strtok(NULL, SPLIT_TOKEN);

        char *buf_msg = param1;
        size_t buf_size = (size_t)param2;

        RPC_Call(sockfd, buf_msg, strlen(buf_msg));

      } else if(strcmp(action, "close") == 0) {

        printf("\nCLOSE\n");

        RPC_Close(sockfd);

      }
    } else if(first_task_done) {

      wait_queue_empty = true;

    }
    nanosleep((const struct timespec[]){{0, 1000000000L}}, NULL);
  }

  return 0;

}

/*--- sut_create ---*/
bool sut_create(sut_task_f fn) {

  // thread descriptor
  threaddesc *tdescptr;
  tdescptr = malloc(sizeof(threaddesc));

  /*
  if(numthreads >= MAX_THREADS) {
    printf("\nFATAL: Maximum thread limit reached... creation failed!\n");
    return -1;
  }
  */

  // Create user-level thread
  getcontext(&(tdescptr->threadcontext)); // save current user-level thread context
  tdescptr->threadid = numthreads;
  tdescptr->threadstack = (char *)malloc(THREAD_STACK_SIZE);
  tdescptr->threadcontext.uc_stack.ss_sp = tdescptr->threadstack; // uc_stack : Stack used for this context
  tdescptr->threadcontext.uc_stack.ss_size = THREAD_STACK_SIZE;
  tdescptr-> threadcontext.uc_link = &main_cexec; // uc_link : This is a pointer to the 'parent' context structure which is used if the context described in the current structure returns.
  tdescptr-> threadcontext.uc_stack.ss_flags = 0;
  tdescptr->threadfunc = &fn;

  makecontext(&(tdescptr->threadcontext), fn, 0);

  // increment number of user-level threads as we just created a new one
  numthreads++;

  // Add thread to back of taskreadyqueue (atomic)
  struct queue_entry *task = queue_new_node(&(tdescptr->threadcontext));
  pthread_mutex_lock(&cexec_mutex);
  queue_insert_tail(&taskreadyqueue, task);
  pthread_mutex_unlock(&cexec_mutex);

  return 0;
}

/*--- sut_yield ---*/
void sut_yield() {

  // Get current context
  ucontext_t current_context;
  getcontext(&current_context);
  struct queue_entry *task = queue_new_node(&current_context);

  // Put task back at end of taskreadyqueue
  pthread_mutex_lock(&cexec_mutex);
  queue_insert_tail(&taskreadyqueue, task);
  pthread_mutex_unlock(&cexec_mutex);

  // swap content from current to c-exec
  swapcontext(&current_context, &main_cexec);
}


/*--- sut_exit ---*/
void sut_exit() {

  // Stop execution of current task
  // and DO NOT put task back of taskreadyqueue
  ucontext_t current;
  getcontext(&current);
  swapcontext(&current, &main_cexec);
}

/*--- sut_open ---*/
void sut_open(char* dest, int port) {

  char tmp[BUFSIZE];

  // sends a message to I-EXEC by enqueuing the message in To-IO queue
  sprintf(tmp, "open%s%s%s%d", SPLIT_TOKEN, dest, SPLIT_TOKEN, port);
  char *msg = strdup(tmp);

  struct queue_entry *open_msg = queue_new_node(msg); // new entry to be added to taskreadyqueue
  pthread_mutex_lock(&io_mutex);
  queue_insert_tail(&toio, open_msg);
  pthread_mutex_unlock(&io_mutex);

}

/*--- sut_write ---*/
void sut_write(char* buf, int size) {

  char tmp[BUFSIZE];

  sprintf(tmp, "write%s%s%s%d", SPLIT_TOKEN, buf, SPLIT_TOKEN, size);
  char *msg = strdup(tmp);

  struct queue_entry *write_msg = queue_new_node(msg);
  pthread_mutex_lock(&io_mutex);
  queue_insert_tail(&toio, write_msg);
  pthread_mutex_unlock(&io_mutex);

}

/*--- sut_close ---*/
void sut_close() {

  // Add a close request to the to-io queue
  char buf[BUFSIZE];
  sprintf(buf, "close%s", SPLIT_TOKEN);
  struct queue_entry *close_req = queue_new_node(buf);

  pthread_mutex_lock(&cexec_mutex);
  queue_insert_tail(&toio, close_req);
  pthread_mutex_unlock(&cexec_mutex);

}

/*--- sut_read ---*/
char* sut_read() {

  // Save current context
  ucontext_t current_context;
  getcontext(&current_context);
  struct queue_entry *task = queue_new_node(&current_context);

  // put task at end of wait queue
  pthread_mutex_lock(&cexec_mutex);
  queue_insert_tail(&waitqueue, task);
  pthread_mutex_unlock(&cexec_mutex);

  // Add a read request to the to-io queue
  char buf[BUFSIZE];
  sprintf(buf, "read%s", SPLIT_TOKEN);
  struct queue_entry *close_req = queue_new_node(buf);

  pthread_mutex_lock(&cexec_mutex);
  queue_insert_tail(&toio, close_req);
  pthread_mutex_unlock(&cexec_mutex);

  swapcontext(&current_context, &main_cexec);

  // When task resumes, peek peek front of fromio to fetch read data from remote source
  struct queue_entry * from_io = queue_peek_front(&fromio);
  if(from_io) {
    // Read from data
    char *res = (char *) from_io->data;
    pthread_mutex_lock(&cexec_mutex);
    queue_pop_head(&fromio);
    pthread_mutex_unlock(&cexec_mutex);
    return res;
  }

  return NULL;

}

/*--- sut_shutdown ---*/
void sut_shutdown() {
  // We need to keep the main thread waiting for the C-EXEC and I-EXEC threads
  // and it one of the important functions of sut_shutdown().
  // In addition, you can put any termination related actions into this function and cleanly terminate the threading library
  pthread_join(cexec, NULL);
  pthread_join(iexec, NULL);
}

/*--- Remote process functions ---*/

/*--- close connectiion to remote process ---*/
int RPC_Close(int socket_fd) {
  return close(socket_fd);
}

/*--- connect to remote process ---*/
int RPC_Connect(const char *host, uint16_t port, int *socket_fd) {
  return connect_to_server(host, port, socket_fd);
}

/*--- send message ---*/
ssize_t RPC_Call(int socket_fd, const char *buf, size_t len) {
  return send_message(socket_fd, buf, len);
}

/*--- receive message ---*/
ssize_t RPC_Receive(int socket_fd, char *buf, size_t len) {
  return recv_message(socket_fd, buf, len);
}
