#pragma once

#include <lua.hpp>

struct stack_pop {
public:
    stack_pop(lua_State* L, int count);
    explicit stack_pop(lua_State* L);
    ~stack_pop();

    stack_pop(const stack_pop&) = delete;
    stack_pop(stack_pop&&) = delete;

private:
    lua_State* state_ = nullptr;
    int count_ = 0;
};

namespace luaget {

template<typename T>
struct getter {};

template<>
struct getter<lua_Number> {
    static lua_Number get(lua_State* L, int idx) {
        return lua_tonumber(L, idx);
    }
};

template<>
struct getter<lua_Integer> {
    static lua_Integer get(lua_State* L, int idx) {
        return lua_tointeger(L, idx);
    }
};

template<>
struct getter<const char*> {
    static const char* get(lua_State* L, int idx) {
        return lua_tostring(L, idx);
    }
};

template<>
struct getter<void*> {
    static void* get(lua_State* L, int idx) {
        return lua_touserdata(L, idx);
    }
};

typedef getter<lua_Number> number;
typedef getter<lua_Integer> integer;
typedef getter<const char*> string;
typedef getter<void*> pointer;

}

struct reg_object {
public:
	reg_object();
	reg_object(lua_State* L, int stack_index);
	reg_object(const reg_object& other);
	~reg_object();

	reg_object& operator=(const reg_object& other);

	void swap(reg_object& other);
	void push(lua_State* L) const;
    void dispose();

    template<typename T>
    T get(luaget::getter<T>) {
        push(state_);
        stack_pop pop(state_, 1);
        return luaget::getter<T>::get(state_, -1);
    }

private:
	lua_State* state_;
	int index_;
};
