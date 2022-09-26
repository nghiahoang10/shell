/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "read-line.h"

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);
extern char* strdup(const char*);
extern TrieNode * root;
// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];
int cursor = 0;

int history_index = 0;
char ** history = NULL;
int history_length = 0;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

    // Set terminal in raw mode
    tty_raw_mode();

    line_length = 0;
    cursor = 0;
    memset(line_buffer, 0, MAX_BUFFER_LINE);
    // Read one line until enter is typed
    while (1) {

        // Read one character in raw mode.
        char ch;
        read(0, &ch, 1);

        if (ch>=32 && ch != 127) {
            // It is a printable character. 

            // Do echo
            write(1,&ch,1);

            // If max number of character reached return.
            if (line_length==MAX_BUFFER_LINE-2) break; 

            // add char to buffer.
            for (int i = line_length; i > cursor; i--) {
                line_buffer[i] = line_buffer[i - 1];
            }
            line_buffer[cursor]=ch;
            line_length++;
            cursor++;
            write(1, line_buffer + cursor, line_length - cursor);
            write(1, " \b", 2);
            for (int i = 0; i < line_length - cursor; i++) {
                char c = 27;
                write(1, &c, 1);
                c = 91;
                write(1, &c, 1);
                c = 68;
                write(1, &c, 1);
            }
        }
        else if (ch==10) {
            // <Enter> was typed. Return line

            // Print newline
            if (line_buffer[0]) {
                history_length++;
                history = (char **) realloc(history, sizeof(char *) * history_length);
                history_index = history_length;
                history[history_length - 1] = strdup(line_buffer);
            }
            write(1,&ch,1);
            break;
        }
        else if (ch == 31) {
            // ctrl-?
            read_line_print_usage();
            line_buffer[0]=0;
            break;
        }
        else if (ch == 8 || ch == 127) {
            // <backspace> was typed. Remove previous character read.

            // Go back one character
            if (cursor == 0) continue;
            /*ch = 8;
            write(1,&ch,1);

            // Write a space to erase the last character read
            ch = ' ';
            write(1,&ch,1);

            // Go back one character
            ch = 8;
            write(1,&ch,1);*/
            for (int i = cursor; i < line_length; i++) {
                line_buffer[i - 1] = line_buffer[i];
            }
            line_buffer[line_length - 1] = '\0';
            line_length--;
            cursor--;
            write(1, "\b \b", 3);
            write(1, line_buffer + cursor, line_length - cursor);
            write(1, " \b", 2);
            for (int i = 0; i < line_length - cursor; i++) {
                char c = 27;
                write(1, &c, 1);
                c = 91;
                write(1, &c, 1);
                c = 68;
                write(1, &c, 1);
            }
            // Remove one character from buffer
            //line_length--;
        } else if (ch == 4) {
            if (cursor == line_length) continue;
            for (int i = cursor + 1; i < line_length; i++) {
                line_buffer[i - 1] = line_buffer[i];
            }
            line_buffer[line_length - 1] = '\0';
            line_length--;
            write(1, line_buffer + cursor, line_length - cursor);
            write(1, " \b", 2);
            for (int i = 0; i < line_length - cursor; i++) {
                char c = 27;
                write(1, &c, 1);
                c = 91;
                write(1, &c, 1);
                c = 68;
                write(1, &c, 1);
            }
        } else if (ch == 9) {
            char pref[100] = {0};     
            tab(root, pref, strchr(line_buffer, ' ') + 1, 0, 0);
            for (int i = line_length, j = 0; i < line_length + strlen(pref); i++, j++) {
                line_buffer[i] = pref[j];
            }
            write(1, line_buffer + line_length, strlen(pref));
            cursor += strlen(pref);
            line_length += strlen(pref);
        } else if (ch == 1) {
            while (cursor > 0) {
                char c = 27;
                write(1, &c, 1);
                c = 91;
                write(1, &c, 1);
                c = 68;
                write(1, &c, 1);
                cursor--;
            }
        } else if (ch == 5) {
            while (cursor < line_length) {
                char c = 27;
                write(1, &c, 1);
                c = 91;
                write(1, &c, 1);
                c = 67;
                write(1, &c, 1);
                cursor++;
            }
        } else if (ch==27) {
            // Escape sequence. Read two chars more
            //
            // HINT: Use the program "keyboard-example" to
            // see the ascii code for the different chars typed.
            //
            char ch1; 
            char ch2;
            read(0, &ch1, 1);
            read(0, &ch2, 1);
            if (ch1==91 && ch2==65) {
                // Up arrow. Print next line in history.

                // Erase old line
                // Print backspaces
                if (history_length == 0 || history_index == 0) continue;
                int i = 0;
                for (i =0; i < line_length; i++) {
                    ch = 8;
                    write(1,&ch,1);
                }

                // Print spaces on top
                for (i =0; i < line_length; i++) {
                    ch = ' ';
                    write(1,&ch,1);
                }

                // Print backspaces
                for (i =0; i < line_length; i++) {
                    ch = 8;
                    write(1,&ch,1);
                }	

                // Copy line from history
                history_index = history_index - 1;
                strcpy(line_buffer, history[history_index]);
                line_length = strlen(line_buffer);
                cursor = strlen(line_buffer);
                write(1, line_buffer, line_length);
            } else if (ch1 == 91 && ch2 == 66) {
                if (history_length == 0 || history_index == history_length) continue;
                int i = 0;
                for (i = 0; i < line_length; i++) {
                    ch = 8;
                    write(1, &ch, 1);
                }
                for (i = 0; i < line_length; i++) {
                    ch = ' ';
                    write(1, &ch, 1);
                }
                for (i = 0; i < line_length; i++) {
                    ch = 8;
                    write(1, &ch, 1);
                }
                if (history_index + 1 == history_length) {
                    history_index = history_length;
                    strcpy(line_buffer, "");
                    line_length = 0;
                    cursor = 0;
                    write(1, line_buffer, line_length);
                    continue;
                }
                history_index = history_index + 1;
                strcpy(line_buffer, history[history_index]);
                line_length = strlen(line_buffer);
                cursor = strlen(line_buffer);
                write(1, line_buffer, line_length);
            } else if (ch1 == 91 && ch2 == 68) {
                if (cursor == 0) continue;
                char c = 27;
                write(1, &c, 1);
                c = 91;
                write(1, &c, 1);
                c = 68;
                write(1, &c, 1);
                cursor--;
            } else if (ch1 == 91 && ch2 == 67) {
                if (cursor == line_length) continue;
                char c = 27;
                write(1, &c, 1);
                c = 91;
                write(1, &c, 1);
                c = 67;
                write(1, &c, 1);
                cursor++;
            }
        }
    }
    // Add eol and null char at the end of string
    line_buffer[line_length]=10;
    line_length++;
    line_buffer[line_length]=0;

    return line_buffer;
}

