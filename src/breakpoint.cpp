#include <sys/ptrace.h>
#include <iostream>
#include "breakpoint.hpp"

/*
A #BP exception occurs when an INT3 instruction is executed.
The INT3 is normally used by debug software to set instruction
breakpoints by replacing instruction-opcode bytes with the
INT3 opcode.
https://kib.kiev.ua/x86docs/AMD/AMD64/24593_APM_v2-r3.08.pdf
*/

void breakpoint::enable() {
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    m_saved_data = static_cast<uint8_t>(data & 0xff);
    uint64_t int3 = 0xcc;
    if (ptrace(PTRACE_POKEDATA, m_pid, (void *)m_addr, (void *)((data & ~0xff) | int3)) == -1) {
        std::cerr << "PTRACE_POKEDATA\n";
        return;
    }
    m_enabled = true;   
}

/*
#BP is a trap-type exception. The saved
instruction pointer points to the byte after the INT3
instruction. This location can be the start of the next
instruction. However, if the INT3 is used to replace the first
opcode bytes of an instruction, the restart location is likely to
be in the middle of an instruction. In the latter case, the debug
software must replace the INT3 byte with the correct
instruction byte. The saved RIP instruction pointer must then
be decremented by one before returning to the interrupted
program. This allows the program to be restarted correctly on
the interrupted-instruction boundary.
https://kib.kiev.ua/x86docs/AMD/AMD64/24593_APM_v2-r3.08.pdf
*/

void breakpoint::disable() {
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    if (ptrace(PTRACE_POKEDATA, m_pid, (void *)m_addr, (void *)((data & ~0xff) | m_saved_data)) == -1) {
        std::cerr << "PTRACE_POKEDATA\n";
        return;
    }
    m_enabled = false;
}
