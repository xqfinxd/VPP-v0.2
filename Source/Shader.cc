#include "Shader.h"

#include <map>

#include <lua.hpp>

namespace VPP {

struct LuaStackPop {
  LuaStackPop(lua_State* L) {
    m_State = L;
    m_Top = lua_gettop(m_State);
  }
  ~LuaStackPop() {
    if (m_State) {
      int newTop = lua_gettop(m_State);
      if (newTop > m_Top) lua_pop(m_State, newTop - m_Top);
    }
  }

  lua_State*  m_State = nullptr;
  int         m_Top = 0;
};

static const char* load_shader_result = "__load_shader_result";

static int LuaLoadShader(lua_State* L) {
  auto argNum = lua_gettop(L);
  if (argNum <= 0) return 0;

  if (!lua_istable(L, 1)) return 0;

  auto tableSize = luaL_len(L, 1);
  if (tableSize <= 0) return 0;

  bool checkType = true;
  for (int i = 0; i < tableSize; i++) {
    auto isUD = LUA_TUSERDATA == lua_rawgeti(L, 1, 1);
    lua_pop(L, 1);
    if (!isUD) checkType = false;
  }

  if (!checkType) return 0;
  
  bool* result = nullptr;
  { 
    lua_getglobal(L, load_shader_result);
    if (lua_islightuserdata(L, -1))
      result = (bool*)lua_touserdata(L, -1);
    lua_pop(L, 1);
  }

  return 0;
}

static int LuaBeginShader(lua_State* L) {
  auto argNum = lua_gettop(L);
  if (argNum <= 0) return 0;

  if (lua_isstring(L, 1)) {
    lua_pushvalue(L, 1);
    lua_pushcclosure(L, LuaLoadShader, 1);
    return 1;
  }

  return 0;
}

} // namespace VPP
