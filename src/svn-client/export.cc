
#include "client.h"

namespace Svn
{

using v8::Function;
using v8::FunctionCallbackInfo;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

void Version(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();

    const svn_version_t *version = svn_client_version();
    Local<Object> object = Object::New(isolate);
    object->Set(String::NewFromUtf8(isolate, "major"), Integer::New(isolate, version->major));
    object->Set(String::NewFromUtf8(isolate, "minor"), Integer::New(isolate, version->minor));
    object->Set(String::NewFromUtf8(isolate, "patch"), Integer::New(isolate, version->patch));

    args.GetReturnValue().Set(object);
}

void Init(Local<Object> exports)
{
    NODE_SET_METHOD(exports, "version", Version);
    Client::Init(exports);
}

NODE_MODULE(addon, Init)
}
