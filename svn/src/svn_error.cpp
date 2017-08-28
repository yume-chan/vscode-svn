#include "svn_error.h"

namespace Svn
{
namespace SvnError
{
using v8::AccessControl;
using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::MaybeLocal;
using v8::Local;
using v8::NewStringType;
using v8::Object;
using v8::Persistent;
using v8::PropertyAttribute;
using v8::String;
using v8::Value;

Persistent<Function> constructor;

void Constructor(const FunctionCallbackInfo<Value> &args)
{
    args.GetReturnValue().Set(args.This());
}

#define DefineReadOnlyValue(object, name, value) (object)->DefineOwnProperty(context, String::NewFromUtf8(isolate, (name), NewStringType::kNormal).ToLocalChecked(), (value), (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete))

void Init(Local<Object> exports, Isolate *isolate, Local<Context> context)
{
    auto template_ = FunctionTemplate::New(isolate, Constructor);

    auto Error = context->Global()->Get(context, String::NewFromUtf8(isolate, "Error", NewStringType::kNormal).ToLocalChecked()).ToLocalChecked().As<Function>();
    //template_->SetPrototypeProviderTemplate(Error);

    auto function = template_->GetFunction();

    constructor.Reset(isolate, function);

    DefineReadOnlyValue(exports, "SvnError", function);
}
}
}
