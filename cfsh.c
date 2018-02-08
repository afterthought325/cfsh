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

    History is stored in ./.cfsh_hist
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MAX_HISTORY 10
#define LINE_SIZE   80


char *prompt="$ >";

int main (void) 
{
    pid_t   childPid;
    int     status = 0;
    char    line[LINE_SIZE + 1];
    char    *token;
    char    *sep = " \t\n";
    char    **args;
    char    *history[MAX_HISTORY]; //the +1 is for the \0.
    int     hist_iter = 0;
    
    
    /* 
     * Don't Type Cast output of malloc, Its bad practice and
     * potentially dangerous! Also, remember to check if malloc 
     * returned NULL! 
     */
    // Set max input for line
    args = malloc (80*sizeof( *args));

    for (int i = 0; i < MAX_HISTORY; i++) {
        history[i] = (char *) NULL;
    }

    fprintf(stderr, "%s", prompt);

    while (fgets(line,80,stdin) != NULL) {
        int token_count = 0;
        token = strtok(line, sep);
        args[token_count++] = token;
        if (args[0] == NULL) {
            fprintf(stderr, "%s", prompt);
            continue;
        }

        while (token != NULL) {
            token = strtok(NULL, sep);
            args[token_count++] = token;
        }
        
        //Store command into history array.
        if (history[hist_iter] == (char *) NULL) {
            history[hist_iter] = malloc(LINE_SIZE + 1);
        }
        strcpy(history[hist_iter],line);
        ++hist_iter;
        hist_iter = hist_iter % (MAX_HISTORY - 1); 

        if (strcmp(args[0], "exit") == 0) {
            exit(0);
        } else if (strcmp(args[0], "history") == 0) {
            for (int i = hist_iter + 1; i != (hist_iter); i++) { 

                i = i % (MAX_HISTORY -1); //Loop back around to beginning of history array. Basically a simple Ring buffer.
                if (history[i]) {
                    fprintf(stderr,"- %s\n",history[i]);
                }
            }
        } else {
            switch (childPid = fork()) {
            case 0:  execvp(args[0], args);   /* child */
                     fprintf(stderr, "ERROR %s no such program\n", line);
                     exit(1);
                     break;
            case -1: fprintf(stderr, "ERROR can't create child process!\n");   /* unlikely but possible if hit a limit */
                     break;
            default: wait(&status);   /* Waits for all child processes */
            }
        }

        fprintf(stderr, "%s", prompt);

    }
    free(args);

    for (int i = 0; i < MAX_HISTORY; i++) {
        if(history[i] != (char *) NULL) {
            free(history[i]);
        }
    }

}
