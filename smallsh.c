#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

#define INPUT_LENGTH 2048
#define MAX_ARGS 512

int lastStatus;

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

int display_status(){
    if(WIFEXITED(lastStatus)){
        printf("exit value %d\n", WEXITSTATUS(lastStatus));
    } else if (WIFSIGNALED(lastStatus)){
        printf("terminated by signal %d\n", WTERMSIG(lastStatus));
    };
};

int main()
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
            // TODO:ensure child processes are terminated
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(command, "cd") == 0)
        {

            change_dir(curr_command);

            // Testing path
            long size;
            char *buf;
            char *ptr;

            size = pathconf(".", _PC_PATH_MAX);

            if ((buf = (char *)malloc((size_t)size)) != NULL)
                ptr = getcwd(buf, (size_t)size);
            printf("Changed dir to: %s \n", ptr);
        }
        else if (strcmp(command, "status") == 0)
        {
            printf("status called \n");
        }
        else
        {
            printf("Default reached");
            // printf("default reached \n");
            // pid_t spawnPid = fork();
            // int childStatus;

            // switch (spawnPid)
            // {
            // case -1:
            //     /* code */
            //     perror("fork()\n");
            //     break;
            // case 0:
            //     printf("CHILD(%d) running command %s", getpid(), command);
            //     execv(command, curr_command->argv);
            //     perror("execve");
            //     exit(2);
            //     break;
            // default:
            //     spawnPid = waitpid(spawnPid, &childStatus, 0);
            //     printf("PARENT(%d): child(%d) terminated.", getpid(), spawnPid);
            //     break;
        };
    };
    return EXIT_SUCCESS;
};

