#pragma once

#include <functional>
#include <future>
#include <type_traits>

#include <uv.h>

namespace uv {
template <class Callback, class Result, class... Args>
class async : public std::enable_shared_from_this<async<Callback, Result, Args...>> {
  public:
    async(Callback callback)
        : callback(std::move(callback))
        , uv_async(new uv_async_t) {
        static_assert(std::is_invocable_v<Callback, Args...>, "callback must be invocable");

        uv_async->data = this;
        uv_async_init(uv_default_loop(), uv_async, invoke_async);
    }

    ~async() {
        uv_close(reinterpret_cast<uv_handle_t*>(uv_async), delete_async);
    }

    Result operator()(Args... args) {
        this->args = std::make_tuple<Args...>(std::forward<Args>(args)...);

        uv_async_send(uv_async);

        auto future = promise.get_future();
        if constexpr (std::is_void_v<Result>) {
            future.get();
        } else {
            return future.get();
        }

        promise = std::promise<Result>();
    }

    std::function<Result(Args...)> to_function() {
        auto _this = this->shared_from_this();
        return [_this](Args... args) -> Result {
            return _this->operator()(args...);
        };
    }

  private:
    template <int... Is>
    struct index {};

    template <int N, int... Is>
    struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

    template <int... Is>
    struct gen_seq<0, Is...> : index<Is...> {};

    static void invoke_async(uv_async_t* handle) {
        auto _this = static_cast<async*>(handle->data);
        _this->invoke_callback(_this->args, gen_seq<sizeof...(Args)>{});
    }

    static void delete_async(uv_handle_t* handle) {
        delete reinterpret_cast<uv_async_t*>(handle);
    }

    template <int... Is>
    void invoke_callback(std::tuple<Args...>& tup, index<Is...>) {
        if constexpr (std::is_void_v<Result>) {
            callback(std::get<Is>(tup)...);
            promise.set_value();
        } else {
            auto result = callback(std::get<Is>(tup)...);
            promise.set_value(result);
        }
    }

    uv_async_t* uv_async;

    Callback            callback;
    std::tuple<Args...> args;

    std::promise<Result> promise;
};

template <class Callback, class... Args>
static decltype(auto) make_async(Callback callback) {
    using Result = std::invoke_result_t<Callback, Args...>;
    return std::make_shared<async<Callback, Result, Args...>>(callback);
}
} // namespace uv
