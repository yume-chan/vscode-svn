#include "../uv/async.hpp"
#include "../uv/semaphore.hpp"

#include "client.hpp"

namespace Svn
{
inline svn_error_t *invoke_callback(void *baton, const char *path, const svn_client_info2_t *info, apr_pool_t *scratch_pool)
{
    auto method = *static_cast<function<void(const char *, const svn_client_info2_t *, apr_pool_t *)> *>(baton);
    method(path, info, scratch_pool);
    return SVN_NO_ERROR;
}

Util_Method(Client::Info)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string"));

    Util_PreparePool();

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));
    auto path = Util_ToAprString(arg);
    Util_RejectIf(path == nullptr, Util_Error(Error, "Argument \"path\" must be a string without null bytes"));

    auto result = Array::New(isolate);
    auto _result = Util_SharedPersistent(Array, result);
    auto semaphore = make_shared<Uv::Semaphore>();
    auto async_callback = [isolate, _result, semaphore](const svn_client_info2_t *status) -> void {
        HandleScope scope(isolate);
        auto context = isolate->GetCallingContext();

        auto result = _result->Get(isolate);

        auto item = Object::New(isolate);
        // Util_Set(item, "path", Util_String(status->local_abspath));
        // Util_Set(item, "kind", Util_New(Integer, status->kind));
        // Util_Set(item, "textStatus", Util_New(Integer, status->text_status));
        // Util_Set(item, "nodeStatus", Util_New(Integer, status->node_status));
        // Util_Set(item, "propStatus", Util_New(Integer, status->prop_status));
        // Util_Set(item, "versioned", Util_New(Boolean, status->versioned));
        // Util_Set(item, "conflicted", Util_New(Boolean, status->conflicted));
        // Util_Set(item, "copied", Util_New(Boolean, status->copied));
        // Util_Set(item, "switched", Util_New(Boolean, status->switched));
        result->Set(context, result->Length(), item);

        semaphore->post();
    };

    auto async = make_shared<Uv::Async<const svn_client_info2_t *>>(uv_default_loop());
    auto send_callback = [async, async_callback, semaphore](const char *, const svn_client_info2_t *info, apr_pool_t *) -> void {
        async->send(async_callback, info);
        semaphore->wait();
    };

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto _send_callback = make_shared<function<void(const char *, const svn_client_info2_t *, apr_pool_t *)>>(move(send_callback));
    auto work = [_result_rev, client, path, _send_callback, pool]() -> svn_error_t * {
        svn_opt_revision_t revision{svn_opt_revision_working};
        return svn_client_info4(path,                 // abspath_or_url
                                &revision,            // peg_revision
                                &revision,            // revision
                                svn_depth_infinity,   // depth
                                false,                // fetch_excluded
                                true,                 // fetch_actual_only
                                true,                 // include_externals
                                nullptr,              // changelists
                                invoke_callback,      // receiver
                                _send_callback.get(), // receiver_baton
                                client->context,      // ctx
                                pool.get());          // scratch_pool
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _result, _result_rev](svn_error_t *error) -> void {
        HandleScope scope(isolate);
        auto context = isolate->GetCallingContext();

        auto resolver = _resolver->Get(isolate);
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        auto result = _result->Get(isolate);

        auto result_rev = *_result_rev;
        if (result_rev != nullptr)
            Util_Set(result, "revision", Util_New(Integer, *result_rev));

        resolver->Resolve(context, result);
    };

    RunAsync();
}
Util_MethodEnd;
}
