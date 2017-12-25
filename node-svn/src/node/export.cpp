#include <node.h>

#include <svn_client.h>
#include <svn_version.h>

#include <node/async_client.hpp>
#include <node/depth.hpp>
#include <node/node_client.hpp>
#include <node/node_kind.hpp>
#include <node/revision_kind.hpp>
#include <node/status_kind.hpp>
#include <node/v8.hpp>

#define InternalizedString(value) v8::New<v8::String>(isolate, value, v8::NewStringType::kInternalized, sizeof(value) - 1)

#define SetReadOnly(object, name, value)                  \
    (object)->DefineOwnProperty(context,                  \
                                InternalizedString(name), \
                                (value),                  \
                                v8::PropertyAttributeEx::ReadOnlyDontDelete)

namespace node {
void version(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    auto version = svn_client_version();

    auto object = v8::Object::New(isolate);
    SetReadOnly(object, "major", v8::New<v8::Integer>(isolate, version->major));
    SetReadOnly(object, "minor", v8::New<v8::Integer>(isolate, version->minor));
    SetReadOnly(object, "patch", v8::New<v8::Integer>(isolate, version->patch));
    args.GetReturnValue().Set(object);
}
//
//static void Test(const v8::FunctionCallbackInfo<v8::Value>& args) {
//    auto isolate = args.GetIsolate();
//    auto context = isolate->GetCurrentContext();
//
//    auto object = args[0].As<v8::Object>();
//
//    auto isEmpty = object.IsEmpty();
//    if (isEmpty)
//        return;
//
//    auto isUndefined = object->IsUndefined();
//    if (isUndefined)
//        return;
//
//    auto value = object->Get(context, v8::New<v8::String>(isolate, "test"));
//    isEmpty    = value.IsEmpty();
//    if (isEmpty)
//        return;
//
//    isUndefined = value.ToLocalChecked()->IsUndefined();
//    if (isUndefined)
//        return;
//}

#include <type_traits>

void init(v8::Local<v8::Object> exports) {
    auto isolate = exports->GetIsolate();
    auto context = isolate->GetCurrentContext();

    exports->SetAccessor(context,                                      // context
                         InternalizedString("version"),                // name
                         version,                                      // getter
                         nullptr,                                      // setter
                         v8::MaybeLocal<v8::Value>(),                  // data
                         v8::AccessControl::ALL_CAN_READ,              // settings
                         v8::PropertyAttributeEx::ReadOnlyDontDelete); // attribute

    //NODE_SET_METHOD(exports, "test", Test);

    async_client::init(exports, isolate, context);
    client::init(exports, isolate, context);

    depth::init(exports, isolate, context);
    node_kind::init(exports, isolate, context);
    revision_kind::init(exports, isolate, context);
    status_kind::init(exports, isolate, context);
    //SvnError::Init(exports, isolate, context);
}

NODE_MODULE(svn, init)
} // namespace node
