#pragma once

#include <memory>
#include "public/config.h"

template class VPP_API std::shared_ptr<void>;

template <typename T>
class ImplBase {
public:
    const T* getImpl() const {
        return static_cast<T*>(impl_.get());
    }
    T* getImpl() {
        return static_cast<T*>(impl_.get());
    }
    std::shared_ptr<T> shareImpl() {
        return std::static_pointer_cast<T>(impl_);
    }

protected:
    void initImpl() {
        impl_ = std::shared_ptr<void>(new T);
    }

private:
    std::shared_ptr<void> impl_;
};
