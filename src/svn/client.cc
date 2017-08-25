#include "client.h"

namespace Svn
{

using v8::Array;
using v8::Boolean;
using v8::Context;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Persistent;
using v8::Promise;
using v8::PropertyAttribute;
using v8::Value;
using v8::Promise;

#define DefineReadOnlyValue(object, name, value) (object)->DefineOwnProperty(context, String::NewFromUtf8(isolate, (name)), (value), (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete))
#define DefineReadOnlyValueInt(object, name, value) (object)->DefineOwnProperty(context, String::NewFromUtf8(isolate, (name)), Integer::New(isolate, (value)), (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete))

Persistent<Function> Client::constructor;

void Client::Init(Local<Object> exports)
{
    auto isolate = exports->GetIsolate();
    auto context = isolate->GetCurrentContext();

    Local<FunctionTemplate> template_ = FunctionTemplate::New(isolate, New);
    template_->SetClassName(String::NewFromUtf8(isolate, "Client"));
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
    exports->Set(String::NewFromUtf8(isolate, "Client"), function);
}

Client::Client()
{
    apr_initialize();
    apr_pool_create(&this->pool, nullptr);
    svn_client_create_context(&this->context, this->pool);
}

Client::~Client()
{
    apr_pool_destroy(this->pool);
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

// template <class P>
// class PersistentAndIsolate
// {
//   public:
//     PersistentAndIsolate(Local<P> local, Isolate *isolate) : persistent(local),
//                                                              isolate(isolate) {}

//     ~PersistentAndIsolate()
//     {
//         persistent->Reset();
//     }

//     Persistent<P> *persistent;
//     Isolate *isolate;

//     Local<P> Get()
//     {
//         return persistent->Get(isolate);
//     }
// };

svn_error_t *Client::StatusCallback(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool)
{
    // auto persistent = (PersistentAndIsolate<Promise::Resolver> *)baton;
    // Local<Promise::Resolver> resolver = persistent->Get();

    auto array = *(Local<Array> *)baton;
    auto isolate = array->GetIsolate();

    auto vStatus = Object::New(isolate);
    vStatus->Set(String::NewFromUtf8(isolate, "path"), String::NewFromUtf8(isolate, path));
    vStatus->Set(String::NewFromUtf8(isolate, "kind"), Integer::New(isolate, status->kind));
    vStatus->Set(String::NewFromUtf8(isolate, "textStatus"), Integer::New(isolate, status->text_status));
    vStatus->Set(String::NewFromUtf8(isolate, "propStatus"), Integer::New(isolate, status->prop_status));
    vStatus->Set(String::NewFromUtf8(isolate, "copied"), Boolean::New(isolate, status->copied));
    vStatus->Set(String::NewFromUtf8(isolate, "switched"), Boolean::New(isolate, status->switched));

    array->Set(array->Length(), vStatus);

    // resolver->Resolve();
    // delete persistent;
    return SVN_NO_ERROR;
}

void Client::Status(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    if (args.Length() < 1)
    {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument `path` is required.")));
        return;
    }

    if (!args[0]->IsString())
    {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument `path` must be a string.")));
        return;
    }

    auto client = ObjectWrap::Unwrap<Client>(args.Holder());

    svn_revnum_t result_rev;
    svn_opt_revision_t revision{svn_opt_revision_working};
    auto status = Array::New(isolate);
    auto error = svn_client_status6(&result_rev,                 // result_rev
                                    client->context,             // ctx
                                    *String::Utf8Value(args[0]), // path
                                    &revision,                   // revision
                                    svn_depth_infinity,          // depth
                                    false,                       // get_all
                                    false,                       // check_out_of_date
                                    false,                       // check_working_copy
                                    false,                       // no_ignore
                                    false,                       // ignore_externals
                                    false,                       // depth_as_sticky,
                                    nullptr,                     // changelists
                                    StatusCallback,              // status_func
                                    &status,                     // status_baton
                                    client->pool);               // scratch_pool

    if (error != SVN_NO_ERROR)
    {
        auto exception = Exception::Error(String::NewFromUtf8(isolate, error->message));
        isolate->ThrowException(exception);
        return;
    }

    auto result = Object::New(isolate);
    result->Set(context, String::NewFromUtf8(isolate, "status"), status);
    result->Set(context, String::NewFromUtf8(isolate, "version"), Integer::New(isolate, result_rev));
    args.GetReturnValue().Set(result);

    // Local<Promise::Resolver> resolver = Promise::Resolver::New(isolate);
    // auto persistent = new PersistentAndIsolate<Promise::Resolver>(resolver, isolate);

    //     resolver->Reject(Exception::Error(String::NewFromUtf8(isolate, error->message)));
    //     delete persistent;
    // }

    // args.GetReturnValue().Set(resolver->GetPromise());
}

void Client::Cat(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();

    if (args.Length() < 1)
    {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument `path` is required.")));
        return;
    }

    if (!args[0]->IsString())
    {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument `path` must be a string.")));
        return;
    }

    auto client = ObjectWrap::Unwrap<Client>(args.Holder());

    apr_hash_t *props;
    auto buffer = svn_stringbuf_create_empty(client->pool);
    auto stream = svn_stream_from_stringbuf(buffer, client->pool);
    svn_opt_revision_t revision{svn_opt_revision_working};
    auto error = svn_client_cat3(&props,                      // props
                                 stream,                      // out
                                 *String::Utf8Value(args[0]), // path_or_url
                                 &revision,                   // peg_revision
                                 &revision,                   // revision
                                 false,                       // expand_keywords
                                 client->context,             // ctx
                                 client->pool,                // result_pool
                                 client->pool);               // scratch_pool

    if (error != SVN_NO_ERROR)
    {
        auto exception = Exception::Error(String::NewFromUtf8(isolate, error->message));
        isolate->ThrowException(exception);
        return;
    }

    auto result = node::Buffer::New(isolate, buffer->data, buffer->len);
    args.GetReturnValue().Set(result.ToLocalChecked());
}
}
