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

#include "utils.h"

namespace Svn
{
class Client : public node::ObjectWrap
{
public:
  static void Init(v8::Local<v8::Object> exports, v8::Isolate *isolate, v8::Local<v8::Context> context);

  std::function<void(const svn_wc_notify_t *)> checkout_callback;
  std::function<void(const svn_wc_notify_t *)> update_callback;

private:
  explicit Client();
  ~Client();

  static void Checkout(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void Status(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void Update(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void Cat(const v8::FunctionCallbackInfo<v8::Value> &args);

  static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
  static v8::Persistent<v8::Function> constructor;

  apr_pool_t *pool;
  svn_client_ctx_t *context;
};
}

#endif
