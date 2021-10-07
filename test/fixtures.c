#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include "common.h"
#include "../src/logger.h"

// TODO: Run each script located in /fixtures, and
//       1. record stdout
//       2. record stderr
//       3. record exit code

#define FIXTURES_DIR     "./test/fixtures/"
#define FIXTURES_DIR_LEN 16

char*  test_stdout_buf;
char*  test_stderr_buf;
size_t test_stdout_max;
size_t test_stderr_max;
FILE*  test_stdout;
FILE*  test_stderr;

static char* fixture_path(const char* fixture_name) {
  char* p = (char*) malloc(FIXTURES_DIR_LEN + strlen(fixture_name) + 1);
  sprintf(p, FIXTURES_DIR "%s", fixture_name);
  return p;
}

static char* fixture_output_path(const char* fixture_path) {
  char* output_path = duplicate_string(fixture_path);
  sprintf(output_path + strlen(fixture_path) - 3, "out"); // replace .lox with .out
  return output_path;
}

static bool has_suffix(struct dirent* f, const char* suffix) {
  char* name = f->d_name;
  size_t name_len = strlen(name);
  return strcmp(&name[name_len - strlen(suffix)], suffix) == 0;
}

void test_fixtures() {
  test_stdout = open_memstream(&test_stdout_buf, &test_stdout_max);
  test_stderr = open_memstream(&test_stderr_buf, &test_stderr_max);
  logger_redirect_out(test_stdout);
  logger_redirect_err(test_stderr);

  DIR* fixtures = opendir(FIXTURES_DIR);
  if (!fixtures) {
    fprintf(stderr, "unable to open fixtures directory");
    exit(EX_IOERR);
  }

  struct dirent* f;
  while ((f = readdir(fixtures)) != NULL) {
    if (f->d_name[0] == '.') continue; // skip `.` and `..`
    if (!has_suffix(f, ".lox")) continue;

    char* path = fixture_path(f->d_name);
    char* output_path = fixture_output_path(path);

    fprintf(stderr, "path: %s\n", path);
    fprintf(stderr, "output_path: %s\n", output_path);

    free(output_path);
    free(path);
  }

  // yay, redirecting output to a buffer works!
  out_printf("hello, world!\n");
  fflush(test_stdout);
  printf("test_stdout: %s\n", test_stdout_buf);

  closedir(fixtures);

  logger_restore_out();
  logger_restore_err();
}

// ---

#undef FIXTURES_DIR
#undef FIXTURES_DIR_LEN
#undef STDOUT_MAX
#undef STDERR_MAX
