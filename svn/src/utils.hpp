#ifndef NODE_SVN_UTILS_H
#define NODE_SVN_UTILS_H

#include <functional>
#include <thread>

#include <svn_client.h>

#include "v8.hpp"

using std::function;
using std::make_shared;
using std::make_unique;
using std::move;
using std::shared_ptr;
using std::string;
using std::vector;

#define Util_String(value) String::NewFromUtf8(isolate, value, NewStringType::kNormal).ToLocalChecked()
#define Util_StringFromStd(value) String::NewFromUtf8(isolate, value.c_str(), NewStringType::kNormal, value.length()).ToLocalChecked()

#define Util_New(type, value) type::New(isolate, value)
#define Util_NewMaybe(type, ...) type::New(context, __VA_ARGS__).ToLocalChecked()

#define Util_Persistent(type, value) new Persistent<type, v8::CopyablePersistentTraits<type>>(isolate, value)
#define Util_SharedPersistent(type, value) make_shared<Persistent<type, v8::CopyablePersistentTraits<type>>>(isolate, value)

#define Util_Undefined Undefined(isolate)

#define ReadOnlyDontDelete (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete)

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

#define Util_ThrowIf(expression, error, ...) \
    if (expression)                          \
    {                                        \
        isolate->ThrowException(error);      \
        return __VA_ARGS__;                  \
    }

#define Util_RejectIf(expression, error, ...) \
    if (expression)                           \
    {                                         \
        resolver->Reject(context, error);     \
        return __VA_ARGS__;                   \
    }

#define Util_FunctionTemplate(callback, length)                       \
    v8::FunctionTemplate::New(isolate,                /* isolate */   \
                              callback,               /* callback */  \
                              Local<Value>(),         /* data */      \
                              Local<v8::Signature>(), /* signature */ \
                              length);                /* length */

namespace Svn
{
namespace Util
{
class WorkData
{
  public:
    WorkData(function<void()> work, function<void()> after_work)
        : work(move(work)),
          after_work(move(after_work)) {}

    WorkData(const WorkData &other) = delete;

    const function<void()> work;
    const function<void()> after_work;
};

static void invoke_uv_work(uv_work_t *req)
{
    auto data = static_cast<WorkData *>(req->data);
    data->work();
}

static void invoke_after_uv_work(uv_work_t *req, int status)
{
    auto data = static_cast<WorkData *>(req->data);
    data->after_work();

    delete data;
    delete req;
}

// @return 0 if success.
inline int32_t QueueWork(uv_loop_t *loop, const function<void()> work, const function<void()> after_work)
{
    if (work == nullptr || after_work == nullptr)
        return -1;

    auto req = new uv_work_t;
    req->data = new WorkData(move(work), move(after_work));
    return uv_queue_work(loop, req, invoke_uv_work, invoke_after_uv_work);
}

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
                              ReadOnlyDontDelete);
}

inline void SetReadOnly(Isolate *isolate, Local<Context> context, Local<Object> object, uint32_t index, Local<Value> value)
{
    object->DefineOwnProperty(context,
                              Local<String>::Cast(Integer::New(isolate, index)),
                              value,
                              ReadOnlyDontDelete);
}
}
}

#endif
