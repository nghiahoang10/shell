#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include "shell.hh"

int yyparse(void);
void yyrestart(FILE * file);
int yylex_destroy(void);
extern struct TrieNode * root;
extern struct TrieNode * getNode();
extern void insert(struct TrieNode * root, char * key);
extern bool isLastNode(struct TrieNode * root);
extern bool hasOneChile(struct TrieNode * root);
extern bool tab(struct TrieNode * root);

void Shell::prompt() {
    char * prompt = getenv("PROMPT");
    if (prompt) {
        printf("%s ", prompt);
    } else {
        printf("myshell>");
    }
    fflush(stdout);
}

extern "C" void ctrlc_handler(int sig) {
    printf("\n");
    if (Shell::_currentCommand._simpleCommands.size() > 0) {
        Shell::_currentCommand.clear();
        return;
    }
    Shell::prompt();
}

extern "C" void killzombie_handler(int sig) {
    wait3(0,0,NULL);
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char ** argv) {
    if (root) {
        free(root);
    }
    /*DIR * dir;
    struct dirent * ent;
    root = getNode();
    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] != '.') {
                insert(root, ent->d_name);
            }
        }
        closedir(dir);
    }*/
    struct sigaction ctrlc;
    ctrlc.sa_handler = ctrlc_handler;
    sigemptyset(&ctrlc.sa_mask);
    ctrlc.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &ctrlc, NULL)) {
        perror("sigaction");
        exit(2);
    }
    struct sigaction killzombie;
    killzombie.sa_handler = killzombie_handler;
    sigemptyset(&killzombie.sa_mask);
    killzombie.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &killzombie, NULL)) {
        perror("sigaction");
        exit(2);
    }
    char * path = realpath(argv[0], NULL);
    setenv("SHELL", path, 1);
    free(path);
    FILE * shellrc = fopen(".shellrc", "r");
    if (shellrc) {
        yyrestart(shellrc);
        yyparse();
        yyrestart(stdin);
        fclose(shellrc);
    } else if (isatty(0)) {
        Shell::prompt();
    }
    yyparse();
}

Command Shell::_currentCommand;
