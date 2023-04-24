#pragma once

#include "config.h"

#include "utility.h"

#include <stdexcept>
#include <vector>

#include <lua.hpp>

#pragma comment(lib, "lua_lib.lib")

using SharedState = std::shared_ptr<lua_State>;

class SectionImpl : public cfg::ISection {
  friend class Config;

 private:
  SharedState state = nullptr;
  int refIndex = LUA_NOREF;

 public:
  std::string getString(const char* name) const {
    lua_geti(state.get(), LUA_REGISTRYINDEX, refIndex);
    lua_getfield(state.get(), -1, name);
    IFNO_THROW(lua_isstring(state.get(), -1), CFmt("[%s] is not string", name));
    std::string value = lua_tostring(state.get(), -1);
    lua_pop(state.get(), 2);
    return value;
  }
  double getNumber(const char* name) const {
    lua_geti(state.get(), LUA_REGISTRYINDEX, refIndex);
    lua_getfield(state.get(), -1, name);
    IFNO_THROW(lua_isnumber(state.get(), -1), CFmt("[%s] is not double", name));
    double value = lua_tonumber(state.get(), -1);
    lua_pop(state.get(), 2);
    return value;
  }
  int64_t getInteger(const char* name) const {
    lua_geti(state.get(), LUA_REGISTRYINDEX, refIndex);
    lua_getfield(state.get(), -1, name);
    IFNO_THROW(lua_isinteger(state.get(), -1),
               CFmt("[%s] is not integer", name));
    int64_t value = lua_tointeger(state.get(), -1);
    lua_pop(state.get(), 2);
    return value;
  }

  ~SectionImpl() {}
};

class Config {
 public:
  Config(const char* fn) {
    state = SharedState(luaL_newstate(), [](lua_State* L) { lua_close(L); });
    IFNO_THROW(state, "fail to create lua_State");
    auto script = ReadFile(fn);
    IFNO_THROW(!script.empty(), CFmt("fail to read [%s]", fn));
    auto result = luaL_dostring(state.get(), script.c_str());
    IFNO_THROW(LUA_OK == result, CFmt("fail to run [%s]", fn));
  }
  ~Config() {}

  cfg::Section find(const char* name) const {
    lua_getglobal(state.get(), name);
    IFNO_THROW(lua_istable(state.get(), -1), CFmt("fail to find [%s]", name));
    auto section = new SectionImpl();
    section->refIndex = luaL_ref(state.get(), LUA_REGISTRYINDEX);
    section->state = state;
    return cfg::Section(section);
  }

 private:
  SharedState state = nullptr;
};

namespace cfg {

static Config* sConfig = nullptr;
void Load(const char* fn) {
  if (!sConfig) {
    sConfig = new Config(fn);
  }
}

void Unload() {
  if (sConfig) {
    delete sConfig;
  }
}

Section Find(const char* name) {
  if (sConfig)
    return sConfig->find(name);
  return nullptr;
}

}  // namespace cfg
