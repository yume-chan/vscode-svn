#ifndef CLIENT_H
#define CLIENT_H

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

#include <apr_pools.h>

#include <svn_client.h>
#include <svn_types.h>
#include <svn_version.h>
#include <svn_wc.h>

namespace Svn
{

class Client : public node::ObjectWrap
{
  public:
    static void Init(v8::Local<v8::Object> exports);

  private:
    explicit Client();
    ~Client();

    static svn_error_t *StatusCallback(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool);
    static void Status(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void Cat(const v8::FunctionCallbackInfo<v8::Value> &args);

    static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
    static v8::Persistent<v8::Function> constructor;

    apr_pool_t *pool;
    svn_client_ctx_t *context;
};
}

#endif
