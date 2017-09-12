#include "client.hpp"

namespace Svn
{
inline svn_error_t *invoke_callback(const svn_commit_info_t *commit_info, void *baton, apr_pool_t *pool)
{
    auto method = *static_cast<function<void(const svn_commit_info_t *commit_info)> *>(baton);
    method(commit_info);
    return SVN_NO_ERROR;
}

Util_Method(Client::Commit)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    auto promise = resolver->GetPromise();
    Util_Return(promise);

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string or array of string"));

    Util_PreparePool();

    auto paths = Util_ToAprStringArray(args[0]);
    if (paths == nullptr)
        return;

    Util_RejectIf(args.Length() == 1, Util_Error(TypeError, "Argument \"message\" must be a string"));

    auto arg = args[1];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"message\" must be a string"));
    auto message = Util_ToAprString(arg);
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
        *_error = svn_client_commit6(paths,              // targets
                                     svn_depth_infinity, // depth
                                     true,               // keep_locks
                                     false,              // keep_changelists
                                     false,              // commit_as_operations
                                     true,               // include_file_externals
                                     true,               // include_dir_externals
                                     nullptr,            // changelists
                                     nullptr,            // revprop_table
                                     invoke_callback,    // commit_callback
                                     _callback.get(),    // commit_baton
                                     client->context,    // ctx
                                     pool.get());        // scratch_pool
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _error]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);

        auto error = *_error;
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        resolver->Resolve(context, Util_Undefined);
    };

    Util_RejectIf(Util::QueueWork(uv_default_loop(), move(work), move(after_work)), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;
}
