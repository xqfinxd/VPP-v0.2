#pragma once

#include <cassert>

template <typename T>
class Singleton {
public:
    static T& getMe() {
        assert(msSingleton);
        return *msSingleton;
    }
    static T* getSingleton() { return msSingleton; }

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
