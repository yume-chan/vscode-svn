#include <apr/apr.hpp>

#include <uv/async.hpp>
#include <uv/semaphore.hpp>

#include "client.hpp"

#define InternalizedString(value) v8::New<String>(isolate, value, NewStringType::kInternalized, sizeof(value) - 1)

#define SetProperty(target, name, value) target->Set(context, InternalizedString(name), value)

namespace node_svn
{
inline svn_error_t *invoke_callback(void *baton, const char *path, const char *changelist, apr_pool_t *pool)
{
    auto method = *static_cast<function<void(const char *, const char *, apr_pool_t *)> *>(baton);
    method(path, changelist, pool);
    return SVN_NO_ERROR;
}

V8_METHOD_BEGIN(Client::ChangelistGet)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    auto promise = resolver->GetPromise();
    Util_Return(promise);

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string or an array of string"));

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));
    auto *path = Util::to_string(arg);

    Local<Value> result;
    shared_ptr<Persistent<Value>> _result;

    auto semaphore = make_shared<Uv::Semaphore>();
    function<void(const char *, const char *)> async_callback;

    arg = args[1];
    if (arg->IsString())
    {
        auto value = Util::to_string(arg);
        auto changelists = std::vector<std::string>{ value };

        result = Array::New(isolate);
        _result = Util_SharedPersistent(Array, result);
        async_callback = [isolate, changelists, _result, semaphore](const char *path, const char *changelist) -> void {
            auto context = isolate->GetCurrentContext();
            auto result = _result->Get(isolate).As<Array>();

            result->Set(context, result->Length(), v8::New<String>(isolate, path));
        };
    }
    else if (arg->IsArray())
    {
    }

    auto async = make_shared<Uv::Async<const char *, const char *>>(uv_default_loop());
    auto send_callback = [async, async_callback, semaphore](const char *path, const char *changelist, apr_pool_t *) -> void {
        async->send(async_callback, path, changelist);
        semaphore->wait();
    };

    auto _send_callback = make_shared<function<void(const char *, const char *, apr_pool_t *)>>(move(send_callback));
    auto work = [path, changelists, _send_callback, client, pool]() -> svn_error_t * {
        return client->get_changelists()
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _result](svn_error_t *error) -> void {
        HandleScope scope(isolate);
        auto context = isolate->GetCallingContext();

        auto resolver = _resolver->Get(isolate);
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        auto result = _result->Get(isolate);
        resolver->Resolve(context, result);
    };

    RunAsync();
}
V8_METHOD_END;
}
