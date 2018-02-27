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

#define _POSIX_SOURCE// Removes implicit declaration error for kill()
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>


#define MAX_HISTORY 10
#define LINE_SIZE   80
#define MAX_PIPES   3
//#define PRINT_CWD // if defined, the current working directory will be printed before each prompt


char *prompt="$ ";
char    cwd[1024];
char    last_cmd[LINE_SIZE];

pid_t   cpid;

void int_child() {
    if(cpid > 0){
        kill(cpid,SIGINT);
        wait(NULL);
    } 
}

void sigusr1_func() {
    fprintf(stderr, "\n\n\nSIGUSR1 debug:\n\n");
    if (getcwd(cwd,sizeof(cwd)) != NULL){
        fprintf(stderr, "Current Working Directory on Exit:\n%s\n\n", cwd);
    }
    fprintf(stderr, "Last Run Command:\n%s\n\n",last_cmd);
    if(cpid > 0){
        kill(cpid,SIGINT);
        wait(NULL);
    } 
    exit(0);
}


//Helper functions to coordinate piping of multiple commands
void set_read(int* lpipe){
    dup2(lpipe[0], STDIN_FILENO);
    close(lpipe[0]); // we have a copy already, so close it
    close(lpipe[1]); // not using this end
}
  
void set_write(int* rpipe){
    dup2(rpipe[1], STDOUT_FILENO);
    close(rpipe[0]); // not using this end
    close(rpipe[1]); // we have a copy already, so close it
}

int split_string(char *input, char **output, char *splitter, int size) {
    int count = 0;
    char *token;
    token = strtok(input, splitter);
    output[count++] = token;

    while (token != NULL && count <= size) {
        token = strtok(NULL, splitter);
        output[count++] = token;
    }
    return count -1;
}

void redirect_and_exec(char * cmd) {
    char **args;
    char **cmd_parts;
    int cmd_part_count = 0;

    // Input and output files
    int input;
    int output;


    args = malloc(sizeof(args[0]) * LINE_SIZE);
    cmd_parts = malloc(sizeof(cmd_parts[0]) * LINE_SIZE);
    cmd_part_count = split_string(cmd, cmd_parts, " \t\n", LINE_SIZE);

    for (int i = 1; i < cmd_part_count; i++) {
        if (strcmp(cmd_parts[i], "<") == 0) {
            //Setup Input Redirection
            input = open(cmd_parts[i + 1], O_RDONLY );
            dup2(input,0);
            close(input);

        } else if (strcmp(cmd_parts[i], ">>") == 0) { //Have to check for >> before >, else a false positive may result
            //Setup Output/Append Redirection 
            output = open(cmd_parts[i + 1], O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(output,1);
            close(output);

        } else if (strcmp(cmd_parts[i], ">") == 0) {
            //Setup Output Redirection
            output = open(cmd_parts[i + 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(output,1);
            close(output);

        }
    }
    args[0] = cmd_parts[0];
    int eoc_pos;

    for (eoc_pos = 0; eoc_pos < cmd_part_count; eoc_pos++) {
        if (strpbrk(cmd_parts[eoc_pos],"<>&")) {
                break;
        }
    }
    for (int i = 1; i < eoc_pos; i++) {
        args[i] = cmd_parts[i];
    }

    execvp(args[0],  args);   /* child */
}

void fork_setup_pipe (int *lpipe, int *rpipe, char *cmd) {
    switch (cpid = fork()) {
        case 0:  if(lpipe) {// there's a pipe from the previous process
                    set_read(lpipe);
                 }
                 if(rpipe) {// there's a pipe to the next process
                    set_write(rpipe);
                 }
                 redirect_and_exec(cmd);
                 fprintf(stderr, "ERROR: File could not be opened for input output redirection\n");
                 fprintf(stderr, "ERROR %s could not be processed\n", cmd);
                 kill(0,SIGINT);
                 break;
        case -1: fprintf(stderr, "ERROR can't create child process!\n");   /* unlikely but possible if hit a limit */
                 break;
        default: break;   /* Waits for all child processes */
    }
}

void print_prompt() {
    //Printout current working directory before the shell prompt each time.
    fflush(stdout);
    fflush(stdout);
#ifdef PRINT_CWD
    if (getcwd(cwd,sizeof(cwd)) != NULL){
        fprintf(stdout, "\n%s\n", cwd);
    }
#endif
    fprintf(stdout, "%s", prompt);
}

int main (void) 
{
    char    line[LINE_SIZE + 1];
    char    history[MAX_HISTORY][LINE_SIZE + 1]; 
    int     hist_iter = 0;
    char    **cmds;
    
    //Signal Handling
    signal(SIGINT,(void (*)())int_child);
    signal(SIGUSR1,(void (*)())sigusr1_func);

    //Set history to null terminated strings 
    for (int i = 0; i < MAX_HISTORY; i++) {
        history[i][0] = '\0';
    }

    //Allocate space on heap for piped_cmds
    cmds = malloc(sizeof(cmds[0]) * MAX_PIPES);

    print_prompt();

    //Here we go! get input from user....
    while (fgets(line,LINE_SIZE,stdin) != NULL) {
        strcpy(last_cmd,line);
        int cmdc = 0;
        cmdc = split_string(line, cmds, "|", MAX_PIPES);
        int pipec = 0;
        if (cmdc > 0) {
            pipec = cmdc -1;
        }
        
        //No Command entered
        if (cmdc == 0) {
            print_prompt();

        } else if (strstr(cmds[0], "exit") != NULL) {
            exit(0);

        } else if (strstr(cmds[0], "cd") != NULL) {
            cmdc = split_string(line, cmds, " \t\n", LINE_SIZE);
            if ( cmdc > 1) {
                if (chdir(cmds[1]) == -1) {
                    int errsv = errno;
                    if (errsv == ENOTDIR) {
                        fprintf(stderr, "ERROR: %s not a directory",cmds[1]);
                    }
                }
            } else {
                fprintf(stderr, "ERROR: no path specified for cd\n");
            }

        } else if (strcmp(cmds[0], "history\n") == 0) {
            if ( hist_iter != 0) {
                for (int i = hist_iter + 1; i != (hist_iter); i++) { 

                    i = i % (MAX_HISTORY -1); //Loop back around to beginning of history array. Basically a simple Ring buffer.
                    if (history[i][0] != '\0') {
                        fprintf(stderr,"- %s",history[i]);
                    }
                }
            } 
        //A Non-Builtin Command has been entered
        } else {
            //Store command into history array.
            strcpy(history[hist_iter],line);
            ++hist_iter;
            hist_iter = hist_iter % (MAX_HISTORY - 1); 

            //No Piping
            if (cmdc == 1) {
                switch(cpid = fork()) {
                    case 0: redirect_and_exec(cmds[0]);
                            fprintf(stderr,"ERROR %s no such program\n",line);
                            break;
                    case -1: fprintf(stderr,"ERROR can't create child process!\n");   /* unlikely but possible if hit a limit */
                             break;
                    default: break;   /* too simple to only do 1 wait - not correct */

                }
            //Setup Piping
            } else {
                // two pipes: one from the previous in the chain, one to the next in the chain
                int lpipe[2], rpipe[2];
                
                // create the first output pipe
                pipe(rpipe);
                
                // first child takes input from somewhere else
                fork_setup_pipe(NULL, rpipe, cmds[0]);
                
                // output pipe becomes input for the next process.
                lpipe[0] = rpipe[0];
                lpipe[1] = rpipe[1];
                
                // chain all but the first and last children
                for(int i = 1; i < pipec - 1; i++)
                {
                    pipe(rpipe); // make the next output pipe
                    fork_setup_pipe(lpipe, rpipe, cmds[i]);
                    close(lpipe[0]); // both ends are attached, close them on parent
                    close(lpipe[1]);
                    lpipe[0] = rpipe[0]; // output pipe becomes input pipe
                    lpipe[1] = rpipe[1];
                }
                
                // fork the last one, its output goes somewhere else      
                fork_setup_pipe(lpipe, NULL, cmds[pipec]);
                close(lpipe[0]);
                close(lpipe[1]);
            }
        }

        //If & does not exist 
        if (strpbrk(line,"&") == NULL){
            wait(NULL);
        }
        print_prompt();
        //Loop back around to beginning of while loop

    }

}
