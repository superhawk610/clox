#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include "repl.h"
#include "vm.h"

// TODO: Tab completion

typedef struct HistoryLine {
  char* chars;

  struct HistoryLine* prev;
  struct HistoryLine* next;
} HistoryLine;

typedef struct {
  HistoryLine* head; // doubly-linked list of previous input
  HistoryLine* tail; // store pointer to tail for faster pushes
  HistoryLine* curr; // pointer to current line

  bool input_pending; // whether the user has some input pending (if they do,
                      // recording new history will be patch instead of push)
} History;

History history;

static void history_line_init(HistoryLine* line);

static void history_init() {
  HistoryLine* line = (HistoryLine*) malloc(sizeof(HistoryLine));
  history_line_init(line);
  line->chars = (char*) malloc(1);
  line->chars[0] = '\0';

  history.head = line;
  history.tail = history.head;
  history.curr = history.head;
  history.input_pending = false;
}

static void history_free() {
  HistoryLine* line = history.head;

  do {
    free(line->chars);
    line = line->next;
  } while (line != NULL);

  history.head = NULL;
  history.tail = NULL;
  history.curr = NULL;
}

static void history_line_init(HistoryLine* line) {
  line->chars = NULL;
  line->prev = NULL;
  line->next = NULL;
}

static char* history_prev() {
  if (history.curr->prev != NULL) history.curr = history.curr->prev;
  return history.curr->chars;
}

static char* history_next() {
  if (history.curr->next != NULL) history.curr = history.curr->next;
  return history.curr->chars;
}

static void history_push(char* chars) {
  HistoryLine* line = (HistoryLine*) malloc(sizeof(HistoryLine));
  history_line_init(line);
  line->chars = chars;

  history.tail->next = line;
  line->prev = history.tail;
  history.tail = line;
}

static void history_drop() {
  HistoryLine* tail = history.tail;
  history.tail = tail->prev;

  if (history.curr == tail) history.curr = history.tail;

  free(tail->chars);
}

static void history_patch(char* line) {
  free(history.tail->chars);
  history.tail->chars = line;
}

static void history_dump() {
  HistoryLine* line = history.head;

  do {
    printf("0x%.16zx prev: 0x%.16zx next: 0x%.16zx chars: 0x%.16zx \"%s\"\n",
      (size_t) line,
      (size_t) line->prev,
      (size_t) line->next,
      (size_t) line->chars,
      line->chars);

    line = line->next;
  } while (line != NULL);

  printf("\nhead: 0x%.16zx tail: 0x%.16zx curr: 0x%.16zx\n", (size_t) history.head,
                                                             (size_t) history.tail,
                                                             (size_t) history.curr);
}

static char* duplicate_string(const char* str) {
  size_t len = strlen(str);
  char* dup = (char*) malloc(len + 1);
  strcpy(dup, str);
  return dup;
}

static int handle_up_arrow(int count, int key) {
  if (!history.input_pending) {
    history_push(duplicate_string(rl_line_buffer));
    history.input_pending = true;
    history.curr = history.tail;
  }

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

  history_init();

  for (;;) {
    line = readline("> ");
    if (!line) break; // break on EOF

    if (history.input_pending) {
      if (line[0] == '\0') history_drop();
      else                 history_patch(line);

      history.input_pending = false;
    } else if (line[0] != '\0') {
      history_push(line);
    }

    history.curr = history.tail;

    interpret(line);
  }

  free(line);
  history_free();
}
