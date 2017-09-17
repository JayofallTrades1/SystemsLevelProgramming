#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#define MAXSIZE 1024
#define WORDSIZE 64

void  parse(char *line, char **argv);
void run_cmd(char **argv, char mode, int index);
int main(void)
{
    char line[WORDSIZE];
    char *argv[64] = {NULL};
    char mode;
    int index;

    while(1)
    {

        printf("$ ");               //user input
        gets(line);
        parse(line, argv);

        if(strcmp(argv[0], "exit") == 0)
        {
            exit(0);
        }
        for(index = 0; argv[index] != NULL; index++)
        {
            if(strcmp(argv[index], "<") == 0)
            {
                mode = '<';
                index++;
                break;
            }
            else if(strcmp(argv[index], ">") == 0)
            {
                mode = '>';
                index++;
                break;
            }
            else if(strcmp(argv[index], "|") == 0)
            {
                mode = '|';
                break;
            }
            else if(strcmp(argv[index], "&") == 0)
            {
                mode = '&';
                break;
            }
            else if(strcmp(argv[index], "cd") == 0)
            {
                mode = 'c';
                break;
            }

        }
        run_cmd(argv, mode, index);
    }
    return 0;
}
void run_cmd(char **argv, char mode, int index)
{
    int fd;
    pid_t pid;

            if(mode == '<')
            {
                pid = fork();
                if(pid == 0)
                {
                    fd = open(argv[index], O_RDONLY, 0);
                    if (fd < 0)
                    {
                        perror("Couldn't open input file");
                        exit(1);
                    }
                    else
                    {
                        dup2(fd, 0); // dup2() copies content of fd in input of preceeding file
                        close(fd);
                        execvp(argv[0], argv);
                        perror("execvp\n");
                    }
                }
                else if(pid < 0)      //error
                {
                    printf("ERROR: fork() failed! \n");
                    exit(1);
                }
                else    //parent
                {
                    waitpid(pid, NULL, 0); //pick up dead children
                    close(fd);               
                }
            }
            if(mode == '>')
            {
                pid = fork();
                if(pid == 0)
                {
                    fd = open(argv[index], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);   //create file to write to
                    if(fd < 0)
                    {
                        printf("Could not create output file\n");
                        exit(1);
                    }
                    else
                    {
                        dup2(fd, 1);      //now all standard out will be redirected to the file
                        close(fd);
                        execvp(argv[0], argv);
                        perror("execvp\n");
                    }
                }
                else if(pid < 0)
                {
                    printf("ERROR: fork() failed! \n");
                    exit(1);
                }
                else    //parent
                {
                    waitpid(pid, 0, 0); //pick up dead children             
                }
            }
            if(mode == '|')
            {
                int pipefd[2];
                pid_t pid;
                pipe(pipefd);
                pid = fork();
                if(pid == 0)
                {   //child handles right hand side of pipe
                    dup2(pipefd[0], 0);
                    close(pipefd[1]);
                    execvp(argv[index], argv);
                }
                else
                {   //parent handles left hand side of pipe
                    dup2(pipefd[1], 1);
                    close(pipefd[0]);
                    execvp(argv[0], argv);
                }
            }
            if(mode == '&')
            {
                pid_t pid;
                pid = fork();
                if(pid == 0)
                {
                    execvp(argv[0], argv);
                }
                else if(pid < 0)
                {
                    printf("ERROR: fork() failed! \n");
                    exit(1);
                }
                else
                {
                    waitpid(pid, 0, 0);
                }

            }
            if(mode == 'c')
            {
                pid_t pid;
                pid = fork();
                if(pid == 0)
                {
                    char path[MAXSIZE];
                    strcpy(path, argv[1]);
                    char cwd[MAXSIZE];
                    if(argv[1][0] != '/')
                    {
                        getcwd(cwd, sizeof(cwd));
                        strcat(cwd, "/");
                        strcat(cwd, argv[1]);
                        chdir(cwd);
                    }
                    else
                    {
                        chdir(argv[1]);
                    }
                }
                else if(pid < 0)
                {
                    printf("ERROR: fork() failed! \n");
                    exit(1);
                }
                else 
                {
                    waitpid(pid, 0 , 0);
                }
            }
}

void  parse(char *line, char **argv)
{
     while (*line != '\0')
    {       /* if not the end of line ....... */
          while (*line == ' ' || *line == '\t' || *line == '\n')
               *line++ = '\0';     /* replace white spaces with 0    */
          *argv++ = line;          /* save the argument position     */
          while (*line != '\0' && *line != ' ' &&
                 *line != '\t' && *line != '\n')
               line++;             /* skip the argument until ...    */
     }
     *argv = '\0';                 /* mark the end of argument list  */
}
