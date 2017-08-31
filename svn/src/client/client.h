#ifndef NODE_SVN_CLIENT_CLIENT_H
#define NODE_SVN_CLIENT_CLIENT_H

#include "../client.h"
#include "../svn_error.h"

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

inline string to_string(Local<Value> value)
{
    String::Utf8Value utf8(value);
    return string(*utf8, utf8.length());
}

#endif
