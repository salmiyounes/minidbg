#include "bestline.h"
#include "breakpoint.hpp"
#include "pmparser.h"
#include "register.hpp"
#include "utils.hpp"
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

class debugger {
private:
  std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;
  std::string prog_name;
  pid_t pid;
  uint64_t load_addr;

public:
  debugger(std::string prog_name, pid_t pid) : prog_name(prog_name), pid(pid) {}

  void run();
  void initialise_load_address();
  uint64_t offset_dwarf_address(uint64_t addr);
  void wait_for_signal();
  void print_help();
  void dump_registers();
  void handle_command(const std::string &line);
  void continue_execution();
  void single_step_instruction();
  void set_breakpoint_at_address(std::intptr_t addr);
};

void debugger::handle_command(const std::string &line) {
  auto args = split(line, ' ');
  if (args.empty())
    return;

  auto command = args[0];

  if (command == "help" || command == "h") {
    print_help();
  } else if (command == "break" || command == "br") {
    std::string addr{args[1], 2};
    set_breakpoint_at_address(std::stol(addr, 0, 16));
  } else if (command == "continue") {
    continue_execution();
  } else if (command == "register" || command == "reg") {
    dump_registers();
  } else if (command == "nexti") {
    single_step_instruction();
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

void debugger::wait_for_signal() {
  int wait_status;
  auto options = 0;
  waitpid(pid, &wait_status, options);
}

void debugger::initialise_load_address() {
  procmaps_iterator maps_iter;
  procmaps_error_t parser_err = PROCMAPS_SUCCESS;

  parser_err = pmparser_parse(pid, &maps_iter);
  if (parser_err) {
    std::cerr
        << "initialise_load_address: failure to parse memory map of process "
        << pid << " (error=" << static_cast<int>(parser_err) << ")"
        << std::endl;
    return;
  }

  // iterate over areas
  procmaps_struct *mem_region = nullptr;

  // Use pmparser_next to get the first memory region
  // The first entry in /proc/self/maps is usually the base load address
  mem_region = pmparser_next(&maps_iter);
  if (mem_region == nullptr) {
    std::cerr
        << "initialise_load_address: Could not find a valid memory region."
        << std::endl;
    return;
  }

  load_addr = reinterpret_cast<uint64_t>(mem_region->addr_start);

  pmparser_free(&maps_iter);
  return;
}

uint64_t debugger::offset_dwarf_address(uint64_t addr) {
  return addr + load_addr;
}

void debugger::set_breakpoint_at_address(std::intptr_t addr) {
  std::cout << "Set breakpoint at address 0x" << std::hex << addr << std::endl;
  breakpoint bp{pid, addr};
  bp.enable();
  m_breakpoints[addr] = bp;
}

void debugger::single_step_instruction() {
  ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);

  wait_for_signal();
}

void debugger::continue_execution() {
  ptrace(PTRACE_CONT, pid, nullptr, nullptr);

  wait_for_signal();
}

void debugger::print_help() {
  std::cout << "Available commands:\n";
  std::cout << "  help     - Show this help message\n";
  std::cout << "  continue - Resume execution of the program\n";
}

void completion(const char *buf, [[maybe_unused]] int pos,
                bestlineCompletions *lc) {
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

  initialise_load_address();
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
    std::cerr << "Usage: ./minidbg <program>\n";
    return -1;
  }

  auto prog = argv[1];

  auto pid = fork();
  if (pid == 0) {
    personality(ADDR_NO_RANDOMIZE);
    start_debugee(prog);
  } else if (pid >= 1) {
    std::cout << "Started debugging process " << pid << '\n';
    debugger dbg{prog, pid};
    dbg.run();
  }
  return 0;
}