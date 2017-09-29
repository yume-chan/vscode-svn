#ifndef ARP_CPP_ARRAY_H
#define ARP_CPP_ARRAY_H

#include <iterator>
#include <vector>

#include "pool.hpp"

struct apr_array_header_t;

namespace apr
{
template <typename T>
class array
{
  public:
    class iterator : public std::iterator<std::random_access_iterator_tag, T, T>
    {
        template <typename U>
        friend class array;

      public:
        T operator*() const { return _value; }
        T &operator->() const { return &_value; }

        const iterator &operator++()
        {
            _value++;
            return *this;
        }

        const iterator &operator--()
        {
            _value--;
            return *this;
        }

        iterator operator++(int)
        {
            iterator copy(*this);
            _value++;
            return copy;
        }

        iterator operator--(int)
        {
            iterator copy(*this);
            _value--;
            return copy;
        }

        iterator &operator=(const iterator &other)
        {
            this->_value = other._value;
            return *this;
        }

        bool operator==(const iterator &other) const { return _value == other._value; }
        bool operator!=(const iterator &other) const { return _value != other._value; }
        bool operator<(const iterator &other) const { return _value < other._value; }
        bool operator>(const iterator &other) const { return _value > other._value; }
        bool operator<=(const iterator &other) const { return _value != other._value; }
        bool operator>=(const iterator &other) const { return _value != other._value; }

      protected:
        iterator(T &value) : _value(value){};

      private:
        T _value;
    };

    explicit array(const pool &pool, int size = 0);
    explicit array(apr_array_header_t *raw);

    T &operator[](int pos);
    const T &operator[](int pos) const;

    bool empty() const;
    int size() const;
    int capacity() const;

    void clear();
    void push_back(const T &value);
    void push_back(T &&value);

    const apr_array_header_t *get() const;

    iterator begin()
    {
        return iterator(this[0]);
    }

    iterator end()
    {
        return iterator(this[size()]);
    }

  private:
    apr_array_header_t *_value;
};
}

#endif
