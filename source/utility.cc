#pragma once

#include "utility.h"

#include <cassert>

StackRestore::StackRestore(lua_State* L, int count) : state(L), popCount(count) {
  assert(L);
}

StackRestore::StackRestore(lua_State* L) : state(L) {
  assert(L);
  popCount = lua_gettop(state);
}

StackRestore::~StackRestore() { lua_pop(state, popCount); }