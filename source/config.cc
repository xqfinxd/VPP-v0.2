#pragma once

#include "config.h"

#include <cassert>
#include <string>
#include <vector>

namespace helper {
static bool AllowChar(char c) {
  if (c == '.') return true;
  if (c >= 'a' && c <= 'z') return true;
  if (c >= 'A' && c <= 'Z') return true;
  if (c >= '0' && c <= '9') return true;
  if (c == '_' || c == '-') return true;

  return false;
}

static std::string TrimStr(const std::string& src, char ch, bool left,
                           bool right) {
  size_t start = 0;
  if (left) start = src.find_first_not_of('.');
  size_t tail = 0;
  if (right) {
    auto end = src.find_last_not_of('.');
    if (end != src.npos)
      return src.substr(start, end - start + 1);
    else
      return src.substr(start);
  } else {
    return src.substr(start);
  }
  return src;
}

static void SpiltStr(std::vector<std::string>& out, const std::string& src,
                     char delm) {
  size_t start = 0;
  size_t pos = src.find('.');
  while (pos != src.npos) {
    auto substr = src.substr(start, pos - start);
    if (!substr.empty()) out.emplace_back(substr);
    start = pos + 1;
    pos = src.find('.', start);
  }
  out.emplace_back(src.substr(start));
}

static bool IsInteger(const std::string& str) {
  for (char ch : str) {
    if (ch < '0' || ch > '9') {
      return false;
    }
  }
  return true;
}
}  // namespace helper

void Config::LoadConfig(const char* fn) {
  state_ = luaL_newstate();
  int code = LUA_OK;
  code = luaL_loadfile(state_, fn);
  assert(code == LUA_OK);
  code = lua_pcall(state_, 0, 1, 0);
  assert(code == LUA_OK);
  assert(lua_istable(state_, -1));
  ref_index_ = luaL_ref(state_, LUA_REGISTRYINDEX);
}

const char* Config::String(const char* key) {
  int stackNum = FindKey(key);
  assert(stackNum && lua_isstring(state_, -1));
  StackRestore restore{state_, stackNum};
  return lua_tostring(state_, -1);
}

lua_Integer Config::Integer(const char* key) {
  int stackNum = FindKey(key);
  assert(stackNum && lua_isinteger(state_, -1));
  StackRestore restore{state_, stackNum};
  return lua_tointeger(state_, -1);
}

lua_Number Config::Float(const char* key) {
  int stackNum = FindKey(key);
  assert(stackNum && lua_isnumber(state_, -1));
  StackRestore restore{state_, stackNum};
  return lua_tonumber(state_, -1);
}

size_t Config::Len(const char* key) {
  int stackNum = FindKey(key);
  assert(stackNum && lua_istable(state_, -1));
  StackRestore restore{state_, stackNum};
  return luaL_len(state_, -1);
}

int Config::FindKey(const char* key) {
  assert(state_ && ref_index_ != LUA_NOREF);

  std::string fmtString{};
  {
    auto len = strlen(key);
    fmtString.reserve(len);
    for (size_t i = 0; i < len; i++) {
      if (helper::AllowChar(key[i])) fmtString.push_back(key[i]);
    }
    fmtString = helper::TrimStr(fmtString, '.', true, true);
  }

  std::vector<std::string> keys{};
  helper::SpiltStr(keys, fmtString, '.');

  lua_geti(state_, LUA_REGISTRYINDEX, ref_index_);
  int stackNum = 1;
  for (const auto& e : keys) {
    if (!lua_istable(state_, -1)) {
      lua_pop(state_, stackNum);
      return 0;
    }

    if (helper::IsInteger(e)) {
      auto len = luaL_len(state_, -1);
      auto idx = std::stoi(e.c_str());
      if (abs(idx) >= 1 && abs(idx) <= len) {
        lua_geti(state_, -1, idx);
        stackNum++;
      }
    } else {
      lua_getfield(state_, -1, e.c_str());
      stackNum++;
    }
  }

  return stackNum;
}