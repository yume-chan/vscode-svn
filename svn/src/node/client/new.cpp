#include "client.hpp"
#include "auth/simple.hpp"

namespace Svn
{
Isolate *Client::isolate;

svn_error_t *log3(const char **log_msg, const char **tmp_file, const apr_array_header_t *commit_items, void *baton, apr_pool_t *pool)
{
    *log_msg = static_cast<const char *>(baton);
    return SVN_NO_ERROR;
}

svn_error_t *ssl_server_trust_prompt_callback(svn_auth_cred_ssl_server_trust_t **cred, void *baton, const char *realm, apr_uint32_t failures, const svn_auth_ssl_server_cert_info_t *cert_info, svn_boolean_t may_save, apr_pool_t *_pool)
{
    auto result = apr::pool::alloc<svn_auth_cred_ssl_server_trust_t>(_pool);
    result->may_save = false;
    result->accepted_failures = failures;
    *cred = result;
    return SVN_NO_ERROR;
};

struct Client::auth_info_t
{
    Auth::Simple *simple;
};

void Client::notify(const svn_wc_notify_t *notify)
{
    switch (notify->action)
    {
    case svn_wc_notify_commit_modified:
        if (commit_notify)
            commit_notify(notify);
        break;
    case svn_wc_notify_add:
        if (add_notify)
            add_notify(notify);
        break;
    case svn_wc_notify_revert:
        if (revert_notify)
            revert_notify(notify);
        break;
    case svn_wc_notify_update_delete:
    case svn_wc_notify_update_add:
    case svn_wc_notify_update_update:
    case svn_wc_notify_update_external:
    case svn_wc_notify_update_replace:
    case svn_wc_notify_update_skip_obstruction:
    case svn_wc_notify_update_skip_working_only:
    case svn_wc_notify_update_skip_access_denied:
    case svn_wc_notify_update_external_removed:
    case svn_wc_notify_update_shadowed_add:
    case svn_wc_notify_update_shadowed_update:
    case svn_wc_notify_update_shadowed_delete:
        if (update_notify)
            update_notify(notify);
        break;
    }
}

Client::Client(Isolate *isolate, const Local<Object> &options)
    : auth_info(new auth_info_t),
      svn::client()
{
    // context->log_msg_func3 = log3;

    svn_auth_provider_object_t *provider;
    auto providers = make_shared<apr::array<svn_auth_provider_object_t>>(_parent, 4);

    svn_auth_get_simple_provider2(&provider, nullptr, nullptr, _parent->get());
    providers->push_back(*provider);

    if (!options.IsEmpty())
    {
        _options.Reset(isolate, options);

        auto context = isolate->GetCurrentContext();
        auto value = Util_GetProperty(options, "getSimpleCredential");
        if (value->IsFunction())
            auth_info->simple = new Auth::Simple(isolate, value.As<Function>(), 2, _parent->get(), providers);
    }

    svn_auth_get_ssl_server_trust_prompt_provider(&provider, ssl_server_trust_prompt_callback, nullptr, _parent->get());
    providers->push_back(*provider);

    svn_auth_baton_t *auth_baton;
    svn_auth_open(&auth_baton, providers->get(), _parent->get());
    // context->auth_baton = auth_baton;
}

Client::~Client()
{
    if (auth_info->simple != nullptr)
        delete auth_info->simple;
}

V8_METHOD_BEGIN(Client::New)
{
    Util_ThrowIf(!args.IsConstructCall(), Util_Error(TypeError, "Class constructor Client cannot be invoked without 'new'"));

    Local<Object> options;
    auto arg = args[0];
    if (arg->IsObject())
        options = arg.As<Object>();

    auto result = new Client(isolate, options);
    result->Wrap(args.This());
}
V8_METHOD_END;
}
