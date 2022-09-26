/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <bits/stdc++.h>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cstring>
#include <fcntl.h>
#include <string.h>
#include "read-line.h"
#include "command.hh"
#include "shell.hh"
#include "y.tab.hh"
using namespace std;

int yylex_destroy(void);
extern char ** environ;

TrieNode * root;

Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _append = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile ) {
        if (_errFile && _errFile != _outFile) {
            delete _errFile;
        }
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    _errFile = NULL;

    _background = false;

    _append = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        if (isatty(0)) {
            Shell::prompt();
        }
        return;
    }

    // Print contents of Command data structure
    //print();
    
    // Add execution here
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec
    if (strcmp(_simpleCommands[0]->_arguments[0]->c_str(), "exit") == 0) {
        printf("\nGood bye!!\n\n");
        delete yylval.cpp_string;
        exit(0);
    }
    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);
    int fdin;
    if (_inFile) {
        fdin = open(_inFile->c_str(), O_RDONLY, 0666);
    } else {
        fdin = dup(tmpin);
    }
    int ret;
    bool builtin = false;
    int fdout, fderr;
    if (_errFile) {
        if (_append) {
            fderr = open(_errFile->c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
        } else {
            fderr = open(_errFile->c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
        }

    } else {
        fderr = dup(tmperr);
    }
    dup2(fderr, 2);
    close(fderr);
    for (int i = 0; i < _simpleCommands.size(); i++) {
        dup2(fdin, 0);
        close(fdin);
        if (i == _simpleCommands.size() - 1) {
            if (_outFile) {
                if (_append) {
                    fdout = open(_outFile->c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
                } else {
                    fdout = open(_outFile->c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
                }
            } else {
                fdout = dup(tmpout);
            }
            setenv("LAST_ARGUMENT", _simpleCommands[i]->_arguments[_simpleCommands[i]->_arguments.size() - 1]->c_str(), 1);
        } else {
            int fdpipe[2];
            pipe(fdpipe);
            fdout = fdpipe[1];
            fdin = fdpipe[0];
        }
        dup2(fdout, 1);
        close(fdout);
        if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "setenv") == 0) {
            setenv(_simpleCommands[i]->_arguments[1]->c_str(), _simpleCommands[i]->_arguments[2]->c_str(), 1);
            builtin = true;
            ret = 1;
        } else if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "unsetenv") == 0) {
            unsetenv(_simpleCommands[i]->_arguments[1]->c_str());
            builtin = true;
            ret = 1;
        } else if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "cd") == 0) {
            if (_simpleCommands[i]->_arguments.size() == 1) {
                chdir(getenv("HOME"));
            } else {
                if (chdir(_simpleCommands[i]->_arguments[1]->c_str()) == -1) {
                    fprintf(stderr, "cd: can't cd to %s\n", _simpleCommands[i]->_arguments[1]->c_str());
                }
            }
            DIR * dir;
            struct dirent * ent;
            if (root) {
                free(root);
            }
            root = getNode();
            if ((dir = opendir(".")) != NULL) {
                while ((ent = readdir(dir)) != NULL) {
                    if (ent->d_name[0] != '.') {
                        insert(root, ent->d_name);
                    }
                }
                closedir(dir);
            }
            builtin = true;
            ret = 1;
        } else {
            ret = fork();
        }
        if (ret == 0) {
            if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv")) {
                char ** p = environ;
                while (*p != NULL) {
                    printf("%s\n", *p);
                    p++;
                }
                close(tmpin);
                close(tmpout);
                close(tmperr);
                close(fdin);
                close(fdout);
                close(fderr);
                exit(0);
            }
            char ** args = new char*[_simpleCommands[i]->_arguments.size() + 1];
            for (int j = 0; j < _simpleCommands[i]->_arguments.size(); j++) {
                args[j] = (char *) _simpleCommands[i]->_arguments[j]->c_str();
            }
            args[_simpleCommands[i]->_arguments.size()] = NULL;
            printf("%s", _simpleCommands[i]->_arguments[_simpleCommands[i]->_arguments.size() - 1]);
            execvp(_simpleCommands[i]->_arguments[0]->c_str(), args);
            //perror("execvp");
            exit(1);
        } else if (ret < 0) {
            perror("fork");
            return;
        }
    }
    dup2(tmpin, 0);
    dup2(tmpout, 1);
    dup2(tmperr, 2);
    close(tmpin);
    close(tmpout);
    close(tmperr);
    close(fdin);
    close(fdout);
    close(fderr);
    if (!_background) {
        int code;
        waitpid(ret, &code, 0);
        setenv("RETURN_CODE", std::to_string(WEXITSTATUS(code)).c_str(), 1);
        if (code != 0 && !builtin) {
            char * msg = getenv("ON_ERROR");
            if (msg) {
                printf("%s\n", msg);
            }
        }
    } else {
        setenv("BACKGROUND_PROCESS", std::to_string(ret).c_str(), 1);
    }
    // Clear to prepare for next command
    clear();
    // Print new prompt
    if (isatty(0)) {
        Shell::prompt();
    }
}

SimpleCommand * Command::_currentSimpleCommand;
