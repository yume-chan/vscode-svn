#ifndef NODE_SVN_CLIENT_H
#define NODE_SVN_CLIENT_H

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <apr/pool.hpp>
#include <svn/client.hpp>

#include <node/v8.hpp>

#define SVN_CLIENT_METHOD_DEFINITION(name)                \
    void name(Isolate*                           isolate, \
              const Local<Context>&              context, \
              const FunctionCallbackInfo<Value>& args,    \
              const Local<Promise::Resolver>&    resolver)

#define SVN_CLIENT_METHOD_DECLARE(name)                                                        \
  public:                                                                                      \
    static V8_METHOD_BEGIN(name) {                                                             \
        auto resolver = v8::New<Promise::Resolver>(context);                                   \
        auto client   = ObjectWrap::Unwrap<Client>(args.Holder());                             \
        auto promise  = resolver->GetPromise();                                                \
        args.GetReturnValue().Set(promise);                                                    \
        try {                                                                                  \
            client->name(isolate, context, args, resolver);                                    \
        } catch (...) {                                                                        \
            if (promise->State() == Promise::kPending)                                         \
                resolver->Reject(Exception::Error(v8::New<String>(isolate, "Unknown Error"))); \
        }                                                                                      \
    }                                                                                          \
    V8_METHOD_END                                                                              \
  private:                                                                                     \
    SVN_CLIENT_METHOD_DEFINITION(name);

namespace node_svn {
using std::unique_ptr;

class Client : public node::ObjectWrap {
  public:
    static void Init(Local<Object> exports, Isolate* isolate, Local<Context> context);

    std::function<void(const svn_wc_notify_t*)> add_notify;
    std::function<void(const svn_wc_notify_t*)> checkout_notify;
    std::function<void(const svn_wc_notify_t*)> commit_notify;
    std::function<void(const svn_wc_notify_t*)> revert_notify;
    std::function<void(const svn_wc_notify_t*)> update_notify;

  private:
    explicit Client(Isolate* isolate, const Local<Object>& options);
    ~Client();

    static Persistent<Function> constructor;
    static V8_METHOD_DECLARE(New);

    static Isolate* isolate;

    Persistent<Object> _options;

    // struct auth_info_t;
    // unique_ptr<auth_info_t> auth_info;

    svn::client _value;

    SVN_CLIENT_METHOD_DECLARE(Add);
    SVN_CLIENT_METHOD_DECLARE(Cat);
    SVN_CLIENT_METHOD_DECLARE(Changelist_add);
    SVN_CLIENT_METHOD_DECLARE(Changelist_get);
    SVN_CLIENT_METHOD_DECLARE(Changelist_remove);
    SVN_CLIENT_METHOD_DECLARE(Checkout);
    SVN_CLIENT_METHOD_DECLARE(Commit);
    SVN_CLIENT_METHOD_DECLARE(Info);
    SVN_CLIENT_METHOD_DECLARE(Remove);
    SVN_CLIENT_METHOD_DECLARE(Status);
    SVN_CLIENT_METHOD_DECLARE(Severt);
    SVN_CLIENT_METHOD_DECLARE(Update);
};
} // namespace node_svn

#endif
