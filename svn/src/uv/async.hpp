#ifndef NODE_SVN_UV_ASYNC_H
#define NODE_SVN_UV_ASYNC_H

#include <functional>

#include <uv.h>

namespace Uv
{
using std::forward;
using std::function;
using std::get;
using std::make_tuple;
using std::tuple;

namespace Helper
{
template <size_t... Indices>
struct indices
{
};

template <size_t N, size_t... Is>
struct build_indices : build_indices<N - 1, N - 1, Is...>
{
};

template <size_t... Is>
struct build_indices<0, Is...> : indices<Is...>
{
};
}

template <typename... Args>
class Async
{
  public:
    Async(uv_loop_t *loop)
    {
        handle = new uv_async_t;
        uv_async_init(loop, handle, invoke_callback);
        handle->data = this;
    }

    ~Async()
    {
        uv_close(reinterpret_cast<uv_handle_t *>(handle), close_callback);
    }

    int32_t send(function<void(Args...)> callback, Args... args)
    {
        this->callback = callback;
        this->args = make_tuple(forward<Args>(args)...);
        return uv_async_send(handle);
    }

  private:
    function<void(Args...)> callback;
    tuple<Args...> args;

    uv_async_t *handle;

    static void close_callback(uv_handle_t *handle)
    {
        delete reinterpret_cast<uv_async_t *>(handle);
    }

    template <typename... Args, int... Is>
    static void expend(function<void(Args...)> callback, tuple<Args...> &args, Helper::indices<Is...>)
    {
        callback(get<Is>(args)...);
    }

    template <typename... Args>
    static void expend(function<void(Args...)> callback, tuple<Args...> &args)
    {
        expend(callback, args, Helper::build_indices<sizeof...(Args)>{});
    }

    static void invoke_callback(uv_async_t *handle)
    {
        auto async = static_cast<Async<Args...> *>(handle->data);
        expend(async->callback, async->args);
    }
};
}

#endif
