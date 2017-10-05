#ifndef NODE_SVN_UTILS_H
#define NODE_SVN_UTILS_H

#include <functional>
#include <thread>

#include <svn_client.h>

#include <apr/apr.hpp>

#include "v8.hpp"

using std::function;
using std::make_shared;
using std::make_unique;
using std::move;
using std::shared_ptr;
using std::string;
using std::vector;

#define Util_New(type, value) type::New(isolate, (value))
#define Util_NewMaybe(type, ...) type::New(context, __VA_ARGS__).ToLocalChecked()

#define Util_Persistent(type, value) new Persistent<type, v8::CopyablePersistentTraits<type>>(isolate, value)
#define Util_SharedPersistent(type, value) make_shared<Persistent<type, v8::CopyablePersistentTraits<type>>>(isolate, value)

#define Util_Undefined Undefined(isolate)

#define ReadOnlyDontDelete (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete)

#define Util_Return(value) args.GetReturnValue().Set(value);

#define Util_Error(type, message) Exception::type(v8::New<String>(isolate, message))

#define Util_ThrowIf(expression, error, ...) \
    if (expression)                          \
    {                                        \
        isolate->ThrowException(error);      \
        return __VA_ARGS__;                  \
    }

#define Util_RejectIf(expression, error, ...) \
    if (expression)                           \
    {                                         \
        resolver->Reject(context, error);     \
        return __VA_ARGS__;                   \
    }

#define Util_FunctionTemplate(callback, length)                       \
    v8::FunctionTemplate::New(isolate,                /* isolate */   \
                              callback,               /* callback */  \
                              Local<Value>(),         /* data */      \
                              Local<v8::Signature>(), /* signature */ \
                              length);                /* length */

#define Util_GetProperty(object, name) (object)->Get(context, v8::New<String>(isolate, name)).ToLocalChecked()

namespace node_svn
{
namespace util
{
static inline std::string to_string(Local<Value> &arg)
{
    String::Utf8Value string(arg);
    auto length = string.length();
    return std::string(*string, length);
}
}
}

#endif
