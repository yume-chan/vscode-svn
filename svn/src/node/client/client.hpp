#ifndef NODE_SVN_CLIENT_CLIENT_H
#define NODE_SVN_CLIENT_CLIENT_H

#include <svn_dirent_uri.h>
#include <svn_types.h>
#include <svn_path.h>
#include <svn_wc.h>

#include <svn/client.hpp>

#include <uv/future.hpp>

#include "../utils.hpp"
#include "../svn_error.hpp"
#include "../client.hpp"

#define Util_PreparePool()                                   \
    auto client = ObjectWrap::Unwrap<Client>(args.Holder()); \
    auto pool = client->_parent->create();

#define Util_ToAprStringArray(arg, name)                                                                                                           \
    if (arg->IsString())                                                                                                                           \
    {                                                                                                                                              \
        auto value = to_string(arg);                                                                                                               \
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
        Util_RejectIf(true, Util_Error(TypeError, "Argument \"##name##\" must be a string or an array of string"));                                \
    }

#define RunAsync() Util_RejectIf(Util::QueueWork<svn_error_t *>(uv_default_loop(), move(work), move(after_work)) != 0, Util_Error(Error, "Failed starting async work"))

#endif
