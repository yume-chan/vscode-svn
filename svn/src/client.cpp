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

#define Util_Method(name)                              \
    void name(const FunctionCallbackInfo<Value> &args) \
    {                                                  \
        auto isolate = args.GetIsolate();              \
        auto context = isolate->GetCurrentContext();
#define Util_MethodEnd }
#define Util_Return(value) args.GetReturnValue().Set(value);

#define Util_Set(object, name, value) Util::Set(isolate, context, object, name, value)
#define Util_SetReadOnly(object, name) Util::SetReadOnly(isolate, context, object, #name, name)

#define Util_String(value) String::NewFromUtf8(isolate, value, NewStringType::kNormal).ToLocalChecked()
#define Util_New(type, value) type::New(isolate, value)
#define Util_NewMaybe(type, ...) type::New(context, __VA_ARGS__).ToLocalChecked()
#define Util_Persistent(type, value) new Persistent<type>(isolate, value)

#define Util_Error(type, message) Exception::type(Util_String(message))
#define Util_Reject(expression, error)    \
    if (!(expression))                    \
    {                                     \
        resolver->Reject(context, error); \
        return;                           \
    }

#define Util_PreparePool()                                      \
    auto client = ObjectWrap::Unwrap<Client>(args.Holder());    \
    shared_ptr<apr_pool_t> pool;                                \
    {                                                           \
        apr_pool_t *_pool;                                      \
        apr_pool_create(&_pool, client->pool);                  \
        pool = shared_ptr<apr_pool_t>(_pool, apr_pool_destroy); \
    }

Persistent<Function> Client::constructor;

void Client::Init(Local<Object> exports, Isolate *isolate, Local<Context> context)
{
    auto ClientTemplate = FunctionTemplate::New(isolate, New);
    ClientTemplate->SetClassName(String::NewFromUtf8(isolate, "Client"));
    // This internal field is used for saving the pointer to a Client instance.
    // Client.wrap will set its pointer to the internal field
    // And ObjectWrap::Unwrap will read the internal field and cast it to Client.
    ClientTemplate->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(ClientTemplate, "status", Status);
    NODE_SET_PROTOTYPE_METHOD(ClientTemplate, "cat", Cat);

    auto Client = ClientTemplate->GetFunction();

    auto Kind = Object::New(isolate);
#define SetKind(name) Util::SetReadOnly(isolate, context, Kind, #name, Util_New(Integer, svn_node_##name))
    SetKind(none);
    SetKind(file);
    SetKind(dir);
    SetKind(unknown);
#undef SetKind
    Util_SetReadOnly(Client, Kind);

    auto StatusKind = Object::New(isolate);
#define SetStatusKind(name) Util::SetReadOnly(isolate, context, StatusKind, #name, Util_New(Integer, svn_wc_status_##name))
    SetStatusKind(none);
    SetStatusKind(unversioned);
    SetStatusKind(normal);
    SetStatusKind(added);
    SetStatusKind(missing);
    SetStatusKind(deleted);
    SetStatusKind(replaced);
    SetStatusKind(modified);
    SetStatusKind(conflicted);
    SetStatusKind(ignored);
    SetStatusKind(obstructed);
    SetStatusKind(external);
    SetStatusKind(incomplete);
#undef SetStatusKind
    Util_SetReadOnly(Client, StatusKind);

    constructor.Reset(isolate, Client);

    Util_SetReadOnly(exports, Client);
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
        auto callback = client->update_notify;
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

Util_Method(Client::New)
{
    if (!args.IsConstructCall())
    {
        isolate->ThrowException(Util_Error(TypeError, "Class constructor Client cannot be invoked without 'new'"));
        return;
    }

    auto result = new Client();
    result->Wrap(args.This());
}
Util_MethodEnd;

string to_string(Local<Value> value)
{
    String::Utf8Value utf8(value);
    return string(*utf8, utf8.length());
}

Util_Method(Client::Cat)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_Reject(args.Length() < 1, Util_Error(TypeError, "Argument `path` is required."));

    auto arg = args[0];
    Util_Reject(arg->IsString(), Util_Error(TypeError, "Argument `path` must be a string."));
    auto path = make_shared<string>(to_string(arg));

    Util_PreparePool();

    auto buffer = svn_stringbuf_create_empty(pool.get());
    auto _error = make_shared<svn_error_t *>();
    auto work = [buffer, pool, client, path, _error]() -> void {
        apr_hash_t *props;

        auto stream = svn_stream_from_stringbuf(buffer, pool.get());

        svn_opt_revision_t revision{svn_opt_revision_working};

        apr_pool_t *scratch_pool;
        apr_pool_create(&scratch_pool, pool.get());

        *_error = svn_client_cat3(&props,          // props
                                  stream,          // out
                                  path->c_str(),   // path_or_url
                                  &revision,       // peg_revision
                                  &revision,       // revision
                                  false,           // expand_keywords
                                  client->context, // ctx
                                  pool.get(),      // result_pool
                                  scratch_pool);   // scratch_pool
    };

    auto _resolver = Util_Persistent(Promise::Resolver, resolver);
    // Capture `pool` in `after_work` because I still need `buffer`
    auto after_work = [isolate, _resolver, _error, buffer, pool]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);
        _resolver->Reset();
        delete _resolver;

        auto error = *_error;
        Util_Reject(error == SVN_NO_ERROR, SvnError::New(isolate, context, error->apr_err, error->message));

        auto result = node::Buffer::New(isolate, buffer->data, buffer->len).ToLocalChecked();
        resolver->Resolve(context, result);
        return;
    };

    Util_Reject(Util::QueueWork(uv_default_loop(), work, after_work), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;

Util_Method(Client::Checkout)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_Reject(args.Length() < 2, Util_Error(TypeError, "Argument `path` is required."));

    auto arg = args[0];
    Util_Reject(arg->IsString(), Util_Error(TypeError, "Argument `path` must be a string."));
    auto url = make_shared<string>(to_string(arg));

    arg = args[1];
    Util_Reject(arg->IsString(), Util_Error(TypeError, "Argument `path` must be a string."));
    auto path = make_shared<string>(to_string(arg));

    Util_PreparePool();

    client->checkout_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto _error = make_shared<svn_error_t *>();
    auto work = [_result_rev, url, path, client, pool, _error]() -> void {
        svn_opt_revision_t revision{svn_opt_revision_head};
        *_error = svn_client_checkout3(*_result_rev,      // result_rev
                                       url->c_str(),      // URL
                                       path->c_str(),     // path
                                       &revision,         // peg_revision
                                       &revision,         // revision
                                       svn_depth_unknown, // depth
                                       false,             // ignore_externals
                                       false,             // allow_unver_obstructions
                                       client->context,   // ctx
                                       pool.get());       // pool
    };

    auto _resolver = Util_Persistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _error]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);
        _resolver->Reset();
        delete _resolver;

        auto error = *_error;
        Util_Reject(error == SVN_NO_ERROR, SvnError::New(isolate, context, error->apr_err, error->message));

        resolver->Resolve(context, v8::Undefined(isolate));
    };

    Util_Reject(Util::QueueWork(uv_default_loop(), work, after_work), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;

Util_Method(Client::Status)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_Reject(args.Length() < 1, Util_Error(TypeError, "Argument `path` is required."));

    auto arg = args[0];
    Util_Reject(arg->IsString(), Util_Error(TypeError, "Argument `path` must be a string."));
    auto path = make_shared<string>(to_string(arg));

    Util_PreparePool();

    auto result = Array::New(isolate);
    auto _result = new Persistent<Array>(isolate, result);
    auto callback = [isolate, _result](const char *path, const svn_client_status_t *status, apr_pool_t *) -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto item = Object::New(isolate);
        Util_Set(item, "path", Util_String(path));
        Util_Set(item, "kind", Util_New(Integer, status->kind));
        Util_Set(item, "textStatus", Util_New(Integer, status->text_status));
        Util_Set(item, "propStatus", Util_New(Integer, status->prop_status));
        Util_Set(item, "copied", Util_New(Boolean, status->copied));
        Util_Set(item, "switched", Util_New(Boolean, status->switched));

        auto result = _result->Get(isolate);
        result->Set(context, result->Length(), item);
    };

    auto _result_rev = make_shared<svn_revnum_t *>();
    auto _error = make_shared<svn_error_t *>();
    auto _callback = make_shared<function<void(const char *, const svn_client_status_t *, apr_pool_t *)>>(callback);
    auto work = [_result_rev, client, path, _callback, _error, pool]() -> void {
        svn_opt_revision_t revision{svn_opt_revision_working};
        *_error = svn_client_status6(*_result_rev,            // result_rev
                                     client->context,         // ctx
                                     path->c_str(),           // path
                                     &revision,               // revision
                                     svn_depth_infinity,      // depth
                                     false,                   // get_all
                                     false,                   // check_out_of_date
                                     false,                   // check_working_copy
                                     false,                   // no_ignore
                                     false,                   // ignore_externals
                                     false,                   // depth_as_sticky,
                                     nullptr,                 // changelists
                                     Util::SvnStatusCallback, // status_func
                                     _callback.get(),         // status_baton
                                     pool.get());             // scratch_pool
    };

    auto _resolver = Util_Persistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _result, _error, _result_rev]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);
        _resolver->Reset();
        delete _resolver;

        auto error = *_error;
        Util_Reject(error == SVN_NO_ERROR, SvnError::New(isolate, context, error->apr_err, error->message));

        auto result = _result->Get(isolate);
        _result->Reset();
        delete _result;

        auto result_rev = *_result_rev;
        if (result_rev != nullptr)
            Util_Set(result, "revision", Util_New(Integer, *result_rev));

        resolver->Resolve(context, result);
        return;
    };

    Util_Reject(Util::QueueWork(uv_default_loop(), work, after_work), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;

Util_Method(Client::Update)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    auto promise = resolver->GetPromise();
    Util_Return(promise);

    Util_Reject(args.Length() < 1, Util_Error(TypeError, "Argument `path` is required."));

    Util_PreparePool();

    auto arg = args[0];
    auto strings = vector<string>();
    apr_array_header_t *_paths;
    if (arg->IsString())
    {
        auto path = to_string(arg);
        _paths = apr_array_make(pool.get(), 1, sizeof(char *));
        strings.push_back(std::move(path));
    }
    else if (arg->IsArray())
    {
        auto paths = arg.As<v8::Array>();
        _paths = apr_array_make(pool.get(), paths->Length(), sizeof(char *));
        for (auto i = 0U; i < paths->Length(); i++)
        {
            auto string = to_string(paths->Get(context, i).ToLocalChecked());
            APR_ARRAY_IDX(_paths, i, const char *) = string.c_str();
            strings.push_back(std::move(string));
        }
    }
    else
    {
        Util_Reject(false, Util_Error(TypeError, "Argument `path` must be a string."));
    }

    client->update_notify = [](const svn_wc_notify_t *notify) -> void {

    };

    auto _result_rev = make_shared<apr_array_header_t *>();
    auto _error = make_shared<svn_error_t *>();
    auto work = [_result_rev, _paths, client, pool, _error]() -> void {
        svn_opt_revision_t revision{svn_opt_revision_working};
        *_error = svn_client_update4(_result_rev.get(),  // result_revs
                                     _paths,             // paths
                                     &revision,          // revision
                                     svn_depth_infinity, // depth
                                     false,              // depth_is_sticky
                                     false,              // ignore_externals
                                     false,              // allow_unver_obstructions
                                     true,               // adds_as_modification
                                     true,               // make_parents
                                     client->context,    // ctx
                                     pool.get());        // pool
    };

    auto _resolver = Util_Persistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _error]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);
        _resolver->Reset();
        delete _resolver;

        auto error = *_error;
        Util_Reject(error == SVN_NO_ERROR, SvnError::New(isolate, context, error->apr_err, error->message));

        resolver->Resolve(context, v8::Undefined(isolate));
    };

    Util_Reject(Util::QueueWork(uv_default_loop(), work, after_work), Util_Error(Error, "Failed starting async work"));
}
Util_MethodEnd;
}
