#ifndef NODE_SVN_CLIENT_POOL_PTR_H
#define NODE_SVN_CLIENT_POOL_PTR_H

#include <memory>

using std::shared_ptr;

struct pool_ptr_deleter
{
    explicit pool_ptr_deleter(shared_ptr<apr_pool_t> pointer)
        : pointer(pointer)
    {
    }

    void operator()(void *ptr)
    {
        pointer.reset();
    }

  private:
    shared_ptr<apr_pool_t> pointer;
};

#define make_pool_ptr(T, ptr, pool) shared_ptr<T>(ptr, pool_ptr_deleter(pool));

#endif
