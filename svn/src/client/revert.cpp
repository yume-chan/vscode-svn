#include "client.hpp"

namespace Svn
{
Util_Method(Client::Revert)
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
        String::Utf8Value string(arg);
        auto length = string.length();
        Util_RejectIf(Util::ContainsNull(*string, length), Util_Error(Error, "Argument \"path\" must be a string without null bytes"));

        length++;
        auto c = (char *)apr_pcalloc(pool.get(), length);
        apr_cpystrn(c, *string, length);

        paths = apr_array_make(pool.get(), 1, sizeof(char *));
        APR_ARRAY_PUSH(paths, const char *) = c;
    }
    else if (arg->IsArray())
    {
        auto array = arg.As<v8::Array>();
        paths = apr_array_make(pool.get(), array->Length(), sizeof(char *));
        for (auto i = 0U; i < array->Length(); i++)
        {
            auto value = array->Get(context, i).ToLocalChecked();
            Util_RejectIf(!value->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string or array of string"));

            String::Utf8Value string(value);
            auto length = string.length();
            Util_RejectIf(Util::ContainsNull(*string, length), Util_Error(Error, "Argument \"path\" must be an array of string without null bytes"));

            length++;
            auto c = (char *)apr_pcalloc(pool.get(), length);
            apr_cpystrn(c, *string, length);

            APR_ARRAY_PUSH(paths, const char *) = c;
        }
    }
    else
    {
        Util_RejectIf(true, Util_Error(TypeError, "Argument \"path\" must be a string or array of string"));
    }

    client->revert_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto _error = make_shared<svn_error_t *>();
    auto work = [paths, client, pool, _error]() -> void {
        *_error = svn_client_revert3(paths,              // paths
                                     svn_depth_infinity, // depth
                                     nullptr,            // changelists
                                     true,               // clear_changelists
                                     false,              // metadata_only
                                     client->context,    // ctx
                                     pool.get());        // pool
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _error]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);

        auto error = *_error;
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        resolver->Resolve(context, Util_Undefined);
        return;
    };

    Util_RejectIf(Util::QueueWork(uv_default_loop(), move(work), move(after_work)), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;
}
