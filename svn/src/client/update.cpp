#include "client.hpp"

namespace Svn
{
Util_Method(Client::Update)
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

    client->update_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto _result_rev = make_shared<apr_array_header_t *>();
    auto _error = make_shared<svn_error_t *>();
    auto work = [_result_rev, paths, client, pool, _error]() -> void {
        svn_opt_revision_t revision{svn_opt_revision_working};
        *_error = svn_client_update4(_result_rev.get(),  // result_revs
                                     paths,              // paths
                                     &revision,          // revision
                                     svn_depth_infinity, // depth
                                     false,              // depth_is_sticky
                                     false,              // ignore_externals
                                     false,              // allow_unver_obstructions
                                     true,               // adds_as_modification
                                     true,               // make_parents
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

        resolver->Resolve(context, v8::Undefined(isolate));
    };

    Util_RejectIf(Util::QueueWork(uv_default_loop(), work, after_work), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;
}
