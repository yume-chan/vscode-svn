#include <svn_io.h>

#include "stringbuffer.hpp"

namespace svn
{
using std::shared_ptr;

stringbuffer::stringbuffer(shared_ptr<apr::pool> parent)
    : _parent(parent),
      _value(svn_stringbuf_create_empty(_parent->get()))
{
}

char *stringbuffer::data() const
{
    return _value->data;
}

size_t stringbuffer::length() const
{
    return _value->len;
}
}
