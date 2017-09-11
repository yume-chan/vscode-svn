#ifndef NODE_SVN_UTILS_H
#define NODE_SVN_UTILS_H

#include <functional>
#include <thread>

#include <v8.h>
#include <uv.h>

#include <svn_client.h>

#include "svn_error.hpp"

using std::function;
using std::make_shared;
using std::make_unique;
using std::move;
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
using v8::NewStringType;
using v8::Object;
using v8::Persistent;
using v8::Promise;
using v8::PropertyAttribute;
using v8::String;
using v8::Undefined;
using v8::Value;

#define Util_String(value) String::NewFromUtf8(isolate, value, NewStringType::kNormal).ToLocalChecked()
#define Util_StringFromStd(value) String::NewFromUtf8(isolate, value.c_str(), NewStringType::kNormal, value.length()).ToLocalChecked()

#define Util_New(type, value) type::New(isolate, value)
#define Util_NewMaybe(type, ...) type::New(context, __VA_ARGS__).ToLocalChecked()

#define Util_Persistent(type, value) new Persistent<type, v8::CopyablePersistentTraits<type>>(isolate, value)
#define Util_SharedPersistent(type, value) make_shared<Persistent<type, v8::CopyablePersistentTraits<type>>>(isolate, value)

#define Util_Undefined Undefined(isolate)

namespace Svn
{
namespace Util
{
int32_t QueueWork(uv_loop_t *loop, function<void()> work, function<void()> after_work);

svn_error_t *SvnStatusCallback(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool);

inline bool ContainsNull(const char *value, size_t length)
{
    for (auto i = 0; i < length; i++)
        if (!value[i])
            return true;
    return false;
}

inline bool ContainsNull(string value)
{
    return ContainsNull(value.c_str(), value.length());
}

inline void Set(Isolate *isolate, Local<Context> context, Local<Object> object, char *name, Local<Value> value)
{
    object->Set(context,
                Util_String(name),
                value);
}

inline void SetReadOnly(Isolate *isolate, Local<Context> context, Local<Object> object, char *name, Local<Value> value)
{
    object->DefineOwnProperty(context,
                              Util_String(name),
                              value,
                              (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete));
}

inline void SetReadOnly(Isolate *isolate, Local<Context> context, Local<Object> object, uint32_t index, Local<Value> value)
{
    object->DefineOwnProperty(context,
                              Local<String>::Cast(Integer::New(isolate, index)),
                              value,
                              (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete));
}

inline svn_error_t *SvnCommitCallback(const svn_commit_info_t *commit_info, void *baton, apr_pool_t *pool)
{
    auto method = *static_cast<function<void(const svn_commit_info_t *commit_info)> *>(baton);
    method(commit_info);
    return SVN_NO_ERROR;
}

inline svn_error_t *SvnStatusCallback(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool)
{
    auto method = *static_cast<function<void(const char *, const svn_client_status_t *, apr_pool_t *)> *>(baton);
    method(path, status, scratch_pool);
    return SVN_NO_ERROR;
}
}
}

#define Util_Method(name)                              \
    void name(const FunctionCallbackInfo<Value> &args) \
    {                                                  \
        auto isolate = args.GetIsolate();              \
        auto context = isolate->GetCurrentContext();
#define Util_MethodEnd }
#define Util_Return(value) args.GetReturnValue().Set(value);

#define Util_Set(object, name, value) Util::Set(isolate, context, object, name, value)

#define Util_SetReadOnly2(object, name) Util_SetReadOnly3(object, #name, name)
#define Util_SetReadOnly3(object, name, value) Util::SetReadOnly(isolate, context, object, name, value)

#define Util_Error(type, message) Exception::type(Util_String(message))
#define Util_RejectIf(expression, error, ...) \
    if (expression)                           \
    {                                         \
        resolver->Reject(context, error);     \
        return __VA_ARGS__;                   \
    }

#endif
