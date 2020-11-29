/*

COMP 310 / ECSE 427 - Operating Systems
Assignment 2
Felix Simard (260865674)

*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "sut.h"

/*----------- Helper functions -----------*/
char** parseMessage(char* msg) {

  // User input parsing
  char cmd[strlen(msg) + 1];  // Make sure there's enough space
  strcpy(cmd, msg);

  char **cmd_arr = NULL;
  int n_spaces = 0;
  int i =0;
  char *cmd_pointer = strtok(cmd, SPLIT_TOKEN); // tokenize the string by token

  // Now, we split the user input by spaces and add each token to a local array for easier manipulations
  while(cmd_pointer) {
    cmd_arr = realloc(cmd_arr, sizeof (char*) * ++n_spaces);
    cmd_arr[n_spaces-1] = cmd_pointer; // add to array
    cmd_pointer = strtok(NULL, SPLIT_TOKEN); // move pointer to next token
  }
  // realloc one slot for last NULL
  cmd_arr = realloc(cmd_arr, sizeof (char*) * (n_spaces+1));
  cmd_arr[n_spaces] = 0;
  //

  return cmd_arr;
}

char* cleanInput(char* msg) {
  char *pos;
  if ((pos=strchr(msg, '\n')) != NULL) {
    *pos = '\0';
  }
  return msg;
}
/*----------------------------------------*/
