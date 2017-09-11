#include "client.hpp"

namespace Svn
{
Util_Method(Client::Checkout)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"url\" must be a string."));

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"url\" must be a string."));
    auto url = make_shared<string>(to_string(arg));
    Util_RejectIf(Util::ContainsNull(*url), Util_Error(Error, "Argument \"url\" must be a string without null bytes"));

    Util_RejectIf(args.Length() == 1, Util_Error(TypeError, "Argument \"path\" must be a string."));

    arg = args[1];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string."));
    auto path = make_shared<string>(to_string(arg));
    Util_RejectIf(Util::ContainsNull(*path), Util_Error(Error, "Argument \"path\" must be a string without null bytes"));

    Util_PreparePool();

    client->checkout_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto _error = make_shared<svn_error_t *>();
    auto work = [_result_rev, url, path, client, pool, _error]() -> void {
        svn_opt_revision_t revision{svn_opt_revision_head};
        *_error = svn_client_checkout3(*_result_rev,      // result_rev
                                       url->c_str(),      // URL
                                       path->c_str(),     // path
                                       &revision,         // peg_revision
                                       &revision,         // revision
                                       svn_depth_unknown, // depth
                                       false,             // ignore_externals
                                       false,             // allow_unver_obstructions
                                       client->context,   // ctx
                                       pool.get());       // pool
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _error]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);

        auto error = *_error;
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error->apr_err, error->message));

        resolver->Resolve(context, v8::Undefined(isolate));
    };

    Util_RejectIf(Util::QueueWork(uv_default_loop(), work, after_work), Util_Error(Error, "Failed to start async work"));
}
Util_MethodEnd;
}
