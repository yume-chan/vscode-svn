#include "client.hpp"

#define InternalizedString(value) v8::New<String>(isolate, #value, NewStringType::kInternalized, sizeof(#value) - 1)

#define SetReadOnly(object, name)                         \
    (object)->DefineOwnProperty(context,                  \
                                InternalizedString(name), \
                                name,                     \
                                ReadOnlyDontDelete)

#define SetConst(target, prefix, name)                                                                             \
    {                                                                                                              \
        auto key = InternalizedString(name);                                                                       \
        target->DefineOwnProperty(context,                                                                         \
                                  key,                                                                             \
                                  v8::New<Integer>(isolate, prefix##name),                                         \
                                  ReadOnlyDontDelete);                                                             \
        target->DefineOwnProperty(context,                                                                         \
                                  v8::New<String>(isolate, to_string(prefix##name), NewStringType::kInternalized), \
                                  key,                                                                             \
                                  ReadOnlyDontDelete);                                                             \
    }

#define SetKind(name) SetConst(Kind, svn_node_, name)
#define SetStatusKind(name) SetConst(StatusKind, svn_wc_status_, name)
#define SetDepth(name) SetConst(Depth, svn_depth_, name)
#define SetRevisionKind(name) SetConst(RevisionKind, svn_opt_revision_, name)

#define SetPrototypeMethod(signature, prototype, name, callback, length)                  \
    /* Add a scope to hide extra variables */                                             \
    {                                                                                     \
        auto function = v8::FunctionTemplate::New(isolate,                /* isolate */   \
                                                  callback,               /* callback */  \
                                                  v8::Local<v8::Value>(), /* data */      \
                                                  signature,              /* signature */ \
                                                  length);                /* length */    \
        function->RemovePrototype();                                                      \
        prototype->Set(InternalizedString(name), function, PropertyAttribute::DontEnum);  \
    }

static svn_error_t *return_error_handler(svn_boolean_t can_return, const char *file, int line, const char *expr)
{
    svn_error_t *err = svn_error_raise_on_malfunction(TRUE, file, line, expr);
    return err;
}

namespace Svn
{
Persistent<Function> Client::constructor;

void Client::Init(Local<Object> exports, Isolate *isolate, Local<Context> context)
{
    auto client_template = Util_FunctionTemplate(New, 0);
    auto client_signature = v8::Signature::New(isolate, client_template);

    client_template->SetClassName(InternalizedString(Client));
    client_template->ReadOnlyPrototype();

    // This internal field is used for saving the pointer to a Client instance.
    // Client.wrap will set its pointer to the internal field
    // And ObjectWrap::Unwrap will read the internal field and cast it to Client.
    client_template->InstanceTemplate()->SetInternalFieldCount(1);

    auto client_prototype = client_template->PrototypeTemplate();
    SetPrototypeMethod(client_signature, client_prototype, add, Add, 1);
    SetPrototypeMethod(client_signature, client_prototype, cat, Cat, 1);
    SetPrototypeMethod(client_signature, client_prototype, changelistAdd, ChangelistAdd, 2);
    SetPrototypeMethod(client_signature, client_prototype, changelistRemove, ChangelistRemove, 1);
    SetPrototypeMethod(client_signature, client_prototype, checkout, Checkout, 2);
    SetPrototypeMethod(client_signature, client_prototype, commit, Commit, 2);
    SetPrototypeMethod(client_signature, client_prototype, delete, Delete, 1);
    SetPrototypeMethod(client_signature, client_prototype, info, Info, 1);
    SetPrototypeMethod(client_signature, client_prototype, status, Status, 1);
    SetPrototypeMethod(client_signature, client_prototype, revert, Revert, 1);
    SetPrototypeMethod(client_signature, client_prototype, update, Update, 1);

    auto Client = client_template->GetFunction();

    auto Kind = Object::New(isolate);
    SetKind(none);
    SetKind(file);
    SetKind(dir);
    SetKind(unknown);
    SetReadOnly(Client, Kind);

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
    SetReadOnly(Client, StatusKind);

    auto Depth = Object::New(isolate);
    SetDepth(unknown);
    SetDepth(exclude);
    SetDepth(empty);
    SetDepth(files);
    SetDepth(immediates);
    SetDepth(infinity);
    SetReadOnly(Client, Depth);

    auto RevisionKind = Object::New(isolate);
    SetRevisionKind(unspecified);
    SetRevisionKind(committed);
    SetRevisionKind(previous);
    SetRevisionKind(base);
    SetRevisionKind(working);
    SetRevisionKind(head);
    SetReadOnly(Client, RevisionKind);

    constructor.Reset(isolate, Client);
    SetReadOnly(exports, Client);

    svn_error_set_malfunction_handler(return_error_handler);
}
}
