#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#define PathCapacity 50


int PathCount = 0;
char PATH[50][100];

void error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

int addPath(char *newPath){
    if(access(newPath, F_OK) == 0){
        if (PathCount < PathCapacity){
            strcpy(PATH[PathCount++], newPath);
            return 1;
        }
    }
    return 0;
}
int validPath(char* path){
    if(access(path, F_OK) == 0){
        if(access(path, X_OK) == 0){
            return 1;
        }
    }
    return 0;
}
void findPath(char *command, char *path){
    for(int i = 0;i < PathCount;i++){
        char line[100];
        strcpy(line,PATH[i]);
        strcat(line, "/");
        strcat(line, command);
        int ok = validPath(line);
        if(ok == 1){
            strcpy(path,line);
            return;
        }
    }
    path[0] = '\0';
}

void freeCommand(char ****commands){
    int commandCount = 0;
    while((*commands)[commandCount] != NULL){
        int argCount = 0;
        while((*commands)[commandCount][argCount] != NULL){
            free((*commands)[commandCount][argCount]);
            argCount++;
        }
        if((*commands)[commandCount] != NULL){
            free((*commands)[commandCount]);
        }
        commandCount++;
    }
    if((*commands) != NULL){
        free((*commands));
    }
}
void parseCommand(char*input, char ***command){
    int argCapacity = 20;
    int argCount = 0;
    *command = (char**)malloc(argCapacity * sizeof(char*));
    if (*command == NULL) {
        error();
        return;
    }
    char *token, *string, *toFree;
    toFree = string = strdup(input);
    while ((token = strsep(&string,  " \t\n")) != NULL){
        if (token[0] == '\0') {
            continue;
        }
        if(argCount >= argCapacity){
            argCapacity *= 2;
            *command = (char**)realloc(*command,argCapacity * sizeof(char*));
            if (*command == NULL){
                error();
                return;
            }
        }
        (*command)[argCount] = (char *)malloc((strlen(token) + 1) * sizeof(char));
        if ((*command)[argCount] == NULL){
            error();
            return;
        }
        strcpy((*command)[argCount], token);
        argCount++;
    }
    free(toFree);
    (*command)[argCount++] = NULL;
    return;
}
int parseLine(char*input, char ****commands){
    int commandCapacity = 20;
    int commandCount = 0;
    *commands = (char***)malloc(commandCapacity * sizeof(char**));
    if (*commands == NULL) {
        error();
        return 0;
    }
    char *token, *string, *toFree;
    toFree = string = strdup(input);
    while ((token = strsep(&string,  "&")) != NULL){
        if (token[0] == '\0') {
            continue;
        }
        if (commandCount >= commandCapacity){
            commandCapacity *= 2;
            *commands = (char ***)realloc(*commands, commandCapacity * sizeof(char **));
            if (*commands == NULL){
                error();
                return 0;
            }
        }
        parseCommand(token, &(*commands)[commandCount]);
        commandCount++;
    }
    free(toFree);
    (*commands)[commandCount] = NULL;
    return commandCount;
}

int redirctingMode(char **args){
    // return -1 if does not have > 
    // return -2 if there is no file or too many files after >
    // normaly return the file postion
    int argsCount = 0;
    while(args[argsCount] != NULL){
        if(strcmp(args[argsCount], ">") == 0){
            if(args[argsCount + 1] != NULL && args[argsCount + 2] == NULL){
                return argsCount + 1;
            } else {
                return -2;
            }
        }
        argsCount++;
    }
    return -1;
}
int executCommand(char **args){
    if (args[0] == NULL){
        return 1;
    } else if (strcmp(args[0], "exit") == 0){
        if(args[1] != NULL){
            error();
        }
        return 0;
    } else if (strcmp(args[0], "cd") == 0){
        if (args[1] == NULL){
            error();
        } else if (chdir(args[1]) == -1){
            error();
        }
    } else if (strcmp(args[0], "path") == 0) {
        PathCount = 0;
        int argCount = 1;
        while(args[argCount] != NULL){
            int added = addPath(args[argCount]);
            if(added == 1){
                argCount++;
            }
        }
    } else {
        char path[100];
        findPath(args[0], path);
        if (path[0] == '\0'){
            error();
            return 1;
        }
        int fileIndex = redirctingMode(args);
        char *writeTo = NULL;
        if (fileIndex == -2){
            error();
            return 1;
        } else if (fileIndex != -1){
            writeTo = malloc(strlen(args[fileIndex]) + 1 * sizeof(char));
            if (writeTo != NULL){
                strcpy(writeTo, args[fileIndex]);
            }
            args[fileIndex - 1] = NULL;
        }
        int rc = fork();
        if (rc < 0) {
            error();
        } else if (rc == 0){
            if (writeTo != NULL){
                int fd = open(writeTo, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd != -1){
                    dup2(fd, 1);
                    dup2(fd, 2);
                }
                close(fd);
                free(writeTo);
            }
            execv(path, args);
            error();
        }
    }
    return 1;
}
int handleInput(char *input){
    char ***commands = NULL;
    int commandCount = parseLine(input, &commands);
    for (int i = 0; i < commandCount; i++){
        int exit = executCommand(commands[i]);
        if (exit == 0){ 
            freeCommand(&commands);
            return 1;
        }
    }
    for (int i = 0; i < commandCount; i++){ // wait for every chile proccess to finish
        pid_t child_pid = wait(NULL);
        if (child_pid == -1){
            break;
        }
    }
    freeCommand(&commands);
    return 0;
}
int main(int argc, char *argv[]){
    addPath("/bin");
    if(argc == 1){ // interactive mode
        while (1){
            printf("wish> ");
            char *input;
            size_t inpsize = 1024;
            int characters;
            input = (char *)malloc(inpsize * sizeof(char));
            if (input == NULL){
                error();
                continue;
            }
            characters = getline(&input, &inpsize, stdin);
            if (characters == -1){
                error();
                free(input);
                return 1;
            }
            int ex = handleInput(input);
            free(input);
            if (ex == 1){
                return 0;
            }
        }
    } else if(argc == 2){ // batch mode
        FILE* readFrom = fopen(argv[1], "r");
        if(readFrom == NULL){
            error();
            return 1;
        }
        char *input;
        size_t inpsize = 1024;
        input = (char *)malloc(inpsize * sizeof(char));
        if (input == NULL){
            error();
            fclose(readFrom);
            return 1;
        }
        while(getline(&input, &inpsize, readFrom) != -1) {
            int ex = handleInput(input); // return 1 if exit
            if (ex == 1){
                break;
            }
        }
        free(input);
        fclose(readFrom);
    } else { // too many args
        error();
        return 1;
    }
    return 0;
}