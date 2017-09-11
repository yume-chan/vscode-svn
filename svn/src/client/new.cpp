#include "client.hpp"

namespace Svn
{
void notify2(void *baton, const svn_wc_notify_t *notify, apr_pool_t *pool)
{
    auto client = static_cast<Client *>(baton);

    switch (notify->action)
    {
    case svn_wc_notify_commit_modified:
        if (client->commit_notify)
            client->commit_notify(notify);
        break;
    case svn_wc_notify_add:
        if (client->add_notify)
            client->add_notify(notify);
        break;
    case svn_wc_notify_revert:
        if (client->revert_notify)
            client->revert_notify(notify);
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
        if (client->update_notify)
            client->update_notify(notify);
        break;
    }
}

svn_error_t *log3(const char **log_msg, const char **tmp_file, const apr_array_header_t *commit_items, void *baton, apr_pool_t *pool)
{
    *log_msg = static_cast<const char *>(baton);
    return SVN_NO_ERROR;
}

Client::Client()
{
    apr_initialize();
    apr_pool_create(&pool, nullptr);
    svn_client_create_context(&context, this->pool);

    context->notify_baton2 = this;
    context->notify_func2 = notify2;

    context->log_msg_func3 = log3;

    apr_array_header_t *providers = apr_array_make(pool, 4, sizeof(svn_auth_provider_object_t *));

    svn_auth_provider_object_t *provider;
    svn_auth_get_simple_provider(&provider, pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

    svn_auth_baton_t *auth_baton;
    svn_auth_open(&auth_baton, providers, pool);
    context->auth_baton = auth_baton;
}

Client::~Client()
{
    apr_pool_destroy(pool);
    apr_terminate();
}

Util_Method(Client::New)
{
    if (!args.IsConstructCall())
    {
        isolate->ThrowException(Util_Error(TypeError, "Class constructor Client cannot be invoked without 'new'"));
        return;
    }

    auto result = new Client();
    result->Wrap(args.This());
}
Util_MethodEnd;
}
