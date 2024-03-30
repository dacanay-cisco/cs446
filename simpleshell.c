//Cisco Dacanay
//PA1: Simple Shell
//9/19/23

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define MAX_LEN 100     //max character length of inputs
#define MAX_ARGS 10     //max number of space-separated command arguments

void parseInput(char input[], char* argList[]);
void executeCommand(char* argList[]);
void changeDirectories(char* argList[]);

int main(int argc, char *argv[]) {
    char currentDir[MAX_LEN];
    char uInput[MAX_LEN];
    char* argList[MAX_ARGS];
    _Bool continueProg = 1;

    while(continueProg) {
        getcwd(currentDir,MAX_LEN);
        printf("cdacanay:%s$ ", currentDir);
        fgets(uInput, MAX_LEN, stdin);
        parseInput(uInput, argList);

        if(strcmp(argList[0], "cd") == 0) {     //check for cd, exit, or other command
            changeDirectories(argList);
        }
        else if(strcmp(argList[0], "exit") == 0) {
            continueProg = 0;       //while loop ends and main returns
        }
        else {
            executeCommand(argList);
        }
    }

    return 0;
}

void parseInput(char input[], char* argList[]) {
    const char delim[2] = " ";
    argList[0] = strtok(input, delim);  //scan first argument
    int i = 0;
    while(argList[i] != NULL) {   //if scanned element was not null/end of list, keep scanning
        argList[i+1] = strtok(NULL, delim);
        i++;
    }
    argList[i-1] = strtok(argList[i-1], "\n");  //remove newline from last element before NULL
}

void executeCommand(char* argList[]) {
    int child_pid = fork();
    if(child_pid < 0) {
        printf("fork Failed\n");
    }
    else if(child_pid == 0) {   //if child
        if(execvp(argList[0], argList) == -1) {
            printf("exec Failed\n");       //error on negative return
            _exit(2);
        }
    }
    else {
        wait(NULL);     //parent wait on child
    }
}

void changeDirectories(char* argList[]) {
    if(argList[1] == NULL || argList[2] != NULL) {  //error if 1 argument or more than 2 arguments
        printf("Path Not Formatted Correctly!\n");
    }
    else {
        if(chdir(argList[1]) < 0) {
            printf("chdir Failed: %s\n", strerror(errno));
        }
    }
}