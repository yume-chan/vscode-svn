#include <string>

#include <uv/async.hpp>
#include <uv/semaphore.hpp>

#include "client.hpp"

#define InternalizedString(value) v8::New<String>(isolate, value, NewStringType::kInternalized, sizeof(value) - 1)

#define SetProperty(target, name, value) target->Set(context, InternalizedString(name), value)

namespace Svn
{
using std::to_string;

inline svn_error_t *invoke_callback(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool)
{
    auto method = *static_cast<function<void(const char *, const svn_client_status_t *, apr_pool_t *)> *>(baton);
    method(path, status, scratch_pool);
    return SVN_NO_ERROR;
}

struct StatusOptions
{
    svn_opt_revision_t revision;
    enum svn_depth_t depth;
    bool getAll;
    bool checkOutOfDate;
    bool checkWorkingCopy;
    bool noIgnore;
    bool ignoreExternals;
    bool depthAsSticky;
    apr_array_header_t *changelists;
};

V8_METHOD_BEGIN(Client::Status)
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

    auto options = pool->alloc<StatusOptions>();
    options->revision = svn_opt_revision_t{svn_opt_revision_working};
    options->depth = svn_depth_infinity;
    options->getAll = false;
    options->checkOutOfDate = false;
    options->checkWorkingCopy = false;
    options->noIgnore = false;
    options->ignoreExternals = false;
    options->depthAsSticky = true;
    options->changelists = nullptr;

    arg = args[1];
    if (arg->IsObject())
    {
        auto object = arg.As<Object>();

        auto depth = Util_GetProperty(object, "depth");
        if (depth->IsNumber())
            options->depth = static_cast<svn_depth_t>(depth->IntegerValue(context).ToChecked());

        auto getAll = Util_GetProperty(object, "getAll");
        if (getAll->IsBoolean())
            options->getAll = getAll->BooleanValue();

        auto checkOutOfDate = Util_GetProperty(object, "checkOutOfDate");
        if (checkOutOfDate->IsBoolean())
            options->checkOutOfDate = checkOutOfDate->BooleanValue();

        auto checkWorkingCopy = Util_GetProperty(object, "checkWorkingCopy");
        if (checkWorkingCopy->IsBoolean())
            options->checkWorkingCopy = checkWorkingCopy->BooleanValue();

        auto noIgnore = Util_GetProperty(object, "noIgnore");
        if (noIgnore->IsBoolean())
            options->noIgnore = noIgnore->BooleanValue();

        auto ignoreExternals = Util_GetProperty(object, "ignoreExternals");
        if (ignoreExternals->IsBoolean())
            options->ignoreExternals = ignoreExternals->BooleanValue();

        auto depthAsSticky = Util_GetProperty(object, "depthAsSticky");
        if (depthAsSticky->IsBoolean())
            options->depthAsSticky = depthAsSticky->BooleanValue();

        auto changelists = Util_GetProperty(object, "changelists");
        if (!changelists->IsUndefined())
            Util_ToAprStringArray(changelists, options->changelists);
    }

    auto result = Array::New(isolate);
    auto _result = Util_SharedPersistent(Array, result);
    auto semaphore = make_shared<Uv::Semaphore>();
    auto async_callback = [isolate, _result, semaphore](const svn_client_status_t *status) -> void {
        HandleScope scope(isolate);
        auto context = isolate->GetCallingContext();

        auto result = _result->Get(isolate);

        auto item = Object::New(isolate);
        SetProperty(item, "kind", Util_New(Integer, status->kind));
        SetProperty(item, "path", v8::New<String>(isolate, status->local_abspath));

        if (status->filesize <= INT32_MAX)
            SetProperty(item, "filesize", Util_New(Integer, static_cast<int32_t>(status->filesize)));
        else
            SetProperty(item, "filesize", v8::New<String>(isolate, to_string(status->filesize).c_str()));

        SetProperty(item, "versioned", Util_New(Boolean, status->versioned));
        SetProperty(item, "conflicted", Util_New(Boolean, status->conflicted));
        SetProperty(item, "nodeStatus", Util_New(Integer, status->node_status));
        SetProperty(item, "textStatus", Util_New(Integer, status->text_status));
        SetProperty(item, "propStatus", Util_New(Integer, status->prop_status));
        SetProperty(item, "copied", Util_New(Boolean, status->copied));
        SetProperty(item, "switched", Util_New(Boolean, status->switched));
        if (status->repos_root_url != nullptr)
            SetProperty(item, "repositoryUrl", v8::New<String>(isolate, status->repos_root_url));
        if (status->repos_relpath != nullptr)
            SetProperty(item, "relativePath", v8::New<String>(isolate, status->repos_relpath));
        result->Set(context, result->Length(), item);

        semaphore->post();
    };

    auto async = make_shared<Uv::Async<const svn_client_status_t *>>(uv_default_loop());
    auto send_callback = [async, async_callback, semaphore](const char *, const svn_client_status_t *status, apr_pool_t *) -> void {
        async->send(async_callback, status);
        semaphore->wait();
    };

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto _send_callback = make_shared<function<void(const char *, const svn_client_status_t *, apr_pool_t *)>>(move(send_callback));
    auto work = [_result_rev, client, path, options, _send_callback, _pool]() -> svn_error_t * {
        SVN_ERR(svn_client_status6(*_result_rev,              // result_rev
                                   client->context,           // ctx
                                   path,                      // path
                                   &options->revision,        // revision
                                   options->depth,            // depth
                                   options->getAll,           // get_all
                                   options->checkOutOfDate,   // check_out_of_date
                                   options->checkWorkingCopy, // check_working_copy
                                   options->noIgnore,         // no_ignore
                                   options->ignoreExternals,  // ignore_externals
                                   options->depthAsSticky,    // depth_as_sticky,
                                   options->changelists,      // changelists
                                   invoke_callback,           // status_func
                                   _send_callback.get(),      // status_baton
                                   _pool));                   // scratch_pool

        return nullptr;
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
            SetProperty(result, "revision", v8::New<Integer>(isolate, *result_rev));

        resolver->Resolve(context, result);
    };

    RunAsync();
}
V8_METHOD_END;
}
