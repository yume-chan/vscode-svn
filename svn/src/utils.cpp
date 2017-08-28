#include "utils.h"

namespace Svn
{
using std::function;

struct queue_work_data
{
	const std::function<void()> work;
	const std::function<void()> after_work;
};

void execute_uv_work(uv_work_t *req)
{
	auto data = (queue_work_data *)req->data;
	data->work();
}

void after_uv_work(uv_work_t *req, int status)
{
	auto data = (queue_work_data *)req->data;
	if (data->after_work != nullptr)
		data->after_work();

	delete data;
	delete req;
}

void queue_work(uv_loop_t *loop, std::function<void()> work, std::function<void()> after_work)
{
	uv_work_t *req = new uv_work_t;
	queue_work_data *data = new queue_work_data{std::move(work), std::move(after_work)};
	req->data = data;
	uv_queue_work(loop, req, execute_uv_work, after_uv_work);
	uv_run(loop, UV_RUN_ONCE);
}

svn_error_t *execute_svn_status(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool)
{
	auto method = *(function<void(const char *, const svn_client_status_t *, apr_pool_t *)> *)baton;
	method(path, status, scratch_pool);
	return SVN_NO_ERROR;
}
}
