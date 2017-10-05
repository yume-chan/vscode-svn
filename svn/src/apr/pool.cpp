#include <apr_pools.h>
#include <apr_strings.h>

#include "pool.hpp"

namespace apr {
pool pool::unmanaged(apr_pool_t* raw) {
    return pool(raw, false);
}

pool::pool()
    : _managed(true) {
    apr_initialize();
    apr_pool_create(&_value, nullptr);
}

pool::pool(apr_pool_t* raw, bool managed)
    : _managed(managed)
    , _value(raw) {
}

pool::pool(pool&& other)
    : _managed(other._managed)
    , _value(other._value) {
    other._managed = false;
    other._value   = nullptr;
}

pool::~pool() {
    if (_managed) {
        apr_pool_destroy(_value);
        apr_terminate();
    }
}

apr_pool_t* pool::get() const {
    return _value;
}

pool pool::create() const {
    apr_pool_t* child;
    apr_pool_create(&child, _value);

    return pool(child, true);
}

template <typename T>
T* pool::alloc() const {
    return static_cast<T*>(apr_pcalloc(_value, sizeof(T)));
}

void* pool::memcpy(void* source, size_t count) const {
    return apr_pmemdup(_value, source, count);
}
} // namespace apr
