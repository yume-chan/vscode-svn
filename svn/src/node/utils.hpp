#ifndef NODE_SVN_UTILS_H
#define NODE_SVN_UTILS_H

#include <functional>
#include <thread>

#include <svn_client.h>

#include <apr/apr.hpp>

#include "v8.hpp"

using std::function;
using std::make_shared;
using std::make_unique;
using std::move;
using std::shared_ptr;
using std::string;
using std::vector;

#define Util_New(type, value) type::New(isolate, (value))
#define Util_NewMaybe(type, ...) type::New(context, __VA_ARGS__).ToLocalChecked()

#define Util_Persistent(type, value) new Persistent<type, v8::CopyablePersistentTraits<type>>(isolate, value)
#define Util_SharedPersistent(type, value) make_shared<Persistent<type, v8::CopyablePersistentTraits<type>>>(isolate, value)

#define Util_Undefined Undefined(isolate)

#define ReadOnlyDontDelete (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete)

#define Util_Return(value) args.GetReturnValue().Set(value);

#define Util_Error(type, message) Exception::type(v8::New<String>(isolate, message))

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

#define Util_GetProperty(object, name) (object)->Get(context, v8::New<String>(isolate, name)).ToLocalChecked()

namespace Svn
{
namespace Util
{
template <class T>
class WorkData
{
  public:
    WorkData(function<T()> work, function<void(T)> after_work)
        : work(move(work)),
          after_work(move(after_work)) {}

    WorkData(const WorkData &other) = delete;

    const function<T()> work;
    const function<void(T)> after_work;

    T result;
};

template <class T>
static void invoke_uv_work(uv_work_t *req)
{
    auto data = static_cast<WorkData<T> *>(req->data);
    data->result = data->work();
}

template <class T>
static void invoke_after_uv_work(uv_work_t *req, int status)
{
    auto data = static_cast<WorkData<T> *>(req->data);
    data->after_work(data->result);

    delete data;
    delete req;
}

// @return 0 if success.
template <class T>
inline int32_t QueueWork(uv_loop_t *loop, const function<T()> work, const function<void(T)> after_work)
{
    if (work == nullptr || after_work == nullptr)
        return -1;

    auto req = new uv_work_t;
    req->data = new WorkData<T>(move(work), move(after_work));
    return uv_queue_work(loop, req, invoke_uv_work<T>, invoke_after_uv_work<T>);
}

static inline std::string to_string(Local<Value> &arg)
{
    String::Utf8Value string(arg);
    auto length = string.length();
    return std::string(*string, length);
}
}
}

#endif
