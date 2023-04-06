#pragma once

#include "lua_ext.h"
#include <cassert>
#include <utility>

StackRestore::StackRestore(lua_State* L, int count)
    : state_(L), count_(count) {
    assert(L);
}

StackRestore::StackRestore(lua_State* L)
    : state_(L) {
    assert(L);
    count_ = lua_gettop(state_);
}

StackRestore::~StackRestore() {
    lua_pop(state_, count_);
}
