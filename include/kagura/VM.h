#pragma once
//===-- VM.h - Virtual Machine obfuscation definitions --------------------===//
//
// Defines the bytecode instruction set and interpreter interface for
// kagura's VM-based obfuscation pass.
//
// Architecture overview:
//
//   Source IR function  -->  [VMObfuscationPass]  --> VM handler table
//                                                      + bytecode array
//                                                      + interpreter loop
//
// The VM uses a simple stack-based architecture with 16 virtual registers,
// a value stack, and a dispatch table of handler functions.  Each original
// LLVM instruction is lowered into one or more VM opcodes.
//
// Opcode encoding:
//   [8-bit opcode][operands...]
//
//===----------------------------------------------------------------------===//

#include <cstdint>

namespace kagura {
namespace vm {

// ── Opcode definitions ──────────────────────────────────────────────────────

enum Opcode : uint8_t {
  // Stack operations
  OP_PUSH_IMM8   = 0x00, // push 8-bit immediate
  OP_PUSH_IMM16  = 0x01, // push 16-bit immediate
  OP_PUSH_IMM32  = 0x02, // push 32-bit immediate
  OP_PUSH_IMM64  = 0x03, // push 64-bit immediate
  OP_PUSH_REG    = 0x04, // push virtual register
  OP_POP_REG     = 0x05, // pop into virtual register
  OP_DUP         = 0x06, // duplicate top of stack
  OP_SWAP        = 0x07, // swap top two stack values

  // Arithmetic (pop 2, push 1)
  OP_ADD         = 0x10,
  OP_SUB         = 0x11,
  OP_MUL         = 0x12,
  OP_UDIV        = 0x13,
  OP_SDIV        = 0x14,
  OP_UREM        = 0x15,
  OP_SREM        = 0x16,

  // Bitwise
  OP_AND         = 0x20,
  OP_OR          = 0x21,
  OP_XOR         = 0x22,
  OP_NOT         = 0x23, // pop 1, push 1
  OP_SHL         = 0x24,
  OP_LSHR        = 0x25,
  OP_ASHR        = 0x26,

  // Comparison (pop 2, push i1)
  OP_ICMP_EQ     = 0x30,
  OP_ICMP_NE     = 0x31,
  OP_ICMP_ULT    = 0x32,
  OP_ICMP_ULE    = 0x33,
  OP_ICMP_UGT    = 0x34,
  OP_ICMP_UGE    = 0x35,
  OP_ICMP_SLT    = 0x36,
  OP_ICMP_SLE    = 0x37,
  OP_ICMP_SGT    = 0x38,
  OP_ICMP_SGE    = 0x39,

  // Control flow
  OP_JMP         = 0x40, // unconditional jump (16-bit offset)
  OP_JZ          = 0x41, // jump if top == 0
  OP_JNZ         = 0x42, // jump if top != 0
  OP_CALL        = 0x43, // call native function pointer (from reg)
  OP_RET         = 0x44, // return top of stack (or void)
  OP_RET_VOID    = 0x45,

  // Memory
  OP_LOAD8       = 0x50, // pop ptr, push i8
  OP_LOAD16      = 0x51,
  OP_LOAD32      = 0x52,
  OP_LOAD64      = 0x53,
  OP_STORE8      = 0x54, // pop val, pop ptr, store
  OP_STORE16     = 0x55,
  OP_STORE32     = 0x56,
  OP_STORE64     = 0x57,

  // Type conversions
  OP_ZEXT        = 0x60, // zero-extend: pop, push widened
  OP_SEXT        = 0x61,
  OP_TRUNC       = 0x62,

  // Argument / return value passing
  OP_LOAD_ARG    = 0x70, // push function argument[index]
  OP_NOP         = 0xFF,
};

// Maximum number of virtual registers per VM frame
static constexpr unsigned kNumRegs   = 16;
// Maximum bytecode size per function (bytes)
static constexpr unsigned kMaxBCSize = 65536;
// VM stack depth limit
static constexpr unsigned kStackSize = 256;

// ── Runtime interpreter context (embedded in target binary) ─────────────────

struct VMContext {
  uint64_t regs[kNumRegs];  // virtual registers
  uint64_t stack[kStackSize];
  uint32_t sp;              // stack pointer
  uint32_t pc;              // program counter
  const uint8_t *bytecode;
  uint32_t bytecodeSize;
};

} // namespace vm
} // namespace kagura
