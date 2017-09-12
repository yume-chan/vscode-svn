#include "client.hpp"

namespace Svn
{
// I need to copy the svn_client_status_t so I can use it in after_work.
// Is there any better way? For example constrcut V8 objects directly in _callback?
// (Currently I cannot find how to use V8 in work, I tried v8::Isolate->Enter and v8::Locker)
// (But v8::Locker cannot acquire the lock, and I worry the lag of thread switching)
struct SvnClientStatus
{
    SvnClientStatus(const svn_client_status_t *status) : kind(status->kind),
                                                         local_abspath(status->local_abspath),
                                                         filesize(status->filesize),
                                                         versioned(status->versioned),
                                                         conflicted(status->conflicted),
                                                         node_status(status->node_status),
                                                         text_status(status->text_status),
                                                         prop_status(status->prop_status),
                                                         copied(status->copied),
                                                         revision(status->revision),
                                                         changed_rev(status->changed_rev),
                                                         switched(status->switched) {}

    svn_node_kind_t kind;
    string local_abspath;
    svn_filesize_t filesize;
    svn_boolean_t versioned;
    svn_boolean_t conflicted;
    enum svn_wc_status_kind node_status;
    enum svn_wc_status_kind text_status;
    enum svn_wc_status_kind prop_status;
    svn_boolean_t copied;
    svn_revnum_t revision;
    svn_revnum_t changed_rev;
    svn_boolean_t switched;
};

inline svn_error_t *invoke_callback(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool)
{
    auto method = *static_cast<function<void(const char *, const svn_client_status_t *, apr_pool_t *)> *>(baton);
    method(path, status, scratch_pool);
    return SVN_NO_ERROR;
}

Util_Method(Client::Status)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string"));

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));
    auto path = make_shared<string>(to_string(arg));
    Util_RejectIf(Util::ContainsNull(*path), Util_Error(Error, "Argument \"path\" must be a string without null bytes"));

    Util_PreparePool();

    auto list = make_shared<vector<SvnClientStatus>>();
    auto callback = [list](const char *path, const svn_client_status_t *status, apr_pool_t *) -> void {
        list->emplace_back(status);
    };

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto _error = make_shared<svn_error_t *>();
    auto _callback = make_shared<function<void(const char *, const svn_client_status_t *, apr_pool_t *)>>(move(callback));
    auto work = [_result_rev, client, path, _callback, pool, _error]() -> void {
        svn_opt_revision_t revision{svn_opt_revision_working};
        *_error = svn_client_status6(*_result_rev,       // result_rev
                                     client->context,    // ctx
                                     path->c_str(),      // path
                                     &revision,          // revision
                                     svn_depth_infinity, // depth
                                     false,              // get_all
                                     false,              // check_out_of_date
                                     false,              // check_working_copy
                                     false,              // no_ignore
                                     false,              // ignore_externals
                                     false,              // depth_as_sticky,
                                     nullptr,            // changelists
                                     invoke_callback,    // status_func
                                     _callback.get(),    // status_baton
                                     pool.get());        // scratch_pool
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _error, list, _result_rev]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);

        auto error = *_error;
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        auto result = Array::New(isolate);

        for (auto status = list->begin(); status < list->end(); status++)
        {
            auto item = Object::New(isolate);
            Util_Set(item, "path", Util_StringFromStd(status->local_abspath));
            Util_Set(item, "kind", Util_New(Integer, status->kind));
            Util_Set(item, "textStatus", Util_New(Integer, status->text_status));
            Util_Set(item, "nodeStatus", Util_New(Integer, status->node_status));
            Util_Set(item, "propStatus", Util_New(Integer, status->prop_status));
            Util_Set(item, "versioned", Util_New(Boolean, status->versioned));
            Util_Set(item, "conflicted", Util_New(Boolean, status->conflicted));
            Util_Set(item, "copied", Util_New(Boolean, status->copied));
            Util_Set(item, "switched", Util_New(Boolean, status->switched));
            result->Set(context, result->Length(), item);
        }

        auto result_rev = *_result_rev;
        if (result_rev != nullptr)
            Util_Set(result, "revision", Util_New(Integer, *result_rev));

        resolver->Resolve(context, result);
    };

    Util_RejectIf(Util::QueueWork(uv_default_loop(), move(work), move(after_work)), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;
}
