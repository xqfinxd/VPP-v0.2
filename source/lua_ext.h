#pragma once

#include <lua.hpp>

class StackRestore {
public:
    StackRestore(lua_State* L, int count);
    explicit StackRestore(lua_State* L);
    ~StackRestore();

    StackRestore(const StackRestore&) = delete;
    StackRestore(StackRestore&&) = delete;

private:
    lua_State* state_ = nullptr;
    int count_ = 0;
};
