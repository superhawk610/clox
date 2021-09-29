#ifndef __CLOX_SCANNER_H__
#define __CLOX_SCANNER_H__

#include "token.h"
#include "trie.h"

// this will be empty until `init_scanner` is called
extern Trie keywords;

void init_scanner(const char* source);

Token scan_token();

#endif // __CLOX_SCANNER_H__
