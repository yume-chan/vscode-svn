#include "../uv/async.hpp"
#include "../uv/semaphore.hpp"

#include "client.hpp"
#include "revision.hpp"
#include "pool_ptr.hpp"

#define InternalizedString(value) v8::New<String>(isolate, value, NewStringType::kInternalized, sizeof(value) - 1)

#define SetProperty(target, name, value) target->Set(context, InternalizedString(name), value)

namespace Svn
{
inline svn_error_t *invoke_callback(void *baton, const char *path, const svn_client_info2_t *info, apr_pool_t *scratch_pool)
{
    auto method = *static_cast<function<void(const char *, const svn_client_info2_t *)> *>(baton);
    method(path, info);
    return SVN_NO_ERROR;
}

struct InfoOptions
{
    svn_opt_revision_t pegRevision;
    svn_opt_revision_t revision;
    enum svn_depth_t depth;
    bool fetchExcluded;
    bool fetchActualOnly;
    bool includeExternals;
    apr_array_header_t *changelists;
};

V8_METHOD_BEGIN(Client::Info)
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

    auto options = make_pool_ptr<InfoOptions>(pool);
    options->pegRevision = svn_opt_revision_t{svn_opt_revision_unspecified};
    options->revision = svn_opt_revision_t{svn_opt_revision_unspecified};
    options->depth = svn_depth_empty;
    options->fetchExcluded = true;
    options->fetchActualOnly = true;
    options->includeExternals = false;
    options->changelists = nullptr;

    arg = args[1];
    if (arg->IsObject())
    {
        auto object = arg.As<Object>();

        options->pegRevision = ParseRevision(isolate, context, Util_GetProperty(object, "pegRevision"), svn_opt_revision_unspecified);
        options->revision = ParseRevision(isolate, context, Util_GetProperty(object, "revision"), svn_opt_revision_unspecified);

        auto depth = Util_GetProperty(object, "depth");
        if (depth->IsNumber())
            options->depth = static_cast<svn_depth_t>(depth->IntegerValue(context).ToChecked());

        auto fetchExcluded = Util_GetProperty(object, "fetchExcluded");
        if (fetchExcluded->IsBoolean())
            options->fetchExcluded = fetchExcluded->BooleanValue();

        auto fetchActualOnly = Util_GetProperty(object, "fetchActualOnly");
        if (fetchActualOnly->IsBoolean())
            options->fetchActualOnly = fetchActualOnly->BooleanValue();

        auto includeExternals = Util_GetProperty(object, "includeExternals");
        if (includeExternals->IsBoolean())
            options->includeExternals = includeExternals->BooleanValue();

        auto changelists = Util_GetProperty(object, "changelists");
        if (!changelists->IsUndefined())
            Util_ToAprStringArray(changelists, options->changelists);
    }

    auto result = Array::New(isolate);
    auto _result = Util_SharedPersistent(Array, result);
    auto semaphore = make_shared<Uv::Semaphore>();
    auto async_callback = [isolate, _result, semaphore](const char *path, const svn_client_info2_t *info) -> void {
        HandleScope scope(isolate);
        auto context = isolate->GetCallingContext();

        auto item = Object::New(isolate);

        SetProperty(item, "path", v8::New<String>(isolate, path));

        auto wc_info = info->wc_info;
        if (wc_info != nullptr)
        {
            auto workingCopy = Object::New(isolate);
            SetProperty(workingCopy, "rootPath", v8::New<String>(isolate, wc_info->wcroot_abspath));
            SetProperty(item, "workingCopy", workingCopy);
        }

        // SetProperty(item, "kind", Util_New(Integer, status->kind));
        // SetProperty(item, "textStatus", Util_New(Integer, status->text_status));
        // SetProperty(item, "nodeStatus", Util_New(Integer, status->node_status));
        // SetProperty(item, "propStatus", Util_New(Integer, status->prop_status));
        // SetProperty(item, "versioned", Util_New(Boolean, status->versioned));
        // SetProperty(item, "conflicted", Util_New(Boolean, status->conflicted));
        // SetProperty(item, "copied", Util_New(Boolean, status->copied));
        // SetProperty(item, "switched", Util_New(Boolean, status->switched));

        auto result = _result->Get(isolate);
        result->Set(context, result->Length(), item);

        semaphore->post();
    };

    auto async = make_shared<Uv::Async<const char *, const svn_client_info2_t *>>(uv_default_loop());
    auto send_callback = [async, async_callback, semaphore](const char *path, const svn_client_info2_t *info) -> void {
        async->send(async_callback, path, info);
        semaphore->wait();
    };

    auto _send_callback = make_shared<function<void(const char *, const svn_client_info2_t *)>>(move(send_callback));
    auto work = [client, path, options, _send_callback, _pool]() -> svn_error_t * {
        SVN_ERR(svn_client_info4(path,                      // abspath_or_url
                                 &options->pegRevision,     // peg_revision
                                 &options->revision,        // revision
                                 options->depth,            // depth
                                 options->fetchExcluded,    // fetch_excluded
                                 options->fetchActualOnly,  // fetch_actual_only
                                 options->includeExternals, // include_externals
                                 options->changelists,      // changelists
                                 invoke_callback,           // receiver
                                 _send_callback.get(),      // receiver_baton
                                 client->context,           // ctx
                                 _pool));                   // scratch_pool

        return nullptr;
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _result, options](svn_error_t *error) -> void {
        HandleScope scope(isolate);
        auto context = isolate->GetCallingContext();

        auto resolver = _resolver->Get(isolate);
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        auto result = _result->Get(isolate);
        if (options->depth == svn_depth_empty && result->Length() == 1)
            resolver->Resolve(context, result->Get(context, 0).ToLocalChecked());
        else
            resolver->Resolve(context, result);
    };

    RunAsync();
}
V8_METHOD_END;
}
