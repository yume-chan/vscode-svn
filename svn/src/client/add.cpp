#include "client.hpp"

namespace Svn
{
Util_Method(Client::Add)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string"));

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));
    auto path = make_shared<string>(to_string(arg));
    Util_RejectIf(Util::ContainsNull(*path), Util_Error(Error, "Argument \"path\" must be a string without null bytes"));

    Util_PreparePool();

    client->add_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto _error = make_shared<svn_error_t *>();
    auto work = [path, client, pool, _error]() -> void {
        *_error = svn_client_add5(path->c_str(),      // path
                                  svn_depth_infinity, // depth
                                  true,               // force
                                  false,              // no_ignore
                                  false,              // no_autoprops
                                  true,               // add_parents
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
        return;
    };

    Util_RejectIf(Util::QueueWork(uv_default_loop(), move(work), move(after_work)), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;
}
