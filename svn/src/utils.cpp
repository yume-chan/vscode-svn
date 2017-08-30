#include "utils.h"

namespace Svn
{
using std::function;

class WorkData
{
  public:
	WorkData(const function<void()> work, const function<void()> after_work)
		: work(work),
		  after_work(after_work) {}

	const function<void()> work;
	const function<void()> after_work;
};

void execute_uv_work(uv_work_t *req)
{
	auto data = (WorkData *)req->data;
	data->work();
}

void after_uv_work(uv_work_t *req, int status)
{
	auto data = (WorkData *)req->data;
	data->after_work();

	delete data;
	delete req;
}

int32_t queue_work(uv_loop_t *loop, const std::function<void()> work, const std::function<void()> after_work)
{
	if (work == nullptr || after_work == nullptr)
		return -1;

	uv_work_t *req = new uv_work_t;
	req->data = new WorkData(std::move(work), std::move(after_work));
	return uv_queue_work(loop, req, execute_uv_work, after_uv_work);
}

svn_error_t *execute_svn_status(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool)
{
	auto method = *(function<void(const char *, const svn_client_status_t *, apr_pool_t *)> *)baton;
	method(path, status, scratch_pool);
	return SVN_NO_ERROR;
}
}
