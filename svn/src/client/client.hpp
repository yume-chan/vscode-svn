#ifndef NODE_SVN_CLIENT_CLIENT_H
#define NODE_SVN_CLIENT_CLIENT_H

#include <svn_dirent_uri.h>
#include <svn_types.h>
#include <svn_path.h>
#include <svn_wc.h>

#include "../utils.hpp"
#include "../svn_error.hpp"
#include "../client.hpp"

#define Util_PreparePool()                                   \
    auto client = ObjectWrap::Unwrap<Client>(args.Holder()); \
    apr_pool_t *_pool;                                       \
    apr_pool_create(&_pool, client->pool);                   \
    auto pool = shared_ptr<apr_pool_t>(_pool, apr_pool_destroy);

#define Util_ToAprStringArray(arg, name)                                                                                                           \
    if (arg->IsString())                                                                                                                           \
    {                                                                                                                                              \
        auto value = Util_ToAprString(arg);                                                                                                        \
        Util_RejectIf(value == nullptr, Util_Error(Error, "Argument \"##name##\" must be a string without null bytes"));                           \
                                                                                                                                                   \
        name = apr_array_make(_pool, 1, sizeof(char *));                                                                                           \
        APR_ARRAY_PUSH(name, const char *) = value;                                                                                                \
    }                                                                                                                                              \
    else if (arg->IsArray())                                                                                                                       \
    {                                                                                                                                              \
        auto array = arg.As<v8::Array>();                                                                                                          \
        name = apr_array_make(_pool, array->Length(), sizeof(char *));                                                                             \
        for (auto i = 0U; i < array->Length(); i++)                                                                                                \
        {                                                                                                                                          \
            auto item = array->Get(context, i).ToLocalChecked();                                                                                   \
            Util_RejectIf(!item->IsString(), Util_Error(TypeError, "Argument \"##name##\" must be a string or an array of string"));               \
                                                                                                                                                   \
            auto value = Util_ToAprString(item);                                                                                                   \
            Util_RejectIf(value == nullptr, Util_Error(Error, "Argument \"##name##\" must be a string or an array of string without null bytes")); \
                                                                                                                                                   \
            APR_ARRAY_PUSH(name, const char *) = value;                                                                                            \
        }                                                                                                                                          \
    }                                                                                                                                              \
    else                                                                                                                                           \
    {                                                                                                                                              \
        Util_RejectIf(true, Util_Error(TypeError, "Argument \"path\" must be a string or an array of string"));                                    \
    }

#define Util_CheckAbsolutePath(path)                                             \
    if (!svn_path_is_url(path))                                                  \
    {                                                                            \
        auto error = svn_dirent_get_absolute(&path, path, _pool);                \
        Util_RejectIf(error != nullptr, SvnError::New(isolate, context, error)); \
    }

#define Util_AprAllocType(type) static_cast<type *>(apr_palloc(_pool, sizeof(type)))

#define RunAsync() Util_RejectIf(Util::QueueWork<svn_error_t *>(uv_default_loop(), move(work), move(after_work)) != 0, Util_Error(Error, "Failed starting async work"))

namespace Svn
{
static inline string to_string(Local<Value> value)
{
    String::Utf8Value utf8(value);
    return string(*utf8, utf8.length());
}
}

#endif
