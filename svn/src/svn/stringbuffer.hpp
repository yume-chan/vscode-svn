#ifndef SVN_CPP_STRINGBUFFER_H
#define SVN_CPP_STRINGBUFFER_H

#include <memory>

#include <apr/pool.hpp>

struct svn_stringbuf_t;

namespace svn
{
struct stringbuffer : public std::enable_shared_from_this<stringbuffer>
{
  public:
    explicit stringbuffer(std::shared_ptr<apr::pool> parent);

    std::shared_ptr<apr::pool> pool() const;

    char *data() const;

    size_t length() const;

    friend class stream;

  private:
    std::shared_ptr<apr::pool> _parent;
    svn_stringbuf_t *_value;
};
}

#endif
