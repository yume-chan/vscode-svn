#include "client.hpp"

// #define to_apr_string(value)                                                                                                            \
//     {                                                                                                                                   \
//         String::Utf8Value string(arg);                                                                                                  \
//         auto length = string.length();                                                                                                  \
//         Util_RejectIf(Util::ContainsNull(*string, length), Util_Error(Error, "Argument \"path\" must be a string without null bytes")); \
//                                                                                                                                         \
//         length++;                                                                                                                       \
//     }

namespace Svn
{
inline char *_to_apr_string(Isolate *isolate, Local<Context> context, Local<Promise::Resolver> resolver, shared_ptr<apr_pool_t> pool, Local<Value> &arg)
{
    String::Utf8Value string(arg);
    auto length = string.length();
    Util_RejectIf(Util::ContainsNull(*string, length), Util_Error(Error, "Argument \"path\" must be a string without null bytes"), nullptr);

    length++;
    auto result = (char *)apr_pcalloc(pool.get(), length);
    apr_cpystrn(result, *string, length);

    return result;
}

#define to_apr_string(value) _to_apr_string(isolate, context, resolver, pool, value)

Util_Method(Client::Commit)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    auto promise = resolver->GetPromise();
    Util_Return(promise);

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string or array of string"));

    Util_PreparePool();

    auto arg = args[0];
    apr_array_header_t *paths;
    if (arg->IsString())
    {
        auto path = to_apr_string(arg);
        if (path == nullptr)
            return;

        paths = apr_array_make(pool.get(), 1, sizeof(char *));
        APR_ARRAY_PUSH(paths, const char *) = path;
    }
    else if (arg->IsArray())
    {
        auto array = arg.As<v8::Array>();
        paths = apr_array_make(pool.get(), array->Length(), sizeof(char *));
        for (auto i = 0U; i < array->Length(); i++)
        {
            auto value = array->Get(context, i).ToLocalChecked();
            Util_RejectIf(!value->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string or array of string"));

            auto path = to_apr_string(value);
            if (path == nullptr)
                return;

            APR_ARRAY_PUSH(paths, const char *) = path;
        }
    }
    else
    {
        Util_RejectIf(true, Util_Error(TypeError, "Argument \"path\" must be a string or array of string"));
    }

    Util_RejectIf(args.Length() == 1, Util_Error(TypeError, "Argument \"message\" must be a string"));

    arg = args[1];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"message\" must be a string"));
    auto message = to_apr_string(arg);
    if (message == nullptr)
        return;
    client->context->log_msg_baton3 = static_cast<void *>(message);

    client->commit_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto callback = [](const svn_commit_info_t *commit_info) -> void {

    };

    auto _error = make_shared<svn_error_t *>();
    auto _callback = make_shared<function<void(const svn_commit_info_t *)>>(move(callback));
    auto work = [paths, _callback, client, pool, _error]() -> void {
        *_error = svn_client_commit6(paths,                   // targets
                                     svn_depth_infinity,      // depth
                                     true,                    // keep_locks
                                     false,                   // keep_changelists
                                     false,                   // commit_as_operations
                                     true,                    // include_file_externals
                                     true,                    // include_dir_externals
                                     nullptr,                 // changelists
                                     nullptr,                 // revprop_table
                                     Util::SvnCommitCallback, // commit_callback
                                     _callback.get(),         // commit_baton
                                     client->context,         // ctx
                                     pool.get());             // scratch_pool
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _error]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);

        auto error = *_error;
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error->apr_err, error->message));

        resolver->Resolve(context, Util_Undefined);
        return;
    };

    Util_RejectIf(Util::QueueWork(uv_default_loop(), move(work), move(after_work)), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;
}
