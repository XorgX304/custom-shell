#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/history.h>

#include "utils.h"
#include "exec.h"
#include "vector.h"

//PER TUTTA LA ROBA DEGLI ALIAS MEGLIO FARE UNA NUOVA LIBRERIA PRIMA O POI

vector vector_alias;

typedef struct elemento {
    char* name;
    char* data;
} elemento;

void vector_alias_initializer() {
    vector_init(&vector_alias);
}

char* parse_alias(char* comando) {
    char *init = (char*)malloc(sizeof(char)*(strlen(comando)));
    strcpy(init, comando);
    char* token = strtok(comando, " ");
    char *ns;
    int ok = 0;
    for(int i=0;i<vector_total(&vector_alias);i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_alias, i);
        
        if(strcmp(token, tmp->name) == 0) {
            ok = 1;
            ns = (char*)malloc(sizeof(char)*(strlen(comando) + strlen(tmp->data)));
            strcpy(ns, tmp->data);
            token = strtok(NULL, " ");
            while(token) {
                strcat(ns, " ");
                strcat(ns, token);
                token = strtok(NULL, " ");
            }
            strcat(ns, "\0");
            break;
        }
        
    }
    
    if(ok == 0) return init;
    else return ns;
}

void list_alias() {
    elemento* tmp;
    for(int i=0;i<vector_total(&vector_alias);i++) {     
        tmp = (elemento*)vector_get(&vector_alias, i);
        printf("%s = %s\n", tmp->name, tmp->data);
    }
}

/*
    Funzionamento:

    exec_line:
        Riceve la linea di comando scritta dall'utente e separa sottocomandi e pipe.
        Una volta estratti le singole "righe di comando" chiama exec_cmd (vedi sotto).
        Es: exec_line("ls | wc") --> exec_line("ls") e exec_line("wc")

    exec_cmd:
        Separa la linea di comando in comando e argomenti, se il comando è una funzione
        interna la gestisce, altrimeni chiama fork_cmd (vedi sotto).
        Es: exec_cmd("ls -l -a") --> fork_cmd(["ls", "-l", "-a"])

    fork_cmd:
        Crea un figlio che esegue il comando con gli argomenti passati.
*/


/*
    Elabora la linea di input dell'utente.
    Se c'è un |, chiama due exec_line, altrimenti chiama fork_cmd e logga.

    Returns:
        Struct con le info sul processo
*/
struct PROCESS exec_line(char* line, int cmd_id, int* subcmd_id, int log_out, int log_err) {
    // printf("----    Exec line: %s\n", line);

    line = parse_alias(line);

    // Cerco dal fondo se c'è un pipe
    int i;
    for (i = strlen(line); i >= 0; i--)
        if (line[i] == '|')
            break;

    if (i >= 0) { // C'è un | nella posizione i
        struct PROCESS child1, child2;
        line[i] = '\0';
        child1 = exec_line(line, cmd_id, subcmd_id, log_out, log_err);
        child2 = exec_line(line+(i+1), cmd_id, subcmd_id, log_out, log_err);

        // Leggi stdout di child1 e scrivilo su stdin di child2
        write_to(child1.stdout, log_out, child2.stdin);

        close(child1.stdout);
        close(child1.stdin);
        close(child1.stderr);
        close(child2.stdin);

        return child2;
    } else { // Non c'era nessun pipe
        (*subcmd_id)++;
        return exec_cmd(line, cmd_id);
    }
}


/*
    Separa la linea di comando in un array di argomenti. Controlla se il
    comando è una funzione interna.
    TODO magari espandendo variabili e percorsi
*/
struct PROCESS exec_cmd(char* line, int cmd_id) {
    // printf("----    exec_cmd #%d.%d: %s\n", cmd_id, subcmd_id, line);
    // Separo comando e argomenti
    char *copy_line = (char*) malloc(sizeof(char) * strlen(line));
    strcpy(copy_line, line);
    int spazi = 0, i = 0;
    for (int c = 0; line[c] != '\0'; c++) if (line[c] == ' ') spazi++;
    char* args[spazi+1]; // L'ultimo elemento dev'essere NULL
    char* token = strtok(line, " ");
    while (token) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL;

    // Controllo se è un comando che voglio gestire internamente
    // TODO anche questi devono avere I/O su file?
    if (strcmp(args[0], "clear") == 0)
        clear();
    else if (strcmp(args[0], "exit") == 0)
        exit(0); // shell_exit(0);
    else if (strcmp(args[0], "help") == 0)
        print_help();
    else if (strcmp(args[0], "alias") == 0) {
        char *tmp = args[1];
        if(tmp == NULL) {
            list_alias();
        } else {
            char *alias = (char*)malloc(sizeof(char) * strlen(line));
            char *content = (char*)malloc(sizeof(char) * strlen(line));
            int active = 0;
            int first = 1;
            int alias_index = 0;
            int content_index = 0;
            for(int i=0;copy_line[i]!='\0';i++) {
                if(active == 1 && first == 1) alias[alias_index++] = copy_line[i];
                if(active == 1 && first == 0) content[content_index++] = copy_line[i];
                if(copy_line[i] == '\'' && active == 0) active = 1;
                else if(copy_line[i] == '\'' && active == 1) {
                    active = 0;
                    first = 0;
                }
            }
            alias[alias_index-1] = '\0';
            content[content_index-1] = '\0';
            /*
            printf("Alias: %s\n", alias);
            printf("Content: %s\n", content);
            */
            
            if(strlen(alias) == 0 || strlen(content) == 0) {
                printcolor("! Errore: formato \'alias\'=\'command\'\n", KRED);
            } else {
                elemento *insert = (elemento*)malloc(sizeof(elemento));
                insert -> name = alias;
                insert -> data = content;

                vector_add(&vector_alias, insert);
            }
            
        }
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        int status = chdir(args[1]);
        if(status == -1) {
            printcolor("! Errore: cartella inesistente\n", KRED);
        }
    }
    else if (strcmp(args[0], "history") == 0)
    {
        
        HIST_ENTRY** hist = history_list();
        char* hist_arg = strtok(NULL, " ");
        int n; // Quanti elementi della cronologia mostrare

        // Se non è specificato li mostro tutti
        if (hist_arg == NULL) n = history_length;
        else n = min(atoi(hist_arg), history_length);

        for (int i = history_length - n; i < history_length; i++)
            printf("  %d\t%s\n", i + history_base, hist[i]->line);
        
    }
    else {
        // È un comando shell
        return fork_cmd(args);
    }
    return exec_cmd(";", cmd_id);
}


/*
    Esegui il vettore di argomenti come comando in un figlio
        Params: Array degli argomenti, il primo elemento sarà il comando
        Returns: Struct PROCESS con info sul figlio
*/
struct PROCESS fork_cmd(char** args) {
    // printf("----    Fork cmd [0] -> %s\n", args[0]);
    // Pipe per comunicare col figlio
    int child_in[2], child_out[2], child_err[2];
    pid_t pid;

    // Habemus Pipe cit.
    if (pipe(child_in)  < 0) perror("Cannot create stdin pipe to child");
    if (pipe(child_out) < 0) perror("Cannot create stdout pipe to child");
    if (pipe(child_err) < 0) perror("Cannot create stderr pipe to child");
    if ((pid = fork())  < 0) perror("Cannot create child");

    if (pid == 0) { // Child
        // Chiudi pipe-end che non servono
        close(child_in[PIPE_WRITE]);
        close(child_out[PIPE_READ]);
        close(child_err[PIPE_READ]);

        // Redirigi in/out/err ed esegui il comando
        dup2(child_in[PIPE_READ],   0);
        dup2(child_out[PIPE_WRITE], 1);
        dup2(child_err[PIPE_WRITE], 2);

        int ret_code = execvp(args[0], args);
        // if (ret_code < 0) perror("Cannot execute command");

        // Chiudi i pipe
        close(child_in[PIPE_READ]);
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);

        exit(ret_code);
    } else { // Parent
        // Chiudi i pipe che non servono
        close(child_in[PIPE_READ]);
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);

        //if(WIFEXITED(status) && WEXITSTATUS(status) != 0) printcolor("! Comando non esistente\n",KRED);
        // wait(NULL);

        struct PROCESS p;
        p.stdin  = child_in[PIPE_WRITE];
        p.stdout = child_out[PIPE_READ];
        p.stderr = child_err[PIPE_READ];
        return p;
    }
}