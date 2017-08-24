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
using v8::Value;
using v8::Promise;

Persistent<Function> Client::constructor;

void Client::Init(Local<Object> exports)
{
    Isolate *isolate = exports->GetIsolate();

    Local<FunctionTemplate> template_ = FunctionTemplate::New(isolate, New);
    template_->SetClassName(String::NewFromUtf8(isolate, "Client"));
    template_->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(template_, "status", Status);

    Local<Function> function = template_->GetFunction();
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_NONE"), Integer::New(isolate, svn_wc_status_none));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_UNVERSIONED"), Integer::New(isolate, svn_wc_status_unversioned));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_NORMAL"), Integer::New(isolate, svn_wc_status_normal));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_ADDED"), Integer::New(isolate, svn_wc_status_added));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_MISSING"), Integer::New(isolate, svn_wc_status_missing));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_DELETED"), Integer::New(isolate, svn_wc_status_deleted));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_REPLACED"), Integer::New(isolate, svn_wc_status_replaced));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_MODIFIED"), Integer::New(isolate, svn_wc_status_modified));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_CONFLICTED"), Integer::New(isolate, svn_wc_status_conflicted));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_IGNORED"), Integer::New(isolate, svn_wc_status_ignored));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_OBSTRUCTED"), Integer::New(isolate, svn_wc_status_obstructed));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_EXTERNAL"), Integer::New(isolate, svn_wc_status_external));
    function->Set(String::NewFromUtf8(isolate, "TEXT_STATUS_INCOMPLETE"), Integer::New(isolate, svn_wc_status_incomplete));

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
    Isolate *isolate = args.GetIsolate();

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

void Client::StatusCallback(void *baton, const char *path, svn_wc_status_t *status)
{
    // auto persistent = (PersistentAndIsolate<Promise::Resolver> *)baton;
    // Local<Promise::Resolver> resolver = persistent->Get();

    Local<Array> array = *(Local<Array> *)baton;
    Isolate *isolate = array->GetIsolate();

    Local<Object> object = Object::New(array->GetIsolate());
    object->Set(String::NewFromUtf8(isolate, "textStatus"), Integer::New(isolate, status->text_status));
    object->Set(String::NewFromUtf8(isolate, "propStatus"), Integer::New(isolate, status->prop_status));
    object->Set(String::NewFromUtf8(isolate, "locked"), Boolean::New(isolate, status->locked));
    object->Set(String::NewFromUtf8(isolate, "copied"), Boolean::New(isolate, status->copied));
    object->Set(String::NewFromUtf8(isolate, "switched"), Boolean::New(isolate, status->switched));

    array->Set(array->Length(), object);

    //                          resolver->Resolve();
    // delete persistent;
}

void Client::Status(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();

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

    Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

    Local<Array> array = Array::New(isolate);

    // Local<Promise::Resolver> resolver = Promise::Resolver::New(isolate);
    // auto persistent = new PersistentAndIsolate<Promise::Resolver>(resolver, isolate);

    svn_revnum_t result_rev;
    svn_opt_revision_t revision{svn_opt_revision_head};
    svn_error_t *error = svn_client_status(&result_rev, *String::Utf8Value(args[0]), &revision, StatusCallback, &array, true, false, false, false, client->context, client->pool);
    if (error != nullptr)
    {
        Local<Value> exception = Exception::Error(String::NewFromUtf8(isolate, error->message));
        isolate->ThrowException(exception);
        return;
    }

    args.GetReturnValue().Set(array);
    //     resolver->Reject(Exception::Error(String::NewFromUtf8(isolate, error->message)));
    //     delete persistent;
    // }

    // args.GetReturnValue().Set(resolver->GetPromise());
}
}
