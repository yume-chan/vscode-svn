#include "client.hpp"

namespace Svn
{
Util_Method(Client::ChangelistAdd)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    auto promise = resolver->GetPromise();
    Util_Return(promise);

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string or an array of string"));

    Util_PreparePool();

    apr_array_header_t *path;
    Util_ToAprStringArray(args[0], path);

    Util_RejectIf(args.Length() == 1, Util_Error(TypeError, "Argument \"changelist\" must be a string"));

    auto arg = args[1];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"changelist\" must be a string"));
    auto changelist = Util_ToAprString(arg);
    Util_RejectIf(changelist == nullptr, Util_Error(Error, "Argument \"changelist\" must be a string without null bytes"));

    auto work = [path, changelist, client, pool]() -> svn_error_t * {
        return svn_client_add_to_changelist(path,               // paths
                                            changelist,         // changelist
                                            svn_depth_infinity, // depth
                                            nullptr,            // changelists
                                            client->context,    // ctx
                                            pool.get());        // scratch_pool
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
