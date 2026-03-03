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

/*
Program: SMALLSH
Name: Michael Fitzgibbon Perry
Description: This program functions as a self built linux shell with exit, status, cd created internally and other
    methods handled using exec(). Signals are captured and handled appropriately to provide users with expected
    behavior of a shell
Input: Program requires no input
Output: Program does not provide output outside of terminal
*/

#define INPUT_LENGTH 2048
#define MAX_ARGS 512

int lastStatus = 0;
bool allowBG = true;

struct command_line
{
    char *argv[MAX_ARGS + 1];
    int argc;
    char *input_file;
    char *output_file;
    bool is_bg;
};

/*
Function: parse_input
Author: This function and its associated struct were provided in starter code
Description: Parses users input into our shells command line and generates a command_line struct for output (see above)
Input: User input into our shell command line
Return: struct command_line current_command
*/
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
/*
Function: change_dir
Author: Michael Fitzgibbon Perry
Description: This function changes the current working directory to either the provided path in command struct,
    or the default home path if none is provided
Input: struct command_line current_command
Return: None
*/
void change_dir(struct command_line *curr_command)
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
                return;
            };
        }
        else // If a path was not provided
        {
            perror("DIRECTORY ERROR");
            return;
        }
        return;
    }
    else
    {
        // Get home dir var and use it as the path
        char *homeDIR = getenv("HOME");
        if (chdir(homeDIR) != 0)
        {
            perror("DIRECTORY ERROR");
            return;
        };

        return;
    }
};

/*
Function: handle_SIGINT
Author: Michael Fitzgibbon Perry
Description: This handles SIGINT to prevent it from interrupting the shell
Input: bool allow_interupt - specifies if we handle the SIGINT normally or ignore it
Return: None
*/
void handle_SIGINT(bool allow_interupt)
{
    struct sigaction SIGINT_action = {0};

    if (allow_interupt)
    {
        SIGINT_action.sa_handler = SIG_DFL; // Restore default behavior
    }
    else
    {
        SIGINT_action.sa_handler = SIG_IGN; // Ignore the signal
    };

    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);
};
/*
Function: handle_SIGSTP
Author: Michael Fitzgibbon Perry
Description: This function handles the toggle of the allow BG global bool to allow the toggling of FG only mode
Input: None
Return: None
*/
void handle_SIGTSTP()
{
    if (allowBG)
    {
        char *message = "Entering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, 52);
        allowBG = !allowBG;
    }
    else
    {
        char *message = "Exiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, 31);
        allowBG = !allowBG;
    }
};
/*
Function: execute_external
Author: Michael Fitzgibbon Perry
Description: This function executes the external shell function when the user inputs a function that is not
    one of our built in functions. Allows for &, >, and <. Does not allow for |.
Input: struct command_line current_command
Return: None
*/
void execute_external(struct command_line *curr_command)
{
    pid_t childPID;
    int childStatus;
    char *input_file = curr_command->input_file;
    char *output_file = curr_command->output_file;

    // set default for bg without input
    if (input_file == NULL && curr_command->is_bg)
    {
        input_file = "/dev/null";
    };

    // set default for bg without ouput
    if (output_file == NULL && curr_command->is_bg)
    {
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

        if (curr_command->is_bg == false && allowBG)
        {
            handle_SIGINT(true);
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
        if (curr_command->is_bg && allowBG)
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
};
/*
Function: get_status
Author: Michael Fitzgibbon Perry
Description: This function prints out either the exit status or the terminating signal of the last foreground process ran by the shell.
Input: None
Return: None
*/
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

/*
Function: get_status
Author: Michael Fitzgibbon Perry
Description: This function prints out either the exit status or the terminating signal of the last foreground process ran by the shell.
Input: None
Return: None
*/

void main()
{
    //Prevent CTRL-C behavior
    handle_SIGINT(false);

    // Toggle allow background mode on Ctrl-Z
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    struct command_line *curr_command;
    while (true)
    {
        curr_command = parse_input(); // Generate curr command struct

        char *command = curr_command->argv[0]; // pull first command arg i.e. command to call

        if (command == NULL || command[0] == '#')// Skip blank or comment lines
        {
            continue; 
        }
        else if (strcmp(command, "exit") == 0)// Exit program
        {
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(command, "cd") == 0)// Change the directory
        {
            change_dir(curr_command);
        }
        else if (strcmp(command, "status") == 0)// Print status
        {
            get_status();
        }
        else // Not a built in function, call externally
        {
            execute_external(curr_command);
        };
    };
};
