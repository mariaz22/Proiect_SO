#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>

int maxim_input = 100;
int maxim_cmnd = 100;


// iesire prin Ctrl+Z
void suspend(int sig) {
    printf("\nExiting terminal. Type 'fg' to return!\n");
    raise(SIGSTOP);
}

//split pt comenzile introduse
void split_input(char *input, char **arguments) {
    char *temp;
    int index = 0;

    temp = strtok(input, " \t\n");
    while (temp != NULL && index < maxim_input) {
        arguments[index++] = temp;
        temp = strtok(NULL, " \t\n");
    }
    arguments[index] = NULL;
}

// initializarea terminalului
void initialize_shell() {
    system("clear");
    printf("\n** Welcome to HM shell **\n");
    char *user = getenv("USER");
    printf("User: @%s\n", user);
    sleep(2);
    system("clear");
}

// calea directorului
void cwd_path() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\n%s$ ", cwd);
}

// istoric
int get_input(char *input_buffer) {
    char *buf = readline("Write a command: ");
    if (strlen(buf) != 0) {
        add_history(buf);
        strcpy(input_buffer, buf);
        return 0;
    } else {
        return 1;
    }
}

// executare comanda simpla
int execute_command(char **arg) {
    pid_t pid = fork();
    int status;

    if (pid == -1) {
        printf("\nExecute command fork error\n");
        return -1;
    } else if (pid == 0) {
        if (execvp(arg[0], arg) < 0) {
            printf("\nCommand execution error: %s\n", arg[0]);
            exit(1); 
        }
    } else {
        wait(&status);
    }
    return WEXITSTATUS(status);
}

// executare comenzi cu pipe
void execute_command_pipe(char **cmd1, char **cmd2) {
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) < 0) {
        printf("\nPipe error.\n");
        return;
    }

    pid1 = fork();
    if (pid1 < 0) {
        printf("\nFork pid1 error.\n");
        return;
    }

    if (pid1 == 0) {
        close(pipefd[0]); 
        dup2(pipefd[1], STDOUT_FILENO); 
        close(pipefd[1]);
        if (execvp(cmd1[0], cmd1) < 0) {
            printf("\nError executing the first command.\n");
            exit(1);
        }
    } else {
        pid2 = fork();
        if (pid2 < 0) {
            printf("\nError second child process failed.\n");
            return;
        }

        if (pid2 == 0) {
            close(pipefd[1]); 
            dup2(pipefd[0], STDIN_FILENO); 
            close(pipefd[0]);
            if (execvp(cmd2[0], cmd2) < 0) {
                printf("\nError executing the second command.\n");
                exit(1);
            }
        } else {
            close(pipefd[0]);
            close(pipefd[1]);
            wait(NULL);
            wait(NULL);
        }
    }
}


// vf daca in comanda exista pipe
int vf_pipe(char *input, char **cmd) {
    int i;
    for (i = 0; i < 2; i++) {
        cmd[i] = strsep(&input, "|");
        if (cmd[i] == NULL)
            break;
    }
    return (cmd[1] != NULL); 
}

// vf si executare 
int execute_command_split(char *input) {
    char *cmd[2];
    if (vf_pipe(input, cmd)) {
        char *cmd1[maxim_input], *cmd2[maxim_input];
        split_input(cmd[0], cmd1);
        split_input(cmd[1], cmd2);
        execute_command_pipe(cmd1, cmd2);
    } else {
        char *args[maxim_input];
        split_input(input, args);
        if (args[0] != NULL) {
            return execute_command(args);
        }
    }
    return 0;
}

//executare comenzi pt op logici sau pipe
int execute_complex(char *input_buffer) {
    char *logic_operators[] = {"&&", "||"};
    char *command1 = NULL, *command2 = NULL;
    int logic_operator = -1; 

    for (int i = 0; i < 2; i++) {
        char *temp = strstr(input_buffer, logic_operators[i]);
        if (temp != NULL) {
            logic_operator = i; 
            *temp = '\0'; 
            command1 = input_buffer; 
            command2 = temp + strlen(logic_operators[i]);
            break;
        }
    }

    if (logic_operator == -1) {
        return execute_command_split(input_buffer);
    }

    int result1 = execute_complex(command1);

    if (logic_operator == 0) { 
        if (result1 == 0) {
            return execute_complex(command2);
        }
    } else if (logic_operator == 1) { 
        if (result1 != 0) {
            return execute_complex(command2);
        }
    }

    return result1;
}


void run_shell() {
    char input_buffer[maxim_cmnd];
    bool ok = true;

    while (ok) {
        cwd_path();
        if (get_input(input_buffer)) continue;

        if (strstr(input_buffer, "exit")) {
            ok = false;
            continue;
        }

        execute_complex(input_buffer);
    }
}

int main() {
    signal(SIGTSTP, suspend);
    initialize_shell();
    run_shell();
    return 0;
}
