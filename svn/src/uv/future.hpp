#ifndef UV_CPP_FUTURE_H
#define UV_CPP_FUTURE_H

#include <functional>
#include <future>

#include <uv.h>

using std::function;
using std::future;
using std::promise;
using std::current_exception;

namespace uv
{
template <typename T>
static void invoke_work(uv_work_t *req)
{
    auto data = static_cast<work_data<T> *>(req->data);

    try
    {
        promise.set_value(data->work());
    }
    catch (...)
    {
        promise.set_exception(current_exception());
    }
}

template <typename T>
static void invoke_after_work(uv_work_t *req, int status)
{
    auto data = static_cast<work_data<T> *>(req->data);

    data->after_work(data->promise.get_future());

    delete data;
    delete req;
}

template <typename T>
struct work_data
{
    promise<T> promise;
    function<T()> work;
    function<void(future<T>)> after_work;
};

template <typename T>
void invoke_async(function<T()> work, function<void(future<T>)> after_work)
{
    promise<T> promise;

    auto req = new uv_work_t;
    req->data = new work_data<T>{move(promise), move(work), move(after_work)};
    uv_queue_work(uv_default_loop(), req, invoke_work, invoke_after_work);
}
}

#endif
