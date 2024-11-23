#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

//add the path file 
//rewite the batch mode 
//work > 

char **PATH = NULL;
int PathCapacity = 20;
int PathCount = 0;
void error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}
void emptyPath(){
    for(int i = 0;i < PathCount;i++){
        if(PATH[i] != NULL){
            free(PATH[i]);
        }
    }
    PathCapacity = 20;
    PathCount = 0;
}
void addPath(char *newPath){
    PATH = (char **)malloc(PathCapacity * sizeof(char *));
    if (PATH == NULL){
        error();
        return;
    }
    if(access(newPath, F_OK) == 0){
        if (PathCount >= PathCapacity){
            PathCapacity *= 2;
            PATH = (char **)realloc(PATH, PathCapacity * sizeof(char *));
            if (PATH == NULL){
                error();
                return;
            }
        }
        PATH[PathCount] = (char *)malloc((strlen(newPath) + 1) * sizeof(char));
        if (PATH[PathCount] == NULL){
            error();
            return;
        }
        strcpy(PATH[PathCount], newPath);
        PathCount++;
    } else {
        error();
    }
    return;
}
void freePath(){
    emptyPath();
    free(PATH);
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
        char line[50];
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
int parseInput(char*input, char ****commands){
    int commandCapacity = 20;
    int commandCount = 0;
    *commands = (char***)malloc(commandCapacity * sizeof(char**));
    if (*commands == NULL) {
        error();
        return 0;
    }
    int argCapacity = 20;
    int argCount = 0;
    (*commands)[0] = (char**)malloc(argCapacity * sizeof(char*));
    if ((*commands)[0] == NULL) {
        error();
        return 0;
    }
    char *token, *string, *toFree;
    toFree = string = strdup(input);
    while ((token = strsep(&string,  " \t\n")) != NULL){
        if (token[0] == '\0') {
            continue;
        }
        if (strcmp(token, "&") == 0){
            (*commands)[commandCount][argCount] = NULL;
            commandCount++;
            if(commandCount >= commandCapacity){
                commandCapacity *= 2;
                *commands = (char***)realloc(*commands,commandCapacity * sizeof(char**));
                if (*commands == NULL) {
                    error();
                    return 0;
                }
            }
            (*commands)[commandCount] = (char**)malloc(argCapacity * sizeof(char*));
            if ((*commands)[commandCount] == NULL){
                error();
                return 0;
            }
            argCount = 0;
            continue;
        }
        if(argCount >= argCapacity){
            argCapacity *= 2;
            (*commands)[commandCount] = (char**)realloc((*commands)[commandCount],argCapacity * sizeof(char*));
            if ((*commands)[commandCount] == NULL){
                error();
                return 0;
            }
        }
        (*commands)[commandCount][argCount] = (char *)malloc((strlen(token) + 1) * sizeof(char));
        if ((*commands)[commandCount][argCount] == NULL){
            error();
            return 0;
        }
        strcpy((*commands)[commandCount][argCount], token);
        argCount++;
    }
    (*commands)[commandCount][argCount] = NULL;
    (*commands)[commandCount + 1] = NULL;
    free(toFree);
    return commandCount + 1;
}
int redircting(char **args){
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
         error();
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
        emptyPath();
        int argCount = 1;
        while(args[argCount] != NULL){
            addPath(args[argCount]);
            argCount++;
        }
    } else {
        char path[100];
        findPath(args[0], path);
        if (path[0] == '\0'){
            error();
            return 1;
        }
        int fileIndex = redircting(args);
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
        int fd = -1;
        int rc = fork();
        if (rc < 0) {
            error();
        } else if (rc == 0){ //child proccess
            if(writeTo != NULL){
                fd = open(writeTo, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(fd != -1){
                    dup2(fd, 1);
                    dup2(fd, 2);
                }
            }
            execv(path, args);
            error();
        } else {
            if(fd != -1){
                close(fd);
            }
        }
    }
    return 1;
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
int handleInput(char *input){
    char ***commands = NULL;
    int commandCount = parseInput(input, &commands);
    for (int i = 0; i < commandCount; i++){
        int exit = executCommand(commands[i]);
        if (exit == 0){
            freeCommand(&commands);
            return 1;
        }
    }
    for (int i = 0; i < commandCount; i++){
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
    if(argc == 1){
        while (1){
            printf("wish> ");
            char *input;
            size_t inpsize = 64;
            int characters;
            input = (char *)malloc(inpsize * sizeof(char));
            if (input == NULL){
                error();
                freePath();
                exit(1);
            }
            characters = getline(&input, &inpsize, stdin);
            if (characters == -1){
                error();
                free(input);
                break;
            }
            int ex = handleInput(input);
            free(input);
            if (ex == 1){
                exit(0);
            }
        }
    } else if(argc == 2){
        FILE* readFrom = fopen(argv[1], "r");
        if(readFrom == NULL){
            freePath();
            exit(1);
        }
        char *input;
        size_t inpsize = 64;
        input = (char *)malloc(inpsize * sizeof(char));
        if (input == NULL){
            error();
            freePath();
            fclose(readFrom);
            exit(1);
        }
        while(getline(&input, &inpsize, readFrom) != -1) {
            int ex = handleInput(input);
            if (ex == 1){
                freePath();
                exit(0);
            }
        }
        free(input);
        fclose(readFrom);
    } else {
        freePath();
        exit(1);
    }
    freePath();
    return 0;
}