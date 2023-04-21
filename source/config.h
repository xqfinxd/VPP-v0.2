#pragma once

#include <lua.hpp>

#include "utility.h"

class Config : public Singleton<Config> {
 public:
  Config() = default;
  ~Config() = default;

  void loadConfig(const char* fn);

  const char* toString(const char* key);
  lua_Integer toInteger(const char* key);
  lua_Number toNumber(const char* key);
  size_t length(const char* key);

 private:
  lua_State* state = nullptr;
  int refIndex = LUA_NOREF;

  int findKey(const char* key);
};
