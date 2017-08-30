#include "client.h"
#include "svn_error.h"

namespace Svn
{
using std::make_shared;
using std::make_unique;
using std::function;
using std::shared_ptr;
using std::string;
using std::vector;

using v8::Array;
using v8::Boolean;
using v8::Context;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::HandleScope;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::NewStringType;
using v8::Persistent;
using v8::Promise;
using v8::PropertyAttribute;
using v8::Value;
using v8::Promise;

#define DefineReadOnlyValue(object, name, value) (object)->DefineOwnProperty(context, String::NewFromUtf8(isolate, (name), NewStringType::kNormal).ToLocalChecked(), (value), (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete))
#define DefineReadOnlyValueInt(object, name, value) (object)->DefineOwnProperty(context, String::NewFromUtf8(isolate, (name), NewStringType::kNormal).ToLocalChecked(), Integer::New(isolate, (value)), (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete))

Persistent<Function> Client::constructor;

void Client::Init(Local<Object> exports, Isolate *isolate, Local<Context> context)
{
    auto template_ = FunctionTemplate::New(isolate, New);
    template_->SetClassName(String::NewFromUtf8(isolate, "Client"));
    // This internal field is used for saving the pointer to a Client instance.
    // Client.wrap will set its pointer to the internal field
    // And ObjectWrap::Unwrap will read the internal field and cast it to Client.
    template_->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(template_, "status", Status);
    NODE_SET_PROTOTYPE_METHOD(template_, "cat", Cat);

    auto function = template_->GetFunction();

    auto kind = Object::New(isolate);
    DefineReadOnlyValueInt(kind, "none", svn_node_none);
    DefineReadOnlyValueInt(kind, "file", svn_node_file);
    DefineReadOnlyValueInt(kind, "dir", svn_node_dir);
    DefineReadOnlyValueInt(kind, "unknown", svn_node_unknown);
    DefineReadOnlyValue(function, "Kind", kind);

    auto statusKind = Object::New(isolate);
    DefineReadOnlyValueInt(statusKind, "none", svn_wc_status_none);
    DefineReadOnlyValueInt(statusKind, "unversioned", svn_wc_status_unversioned);
    DefineReadOnlyValueInt(statusKind, "normal", svn_wc_status_normal);
    DefineReadOnlyValueInt(statusKind, "added", svn_wc_status_added);
    DefineReadOnlyValueInt(statusKind, "missing", svn_wc_status_missing);
    DefineReadOnlyValueInt(statusKind, "deleded", svn_wc_status_deleted);
    DefineReadOnlyValueInt(statusKind, "replaced", svn_wc_status_replaced);
    DefineReadOnlyValueInt(statusKind, "modified", svn_wc_status_modified);
    DefineReadOnlyValueInt(statusKind, "conflicted", svn_wc_status_conflicted);
    DefineReadOnlyValueInt(statusKind, "ignored", svn_wc_status_ignored);
    DefineReadOnlyValueInt(statusKind, "obstructed", svn_wc_status_obstructed);
    DefineReadOnlyValueInt(statusKind, "external", svn_wc_status_external);
    DefineReadOnlyValueInt(statusKind, "incomplete", svn_wc_status_incomplete);
    DefineReadOnlyValue(function, "StatusKind", statusKind);

    constructor.Reset(isolate, function);

    DefineReadOnlyValue(exports, "Client", function);
}

void notify2(void *baton, const svn_wc_notify_t *notify, apr_pool_t *pool)
{
    auto client = static_cast<Client *>(baton);

    switch (notify->action)
    {
    case svn_wc_notify_update_delete:
    case svn_wc_notify_update_add:
    case svn_wc_notify_update_update:
    case svn_wc_notify_update_external:
    case svn_wc_notify_update_replace:
    case svn_wc_notify_update_skip_obstruction:
    case svn_wc_notify_update_skip_working_only:
    case svn_wc_notify_update_skip_access_denied:
    case svn_wc_notify_update_external_removed:
    case svn_wc_notify_update_shadowed_add:
    case svn_wc_notify_update_shadowed_update:
    case svn_wc_notify_update_shadowed_delete:
        auto callback = client->update_callback;
        if (callback != nullptr)
            callback(notify);
        break;
    }

    auto callback = *static_cast<function<void(const svn_wc_notify_t *)> *>(baton);
    callback(notify);
}

Client::Client()
{
    apr_initialize();
    apr_pool_create(&pool, nullptr);
    svn_client_create_context(&context, this->pool);

    context->notify_baton2 = this;
    context->notify_func2 = notify2;
}

Client::~Client()
{
    apr_pool_destroy(pool);
    apr_terminate();
}

void Client::New(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();

    if (!args.IsConstructCall())
    {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Class constructor Client cannot be invoked without 'new'")));
        return;
    }

    Client *result = new Client();
    result->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
}

string to_string(Local<Value> value)
{
    String::Utf8Value utf8(value);
    return string(*utf8, utf8.length());
}

void Client::Checkout(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    auto client = ObjectWrap::Unwrap<Client>(args.Holder());

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto url = to_string(args[0]);
    auto path = to_string(args[1]);
    svn_opt_revision_t revision{svn_opt_revision_head};
    svn_client_checkout3(*_result_rev,      // result_rev
                         url.c_str(),       // URL
                         path.c_str(),      // path
                         &revision,         // peg_revision
                         &revision,         // revision
                         svn_depth_unknown, // depth
                         false,             // ignore_externals
                         false,             // allow_unver_obstructions
                         client->context,   // ctx
                         client->pool);     // pool
}

void Client::Status(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    auto resolver = Promise::Resolver::New(context).ToLocalChecked();
    args.GetReturnValue().Set(resolver->GetPromise());

    if (args.Length() < 1)
    {
        resolver->Reject(context, Exception::TypeError(String::NewFromUtf8(isolate, "Argument `path` is required.")));
        return;
    }

    if (!args[0]->IsString())
    {
        resolver->Reject(context, Exception::TypeError(String::NewFromUtf8(isolate, "Argument `path` must be a string.")));
        return;
    }

    auto result = Array::New(isolate);
    auto _result = new Persistent<Array>(isolate, result);
    auto callback = [isolate, _result](const char *path, const svn_client_status_t *status, apr_pool_t *) -> void {
        auto result = _result->Get(isolate);

        auto item = Object::New(isolate);
        item->Set(String::NewFromUtf8(isolate, "path"), String::NewFromUtf8(isolate, path));
        item->Set(String::NewFromUtf8(isolate, "kind"), Integer::New(isolate, status->kind));
        item->Set(String::NewFromUtf8(isolate, "textStatus"), Integer::New(isolate, status->text_status));
        item->Set(String::NewFromUtf8(isolate, "propStatus"), Integer::New(isolate, status->prop_status));
        item->Set(String::NewFromUtf8(isolate, "copied"), Boolean::New(isolate, status->copied));
        item->Set(String::NewFromUtf8(isolate, "switched"), Boolean::New(isolate, status->switched));

        result->Set(result->Length(), item);
    };

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto client = ObjectWrap::Unwrap<Client>(args.Holder());
    auto path = make_shared<string>(to_string(args[0]));
    auto _error = make_shared<svn_error_t *>();
    auto _callback = make_shared<function<void(const char *, const svn_client_status_t *, apr_pool_t *)>>(callback);
    auto work = [_result_rev, client, path, _callback, _error]() -> void {
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
                                     execute_svn_status, // status_func
                                     _callback.get(),    // status_baton
                                     client->pool);      // scratch_pool
    };

    auto _resolver = new Persistent<Promise::Resolver>(isolate, resolver);
    auto after_work = [isolate, _resolver, _result, _error, _result_rev]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);
        _resolver->Reset();
        delete _resolver;

        auto error = *_error;
        if (error != SVN_NO_ERROR)
        {
            resolver->Reject(context, SvnError::New(isolate, context, error->apr_err, error->message));
            return;
        }

        auto result = _result->Get(isolate);
        _result->Reset();
        delete _result;

        auto result_rev = *_result_rev;
        if (result_rev != nullptr)
            result->Set(context, String::NewFromUtf8(isolate, "revision", NewStringType::kNormal).ToLocalChecked(), Integer::New(isolate, *result_rev));

        resolver->Resolve(context, result);
        return;
    };

    if (!queue_work(uv_default_loop(), work, after_work))
    {
        resolver->Reject(context, Exception::Error(String::NewFromUtf8(isolate, "Failed starting async work")));
    }
}

void Client::Update(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    auto resolver = Promise::Resolver::New(context).ToLocalChecked();
    auto promise = resolver->GetPromise();
    args.GetReturnValue().Set(promise);

    auto client = ObjectWrap::Unwrap<Client>(args.Holder());

    auto notify = [](const svn_wc_notify_t *notify) -> void {

    };
    auto svn_client = client->context;
    svn_client->notify_baton2 = new function<void(svn_wc_notify_t *)>(notify);

    auto _result_rev = make_shared<apr_array_header_t *>();
    auto paths = args[0].As<v8::Array>();
    auto strings = vector<string>();
    auto _paths = apr_array_make(client->pool, paths->Length(), sizeof(char *));
    for (auto i = 0; i < paths->Length(); i++)
    {
        auto string = to_string(paths->Get(context, i).ToLocalChecked());
        APR_ARRAY_IDX(_paths, i, const char *) = string.c_str();
        strings.push_back(std::move(string));
    }
    svn_opt_revision_t revision{svn_opt_revision_working};
    svn_client_update4(_result_rev.get(),  // result_revs
                       _paths,             // paths
                       &revision,          // revision
                       svn_depth_infinity, // depth
                       false,              // depth_is_sticky
                       false,              // ignore_externals
                       false,              // allow_unver_obstructions
                       true,               // adds_as_modification
                       true,               // make_parents
                       svn_client,         // ctx
                       client->pool);      // pool
}

void Client::Cat(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    auto resolver = Promise::Resolver::New(context).ToLocalChecked();
    args.GetReturnValue().Set(resolver->GetPromise());

    if (args.Length() < 1)
    {
        resolver->Reject(context, Exception::TypeError(String::NewFromUtf8(isolate, "Argument `path` is required.")));
        return;
    }

    if (!args[0]->IsString())
    {
        resolver->Reject(context, Exception::TypeError(String::NewFromUtf8(isolate, "Argument `path` must be a string.")));
        return;
    }

    auto client = ObjectWrap::Unwrap<Client>(args.Holder());
    auto path = make_shared<string>(to_string(args[0]));
    auto _buffer = make_shared<svn_stringbuf_t *>();
    auto _error = make_shared<svn_error_t *>();
    auto work = [_buffer, client, path, _error]() -> void {
        apr_hash_t *props;
        *_buffer = svn_stringbuf_create_empty(client->pool);
        auto stream = svn_stream_from_stringbuf(*_buffer, client->pool);
        svn_opt_revision_t revision{svn_opt_revision_working};
        *_error = svn_client_cat3(&props,          // props
                                  stream,          // out
                                  path->c_str(),   // path_or_url
                                  &revision,       // peg_revision
                                  &revision,       // revision
                                  false,           // expand_keywords
                                  client->context, // ctx
                                  client->pool,    // result_pool
                                  client->pool);   // scratch_pool
    };

    auto _resolver = new Persistent<Promise::Resolver>(isolate, resolver);
    auto after_work = [isolate, _resolver, _error, _buffer]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);
        _resolver->Reset();
        delete _resolver;

        auto error = *_error;
        if (error != SVN_NO_ERROR)
        {
            resolver->Reject(context, SvnError::New(isolate, context, error->apr_err, error->message));
            return;
        }

        auto buffer = *_buffer;

        auto result = node::Buffer::New(isolate, buffer->data, buffer->len).ToLocalChecked();
        resolver->Resolve(context, result);
        return;
    };

    if (!queue_work(uv_default_loop(), work, after_work))
    {
        resolver->Reject(context, Exception::Error(String::NewFromUtf8(isolate, "Failed starting async work")));
    }
}
}
