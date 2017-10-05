#include "client.hpp"

namespace Svn {
V8_METHOD_BEGIN(Client::Add) {
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string"));

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));

    auto path   = Util::to_string(arg);
    auto client = ObjectWrap::Unwrap<Client>(args.Holder());

    client->add_notify = [](const svn_wc_notify_t* notify) -> void {
    };

    auto work = [client, path]() -> void {
        client->add(path);
    };

    auto _resolver  = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver](future<void> future) -> void {
        HandleScope scope(isolate);
        auto        context = isolate->GetCallingContext();

        auto resolver = _resolver->Get(isolate);

        try {
            future.get();
            resolver->Resolve(context, Util_Undefined);
        } catch (svn_error_t& ex) {
            Util_RejectIf(true, SvnError::New(isolate, context, ex));
        }
    };

    uv::invoke_async<void>(work, after_work);
}
V8_METHOD_END;
} // namespace Svn
