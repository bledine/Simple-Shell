#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>

#define CMDLINE_MAX 512
#define MAX_ARGUMENTS 20

/**
* @brief Represents the comand from user
* This struct stores the input args in an array and the size of the array
*/
struct Comand
{
    char **args;
    size_t size;
};

void deallocateComand(struct Comand *inputArg)
{
        if (inputArg == NULL) {
                return;
        }

        /*free each element in array*/
        if(inputArg->args != NULL) {
                for (int i = 0; i < inputArg->size; i++) {
                        free(inputArg->args[i]);
                }

        free(inputArg->args);        /*deallocate array itself*/
        }

        free(inputArg);        /*deallocate struct*/

}

/*
* @brief Breaks down command string into an array of strings
* @param the string that needs to be tokenized
* @param the delimiter to tokenize string
* @return pointer to the array of strings
*/
char** tokenizeCmd(char cmd[], char *tokenChar)
{
        char* token = strtok(cmd, tokenChar);
        char** inputArgs = malloc(MAX_ARGUMENTS * sizeof(char*));
        int i = 0;
        /*seperate the string at the delimeter char*/
        while (token && i < MAX_ARGUMENTS - 1) {
                inputArgs[i] = malloc(strlen(token) + 1);
                strcpy(inputArgs[i], token);
                token = strtok(NULL, tokenChar);
                i++;
        }
        /*add null char at end of array*/
        inputArgs[i] = NULL;
        return inputArgs;
}

/**
* @brief parse the array of args to find errors
* @param pointer to array of args
* @param the size of the array
* @return -1 if errors where found and 0 otherwise
*/
int parseErrors(char **inputArgs, int size)
{
        int argLength = strlen(inputArgs[size -1]);

        if (strcmp(inputArgs[0], ">") == 0 || inputArgs[0][0] == '>' ){
                printf("Error: missing command\n");
                return -1;
        }

        if (strcmp(inputArgs[size-1], "|") == 0 ||inputArgs[0][0] == '|' || strcmp(inputArgs[0], "|") == 0 || inputArgs[size -1][argLength-1] == '|'){
                printf("Error: missing command\n");
                return -1;
        }

        if (strcmp(inputArgs[size-1], ">") == 0 || inputArgs[size -1][argLength-1] == '>'){
                printf("Error: no output file\n");
                return -1;
        }

        if ( size >= 16) {
                printf("Error: too many process arguments\n");
                return -1;
        }

        return 0;
}

/**
* @brief execute non-builtin commands from input
* @param pointer to array of arguments from input
* @param original command line from input
* @return exit status of the execution of the command
*/
int execCommand(char **inputArgs, char cmd[])
{
        int retval;
        pid_t pid = fork();

        /*for child*/
        if(pid == 0) {
                execvp(inputArgs[0], inputArgs);
                fprintf(stderr, "Error: command not found\n");
                exit(EXIT_FAILURE);
        }

        /*for parent*/
        else if(pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                retval = WEXITSTATUS(status);
        }
        return retval;
}

/**
* @brief get current directory and print it out
*/
void pwd()
{
        char buffer[256];
        getcwd(buffer, sizeof(buffer));
        printf("%s\n", buffer);
}

/**
* @brief changes directory
* @param pointer to array of arguments from input
* @param size of the array
* @return -1 if errors where found and 0 otherwise
*/
int cd(char** inputArgs, int size)
{
        /*initialize variables with 2 so there is no garbage in it*/
        int chdirError = 2;
        int returnVal = 2;

        /*make sure the cd command only has 2 arguments*/
        if (size > 2 ) {
                printf("Error: too many process arguments\n");
                returnVal = -1;
        }

        /*if there is no argument for cd, we cd into home directory*/
        if (size == 1){ //strcmp(inputArgs[1], "~/"
                chdirError = chdir(getenv("HOME"));
                returnVal = 0;
        }

        /*if there are 2 arguments present pass them to chdir()*/
        if (size == 2) {
                chdirError = chdir(inputArgs[1]);
                returnVal = 0;
        }

        /*print error if we are unable to cd into directory*/
        if (chdirError == -1) {
                printf("Error: cannot cd into directory\n");
                returnVal = 1;
        }

        return returnVal;
}

/**
* @brief prints the files in current directory with their byte size
* @return return 1 if unable to open directory and 0 otherwise
*/
int sls()
{
        DIR *curDirectory;
        struct stat directoryStat;
        struct dirent *dp;

        /*open current directory*/
        curDirectory = opendir(".");
        if (curDirectory == NULL) {
                printf("Error: cannot open directory\n");
                return 1;
        }

        /*iterate through all the files in directory*/
        while ((dp = readdir(curDirectory)) != NULL) {
                /*skip over files that start with '.'*/
                if (dp->d_name[0] == '.')
                        continue;

                stat(dp->d_name, &directoryStat);
                printf("%s (%lld bytes)\n", dp->d_name, (long long) directoryStat.st_size);
        }

        closedir(curDirectory);

        return 0;
}

/**
* @brief helper function for piping that executes commands
* @param pointer to array of of arguments to be executed
* @param first file desrciptor
* @param second file descriptor
* @return the exit value of the executed argument
*/
int execPiping(char **args, int fd1, int fd2)
{
        pid_t pid = fork();
        int exitVal;

        /*for child*/
        if (pid == 0) {
                /*redirect input to STDIN if necessary*/
                if (fd1 != STDIN_FILENO) {
                        dup2(fd1, STDIN_FILENO);
                        close(fd1);
                }

                /*redirect output to STDOUT if necessary*/
                if (fd2 != STDOUT_FILENO) {
                        dup2(fd2, STDOUT_FILENO);
                        close(fd2);
                }

                /*execute the command*/
                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
        }
        /*for parent*/
        else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                exitVal = WEXITSTATUS(status);
        }

        return exitVal;
}

/**
* @brief function to execute piping commands
* @param pointer to array of commands from input
* @param number of commands
* @param pointer to original command from input
*/
void piping(char **commands, int numCmd, char *inputCmd)
{
        int input_fd = STDIN_FILENO;
        int pipe_fds[2];
        int exitVals[numCmd];
        char **inputArg;
        char *tokenChar = " ";

        for (int i = 0; i < numCmd; i++) {
                /*create a pipe (except for the last command)*/
                if (i < numCmd - 1) {
                        if (pipe(pipe_fds) == -1) {
                                perror("pipe");
                                exit(EXIT_FAILURE);
                        }
                }

                /*if last command, connect output to STDOUT*/
                if (i >= numCmd - 1) {
                        pipe_fds[1] = STDOUT_FILENO;
                }

                /*tokenize commands by delimiter " " to execute the command*/
                inputArg = tokenizeCmd(commands[i], tokenChar);
                exitVals[i] = execPiping(inputArg, input_fd,  pipe_fds[1]);
                free(inputArg);

                /*close the write end of the pipe if it's not the last command*/
                if (i < numCmd - 1) {
                        close(pipe_fds[1]);
                }

                /*save the read end of the pipe for the next iteration*/
                if (i < numCmd - 1) {
                        input_fd = pipe_fds[0];         /*Read from the pipe*/
                }
                else {
                        input_fd = STDIN_FILENO;        /*Read from STDIN input*/
                }
        }

        /*print completion message to stderr*/
        fprintf(stderr, "+ completed '%s' ", inputCmd);

        /*print exit value for each command*/
        for (int i = 0; i < numCmd; i++) {
                fprintf(stderr, "[%d]", exitVals[i]);
        }
        fprintf(stderr, "\n");

}

/**
* @brief check if user has permission to access file
* @param name of the file
* @return -1 if the user doesn't have permision and 0 otherwise
*/
int filePermissions(char fileName[])
{
        /*check if file exists*/
        if (access(fileName, F_OK) != -1) {
                /*check if we have access to the file*/
                if (access(fileName, R_OK | W_OK) == 0) {}
                else {
                        return -1;
                }
        }

        /*file does not exist*/
        else {
                /*check if we have access to directory*/
                const char *directory = ".";
                if (access(directory, R_OK | W_OK | X_OK) == 0) {}
                else {
                        return -1;
                }
        }

        return 0;
}

/**
* @brief redirects input to a file
* @param original command from input
* @param name of the file
* @param value representing if we should append to file or not
* @return the original command line up until the redirection character
*/
char *outputRedirect(char cmd[], int append)
{
        static char cmd1[CMDLINE_MAX];
        strcpy(cmd1, cmd);
        char *tokenChar = ">";

        /*change token character if we are appending*/
        if (append == 1) {
                tokenChar = ">>";
        }
        char **inputArgsRedirect = tokenizeCmd(cmd1, tokenChar);
        int fd;

        /*check if redirection sign was misplaced*/
        if (strchr(inputArgsRedirect[1], '|')) {
                free(inputArgsRedirect);
                return "-2";
        }

        tokenChar = " ";
        char **file = tokenizeCmd(inputArgsRedirect[1], tokenChar);
        char *fileName = file[0];
        /*open file and append to it if it already exist*/
        if(append == 1) {
                fd = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0644);
        }

        /*open file and ovewrite it if it already exists*/
        else {
                fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }

        dup2(fd, STDOUT_FILENO);        /*redirects stdout to file*/
        close(fd);

        /*free allocated memory*/
        free(file);
        free(inputArgsRedirect);
        free(fileName);

        return cmd1;
}

/**
* @brief parse the command line from input and call relevant functions and print errors
* @param command line from input
*/
void processInput(char cmd[])
{
        /*check if the command line has no character except space*/
        if (strspn(cmd, " \t\n\r\f\v") == strlen(cmd)) {
                printf("Error: missing command\n");
                return;
        }

        int returnVal = 0;
        int checkPipe = 0;

        /*copy cmd*/
        char cmd1[CMDLINE_MAX];
        strcpy(cmd1, cmd);
        char cmd3[CMDLINE_MAX];
        strcpy(cmd3, cmd);

        /*declare struct to store comand args array and size*/
        struct Comand *inputArg = malloc(sizeof(struct Comand));
        /*seperate input into array by spaces*/
        char *tokenChar = " ";
        inputArg->args = tokenizeCmd(cmd, tokenChar);
        /*compute size of args*/
        for (inputArg->size = 0; inputArg->args[inputArg->size] != NULL; inputArg->size++) {}

        /*copy file redirection value for STDOUT in case we redirect output*/
        int originalSTOUT = dup(STDOUT_FILENO);

        /*check input for error if any is found, return*/
        if (parseErrors(inputArg->args, inputArg->size) == -1) {
                deallocateComand(inputArg);
                return;
        }

        /*check if input has append redirection character*/
        int append;
        if (strstr(cmd3, ">>")) {
                append = 1;
        }

        /*check if input has redirection sign*/
        if (strchr(cmd3, '>') != NULL) {
                /*check for errors*/
                if (filePermissions(inputArg->args[inputArg->size-1]) == -1){
                        printf("Error: cannot open output file\n");
                        deallocateComand(inputArg);
                        return;
                }

                /*call outputRedirection and copy the rest of the command into cmd2 to be executed*/
                char cmd2[CMDLINE_MAX];
                strcpy(cmd2, outputRedirect(cmd3, append));
                if (strstr(cmd2, "-2")) {
                        printf("Error: mislocated output redirection\n");
                        deallocateComand(inputArg);
                        return;
                }
                strcpy(cmd3, cmd2);

                /*break up left over command by spaces*/
                free(inputArg->args);
                tokenChar = " ";
                inputArg->args = tokenizeCmd(cmd2, tokenChar);
        }

        /*check for piping character*/
        if (strchr(cmd3 , '|') != NULL) {
                tokenChar = "|";
                int numCmd = 0;
                checkPipe = 1;

                /*parse input and count the number of pipes*/
                for(int j = 0; cmd3[j] != '\0' ; j++) {
                        if(cmd3[j] == '|') {
                                numCmd++;
                        }
                }

                /*split command line by '|' sign*/
                char **pipeArgs = tokenizeCmd(cmd3, tokenChar);
                piping(pipeArgs, numCmd+1, cmd1);

                /*free allocated memory*/
                free(pipeArgs);
        }

        /*execute other commands*/
        else if (strcmp(inputArg->args[0], "pwd") == 0) {
                pwd();
                returnVal = 0;
        }
        else if (strcmp(inputArg->args[0], "cd") == 0) {
                returnVal = cd(inputArg->args, inputArg->size);
        }
        else if (strcmp(inputArg->args[0], "sls") == 0) {
                returnVal == sls();
        }
        else {
                returnVal = execCommand(inputArg->args, cmd);
        }

        /*redirects stdout back to terminal incase it redirected earlier*/
        dup2(originalSTOUT, STDOUT_FILENO);

        /*print completion message given no errors where found from cd function*/
        if (returnVal != -1 && checkPipe != 1) {
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd1, returnVal);
        }

        deallocateComand(inputArg);
}


int main(void)
{
        /*input string as a array of characters*/
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;

                /* print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed '%s' [0]\n", cmd);
                        break;
                }

                /*process input if exit was not called*/
                else {
                        processInput(cmd); /*calls the process input function*/
                }
        }

        return EXIT_SUCCESS;
}