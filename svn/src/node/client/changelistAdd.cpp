#include "client.hpp"

namespace node_svn {
V8_METHOD_BEGIN(Client::ChangelistAdd) {
    auto resolver = Util_NewMaybe(Promise::Resolver);
    auto promise  = resolver->GetPromise();
    Util_Return(promise);

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string or an array of string"));

    auto client = ObjectWrap::Unwrap<Client>(args.Holder());

    string           changelist;
    function<void()> work;

    auto arg = args[0];
    if (arg->IsString()) {
        auto path = Util::to_string(arg);

        work = [path, changelist, client]() -> void {
            client->add_to_changelist(path, changelist);
        };
    } else if (arg->IsArray()) {

    }

    Util_RejectIf(args.Length() == 1, Util_Error(TypeError, "Argument \"changelist\" must be a string"));

    auto arg = args[1];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"changelist\" must be a string"));
    changelist = Util::to_string(arg);

    auto _resolver  = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver](svn_error_t* error) -> void {
        HandleScope scope(isolate);
        auto        context = isolate->GetCallingContext();

        auto resolver = _resolver->Get(isolate);
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        resolver->Resolve(context, Util_Undefined);
    };

    RunAsync();
}
V8_METHOD_END;
} // namespace node_svn
