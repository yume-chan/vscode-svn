#include "client.h"

namespace Svn
{

using v8::FunctionCallbackInfo;
using v8::Integer;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

void Version(const FunctionCallbackInfo<Value> &args)
{
    auto isolate = args.GetIsolate();

    auto version = svn_client_version();
    auto oVersion = Object::New(isolate);
    oVersion->Set(String::NewFromUtf8(isolate, "major"), Integer::New(isolate, version->major));
    oVersion->Set(String::NewFromUtf8(isolate, "minor"), Integer::New(isolate, version->minor));
    oVersion->Set(String::NewFromUtf8(isolate, "patch"), Integer::New(isolate, version->patch));
    args.GetReturnValue().Set(oVersion);
}

void Init(Local<Object> exports)
{
    NODE_SET_METHOD(exports, "version", Version);
    Client::Init(exports);
}

NODE_MODULE(svn, Init)
}
