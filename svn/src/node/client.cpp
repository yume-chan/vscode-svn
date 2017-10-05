#include <svn_client.h>

#include "client.hpp"

#define SVN_CLIENT_METHOD_BEGIN(name)                        \
    V8_METHOD_BEGIN(client::name)                            \
        auto resolver = v8::New<Promise::Resolver>(context); \
        args.GetReturnValue().Set(resolver);

#define SVN_CLIENT_METHOD_END V8_METHOD_END

namespace node_svn {
Client::Client(Isolate* isolate, const Local<Object>& options)
    : _value() {}

SVN_CLIENT_METHOD_DEFINITION(Client::Add) {
}
} // namespace node_svn
