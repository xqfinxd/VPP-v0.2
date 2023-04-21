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
  lua_State* state_ = nullptr;
  int count_ = 0;
};

template <typename T>
class Singleton {
 public:
  static T* Get() {
    if (!instance_) {
      std::unique_lock<std::mutex> lock(mutex_);
      if (!instance_) {
        instance_ = new (std::nothrow) T;
      }
    }
    return instance_;
  }

  static void Destroy() { delete instance_; }

 protected:
  Singleton() {}

 private:
  static T* instance_;
  static std::mutex mutex_;
};

template <typename T>
T* Singleton<T>::instance_ = nullptr;

template <typename T>
std::mutex Singleton<T>::mutex_{};