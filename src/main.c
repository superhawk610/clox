#include "common.h"
#include "chunk.h"
#include "debug.h"


int main(int argc, const char* argv[]) {
#define LINE_NO 123

  Chunk chunk;

  init_chunk(&chunk);

  write_constant(&chunk, 1.23, LINE_NO);
  write_constant(&chunk, 2.34, LINE_NO);
  write_chunk(&chunk, OP_RETURN, LINE_NO);

  disasm_chunk(&chunk, "test chunk");

  free_chunk(&chunk);

  return 0;

#undef LINE_NO
}
