#include "client.hpp"

namespace Svn
{
inline svn_error_t *invoke_callback(const svn_commit_info_t *commit_info, void *baton, apr_pool_t *pool)
{
    auto method = *static_cast<function<void(const svn_commit_info_t *)> *>(baton);
    method(commit_info);
    return SVN_NO_ERROR;
}

Util_Method(Client::Delete)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string or an array of string"));

    Util_PreparePool();

    apr_array_header_t *path;
    Util_ToAprStringArray(args[0], path);

    auto callback = [](const svn_commit_info_t *commit_info) -> void {

    };

    auto _callback = make_shared<function<void(const svn_commit_info_t *)>>(move(callback));
    auto work = [path, _callback, client, pool]() -> svn_error_t * {
        SVN_ERR(svn_client_delete4(path,            // paths
                                   true,            // force
                                   false,           // keep_local
                                   nullptr,         // revprop_table
                                   invoke_callback, // commit_callback
                                   _callback.get(), // commit_baton
                                   client->context, // ctx
                                   pool.get()));    // scratch_pool

        return nullptr;
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver](svn_error_t *error) -> void {
        HandleScope scope(isolate);
        auto context = isolate->GetCallingContext();

        auto resolver = _resolver->Get(isolate);
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        resolver->Resolve(context, Util_Undefined);
    };

    RunAsync();
}
Util_MethodEnd;
}
