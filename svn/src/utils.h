#ifndef NODE_SVN_UTILS_H
#define NODE_SVN_UTILS_H

#include <functional>

#include <uv.h>

#include <svn_client.h>

namespace Svn
{
void queue_work(uv_loop_t *loop, std::function<void()> work, std::function<void()> after_work = nullptr);

svn_error_t *execute_svn_status(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool);
}

#endif
