#ifndef EXEC_H
#define EXEC_H

#define PIPE_READ 0
#define PIPE_WRITE 1

struct PROCESS {
    int stdout,
        stderr,
        stdin;
};

struct PROCESS exec_line(char* line, int cmd_id, int* subcmd_id, int log_out, int log_err);
struct PROCESS exec_cmd(char* line, int cmd_id);
struct PROCESS fork_cmd(char** args);

void vector_alias_initializer();

#endif