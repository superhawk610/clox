#ifndef __CLOX_COMPILER_H__
#define __CLOX_COMPILER_H__

#include "object.h"
#include "vm.h"

ObjFunction* compile(const char* source);

#endif // __CLOX_COMPILER_H__
