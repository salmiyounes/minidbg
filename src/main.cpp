#include "bestline.h"
#include "breakpoint.hpp"
#include "register.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(s);
  std::string item;

  while (std::getline(ss, item, delim)) {
    result.push_back(item);
  }

  return result;
}

bool starts_with(const std::string &str, const std::string &sub,
                 bool ignore_case = false) {
  int str_len = str.size();
  int sub_len = sub.size();
  if (str_len < sub_len)
    return false;

  if (ignore_case) {
    return std::equal(sub.begin(), sub.end(), str.begin(), [](char a, char b) {
      return std::tolower(a) == std::tolower(b);
    });
  }
  return str.compare(0, sub_len, sub) == 0;
}

class debugger {
private:
  std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;
  std::string prog_name;
  pid_t pid;

public:
  debugger(std::string prog_name, pid_t pid) : prog_name(prog_name), pid(pid) {}

  void run();
  void print_help();
  void dump_registers();
  void handle_command(const std::string &line);
  void continue_execution();
  void set_breakpoint_at_address(std::intptr_t addr);
};

void debugger::handle_command(const std::string &line) {
  auto args = split(line, ' ');
  auto command = args[0];

  if (starts_with(command, "help")) {
    print_help();
  } else if (starts_with(command, "break")) {
    std::string addr{args[1], 2};
    set_breakpoint_at_address(std::stol(addr, 0, 16));
  } else if (starts_with(command, "continue")) {
    continue_execution();
  } else if (starts_with(command, "info")) {
    if (starts_with(args[1], "registers")) {
      dump_registers();
    } else {
      std::cerr << "Unknown command\n";
    }
  } else {
    std::cerr << "Unknown command\n";
  }
}

void debugger::dump_registers() {
  for (const auto &rd : g_register_descriptors) {
    std::cout << rd.name << " 0x" << std::setfill('0') << std::setw(16)
              << std::hex << get_register_value(pid, rd.r) << std::endl;
  }
}

void debugger::set_breakpoint_at_address(std::intptr_t addr) {
  std::cout << "Set breakpoint at address 0x" << std::hex << addr << std::endl;
  breakpoint bp{pid, addr};
  bp.enable();
  m_breakpoints[addr] = bp;
}

void debugger::continue_execution() {
  ptrace(PTRACE_CONT, pid, nullptr, nullptr);

  int wait_status;
  auto options = 0;
  waitpid(pid, &wait_status, options);
}

void debugger::print_help() {
  std::cout << "Available commands:\n";
  std::cout << "  help     - Show this help message\n";
  std::cout << "  continue - Resume execution of the program\n";
}

void completion(const char *buf, int pos, bestlineCompletions *lc) {
  (void)pos;
  if (starts_with(buf, "h")) {
    bestlineAddCompletion(lc, "help");
  } else if (starts_with(buf, "c")) {
    bestlineAddCompletion(lc, "continue");
  } else if (starts_with(buf, "br")) {
    bestlineAddCompletion(lc, "break");
  } else if (starts_with(buf, "info")) {
    bestlineAddCompletion(lc, "info");
  } else if (starts_with(buf, "reg")) {
    bestlineAddCompletion(lc, "register");
  }
}

void debugger::run() {
  int wait_status;
  auto options = 0;
  waitpid(pid, &wait_status, options);

  // Set the completion callback
  bestlineSetCompletionCallback(completion);

  char *line = nullptr;
  while ((line = bestline("(minidbg) ")) != nullptr) {
    bestlineHistoryAdd(line);
    handle_command(line);
    free(line);
  }
}

void start_debugee(const std::string &prog_name) {
  if (ptrace(PTRACE_TRACEME, nullptr, nullptr, nullptr) < 0) {
    std::cerr << "Error in ptrace\n";
    return;
  }
  execl(prog_name.c_str(), prog_name.c_str(), nullptr);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: ./minidbg <program>";
    return -1;
  }

  auto prog = argv[1];

  auto pid = fork();
  if (pid == 0) {
    personality(ADDR_NO_RANDOMIZE);
    start_debugee(prog);
  } else if (pid >= 1) {
    debugger dbg{prog, pid};
    dbg.run();
  }
  return 0;
}