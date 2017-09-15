#include <string>

#include "../uv/async.hpp"
#include "../uv/semaphore.hpp"

#include "client.hpp"

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

Util_Method(Client::Status)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string"));

    Util_PreparePool();

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));
    auto path = Util_ToAprString(arg);
    Util_RejectIf(path == nullptr, Util_Error(Error, "Argument \"path\" must be a string without null bytes"));

    auto options = make_shared<StatusOptions>();
    options->revision = svn_opt_revision_t{svn_opt_revision_working};
    options->depth = svn_depth_infinity;

    arg = args[1];
    if (arg->IsObject())
    {
        auto object = arg.As<Object>();

        auto depth = Util_GetProperty(object, "depth");
        if (depth->IsNumber())
            options->depth = static_cast<svn_depth_t>(depth->IntegerValue(context).ToChecked());

        options->getAll = Util_GetProperty(object, "getAll")->BooleanValue();
        options->checkOutOfDate = Util_GetProperty(object, "checkOutOfDate")->BooleanValue();
        options->checkWorkingCopy = Util_GetProperty(object, "checkWorkingCopy")->BooleanValue();
        options->noIgnore = Util_GetProperty(object, "noIgnore")->BooleanValue();
        options->ignoreExternals = Util_GetProperty(object, "ignoreExternals")->BooleanValue();
        options->depthAsSticky = Util_GetProperty(object, "depthAsSticky")->BooleanValue();

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
        Util_Set(item, "kind", Util_New(Integer, status->kind));
        Util_Set(item, "path", Util_String(status->local_abspath));

        if (status->filesize <= INT32_MAX)
            Util_Set(item, "filesize", Util_New(Integer, static_cast<int32_t>(status->filesize)));
        else
            Util_Set(item, "filesize", Util_String(to_string(status->filesize).c_str()));

        Util_Set(item, "versioned", Util_New(Boolean, status->versioned));
        Util_Set(item, "conflicted", Util_New(Boolean, status->conflicted));
        Util_Set(item, "nodeStatus", Util_New(Integer, status->node_status));
        Util_Set(item, "textStatus", Util_New(Integer, status->text_status));
        Util_Set(item, "propStatus", Util_New(Integer, status->prop_status));
        Util_Set(item, "copied", Util_New(Boolean, status->copied));
        Util_Set(item, "switched", Util_New(Boolean, status->switched));
        if (status->repos_root_url != nullptr)
            Util_Set(item, "repositoryUrl", Util_String(status->repos_root_url));
        if (status->repos_relpath != nullptr)
            Util_Set(item, "relativePath", Util_String(status->repos_relpath));
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
    auto work = [_result_rev, client, path, options, _send_callback, pool]() -> svn_error_t * {
        return svn_client_status6(*_result_rev,              // result_rev
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
                                  pool.get());               // scratch_pool
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
