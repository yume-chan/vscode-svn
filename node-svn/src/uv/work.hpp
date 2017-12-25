#pragma once

#include <future>
#include <type_traits>

#include <uv.h>

namespace uv {
template <class Work, class AfterWork, class Result>
class work {
  public:
    work(Work do_work, AfterWork after_work)
        : do_work(std::move(do_work))
        , after_work(std::move(after_work)) {
        uv_work.data = this;
        uv_queue_work(uv_default_loop(), &uv_work, invoke_work, invoke_after_work);
    }

  private:
    static void invoke_work(uv_work_t* req) {
        auto _this = static_cast<work*>(req->data);
        try {
            if constexpr (std::is_void_v<Result>) {
                _this->do_work();
                _this->promise.set_value();
            } else {
                auto result = _this->do_work();
                _this->promise.set_value(result);
            }
        } catch (...) {
            _this->promise.set_exception(std::current_exception());
        }
    }

    static void invoke_after_work(uv_work_t* req, int status) {
        auto _this  = static_cast<work*>(req->data);
        auto future = _this->promise.get_future();
        _this->after_work(std::move(future));

        delete _this;
    }

    ~work() {}

    uv_work_t uv_work;

    const Work      do_work;
    const AfterWork after_work;

    std::promise<Result> promise;
};

template <class Work, class AfterWork>
static void queue_work(Work do_work, AfterWork after_work) {
    static_assert(std::is_move_constructible_v<Work>, "do_work must be move constructible");
    static_assert(std::is_move_constructible_v<AfterWork>, "after_work must be move constructible");

    static_assert(std::is_invocable_v<Work>, "do_work must be invocable");

    using Result = decltype(do_work());
    static_assert(std::is_invocable_r_v<void, AfterWork, std::future<Result>>, "after_work must be invocable");

    new work<Work, AfterWork, Result>(std::move(do_work), std::move(after_work));
}
} // namespace uv
