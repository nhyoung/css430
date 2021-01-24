#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


#define MAX_LINE 80

int main(void) {

    int status = 0;
    char *args[MAX_LINE / 2 + 1];
    int should_run = 1;

    // stores most recent command
    char histBuff[MAX_LINE + 1] = "No commands in history.\n";

    while (should_run) {
        printf("osh>");
        fflush(stdout);

        // (0) read user input
        char input[MAX_LINE + 1];
        fgets(input, MAX_LINE + 1, stdin); // save user input to "input" variable

        if (strcmp(input,"exit\n") == 0) { // exit shell and terminate
            should_run = 0;
        } else { // handle commands

            // (1) fork a child process using fork()
            int pid = fork();

            // (2) the child process will invoke execvp()
            if (pid == 0) { // Child
                char *tokens;
                if (strcmp(input,"!!\n") == 0) { // echo and fetch most recent command
                    printf("%s", histBuff);
                    tokens = histBuff;
                } else { // all other inputs
                    tokens = input;
                    strcpy(histBuff,input);
                }

                // flags and indices for '&', '>', '<', and '|'
                // if input contains any of the above, set respective flag to true
                bool ampersand = false;
                int ampIndex;
                bool redirOut = false;
                bool redirIn = false;
                int redirIndex;
                bool hasPipe = false;
                int pipeIndex;

                // set last character to "null"
                tokens[strlen(tokens)-1] = 0;
                tokens = strtok(tokens, " ");

                // split string and insert tokens to 'args'
                int i = 0;
                while (tokens) {
                    args[i] = tokens;
                    if (strcmp(args[i],"&") == 0) {
                        ampersand = true;
                        ampIndex = i;
                    } else if (strcmp(args[i],">") == 0) {
                        redirOut = true;
                        redirIndex = i;
                    } else if (strcmp(args[i],"<") == 0) {
                        redirIn = true;
                        redirIndex = i;
                    } else if (strcmp(args[i],"|") == 0) {
                        hasPipe = true;
                        args[i] = NULL;
                        pipeIndex = i;

                        // create pipe
                        int pipeFD[2];
                        pipe(pipeFD);

                        int pidPipe = fork();

                        // left side of pipe
                        if (pidPipe == 0) {
                            close(pipeFD[0]);
                            dup2(pipeFD[1],STDOUT_FILENO);
                            execvp(args[0], args);

                        // right side of pipe
                        } else {
                            close(pipeFD[1]);
                            dup2(pipeFD[0],STDIN_FILENO);
                            wait(NULL);
                        }
                    }
                    tokens = strtok (NULL, " ");
                    i++;
                }
                // one extra element for the last NULL
                args[i] = NULL;

                // dont use '&' as argument
                if (ampersand) {
                    args[ampIndex] = NULL;
                }

                // redirects the output of a command to a file
                if (redirOut) {
                    args[redirIndex] = NULL;
                    FILE *fp = fopen(args[redirIndex + 1], "w");
                    int fd = fileno(fp);
                    dup2(fd,STDOUT_FILENO);
                    fclose(fp);

                // redirects the input to a command from a file
                } else if (redirIn) {
                    args[redirIndex] = NULL;
                    FILE *fp = fopen(args[redirIndex + 1], "r");
                    int fd = fileno(fp);
                    dup2(fd,STDIN_FILENO);
                    fclose(fp);

                // special case for pipe: execvp() from after pipe
                } else if(hasPipe) {
                    execvp(args[pipeIndex + 1], args);
                }
                execvp(args[0], args);
                exit(status);
            } else { // Parent

                // copy input to most recent command
                if (strcmp(input,"!!\n") != 0) {
                    strcpy(histBuff, input);
                }

                // (3) parent will invoke wait() unless command included '&'
                if (input[strlen(input) - 2] != '&') {
                    wait(NULL);
                    sleep(1);
                }
            }
        }
    }
    return 0;
}

