#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
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

History* history;

static void history_init(History* h) {
  h->line = NULL;
  h->prev = NULL;
  h->next = NULL;
}

static void history_free() {
  // TODO: Walk back to front of list, then walk forward and free
  //       as we go.
}

static void setup_history() {
  History* h = (History*) malloc(sizeof(History));
  history_init(h);
  h->line = (char*) malloc(1);
  h->line[0] = '\0';

  history = h;
}

static char* history_prev() {
  if (history->prev != NULL) history = history->prev;
  return history->line;
}

static char* history_next() {
  if (history->next != NULL) history = history->next;
  return history->line;
}

static void history_push(char* line) {
  if (history->next != NULL) {
    // TODO: We're rewriting history, so free anything that
    //       will no longer be reachable.
  }

  History* new_history = (History*) malloc(sizeof(History));
  history_init(new_history);
  new_history->line = line;

  if (history == NULL) {
    history = new_history;
  } else {
    history->next = new_history;
    new_history->prev = history;
    history = new_history;
  }
}

static void history_dump() {
  History* h = history;
  while (h->prev != NULL) h = h->prev;

  do {
    printf("prev: 0x%.16zx next: 0x%.16zx line: \"%s\"\n", (size_t) h->prev,
                                                             (size_t) h->next,
                                                             h->line);

    h = h->next;
  } while (h != NULL);
}

static int handle_up_arrow(int count, int key) {
  // TODO: The first time this is called, store the current input
  //       somewhere so the user can get back to it.

  char* prev = history_prev();
  if (prev == NULL) return 0;

  rl_replace_line(prev, 0);
  rl_point = rl_end; // move cursor to end of line
  return 0;
}

static int handle_down_arrow(int count, int key) {
  char* next = history_next();
  if (next == NULL) return 0;

  rl_replace_line(next, 0);
  rl_point = rl_end;
  return 0;
}

static int handle_tab(int count, int key) { return 0; }

void repl() {
  char* line;

  rl_bind_keyseq("\\e[A", handle_up_arrow);
  rl_bind_keyseq("\\e[B", handle_down_arrow);
  rl_bind_key('\t', handle_tab);

  setup_history();

  for (;;) {
    line = readline("> ");
    if (!line) break; // break on EOF

    history_push(line);
    history_dump();
    interpret(line);
  }

  history_free();
}
