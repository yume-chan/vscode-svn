#ifndef NODE_SVN_V8_H
#define NODE_SVN_V8_H

#include <string>

using std::string;
using std::to_string;

#include <v8.h>
#include <uv.h>

using v8::AccessControl;
using v8::Array;
using v8::Boolean;
using v8::Context;
using v8::Exception;
using v8::External;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::HandleScope;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::MaybeLocal;
using v8::Name;
using v8::NewStringType;
using v8::Object;
using v8::Persistent;
using v8::Promise;
using v8::PropertyAttribute;
using v8::PropertyCallbackInfo;
using v8::String;
using v8::Undefined;
using v8::Value;

namespace
{
template <typename T>
struct Factory;

template <>
struct Factory<Object>
{
    static inline Local<Object> New(Isolate *isolate)
    {
        return Object::New(isolate);
    }
};

template <>
struct Factory<String>
{
    static inline Local<String> New(Isolate *isolate, const char *value, NewStringType type = NewStringType::kNormal, size_t length = -1)
    {
        return String::NewFromUtf8(isolate, value, type, static_cast<int>(length)).ToLocalChecked();
    }

    static inline Local<String> New(Isolate *isolate, string value, NewStringType type = NewStringType::kNormal)
    {
        return String::NewFromUtf8(isolate, value.c_str(), type, static_cast<int>(value.length())).ToLocalChecked();
    }
};

template <>
struct Factory<Integer>
{
    static inline Local<Integer> New(Isolate *isolate, int32_t value)
    {
        return Integer::New(isolate, value);
    }
};

template <>
struct Factory<External>
{
    static inline Local<External> New(Isolate *isolate, void *value)
    {
        return External::New(isolate, value);
    }
};
};

namespace v8
{
template <typename T, typename A0>
inline Local<T> New(A0 a0)
{
    return Factory<T>::New(a0);
}

template <typename T, typename A0, typename A1>
inline Local<T> New(A0 a0, A1 a1)
{
    return Factory<T>::New(a0, a1);
}

template <typename T, typename A0, typename A1, typename A2>
inline Local<T> New(A0 a0, A1 a1, A2 a2)
{
    return Factory<T>::New(a0, a1, a2);
}

template <typename T, typename A0, typename A1, typename A2, typename A3>
inline Local<T> New(A0 a0, A1 a1, A2 a2, A3 a3)
{
    return Factory<T>::New(a0, a1, a2, a3);
}
}

#define V8_METHOD_DECLARE(name) void name(const FunctionCallbackInfo<Value> &args)
#define V8_METHOD_BEGIN(name)             \
    V8_METHOD_DECLARE(name)               \
    {                                     \
        auto isolate = args.GetIsolate(); \
        auto context = isolate->GetCurrentContext();
#define V8_METHOD_END }

#endif
