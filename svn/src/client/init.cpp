#include "client.h"

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

    NODE_SET_PROTOTYPE_METHOD(ClientTemplate, "status", Status);
    NODE_SET_PROTOTYPE_METHOD(ClientTemplate, "cat", Cat);

    auto Client = ClientTemplate->GetFunction();

    auto Kind = Object::New(isolate);
#define SetKind(name) Util::SetReadOnly(isolate, context, Kind, #name, Util_New(Integer, svn_node_##name))
    SetKind(none);
    SetKind(file);
    SetKind(dir);
    SetKind(unknown);
#undef SetKind
    Util_SetReadOnly(Client, Kind);

    auto StatusKind = Object::New(isolate);
#define SetStatusKind(name) Util::SetReadOnly(isolate, context, StatusKind, #name, Util_New(Integer, svn_wc_status_##name))
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
#undef SetStatusKind
    Util_SetReadOnly(Client, StatusKind);

    constructor.Reset(isolate, Client);

    Util_SetReadOnly(exports, Client);
}
}
