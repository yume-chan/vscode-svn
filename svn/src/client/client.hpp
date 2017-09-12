#ifndef NODE_SVN_CLIENT_CLIENT_H
#define NODE_SVN_CLIENT_CLIENT_H

#include "../utils.hpp"
#include "../svn_error.hpp"
#include "../client.hpp"

#define Util_PreparePool()                                      \
    auto client = ObjectWrap::Unwrap<Client>(args.Holder());    \
    shared_ptr<apr_pool_t> pool;                                \
    { /* Add a scope to hide extra variables */                 \
        apr_pool_t *_pool;                                      \
        apr_pool_create(&_pool, client->pool);                  \
        pool = shared_ptr<apr_pool_t>(_pool, apr_pool_destroy); \
    }

#define Util_ToAprString(value) _to_apr_string(isolate, context, resolver, value, pool)

#define Util_ToAprStringArray(arg) _to_apr_string_array(isolate, context, resolver, arg, pool)

namespace Svn
{
static inline string to_string(Local<Value> value)
{
    String::Utf8Value utf8(value);
    return string(*utf8, utf8.length());
}

static inline char *_to_apr_string(Isolate *isolate, Local<Context> context, Local<Promise::Resolver> resolver, Local<Value> &arg, shared_ptr<apr_pool_t> pool)
{
    String::Utf8Value string(arg);
    auto length = string.length();
    Util_RejectIf(Util::ContainsNull(*string, length), Util_Error(Error, "Argument \"path\" must be a string without null bytes"), nullptr);

    return static_cast<char *>(apr_pmemdup(pool.get(), *string, length + 1));
}

static inline apr_array_header_t *_to_apr_string_array(Isolate *isolate, Local<Context> context, Local<Promise::Resolver> resolver, Local<Value> &arg, shared_ptr<apr_pool_t> pool)
{
    apr_array_header_t *result;
    if (arg->IsString())
    {
        auto value = Util_ToAprString(arg);
        if (value == nullptr)
            return nullptr;

        result = apr_array_make(pool.get(), 1, sizeof(char *));
        APR_ARRAY_PUSH(result, const char *) = value;
    }
    else if (arg->IsArray())
    {
        auto array = arg.As<v8::Array>();
        result = apr_array_make(pool.get(), array->Length(), sizeof(char *));
        for (auto i = 0U; i < array->Length(); i++)
        {
            auto item = array->Get(context, i).ToLocalChecked();
            Util_RejectIf(!item->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string or array of string"), nullptr);

            auto value = Util_ToAprString(item);
            if (value == nullptr)
                return nullptr;

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
