
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include "bestline.h"

void handle_command(const char *command) {
    (void)command;
}

void main_debugger_loop() {
    char *line;

    while ((line = bestline("(lc3-dbg) ")) != NULL) {
        bestlineHistoryAdd(line);
        free(line);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./main <file>. \n");
        return -1;
    }
    pid_t pid = fork();
    if (pid == 0) {
        //
    } else if (pid >= 1) {
        main_debugger_loop();
    }
    return 0;
}
