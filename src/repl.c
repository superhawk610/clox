#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include "repl.h"
#include "vm.h"

// TODO: Track command history (readline has some built-ins for this)
// TODO: Tab completion (readline should be of some help here as well)
// https://web.mit.edu/gnu/doc/html/rlman_2.html

static int handle_up_arrow(int count, int key) { return 0; }
static int handle_down_arrow(int count, int key) { return 0; }
static int handle_tab(int count, int key) { return 0; }

void repl() {
  char* line;

  rl_bind_keyseq("\\e[A", handle_up_arrow);
  rl_bind_keyseq("\\e[B", handle_down_arrow);
  rl_bind_key('\t', handle_tab);

  for (;;) {
    line = readline("> ");
    if (!line) break; // break on EOF

    interpret(line);
    free(line);
  }
}
