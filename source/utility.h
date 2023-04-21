#pragma once

#include <lua.hpp>
#include <mutex>

class StackRestore {
 public:
  StackRestore(lua_State* L, int count);
  explicit StackRestore(lua_State* L);
  ~StackRestore();

  StackRestore(const StackRestore&) = delete;
  StackRestore(StackRestore&&) = delete;

 private:
  lua_State* state = nullptr;
  int popCount = 0;
};

template <typename T>
class Singleton {
 public:
  static T* Get() {
    if (!singleInstance) {
      std::unique_lock<std::mutex> lock(lockMutex);
      if (!singleInstance) {
        singleInstance = new (std::nothrow) T;
      }
    }
    return singleInstance;
  }

  static void Destroy() { delete singleInstance; }

 protected:
  Singleton() {}

 private:
  static T* singleInstance;
  static std::mutex lockMutex;
};

template <typename T>
T* Singleton<T>::singleInstance = nullptr;

template <typename T>
std::mutex Singleton<T>::lockMutex{};