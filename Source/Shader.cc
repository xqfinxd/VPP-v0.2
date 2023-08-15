#include "Shader.h"

#include <map>
#include <lua.hpp>

#include "ShaderImpl.h"

namespace VPP {

static const char* load_shader_result = "__load_shader_result";

class ShaderLoader {
public:
  ShaderLoader() {
    m_State = luaL_newstate();
    lua_rawgeti(m_State, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    luaL_Reg funcs[] = {
        {"Shader", BeginShader},
        {"Pass", SetPass},
    };
    lua_pushlightuserdata(m_State, this); // shared upvalue : this
    luaL_setfuncs(m_State, funcs, 1);
    lua_pop(m_State, 1);
  }

  ~ShaderLoader() {
    lua_close(m_State);
  }

  // return a func for loading table actually 
  static int BeginShader(lua_State* L) {
    auto argNum = lua_gettop(L);
    if (argNum <= 0) return 0;

    if (lua_isstring(L, 1)) {
      lua_pushvalue(L, 1); // first upvalue : shader name
      lua_pushvalue(L, lua_upvalueindex(1)); // second upvalue : this
      lua_pushcclosure(L, LoadShader, 2);
      return 1;
    }

    return 0;
  }

  // handle all data
  static int LoadShader(lua_State* L) {
    auto argNum = lua_gettop(L);
    if (argNum <= 0) return 0;

    if (!lua_istable(L, 1)) return 0;

    // array index for pass table
    auto tableSize = luaL_len(L, 1);
    if (tableSize <= 0) return 0;
    bool checkType = true;
    for (int i = 0; i < tableSize; i++) {
      checkType = checkType && LUA_TTABLE == lua_rawgeti(L, 1, 1);
      lua_pop(L, 1);
    }

    if (!checkType) return 0;

    for (int i = 0; i < tableSize; i++) {
      auto anchor = lua_gettop(L);
      checkType = checkType && LUA_TTABLE == lua_rawgeti(L, 1, 1);
      


      auto extra = lua_gettop(L) - anchor;
      assert(extra >= 0);
      lua_pop(L, extra);
    }

    return 0;
  }

  // only forward pass table
  static int SetPass(lua_State* L) {
    auto argNum = lua_gettop(L);
    if (argNum <= 0) return 0;

    if (lua_istable(L, 1)) {
      lua_pushvalue(L, 1);
      return 1;
    }

    return 0;
  }

private:
  lua_State* m_State;
};

void ShaderImplDeleter::operator()(ShaderImpl* impl) const {
  delete impl;
}

} // namespace VPP
