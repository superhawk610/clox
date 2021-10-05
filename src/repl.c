#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include "repl.h"
#include "vm.h"

// TODO: Track command history
// TODO: Tab completion
// https://web.mit.edu/gnu/doc/html/rlman_2.html

typedef struct History {
  char* line;

  struct History* prev;
  struct History* next;
} History;

History* history = NULL;

static char* history_prev() {
  if (history == NULL || history->prev == NULL) return NULL;

  history = history->prev;
  return history->line;
}

static char* history_next() {
  if (history == NULL || history->next == NULL) return NULL;

  history = history->next;
  return history->line;
}

static void history_push(char* line) {
  if (history->next != NULL) {
    // TODO: We're rewriting history, so free anything that
    //       will no longer be reachable.
  }

  History* new_history = (History*) malloc(sizeof(History));
  history->next = new_history;
  new_history->prev = history;
  history = new_history;
}

static void history_free() {
  // TODO: Walk back to front of list, then walk forward and free
  //       as we go.
}

static int handle_up_arrow(int count, int key) {
  char* prev = history_prev();
  if (prev == NULL) return 0;

  rl_replace_line(prev, 0);
  return 0;
}

static int handle_down_arrow(int count, int key) {
  char* next = history_next();
  if (next == NULL) return 0;

  rl_replace_line(next, 0);
  return 0;
}

static int handle_tab(int count, int key) { return 0; }

void repl() {
  char* line;

  rl_bind_keyseq("\\e[A", handle_up_arrow);
  rl_bind_keyseq("\\e[B", handle_down_arrow);
  rl_bind_key('\t', handle_tab);

  for (;;) {
    line = readline("> ");
    if (!line) break; // break on EOF

    history_push(line);
    interpret(line);
  }

  history_free();
}
