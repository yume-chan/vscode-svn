#include "client.h"

namespace Svn
{
Util_Method(Client::Update)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    auto promise = resolver->GetPromise();
    Util_Return(promise);

    Util_Reject(args.Length() < 1, Util_Error(TypeError, "Argument `path` is required."));

    Util_PreparePool();

    auto arg = args[0];
    auto strings = vector<string>();
    apr_array_header_t *_paths;
    if (arg->IsString())
    {
        auto path = to_string(arg);
        _paths = apr_array_make(pool.get(), 1, sizeof(char *));
        strings.push_back(std::move(path));
    }
    else if (arg->IsArray())
    {
        auto paths = arg.As<v8::Array>();
        _paths = apr_array_make(pool.get(), paths->Length(), sizeof(char *));
        for (auto i = 0U; i < paths->Length(); i++)
        {
            auto string = to_string(paths->Get(context, i).ToLocalChecked());
            APR_ARRAY_IDX(_paths, i, const char *) = string.c_str();
            strings.push_back(std::move(string));
        }
    }
    else
    {
        Util_Reject(false, Util_Error(TypeError, "Argument `path` must be a string."));
    }

    client->update_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto _result_rev = make_shared<apr_array_header_t *>();
    auto _error = make_shared<svn_error_t *>();
    auto work = [_result_rev, _paths, client, pool, _error]() -> void {
        svn_opt_revision_t revision{svn_opt_revision_working};
        *_error = svn_client_update4(_result_rev.get(),  // result_revs
                                     _paths,             // paths
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

    auto _resolver = Util_Persistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _error]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);
        _resolver->Reset();
        delete _resolver;

        auto error = *_error;
        Util_Reject(error == SVN_NO_ERROR, SvnError::New(isolate, context, error->apr_err, error->message));

        resolver->Resolve(context, v8::Undefined(isolate));
    };

    Util_Reject(Util::QueueWork(uv_default_loop(), work, after_work), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;
}
