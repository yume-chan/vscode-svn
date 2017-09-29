#include "client.hpp"

namespace Svn
{
V8_METHOD_BEGIN(Client::Checkout)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"url\" must be a string."));

    Util_PreparePool();

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"url\" must be a string"));
    auto url = Util_ToAprString(arg);
    Util_RejectIf(url == nullptr, Util_Error(Error, "Argument \"url\" must be a string without null bytes"));

    Util_RejectIf(args.Length() == 1, Util_Error(TypeError, "Argument \"path\" must be a string."));

    arg = args[1];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));
    const char *path = Util_ToAprString(arg);
    Util_RejectIf(path == nullptr, Util_Error(Error, "Argument \"path\" must be a string without null bytes"));
    Util_CheckAbsolutePath(path);

    client->checkout_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto work = [_result_rev, url, path, client, pool]() -> svn_error_t * {
        svn_opt_revision_t revision{svn_opt_revision_head};
        SVN_ERR(svn_client_checkout3(*_result_rev,      // result_rev
                                     url,               // URL
                                     path,              // path
                                     &revision,         // peg_revision
                                     &revision,         // revision
                                     svn_depth_unknown, // depth
                                     false,             // ignore_externals
                                     false,             // allow_unver_obstructions
                                     client->context,   // ctx
                                     pool.get()));      // pool

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
V8_METHOD_END;
}
