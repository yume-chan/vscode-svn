#include "uv_utils.h"

namespace Svn
{
using std::function;

void execute_uv_work(uv_work_t *req)
{
    auto method = *(function<void(uv_work_t *)> *)req->data;
    method(req);
}

void after_uv_work(uv_work_t *req, int status)
{
    // Do nothing
}

void queue_work(uv_loop_t *loop, function<void(uv_work_t *)> method)
{
    uv_work_t req;
    req.data = &method;
    uv_queue_work(loop, &req, execute_uv_work, after_uv_work);
    uv_run(loop, UV_RUN_ONCE);
}

svn_error_t *execute_svn_status(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool)
{
    auto method = *(function<void(const char *, const svn_client_status_t *, apr_pool_t *)> *)baton;
    method(path, status, scratch_pool);
    return SVN_NO_ERROR;
}
}
