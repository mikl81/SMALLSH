#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INPUT_LENGTH 2048
#define MAX_ARGS 512
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

int main()
{
    struct command_line *curr_command;
    while (true)
    {
        curr_command = parse_input();

        char* arg = curr_command->argv[0];

        printf("Begin execution of first command %s \n", arg);

        if(arg == NULL || arg[0] == '#'){
            printf("Blank or comment line detected \n");
            continue;
        } else if(strcmp(arg, "exit") == 0){
            printf("exit called \n");
            exit(EXIT_SUCCESS);
        } else if(strcmp(arg, "cd") == 0){
            printf("CD called \n");
        } else if(strcmp(arg, "status") == 0){
            printf("status called \n");
        } else {
            printf("default reached \n");
        };
    }
    return EXIT_SUCCESS;
}


