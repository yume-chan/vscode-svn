#include "client.hpp"

namespace Svn
{
Util_Method(Client::Add)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string"));

    Util_PreparePool();

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));
    const char *path = Util_ToAprString(arg);
    Util_RejectIf(path == nullptr, Util_Error(Error, "Argument \"path\" must be a string without null bytes"));
    Util_CheckAbsolutePath(path);

    client->add_notify = [](const svn_wc_notify_t *notify) -> void {
    };

    auto work = [path, client, pool]() -> svn_error_t * {
        SVN_ERR(svn_client_add5(path,               // path
                                svn_depth_infinity, // depth
                                true,               // force
                                false,              // no_ignore
                                false,              // no_autoprops
                                true,               // add_parents
                                client->context,    // ctx
                                pool.get()));       // scratch_pool

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
