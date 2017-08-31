#include "client.h"

namespace Svn
{
Util_Method(Client::Status)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_Reject(args.Length() < 1, Util_Error(TypeError, "Argument `path` is required."));

    auto arg = args[0];
    Util_Reject(arg->IsString(), Util_Error(TypeError, "Argument `path` must be a string."));
    auto path = make_shared<string>(to_string(arg));

    Util_PreparePool();

    auto result = Array::New(isolate);
    auto _result = new Persistent<Array>(isolate, result);
    auto callback = [isolate, _result](const char *path, const svn_client_status_t *status, apr_pool_t *) -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto item = Object::New(isolate);
        Util_Set(item, "path", Util_String(path));
        Util_Set(item, "kind", Util_New(Integer, status->kind));
        Util_Set(item, "textStatus", Util_New(Integer, status->text_status));
        Util_Set(item, "propStatus", Util_New(Integer, status->prop_status));
        Util_Set(item, "copied", Util_New(Boolean, status->copied));
        Util_Set(item, "switched", Util_New(Boolean, status->switched));

        auto result = _result->Get(isolate);
        result->Set(context, result->Length(), item);
    };

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto _error = make_shared<svn_error_t *>();
    auto _callback = make_shared<function<void(const char *, const svn_client_status_t *, apr_pool_t *)>>(callback);
    auto work = [_result_rev, client, path, _callback, _error, pool]() -> void {
        svn_opt_revision_t revision{svn_opt_revision_working};
        *_error = svn_client_status6(*_result_rev,            // result_rev
                                     client->context,         // ctx
                                     path->c_str(),           // path
                                     &revision,               // revision
                                     svn_depth_infinity,      // depth
                                     false,                   // get_all
                                     false,                   // check_out_of_date
                                     false,                   // check_working_copy
                                     false,                   // no_ignore
                                     false,                   // ignore_externals
                                     false,                   // depth_as_sticky,
                                     nullptr,                 // changelists
                                     Util::SvnStatusCallback, // status_func
                                     _callback.get(),         // status_baton
                                     pool.get());             // scratch_pool
    };

    auto _resolver = Util_Persistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _result, _error, _result_rev]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);
        _resolver->Reset();
        delete _resolver;

        auto error = *_error;
        Util_Reject(error == SVN_NO_ERROR, SvnError::New(isolate, context, error->apr_err, error->message));

        auto result = _result->Get(isolate);
        _result->Reset();
        delete _result;

        auto result_rev = *_result_rev;
        if (result_rev != nullptr)
            Util_Set(result, "revision", Util_New(Integer, *result_rev));

        resolver->Resolve(context, result);
        return;
    };

    Util_Reject(Util::QueueWork(uv_default_loop(), work, after_work), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;
}
