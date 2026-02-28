
#include "bestline.h"
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

class debugger {
private:
  std::string prog_name;
  pid_t pid;

public:
  debugger(std::string prog_name, pid_t pid) : prog_name(prog_name), pid(pid) {}

  void run();
};

void debugger::run() {
  char *line = nullptr;
  while ((line = bestline("(lc3-dbg) ")) != nullptr) {
    bestlineHistoryAdd(line);
    free(line);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: ./main <program>";
    return -1;
  }

  auto prog = argv[1];

  auto pid = fork();
  if (pid == 0) {
    //
  } else if (pid >= 1) {
    debugger dbg{prog, pid};
    dbg.run();
  }
  return 0;
}
