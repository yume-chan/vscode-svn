#ifndef NODE_SVN_CLIENT_AUTH_SIMPLE_H
#define NODE_SVN_CLIENT_AUTH_SIMPLE_H

#include <functional>
#include <memory>

#include <uv.h>

#include <apr_pools.h>

#include <svn_auth.h>

#include "../../uv/async.hpp"
#include "../../uv/semaphore.hpp"

#include "../../v8.hpp"
#include "../../utils.hpp"

#include "../../apr/pool.hpp"

namespace node_svn
{
namespace Auth
{
using std::function;
using std::make_unique;

using v8::External;

#define StringOrUndefined(value) value != nullptr ? v8::New<String>(isolate, value).As<Value>() : Undefined(isolate).As<Value>()

class Simple
{
  public:
    typedef Uv::Async<Simple *, Uv::Semaphore *, svn_auth_cred_simple_t **, const char *, const char *, apr_pool_t *> SimpleAsync;

    Simple(Isolate *isolate,
           Local<Function> &callback,
           int32_t retry_count,
           shared_ptr<apr::pool> pool,
           apr_array_header_t *providers)
        : pool(pool),
          isolate(isolate),
          async(SimpleAsync(uv_default_loop()))
    {
        _callback.Reset(isolate, callback);

        auto handle = new svn_auth_provider_object_t;
        svn_auth_get_simple_prompt_provider(&handle, send_callback, this, retry_count, pool->get());
        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = handle;
    }

    ~Simple()
    {
        _callback.Reset();
    }

  private:
    shared_ptr<apr::pool> pool;
    Isolate *isolate;
    Persistent<Function> _callback;
    SimpleAsync async;

    struct SimpleState
    {
        Uv::Semaphore *semaphore;
        apr_pool_t *pool;
        svn_auth_cred_simple_t **result;
    };

    static V8_METHOD_BEGIN(promise_then)
    {
        auto state = static_cast<SimpleState *>(args.Data().As<External>()->Value());

        auto value = args[0];
        if (!value->IsObject())
        {
            state->semaphore->post();
            return;
        }

        auto object = value.As<Object>();
        auto username = Util_GetProperty(object, "username");
        if (!username->IsString())
        {
            state->semaphore->post();
            return;
        }
        String::Utf8Value _username(username);

        auto password = Util_GetProperty(object, "password");
        if (!password->IsString())
        {
            state->semaphore->post();
            return;
        }
        String::Utf8Value _password(password);

        auto save = Util_GetProperty(object, "save")->BooleanValue();

        auto pool = state->pool;
        auto result = apr::pool::alloc<svn_auth_cred_simple_t>(pool);
        result->may_save = save;
        result->username = apr::string::strdup(pool, *_username, _username.length());
        result->password = apr::string::strdup(pool, *_password, _password.length());
        *state->result = result;

        state->semaphore->post();
    }
    V8_METHOD_END;

    static void invoke_callback(Simple *simple,
                                Uv::Semaphore *semaphore,
                                svn_auth_cred_simple_t **result,
                                const char *realm,
                                const char *username,
                                apr_pool_t *pool)
    {
        if (simple->_callback.IsEmpty())
        {
            semaphore->post();
            return;
        }

        auto isolate = simple->isolate;
        HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto callback = simple->_callback.Get(isolate);
        const auto argc = 2;
        Local<Value> argv[argc] = {StringOrUndefined(realm),
                                   StringOrUndefined(username)};
        auto value = callback->Call(context, Util_Undefined, argc, argv).ToLocalChecked();
        if (!value->IsPromise())
        {
            semaphore->post();
            return;
        }

        auto promise = value.As<Promise>();
        auto state = new SimpleState{semaphore, pool, result};
        auto then = Util_NewMaybe(Function, promise_then, v8::New<External>(isolate, state), 1);
        promise->Then(context, then);
    }

    static svn_error_t *send_callback(svn_auth_cred_simple_t **result,
                                      void *baton,
                                      const char *realm,
                                      const char *username,
                                      svn_boolean_t may_save,
                                      apr_pool_t *pool)
    {
        auto simple = static_cast<Simple *>(baton);
        auto semaphore = make_unique<Uv::Semaphore>();
        simple->async.send(invoke_callback, simple, semaphore.get(), result, realm, username, pool);
        semaphore->wait();

        return SVN_NO_ERROR;
    }
};
}
}

#endif
