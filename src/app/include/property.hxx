
#pragma once
#include <functional>
#include <memory>

namespace k {
namespace app {

// Basic Property implementation.
// Quite flexible, albeit extremely inefficient.

template <typename T>
class IProperty {
    T value;
public:
    virtual T get () { return value; }
    virtual void set (T v) { value = v; }
};

template <typename T>
class Property {
    std::unique_ptr<IProperty<T>> impl;
public:
    Property (IProperty<T>* impl) : impl{impl} {}

    T get () const { return impl->get(); }
    void set (T v) { impl->set(v); }
};

}; // namespace app
}; // namespace k
