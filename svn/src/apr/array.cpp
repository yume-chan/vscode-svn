#include <apr_tables.h>

#include "array.hpp"

namespace apr
{
template <typename T>
array<T>::array(const pool &pool, int size)
    : _value(apr_array_make(pool.get(), size, sizeof(T)))
{
}

template <typename T>
array<T>::array(const apr_array_header_t *raw)
    : _value(raw)
{
}

template <typename T>
T &array<T>::operator[](int pos)
{
    return APR_ARRAY_IDX(_value, pos, T);
}

template <typename T>
const T &array<T>::operator[](int pos) const
{
    return APR_ARRAY_IDX(_value, pos, const T);
}

template <typename T>
bool array<T>::empty() const
{
    return size() == 0;
}

template <typename T>
int array<T>::size() const
{
    return _value->nelts;
}

template <typename T>
int array<T>::capacity() const
{
    return _value->nalloc;
}

template <typename T>
void array<T>::clear()
{
    apr_array_clear(_value);
}

template <typename T>
void array<T>::push_back(const T &value)
{
    APR_ARRAY_PUSH(_value, T) = value;
}

template <typename T>
void array<T>::push_back(T &&value)
{
    APR_ARRAY_PUSH(_value, T) = value;
}

template <typename T>
const apr_array_header_t *array<T>::get() const
{
    return _value;
}
}
