#include "client.h"

#define SetKind(name)                                                                     \
    Util::SetReadOnly(isolate, context, Kind, #name, Util_New(Integer, svn_node_##name)); \
    Util::SetReadOnly(isolate, context, Kind, svn_node_##name, Util_String(#name))

#define SetStatusKind(name)                                                                          \
    Util::SetReadOnly(isolate, context, StatusKind, #name, Util_New(Integer, svn_wc_status_##name)); \
    Util::SetReadOnly(isolate, context, StatusKind, svn_wc_status_##name, Util_String(#name))

#define SetPrototypeMethod(receiver, prototype, name, callback, length)                                   \
    { /* Add a scope to hide extra variables */                                                           \
        auto signature = v8::Signature::New(isolate, receiver);                                           \
        auto function = v8::FunctionTemplate::New(isolate,                /* isolate */                   \
                                                  callback,               /* callback */                  \
                                                  v8::Local<v8::Value>(), /* data */                      \
                                                  signature,              /* signature */                 \
                                                  length);                /* length */                    \
        auto key = Util_String(name);                                                                     \
        function->SetClassName(key);                                                                      \
        prototype->Set(key,                                                                               \
                       function,                                                                          \
                       (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete)); \
    }

namespace Svn
{
Persistent<Function> Client::constructor;

void Client::Init(Local<Object> exports, Isolate *isolate, Local<Context> context)
{
    auto ClientTemplate = FunctionTemplate::New(isolate, New);
    ClientTemplate->SetClassName(String::NewFromUtf8(isolate, "Client"));
    // This internal field is used for saving the pointer to a Client instance.
    // Client.wrap will set its pointer to the internal field
    // And ObjectWrap::Unwrap will read the internal field and cast it to Client.
    ClientTemplate->InstanceTemplate()->SetInternalFieldCount(1);

    auto prototype = ClientTemplate->PrototypeTemplate();
    SetPrototypeMethod(ClientTemplate, prototype, "cat", Cat, 1);
    SetPrototypeMethod(ClientTemplate, prototype, "checkout", Checkout, 2);
    SetPrototypeMethod(ClientTemplate, prototype, "status", Status, 1);
    SetPrototypeMethod(ClientTemplate, prototype, "update", Update, 1);

    auto Client = ClientTemplate->GetFunction();

    auto Kind = Object::New(isolate);
    SetKind(none);
    SetKind(file);
    SetKind(dir);
    SetKind(unknown);
    Util_SetReadOnly2(Client, Kind);

    auto StatusKind = Object::New(isolate);
    SetStatusKind(none);
    SetStatusKind(unversioned);
    SetStatusKind(normal);
    SetStatusKind(added);
    SetStatusKind(missing);
    SetStatusKind(deleted);
    SetStatusKind(replaced);
    SetStatusKind(modified);
    SetStatusKind(conflicted);
    SetStatusKind(ignored);
    SetStatusKind(obstructed);
    SetStatusKind(external);
    SetStatusKind(incomplete);
    Util_SetReadOnly2(Client, StatusKind);

    constructor.Reset(isolate, Client);
    Util_SetReadOnly2(exports, Client);
}
}
