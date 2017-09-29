#ifndef NODE_SVN_CLIENT_H
#define NODE_SVN_CLIENT_H

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <svn_client.h>

#include <apr/pool.hpp>
#include <svn/client.hpp>

#include "utils.hpp"

namespace Svn
{
using std::unique_ptr;

class Client : public node::ObjectWrap,
               public svn::client
{
  public:
    static void Init(Local<Object> exports, Isolate *isolate, Local<Context> context);

    std::function<void(const svn_wc_notify_t *)> add_notify;
    std::function<void(const svn_wc_notify_t *)> checkout_notify;
    std::function<void(const svn_wc_notify_t *)> commit_notify;
    std::function<void(const svn_wc_notify_t *)> revert_notify;
    std::function<void(const svn_wc_notify_t *)> update_notify;

  protected:
    void notify(const svn_wc_notify_t *notify) override;

  private:
    explicit Client(Isolate *isolate, const Local<Object> &options);
    ~Client();

    static V8_METHOD_DECLARE(Add);
    static V8_METHOD_DECLARE(Cat);
    static V8_METHOD_DECLARE(ChangelistAdd);
    static V8_METHOD_DECLARE(ChangelistGet);
    static V8_METHOD_DECLARE(ChangelistRemove);
    static V8_METHOD_DECLARE(Checkout);
    static V8_METHOD_DECLARE(Commit);
    static V8_METHOD_DECLARE(Delete);
    static V8_METHOD_DECLARE(Info);
    static V8_METHOD_DECLARE(Status);
    static V8_METHOD_DECLARE(Revert);
    static V8_METHOD_DECLARE(Update);

    static Persistent<Function> constructor;
    static V8_METHOD_DECLARE(New);

    static Isolate *isolate;

    Persistent<Object> _options;

    struct auth_info_t;
    unique_ptr<auth_info_t> auth_info;
};
}

#endif
