
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN GREAT NEWLINE LESS GREATGREAT GREATAMPERSAND TWOGREAT AMPERSAND PIPE GREATGREATAMPERSAND

%{
//#define yylex yylex
#include <cstdio>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <dirent.h>
#include <algorithm>
#include <unistd.h>
#include "shell.hh"

#define MAXFILENAME 1024

void yyerror(const char * s);
int yylex();
void expandWildcard(char * suffix, char * prefix);
bool cmp(char * s1, char * s2) {
    return strcmp(s1, s2) < 0;
}
std::vector<char *> arguments;
%}

%%

goal:
  commands 
  ;

commands:
  command
  | commands command
  ;

command: simple_command
       ;

simple_command:	
  pipe_list iomodifier_opt_list background_optional NEWLINE {
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();  
  }
  | NEWLINE {
    if (isatty(0)) Shell::prompt();
  } 
  | error NEWLINE { yyerrok; }
  ;

command_and_args:
  command_word argument_list {
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | command_and_args
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    char * arg = (char *) "";
    expandWildcard(arg, (char *) $1->c_str());
    std::sort(arguments.begin(), arguments.end(), cmp);
    for (auto argument : arguments) {
        //Command::_currentSimpleCommand->insertArgument(new std::string(argument));
        if (strchr(argument, '*')) {
            continue;
        }
        if (strcmp(argument, "") == 0) {
            continue;
        }
        Command::_currentSimpleCommand->insertArgument(new std::string(argument));
    }
    for (int i = 0; i < arguments.size(); i++) {
        free(arguments[i]);
    }
    arguments.clear(); 
    free($1); 
  }
  ;

command_word:
  WORD {
    //printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

iomodifier_opt:
  GREAT WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._outFile) {
        printf("Ambiguous output redirect.\n");
        exit(1);
    }
    Shell::_currentCommand._outFile = $2;
  }
  | LESS WORD {
    //printf("   Yacc: insert input \"%s\"\n", $2->c_str());
    Shell::_currentCommand._inFile = $2;
  }
  | TWOGREAT WORD {
    //printf("   Yacc: insert error \"%s\"\n", $2->c_str());
    Shell::_currentCommand._errFile = $2;
  }
  | GREATAMPERSAND WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    //printf("   Yacc: insert error \"%s\"\n", $2->c_str());
    Shell::_currentCommand._errFile = $2;
  }
  | GREATGREAT WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._append = true;
  }
  | GREATGREATAMPERSAND WORD {
    //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
    Shell::_currentCommand._outFile = $2;
    //printf("   Yacc: insert error \"%s\"\n", $2->c_str());
    Shell::_currentCommand._errFile = $2;
    Shell::_currentCommand._append = true;
  }
  ;

iomodifier_opt_list:
  iomodifier_opt_list iomodifier_opt
  |  
  ;

background_optional:
  AMPERSAND {
    Shell::_currentCommand._background = true;
  }
  |
  ;

%%

void expandWildcard(char * prefix, char * suffix) {
    if (!suffix[0]) {
        arguments.push_back(strdup(prefix));
        return;
    }
    char cur[MAXFILENAME];
    if (!prefix[0]) {
        if (suffix[0] == '/') {
            suffix = suffix + 1;
            sprintf(cur, "%s/", prefix);
        } else {
            sprintf(cur, "%s", prefix);
        }
    } else {
        sprintf(cur, "%s/", prefix);
    }
    char * index = strchr(suffix, '/');
    char component[MAXFILENAME];
    if (index) {
        strncpy(component, suffix, index - suffix);
        component[index - suffix] = 0;
        suffix = index + 1;
    } else {
        sprintf(component, "%s", suffix);
        suffix = suffix + strlen(suffix);
    }
    char newPrefix[MAXFILENAME];
    if (strchr(component, '*') == NULL && strchr(component, '?') == NULL) {
        if (!cur[0]) {
            sprintf(newPrefix, "%s", component);
        } else {
            sprintf(newPrefix, "%s/%s", prefix, component);
        }
        expandWildcard(newPrefix, suffix);
        return;
    }
    char * reg = (char *) malloc(2 * strlen(component) + 10);
    char * a = component;
    char * r = reg;
    *r = '^';
    r++;
    
    while (*a) {
        if (*a == '*') {
            *r = '.'; r++;
            *r = '*'; r++;
        } else if (*a == '?') {
            *r = '.'; r++;
        } else if (*a == '.') {
            *r = '\\'; r++; *r = '.'; r++;
        } else {
            *r = *a; r++;   
        }
        a++;
    }
    *r = '$'; r++;
    *r = 0;

    regex_t re;
    int expbuf = regcomp(&re, reg, REG_EXTENDED | REG_NOSUB);
    char * dir;
    if (!cur[0]) {
        dir = ".";
    } else {
        dir = cur;
    }
    DIR * Dir = opendir(dir);
    if (!Dir) {
        return;
    }
    struct dirent * ent;
    bool flag = false;
    while ((ent = readdir(Dir))) {
        if (regexec(&re, ent->d_name, 1, NULL, 0) == 0) {
            flag = true;
            if (!cur[0]) {
                sprintf(newPrefix, "%s", ent->d_name);
            } else {
                sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
            }
            if (ent->d_name[0] == '.') {
                if (component[0] == '.') {
                    expandWildcard(newPrefix, suffix);
                }
            } else {
                expandWildcard(newPrefix, suffix);
            }
        }
    }
    if (!flag) {
        if (!cur[0]) sprintf(newPrefix, "%s", component);
        else sprintf(newPrefix, "%s/%s", prefix, component);
        expandWildcard(newPrefix, suffix);
    }
    closedir(Dir);
    free(reg);
    regfree(&re);  
    //regfree(expbuf);  
}

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
