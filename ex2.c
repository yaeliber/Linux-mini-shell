
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define CMD_SIZE 510
#define WRITE 1
#define READ 0

pid_t pid;
pid_t leftPid, rightPid;

int NumOfRedirection = 0;
int NumOfPipe = 0;
int redirectionFlag = 0;

void executeRedirectionOut(char **argv, char *secondCmd, int flagdir, int flagErr);
void executeRedirectionIn(char **argv, char *secondCmd);

int getNumOfArgs(char *cmd)
{ //This function counts the number of arguments in the string cmd
    char temp[CMD_SIZE];
    strcpy(temp, cmd);
    int counter = 0;

    char *ptr = strtok(temp, " \n");
    while (ptr != NULL)
    {
        counter++;
        ptr = strtok(NULL, " \n"); //If you want to switch to space you have to decrease \ N at the end of the input
    }
    return counter;
}

void fillArgs(char **argv, char *cmd)
{ // This function disassemble the arguments from the "cmd" String and fills the argv array with them.
    int i = 0;
    char temp[CMD_SIZE];
    strcpy(temp, cmd);
    char *ptr = strtok(temp, " \n");
    while (ptr != NULL)
    {
        argv[i] = (char *)malloc(strlen(ptr) + 1);
        assert(argv[i] != NULL);
        strcpy(argv[i], ptr);
        argv[i][strlen(ptr)] = '\0';
        i++;
        ptr = strtok(NULL, " \n");
    }
    argv[i] = NULL;
}

void deleteArgs(char **argv)
{ // This function deletes the argv array and all it's Strings
    int i = 0;
    while (argv[i] != NULL)
    {
        free(argv[i]);
        i++;
    }
    free(argv);
}

void printArgs(char **argv)
{ // This function prints the arguments array (for debugging)
    if (argv == NULL)
    {
        printf("Command array is null\n");
        return;
    }
    int i = 0;
    while (argv[i] != NULL)
    {
        printf("argument %d: |%s|\n", i + 1, argv[i]);
        i++;
    }
}

char *isContaining(char *cmd, char tav)
{ // This function checks if the command has a tav in it and returns the place of the right args else return null
    int i = 0;
    while (cmd[i] != '\n')
    {
        // if(cmd[i]=='"')
        // {// " " Bonus
        //     i++;
        //     while(cmd[i] != '"')
        //     {
        //         if(cmd[i] != '\n')
        //         return NULL;
        //     i++;
        //     }
        // }
        if (cmd[i] == tav)
        {
            cmd[i] = '\0';
            return (cmd + i + 1);
        }
        i++;
    }
    return NULL;
}

char *isPipe(char *cmd)
{ // This function checks if the command has a pipe in it and returns the place of the second args else return null
    return isContaining(cmd, '|');
}

char *isRedirectionErrOut(char *cmd)
{ // This function checks if the command has redirectionErrOut in it and returns the place of the right char else return null
    int i = 0;
    while (cmd[i] != '\n')
    {
        // if(cmd[i]=='"')
        // {// " " Bonus
        //     i++;
        //     while(cmd[i] != '"')
        //     {
        //         if(cmd[i] != '\n')
        //         return NULL;
        //     i++;
        //     }
        // }
        if (cmd[i] == '>')
        { //Assuming the command(cmd) does not start with>

            if (cmd[i - 1] == '2')
            {
                cmd[i - 1] = '\0';
                cmd[i] = '\0';
                return (cmd + i + 1);
            }
        }
        i++;
    }
    return NULL;
}

char *isRedirectionOut(char *cmd)
{ // This function checks if the command has redirectionOut in it and returns the place of the right char else return null
    return isContaining(cmd, '>');
}

char *isRedirectionIn(char *cmd)
{ // This function checks if the command has redirectionIn in it and returns the place of the right char else return null
    return isContaining(cmd, '<');
}

void executeSingle(char **argv)
{ // This function executes a son process with a singular command

    if (strcmp(argv[0], "cd") == 0)
    {
        printf("Command not supported (Yet)\n");
        deleteArgs(argv);
    }
    else
    {
        pid = fork();
        if (pid < 0)
        { //error occurred while forking
            printf("ERROR: Could not open a new proccess\nThe program will now terminate.\n");
            exit(1);
        }
        else if (pid == 0)
        { //son gets the command and attempting to execute it.
            execvp(argv[0], argv);
            deleteArgs(argv);
            //printf("ERROR: Problem encountered when trying to execute the command\nThe command was not executed.\n");
            exit(1); // exit with error in case execvp failed
        }
        else
        { // parent waiting for son to finish
            wait(NULL);
            //checking?
        }
    }
}

void executePipe(char **argv, char **secondArgv, int redirectionFlag, char *secondCmd2)
{ //This function executes two son procceses with a pipe connecting them
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("cannot open pipe\n");
        exit(1);
    }
    if ((leftPid = fork()) < 0)
    {
        printf("ERROR: Could not open a new proccess\nThe program will now terminate.\n");
        exit(1);
    }
    else if (leftPid == 0)
    {
        close(pipe_fd[READ]);
        dup2(pipe_fd[WRITE], STDOUT_FILENO);
        execvp(argv[0], argv);
        printf("ERROR: Problem encountered when trying to execute the left command of the pipe\nThe command was not executed.\n");
        exit(1);
    }
    else
    {
        if ((rightPid = fork()) < 0)
        {
            printf("ERROR: Could not open a new proccess\nThe program will now terminate.\n");
            exit(1);
        }
        else if (rightPid == 0)
        {
            close(pipe_fd[WRITE]);
            dup2(pipe_fd[READ], STDIN_FILENO);
            if (redirectionFlag != 0) //pipe & redirection
            {
                if (redirectionFlag == 2) //>>
                {
                    executeRedirectionOut(secondArgv, secondCmd2, 1, 0);
                    exit(0);
                }
                else if (redirectionFlag == 3) //2>
                {
                    executeRedirectionOut(secondArgv, secondCmd2, 0, 1);
                    exit(0);
                }
                else if (redirectionFlag == 1) //>
                {
                    executeRedirectionOut(secondArgv, secondCmd2, 0, 0);
                    exit(0);
                }
                else //redirectionFlag == 4 <
                {
                    executeRedirectionIn(secondArgv, secondCmd2);
                    exit(0);
                }
            }
            else
            {
                execvp(secondArgv[0], secondArgv);
                printf("ERROR: Problem encountered when trying to execute the right command of the pipe\nThe command was not executed.\n");
                exit(1);
            }
        }
        else
        {
            close(pipe_fd[WRITE]);
            close(pipe_fd[READ]);
            wait(NULL);
            wait(NULL);
        }
    }
}
void executeRedirectionOut(char **argv, char *secondCmd2, int flagdir, int flagErr) //if flagdir=1 >>, flagErr=1 2>
{
    while (secondCmd2[0] == ' ') //No file will be created with a space before
        secondCmd2++;

    pid = fork();
    if (pid < 0)
    { //error occurred while forking
        printf("ERROR: Could not open a new proccess\nThe program will now terminate.\n");
        exit(1);
    }
    else if (pid == 0)
    { //son gets the command and attempting to execute it.
        int fd;
        if (flagdir == 1) //>>
            fd = open(secondCmd2, O_RDWR | O_CREAT | O_APPEND, S_IRWXU | S_IRGRP | S_IROTH);

        else
            fd = open(secondCmd2, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);

        if (fd == -1)
        {
            perror("cannot open file");
            exit(1);
        }
        if (flagErr == 1) //2>
            dup2(fd, STDERR_FILENO);
        else
            dup2(fd, STDOUT_FILENO);

        execvp(argv[0], argv);

        printf("ERROR: Problem encountered when trying to execute with redirection\nThe command was not executed.\n");
        deleteArgs(argv);
        exit(1); // exit with error in case execvp failed
    }
    else
    { // parent waiting for son to finish
        wait(NULL);
        //checking?
    }
}
void executeRedirectionIn(char **argv, char *secondCmd)
{
    while (secondCmd[0] == ' ') //No file will be created with a space before
        secondCmd++;

    pid = fork();
    if (pid < 0)
    { //error occurred while forking
        printf("ERROR: Could not open a new proccess\nThe program will now terminate.\n");
        exit(1);
    }
    else if (pid == 0)
    { //son gets the command and attempting to execute it.
        int fd;
        fd = open(secondCmd, O_RDONLY, 0);
        if (fd == -1)
        {
            perror("cannot open file");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        execvp(argv[0], argv);
        deleteArgs(argv);
        printf("ERROR: Problem encountered when trying to execute with redirection\nThe command was not executed.\n");
        exit(1); // exit with error in case execvp failed
    }
    else
    { // parent waiting for son to finish
        wait(NULL);
        //checking?
    }
}

int main(int argc, char *argv1[])
{
    int fd = open(argv1[1], O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);
    if (fd == -1)
    {
        perror("cannot open file");
        exit(1);
    }
    pid_t pid;          //used to different the father process from the son process.
    char **argv = NULL; //String array used to store the bash command for the son process.
    char **secondArgv = NULL;
    int NumOfCommands = 0, CommandsLength = 0;
    int flagredi = 0; //>> false=0

    int pipeFlag = 0, RedirectionFlagOut = 0, RedirectionFlagIn = 0, RedirectionFlagErrOut = 0;
    char *loginName; // pointer to String of the login name(User name)
    struct passwd *st;
    st = getpwuid(getuid());
    loginName = st->pw_name;
    // if (loginName == NULL) // In case login name was not recieved, "login" will be the value.
    //     loginName = "login";

    char currentFolder[1000];
    getcwd(currentFolder, sizeof(currentFolder));
    if (currentFolder == NULL)
    {
        perror("getcwd() error");
        return 1;
    }

    char *cmd = (char *)malloc(CMD_SIZE); //stores the String of the command for the son process.
    assert(cmd != NULL);
    char *secondCmd;
    char *cmdSave = cmd; //saves the allocation pointer before spacesString
    char *secondCmd2;

    //First command line
    printf("%s@%s>", loginName, currentFolder);
    fgets(cmd, CMD_SIZE, stdin);
    int place = strlen(cmd) - 1;
    write(fd, cmd, strlen(cmd));
    if (strcmp(cmd, "\n") != 0)
    {
        NumOfCommands++;
        CommandsLength = CommandsLength + strlen(cmd) - 1;
    }

    // the mini shell terminates when the command "done" is entered.
    while (strcmp(cmd, "done\n") != 0)
    {

        if ((secondCmd = isPipe(cmd)) != NULL)
        { //There is a pipe
            secondArgv = (char **)malloc((getNumOfArgs(secondCmd) + 1) * sizeof(char *));
            assert(secondArgv != NULL);
            NumOfPipe++;
            fillArgs(secondArgv, secondCmd);
            pipeFlag = 1; //true=1
                          //
        }
        if (secondCmd == NULL)
            secondCmd = cmd;

        if ((secondCmd2 = isRedirectionErrOut(secondCmd)) != NULL)
        {
            NumOfRedirection++;
            RedirectionFlagOut = 1;
            RedirectionFlagErrOut = 1;
            redirectionFlag = 3;
        }

        else if ((secondCmd2 = isRedirectionOut(secondCmd)) != NULL)
        {                           //There is < redirection
            RedirectionFlagOut = 1; //true
            if (secondCmd2[0] == '>')
            {
                flagredi = 1;
                secondCmd2[0] = '\0';
                secondCmd2 = secondCmd2 + 1;
                redirectionFlag = 2;
            }
            else
                redirectionFlag = 1;

            NumOfRedirection++;
        }
        else if ((secondCmd2 = isRedirectionIn(secondCmd)) != NULL)
        {
            RedirectionFlagIn = 1; //true
            NumOfRedirection++;
            redirectionFlag = 4;
        }

        if (redirectionFlag != 0 && pipeFlag == 1) //pipe & rederiction
        {                                          //Delete 2 arguments in an array
                                                   // printArgs(secondArgv);
            NumOfRedirection--;
            int countWords = 0;
            char **temp;
            while (secondArgv[countWords] != NULL)
                countWords++;

            temp = (char **)malloc(sizeof(char *) * (countWords - 1));
            int i = 0;
            while (i < countWords - 2)
            {
                temp[i] = secondArgv[i];
                i++;
            }
            temp[i] = NULL;

            secondCmd2 = secondArgv[i + 1]; //free
            free(secondArgv[i]);
            free(secondArgv);
            secondArgv = temp;
        }

        argv = (char **)malloc((getNumOfArgs(cmd) + 1) * sizeof(char *)); //allocating the exact amount of bytes for argv.
        assert(argv != NULL);
        fillArgs(argv, cmd);
        //printArgs(argv);

        if (argv[0] == NULL || strcmp(cmd, "\n") == 0)
        {
        }
        else
        {
            cmd = cmdSave;
            cmd[place] = '\0'; //cmd[strlen(cmd)-1];
            if (pipeFlag == 1)
                executePipe(argv, secondArgv, redirectionFlag, secondCmd2);
            else if (RedirectionFlagOut == 1)
                executeRedirectionOut(argv, secondCmd2, flagredi, RedirectionFlagErrOut);
            else if (RedirectionFlagIn == 1)
                executeRedirectionIn(argv, secondCmd2);
            else
                executeSingle(argv);
        }
        deleteArgs(argv); // deleting the allocated memory for argv since it gets new allocation every iteration of the loop.
        if (pipeFlag == 1)
        {
            deleteArgs(secondArgv);
            secondArgv = NULL;
        }
        if(redirectionFlag != 0 & pipeFlag == 1) 
        { 
         free(secondCmd2);
         secondCmd2=NULL;
        }

        pipeFlag = 0; //false
        RedirectionFlagOut = 0;
        RedirectionFlagIn = 0;
        RedirectionFlagErrOut = 0;
        redirectionFlag = 0;
        flagredi = 0;

        printf("%s@%s>", loginName, currentFolder);
        cmd = cmdSave;
        fgets(cmd, CMD_SIZE, stdin);
        place = strlen(cmd) - 1;
        write(fd, cmd, strlen(cmd));
        if (strcmp(cmd, "\n") != 0)
        {
            NumOfCommands++;
            CommandsLength = CommandsLength + strlen(cmd) - 1;
        }
    }
    free(cmd);
    close(fd);

    printf("Num of commands: %d\n", NumOfCommands);
    printf("Total length of all commands: %d\n", CommandsLength);
    double average = (double)CommandsLength / NumOfCommands;
    printf("Average length of all commands: %.6f\n", average);
    printf("Number of command that include pipe: %d\n", NumOfPipe);
    printf("Number of command that include redirection: %d\n", NumOfRedirection);
    printf("See you Next time !\n");

    return 0;
}
