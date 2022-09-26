
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <unistd.h>
#include "y.tab.hh"

////////////  Start added code ///////////

extern "C" char * read_line();

int mygetc(FILE * f) {
    static char * p;
    char ch;
    if (!isatty(0)) {
        return getc(f);
    }
    if (p == NULL || *p == 0) {
        char * s = read_line(); 
        p = s;
    }
    ch = *p;
    p++;
    return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%s SOURCE_SC

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"<" {
    return LESS;
}

">>" {
    return GREATGREAT;
}

">&" {
    return GREATAMPERSAND;
}

"2>" {
    return TWOGREAT;
}

">>&" {
    return GREATGREATAMPERSAND;
}

"|" {
    return PIPE;
}

"&" {
    return AMPERSAND;
}

"source" {
    BEGIN(SOURCE_SC);
}

<SOURCE_SC>[^ \|><\t\n][^ \|><\t\n]* {
    FILE * file = fopen(yytext, "r");
    YY_BUFFER_STATE state = yy_create_buffer(file, YY_BUF_SIZE);
    yypush_buffer_state(state);
    BEGIN(0);
    yyparse();
    yypop_buffer_state();
}

~[^ \t\n]* {
    std::string arg = std::string(yytext);
    std::string dir = getenv("HOME");
    if (arg.length() == 1) {
        yylval.cpp_string = new std::string(dir);
    } else { 
        if (arg[1] == '/') {
            yylval.cpp_string = new std::string(dir + arg.substr(1));
        } else {
            yylval.cpp_string = new std::string("/homes/" + arg.substr(1));
        }
    }
    return WORD;
}

\".*\" {
    std::string arg = std::string(yytext);
    arg = arg.substr(1, arg.size() - 2);
    yylval.cpp_string = new std::string(arg);
    return WORD;
}

[^ \\\n]*(\\[^nt])((\\[^nt])|[^ \\\n])* {
    std::string args = std::string(yytext);
    int index = args.find('\\');
    while (index != -1) {
        args = args.substr(0, index) + args.substr(index + 1, args.length() - index - 1);
        index = args.find('\\', index + 1);
    }
    yylval.cpp_string = new std::string(args);
    return WORD;
}

$\(.*\) {
    std::string cmd = std::string(yytext);
    cmd = cmd.substr(2, cmd.length() - 3);
    cmd += "\nexit\n";
    int pin[2], pout[2];
    int tmpin = dup(0), tmpout = dup(1);
    pipe(pin); pipe(pout);
    write(pin[1], cmd.c_str(), cmd.length());
    close(pin[1]);
    int ret = fork();
    if (ret == 0) {
        dup2(pin[0], 0);
        dup2(pout[1], 1);
        close(pin[0]);
        close(pout[1]);
        char *args[2];
        args[0] = (char *) "/proc/self/exe";
        args[1] = NULL;
        execvp(args[0], args);
        exit(1);
    } else {
        waitpid(ret, NULL, 0);
        dup2(tmpin, 0);
        dup2(tmpout, 1);
        close(tmpin);
        close(tmpout);
        char * out = (char *) malloc(1024);
        int len = read(pout[0], out, 1024);
        close(pout[0]);
        std::string exitmsg = " good bye!!\n";
        for (int i = len - 1 - exitmsg.length(); i >= 0; i--) {
            if (out[i] == '\n') {
                out[i] = ' ';
            }
            myunputc(out[i]);
        }
        free(out);
    }
}

$\{[^ \}\n]*\}([^ \n]*|$\{[^ \}\n]*\})* {
    std::string arg = std::string(yytext);
    int index = arg.find('$');
    while (index != -1) {
        if (arg[index + 1] == '{') {
            int close = arg.find('}', index + 1);
            if (close != -1) {
                if (arg.substr(index + 2, close - index - 2) == "$") {
                    pid_t pid = getpid();
                    arg = arg.substr(0, index) + std::to_string(pid) + arg.substr(close + 1);
                } else if (arg.substr(index + 2, close - index - 2) == "?") {
                    arg = arg.substr(0, index) + getenv("RETURN_CODE") + arg.substr(close + 1);
                } else if (arg.substr(index + 2, close - index - 2) == "!") {
                    arg = arg.substr(0, index) + getenv("BACKGROUND_PROCESS") + arg.substr(close + 1);
                } else if (arg.substr(index + 2, close - index - 2) == "_") {
                    arg = arg.substr(0, index) + getenv("LAST_ARGUMENT") + arg.substr(close + 1);
                } else if (arg.substr(index + 2, close - index - 2) == "SHELL") {
                    arg = arg.substr(0, index) + getenv("SHELL") + arg.substr(close + 1); 
                } else if (getenv(arg.substr(index + 2, close - index - 2).c_str())) {
                    arg = arg.substr(0, index) + getenv(arg.substr(index + 2, close - index - 2).c_str()) + arg.substr(close + 1);
                }
            }
        }
        index = arg.find('$', index + 1);
    }
    yylval.cpp_string = new std::string(arg);
    return WORD; 
}    

[^ \|><\t\n][^ \|><\t\n]*  {
  /* Assume that file names have only alpha chars */
    yylval.cpp_string = new std::string(yytext);
    return WORD;
}
