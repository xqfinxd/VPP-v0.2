#pragma once

#include "lua_ext.h"
#include <cassert>
#include <utility>

stack_pop::stack_pop(lua_State* L, int count)
    : state_(L), count_(count) {
    assert(L);
}

stack_pop::stack_pop(lua_State* L)
    : state_(L) {
    assert(L);
    count_ = lua_gettop(state_);
}

stack_pop::~stack_pop() {
    lua_pop(state_, count_);
}

reg_object::reg_object()
	: state_(nullptr), index_(LUA_NOREF) {
}

reg_object::reg_object(const reg_object& other)
	: state_(other.state_), index_(LUA_NOREF) {
	if (state_ == nullptr) return;
	lua_rawgeti(state_, LUA_REGISTRYINDEX, other.index_);
	index_ = luaL_ref(state_, LUA_REGISTRYINDEX);
}

reg_object::reg_object(lua_State* L, int stack_index)
	: state_(L), index_(LUA_NOREF) {
	lua_pushvalue(L, stack_index);
	index_ = luaL_ref(L, LUA_REGISTRYINDEX);
}

reg_object::~reg_object() {
	dispose();
}

reg_object& reg_object::operator=(const reg_object& other) {
	reg_object(other).swap(*this);
	return *this;
}

void reg_object::swap(reg_object& other) {
	std::swap(state_, other.state_);
	std::swap(index_, other.index_);
}

void reg_object::push(lua_State* L) const {
	lua_rawgeti(L, LUA_REGISTRYINDEX, index_);
}

void reg_object::dispose() {
	if (state_ && index_ != LUA_NOREF)
		luaL_unref(state_, LUA_REGISTRYINDEX, index_);
	state_ = nullptr;
	index_ = LUA_NOREF;
}
