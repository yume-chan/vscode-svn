#ifndef SVN_CPP_STREAM_H
#define SVN_CPP_STREAM_H

#include <memory>

#include <apr/pool.hpp>

#include "stringbuffer.hpp"

struct svn_stream_t;

namespace svn
{
class stream : public std::enable_shared_from_this<stream>
{
  public:
    explicit stream(std::shared_ptr<stringbuffer> buffer);

    friend class client;

  private:
    std::shared_ptr<stringbuffer> _buffer;
    svn_stream_t *_value;
};
}

#endif
