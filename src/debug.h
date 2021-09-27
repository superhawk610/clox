#ifndef __CLOX_DEBUG_H__
#define __CLOX_DEBUG_H__

#include "chunk.h"

void disasm_chunk(Chunk* chunk, const char* name);

/* Disassemble an instruction at `offset` in `chunk`.
 *
 * @param chunk   The chunk containing the instruction.
 * @param offset  The instruction's offset within the chunk.
 * @return offset of the next instruction
 */
int disasm_instruction(Chunk* chunk, int offset);

#endif // __CLOX_DEBUG_H__
