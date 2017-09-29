#include <svn_io.h>

#include "stream.hpp"

namespace svn
{
using std::shared_ptr;

stream::stream(shared_ptr<stringbuffer> buffer)
    : _buffer(buffer),
      _value(svn_stream_from_stringbuf(_buffer->_value, _buffer->_parent->get()))
{
}
}
