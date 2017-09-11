#include "svn_error.hpp"

namespace Svn
{
namespace SvnError
{
using v8::AccessControl;
using v8::Context;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Integer;
using v8::Isolate;
using v8::MaybeLocal;
using v8::Local;
using v8::NewStringType;
using v8::Object;
using v8::Persistent;
using v8::PropertyAttribute;
using v8::String;
using v8::Value;

Persistent<Function> _svn_error;

#define DefineReadOnlyValue(object, name, value) (object)->DefineOwnProperty(context, String::NewFromUtf8(isolate, (name), NewStringType::kNormal).ToLocalChecked(), (value), (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete))
#define GetProperty(object, name) (object)->Get(context, String::NewFromUtf8(isolate, (name), NewStringType::kNormal).ToLocalChecked()).ToLocalChecked()
#define SetProperty(object, name, value) (object)->Set(context, String::NewFromUtf8(isolate, (name), NewStringType::kNormal).ToLocalChecked(), (value))

void Constructor(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    if (!args.IsConstructCall())
    {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Class constructor SvnError cannot be invoked without 'new'")));
        return;
    }

    if (args.Length() < 1)
    {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Arguments `code` and `message` are required.")));
        return;
    }

    if (!args[0]->IsNumber())
    {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument `code` must be a number.")));
        return;
    }

    if (!args[1]->IsString())
    {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Argument `message` must be a string.")));
        return;
    }

    auto _this = args.This();

    SetProperty(_this, "code", args[0]);

    SetProperty(_this, "message", args[1]->ToString(context).ToLocalChecked());

    auto error = Exception::Error(String::NewFromUtf8(isolate, "SvnError", NewStringType::kNormal).ToLocalChecked()).As<Object>();
    SetProperty(_this, "stack", GetProperty(error, "stack"));
}

void Init(Local<Object> exports, Isolate *isolate, Local<Context> context)
{
    auto template_ = FunctionTemplate::New(isolate,
                                           Constructor,
                                           Local<Value>(),
                                           Local<v8::Signature>(), 2);
    template_->SetClassName(String::NewFromUtf8(isolate, "SvnError"));
    template_->InstanceTemplate()->SetInternalFieldCount(1);
    auto function = template_->GetFunction();

    auto global = context->Global();
    auto error = GetProperty(global, "Error").As<Function>();
    auto error_prototype = GetProperty(error, "prototype");

    auto svn_error_prototype = GetProperty(function, "prototype").As<Object>();
    svn_error_prototype->SetPrototype(context, error_prototype);
    SetProperty(svn_error_prototype, "name", String::NewFromUtf8(isolate, "SvnError", NewStringType::kNormal).ToLocalChecked());

    _svn_error.Reset(isolate, function);
    DefineReadOnlyValue(exports, "SvnError", function);
}

Local<Value> New(Isolate *isolate, Local<Context> context, int code, const char *message, Local<Value> &child)
{
    auto error = _svn_error.Get(isolate);
    const auto argc = 3;
    Local<Value> argv[argc] = {
        Integer::New(isolate, code),
        String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked(),
        child};
    return error->CallAsConstructor(isolate->GetCurrentContext(), argc, argv).ToLocalChecked();
}
}
}
