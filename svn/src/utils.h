#ifndef NODE_SVN_UTILS_H
#define NODE_SVN_UTILS_H

#include <functional>

#include <uv.h>

#include <svn_client.h>

namespace Svn
{
void queue_work(uv_loop_t *loop, std::function<void(uv_work_t *)> method);
svn_error_t *execute_svn_status(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool);
#define svn_status_callback &(function<void(const char *, const svn_client_status_t *, apr_pool_t *)>)[&](const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool)->void
}

#endif
