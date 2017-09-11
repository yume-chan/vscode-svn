#ifndef NODE_SVN_CLIENT_CLIENT_H
#define NODE_SVN_CLIENT_CLIENT_H

#include "../client.hpp"

#define Util_PreparePool()                                      \
    auto client = ObjectWrap::Unwrap<Client>(args.Holder());    \
    shared_ptr<apr_pool_t> pool;                                \
    { /* Add a scope to hide extra variables */                 \
        apr_pool_t *_pool;                                      \
        apr_pool_create(&_pool, client->pool);                  \
        pool = shared_ptr<apr_pool_t>(_pool, apr_pool_destroy); \
    }

inline string to_string(Local<Value> value)
{
    String::Utf8Value utf8(value);
    return string(*utf8, utf8.length());
}

#endif
