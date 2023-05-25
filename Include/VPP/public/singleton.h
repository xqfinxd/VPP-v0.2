#pragma once

#include <cassert>

template <typename T>
class Singleton {
public:
    static T& GetMe() {
        assert(s_Singleton_);
        return *s_Singleton_;
    }
    static T* GetSingleton() { return s_Singleton_; }

protected:
    Singleton() {
        assert(!s_Singleton_);
        intptr_t offset = (intptr_t)(T*)1 - (intptr_t)(Singleton<T>*)(T*)1;
        s_Singleton_ = (T*)((intptr_t)this + offset);
    }
    ~Singleton() {
        assert(s_Singleton_);
        s_Singleton_ = nullptr;
    }

private:
    static T* s_Singleton_;
};

template <typename T>
T* Singleton<T>::s_Singleton_ = nullptr;
