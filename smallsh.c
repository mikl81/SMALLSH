#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define INPUT_LENGTH 2048
#define MAX_ARGS 512

int lastStatus = 0;

struct command_line
{
    char *argv[MAX_ARGS + 1];
    int argc;
    char *input_file;
    char *output_file;
    bool is_bg;
};
struct command_line *parse_input()
{
    char input[INPUT_LENGTH];
    struct command_line *curr_command = (struct command_line *)calloc(1, sizeof(struct command_line));
    // Get input
    printf(": ");
    fflush(stdout);
    fgets(input, INPUT_LENGTH, stdin);
    // Tokenize the input
    char *token = strtok(input, " \n");
    while (token)
    {
        if (!strcmp(token, "<"))
        {
            curr_command->input_file = strdup(strtok(NULL, " \n"));
        }
        else if (!strcmp(token, ">"))
        {
            curr_command->output_file = strdup(strtok(NULL, " \n"));
        }
        else if (!strcmp(token, "&"))
        {
            curr_command->is_bg = true;
        }
        else
        {
            curr_command->argv[curr_command->argc++] = strdup(token);
        }
        token = strtok(NULL, " \n");
    }
    return curr_command;
}

int change_dir(struct command_line *curr_command)
{
    // Pull the specified path
    const char *path = curr_command->argv[1];
    if (path != NULL) // If a path was provided
    {
        DIR *targetDir = opendir(path);
        if (targetDir)
        {
            if (chdir(path) != 0)
            {
                perror("DIRECTORY ERROR");
                return -1;
            };
        }
        else // If a path was not provided
        {
            perror("DIRECTORY ERROR");
            return -1;
        }
        return 0;
    }
    else
    {
        // Get home dir var and use it as the path
        char *homeDIR = getenv("HOME");
        if (chdir(homeDIR) != 0)
        {
            perror("DIRECTORY ERROR");
            return -1;
        };

        return 0;
    }
};

int execute_external(struct command_line *curr_command)
{
    pid_t childPID;
    int childStatus;
    char *input_file = curr_command->input_file;
    char *output_file = curr_command->output_file;

    // set default for bg without input
    if (input_file == NULL && curr_command->is_bg)
    {
        printf("Setting input to default");
        input_file = "/dev/null";
    };

    // set default for bg without ouput
    if (output_file == NULL && curr_command->is_bg)
    {
        printf("Setting output to default");
        output_file = "/dev/null";
    };

    // Fork child
    childPID = fork();

    switch (childPID)
    {
    case -1:
        /* Fork failed */
        perror("fork()\n");
        exit(1);
    case 0:
        /*Child process*/

        if (curr_command->is_bg == false)
        {
            // TODO: handle sigint
        };

        if (input_file != NULL)
        {
            // handle input redirection
            int sourceFD = open(input_file, O_RDONLY);

            if (sourceFD == -1)
            {
                perror("Error on input open");
                exit(1);
            };

            if (dup2(sourceFD, 0) == -1)
            {
                perror("Error on input file dup2");
                exit(1);
            };

            close(sourceFD);
        };

        if (output_file != NULL)
        {
            // handle output redirection

            int sourceFD = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

            if (sourceFD == -1)
            {
                perror("Error on output open");
                exit(1);
            };

            if (dup2(sourceFD, 1) == -1)
            {
                perror("Error on output file dup2");
                exit(1);
            };

            close(sourceFD);
        };

        // printf("CHILD(%d) running command %s\n", getpid(), curr_command->argv[0]);
        execvp(curr_command->argv[0], curr_command->argv);
        perror("execpe failed");
        exit(1);
        break;
    default:
        /*Parent process*/
        if (curr_command->is_bg)
        {
            printf("background pid is %d", childPID);
            fflush(stdout);
        }
        else
        {
            waitpid(childPID, &childStatus, 0);

            lastStatus = childStatus;
            if (WIFSIGNALED(childStatus))
            {
                printf("terminated by signal %d\n", WTERMSIG(childStatus));
                fflush(stdout);
            }
        };

        // printf("PARENT(%d): child(%d) terminated.\n", getpid(), childPID);
        break;
    };

    return -1;
};

int get_status()
{
    if (WIFEXITED(lastStatus))
    {
        printf("exit value %d\n", WEXITSTATUS(lastStatus));
    }
    else if (WIFSIGNALED(lastStatus))
    {
        printf("terminated by signal %d\n", WTERMSIG(lastStatus));
    }
};

void main()
{
    struct command_line *curr_command;
    while (true)
    {
        curr_command = parse_input(); // Generate curr command struct

        char *command = curr_command->argv[0]; // pull first command arg i.e. command to call

        if (command == NULL || command[0] == '#')
        {
            continue; // Skip blank or comment lines
        }
        else if (strcmp(command, "exit") == 0)
        {
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(command, "cd") == 0)
        {

            change_dir(curr_command);

            // Testing path
            // long size;
            // char *buf;
            // char *ptr;

            // size = pathconf(".", _PC_PATH_MAX);

            // if ((buf = (char *)malloc((size_t)size)) != NULL)
            //     ptr = getcwd(buf, (size_t)size);
            // printf("Changed dir to: %s \n", ptr);
        }
        else if (strcmp(command, "status") == 0)
        {
            get_status();
        }
        else
        {
            execute_external(curr_command);
        };
    };
};
