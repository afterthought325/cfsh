/*
written by Chaise Farrar, 1-30-2018

Simple Shell written in C. Can handle the following shell features:
  * Output Redirection            > 
  * Output Redirection/Append     >>
  * Input Redirection             <
  * Piping Support                |
  * Background processing         &
  * Following Built-ins:
    - cd
    - history
    - pwd
    - clear
    - ect....
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>



#define MAX_HISTORY 200

char *prompt="$ >";

int main (void) 
{
    pid_t   childPid,wPid;
    char    status = 0;
    char    line[81];
    char    *token;
    char    *sep = " \t\n";
    char    **args;
    char    **history;
    int     hist_iter;
    
    
    /* 
     * Don't Type Cast output of malloc, Its bad practice and
     * potentially dangerous! Also, remember to check if malloc 
     * returned NULL! 
     */
    // Set max input for line
    args = malloc (80*sizeof( *args));

    //Allocate Space for history
    history = malloc(MAX_HISTORY*sizeof( *history));

    //If history was successfully allocated, allocate enough spots for its items.
    if (history) {
        for (int x = 0; x < MAX_HISTORY; x++) {
            history[x] = malloc(sizeof(history[x]));
        }
    }

    fprintf(stderr, "%s", prompt);

    while (fgets(line,80,stdin) != NULL) {
        //Copy command into history array.
        strcpy(history[hist_iter],line);
        hist_iter = ++hist_iter % MAX_HISTORY; //Possibly the SegFault issue?
        int token_count = 0;
        token = strtok(line, sep);
        args[token_count++] = token;

        while (token != NULL) {
            token = strtok(NULL, sep);
            args[token_count++] = token;
        }
        //Add null pointer to keep array a valid C string.
        args[token_count] = (char *) NULL;

        if (strcmp(args[0], "exit") == 0) {
            exit(0);
        } else if (strcmp(args[0], "history") == 0) {
            //TODO: Fix this crap
            for (int i = 0; i <= hist_iter; i++) {
                fprintf(stderr,"%d: %s\n",i,history[i]);
            }
        }
        else {
            switch (childPid = fork()) {
            case 0:  execvp(args[0], args);   /* child */
                     fprintf(stderr, "ERROR %s no such program\n", line);
                     exit(1);
                     break;
            case -1: fprintf(stderr, "ERROR can't create child process!\n");   /* unlikely but possible if hit a limit */
                     break;
            default: wPid = wait(&status);   /* Waits for all child processes */
            }
            fprintf(stderr, "%s", prompt);
        }

    }
    free(args);
    for (int x = 0; x < MAX_HISTORY; x++) {
        free(history[x]);
    }
    free(history);
}
