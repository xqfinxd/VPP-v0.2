#pragma once

#include <cassert>
#include <string>

std::string ReadFile(const char* fn);

constexpr size_t CFMT_MAX_LENGTH = 1024;
const char* CFmt(const char* fmt, ...);

template <typename T>
class Singleton {
 public:
  static T& getMe() {
    assert(msSingleton);
    return *msSingleton;
  }
  static T& getSingleton() { return msSingleton; }

 protected:
  Singleton() {
    assert(!msSingleton);
    intptr_t offset = (intptr_t)(T*)1 - (intptr_t)(Singleton<T>*)(T*)1;
    msSingleton = (T*)((intptr_t)this + offset);
  }
  ~Singleton() {
    assert(msSingleton);
    msSingleton = nullptr;
  }

 private:
  static T* msSingleton;
};

template <typename T>
T* Singleton<T>::msSingleton = nullptr;

#define IFNO_THROW(cond, msg)      \
  if (!(cond)) {                      \
    throw std::runtime_error(msg); \
  }                                \
  void(0)