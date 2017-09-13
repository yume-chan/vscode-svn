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

    auto paths = Util_ToAprStringArray(args[0]);
    if (paths == nullptr)
        return;

    client->update_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto _result_rev = make_shared<apr_array_header_t *>();
    auto work = [_result_rev, paths, client, pool]() -> svn_error_t * {
        svn_opt_revision_t revision{svn_opt_revision_working};
        return svn_client_update4(_result_rev.get(),  // result_revs
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
