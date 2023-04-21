#pragma once

#include <lua.hpp>

#include "utility.h"

class Config : public Singleton<Config> {
 public:
  Config() = default;
  ~Config() = default;

  void LoadConfig(const char* fn);

  const char* String(const char* key);
  lua_Integer Integer(const char* key);
  lua_Number Float(const char* key);
  size_t Len(const char* key);

 private:
  lua_State* state_ = nullptr;
  int ref_index_ = LUA_NOREF;

  int FindKey(const char* key);
};
