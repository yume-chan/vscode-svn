#include "utils.hpp"

namespace Svn
{
namespace Util
{
class WorkData
{
  public:
    WorkData(function<void()> work, function<void()> after_work)
        : work(move(work)),
          after_work(move(after_work)) {}

    WorkData(const WorkData &other) = delete;

    const function<void()> work;
    const function<void()> after_work;
};

void execute_uv_work(uv_work_t *req)
{
    auto data = static_cast<WorkData *>(req->data);
    data->work();
}

void after_uv_work(uv_work_t *req, int status)
{
    auto data = static_cast<WorkData *>(req->data);
    data->after_work();

    delete data;
    delete req;
}

// @return 0 if success.
int32_t QueueWork(uv_loop_t *loop, const std::function<void()> work, const std::function<void()> after_work)
{
    if (work == nullptr || after_work == nullptr)
        return -1;

    auto req = new uv_work_t;
    req->data = new WorkData(move(work), move(after_work));
    return uv_queue_work(loop, req, execute_uv_work, after_uv_work);
}
}
}
