#ifndef NODE_SVN_CLIENT_CLIENT_H
#define NODE_SVN_CLIENT_CLIENT_H

#include <svn_types.h>
#include <svn_wc.h>

#include "../utils.hpp"
#include "../svn_error.hpp"
#include "../client.hpp"

#define Util_PreparePool()                                   \
    auto client = ObjectWrap::Unwrap<Client>(args.Holder()); \
    apr_pool_t *_pool;                                       \
    apr_pool_create(&_pool, client->pool);                   \
    auto pool = shared_ptr<apr_pool_t>(_pool, apr_pool_destroy);

#define Util_ToAprStringArray(arg) _to_apr_string_array(isolate, context, resolver, arg, _pool)

#define RunAsync() Util_RejectIf(Util::QueueWork<svn_error_t*>(uv_default_loop(), move(work), move(after_work)) != 0, Util_Error(Error, "Failed starting async work"))

namespace Svn
{
static inline string to_string(Local<Value> value)
{
    String::Utf8Value utf8(value);
    return string(*utf8, utf8.length());
}

static inline apr_array_header_t *_to_apr_string_array(Isolate *isolate, Local<Context> context, Local<Promise::Resolver> resolver, Local<Value> &arg, apr_pool_t *_pool)
{
    apr_array_header_t *result;
    if (arg->IsString())
    {
        auto value = Util_ToAprString(arg);
        Util_RejectIf(value == nullptr, Util_Error(Error, "Argument \"path\" must be a string without null bytes"), nullptr);

        result = apr_array_make(_pool, 1, sizeof(char *));
        APR_ARRAY_PUSH(result, const char *) = value;
    }
    else if (arg->IsArray())
    {
        auto array = arg.As<v8::Array>();
        result = apr_array_make(_pool, array->Length(), sizeof(char *));
        for (auto i = 0U; i < array->Length(); i++)
        {
            auto item = array->Get(context, i).ToLocalChecked();
            Util_RejectIf(!item->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string or array of string"), nullptr);

            auto value = Util_ToAprString(item);
            Util_RejectIf(value == nullptr, Util_Error(Error, "Argument \"path\" must be a string without null bytes"), nullptr);

            APR_ARRAY_PUSH(result, const char *) = value;
        }
    }
    else
    {
        Util_RejectIf(true, Util_Error(TypeError, "Argument \"path\" must be a string or array of string"), nullptr);
    }
    return result;
}
}

#endif
