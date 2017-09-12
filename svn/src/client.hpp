#ifndef NODE_SVN_CLIENT_H
#define NODE_SVN_CLIENT_H

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <apr_pools.h>

#include <svn_client.h>
#include <svn_types.h>
#include <svn_version.h>
#include <svn_wc.h>

#include "utils.hpp"

namespace Svn
{
class Client : public node::ObjectWrap
{
public:
  static void Init(Local<Object> exports, Isolate *isolate, Local<Context> context);

  std::function<void(const svn_wc_notify_t *)> add_notify;
  std::function<void(const svn_wc_notify_t *)> checkout_notify;
  std::function<void(const svn_wc_notify_t *)> commit_notify;
  std::function<void(const svn_wc_notify_t *)> revert_notify;
  std::function<void(const svn_wc_notify_t *)> update_notify;

private:
  explicit Client();
  ~Client();

  static void Add(const FunctionCallbackInfo<Value> &args);
  static void Cat(const FunctionCallbackInfo<Value> &args);
  static void Checkout(const FunctionCallbackInfo<Value> &args);
  static void Commit(const FunctionCallbackInfo<Value> &args);
  static void Status(const FunctionCallbackInfo<Value> &args);
  static void Revert(const FunctionCallbackInfo<Value> &args);
  static void Update(const FunctionCallbackInfo<Value> &args);

  static void New(const FunctionCallbackInfo<Value> &args);
  static Persistent<Function> constructor;

  apr_pool_t *pool;
  svn_client_ctx_t *context;
};
}

#endif
