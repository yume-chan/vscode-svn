#include "node_client.hpp"

#define CLASS_NAME client
#define EXPORT_NAME "Client"

#define METHOD_BEGIN(name)                                               \
    void client::name(const v8::FunctionCallbackInfo<v8::Value>& args) { \
        auto isolate = args.GetIsolate();                                \
        auto context = isolate->GetCurrentContext();                     \
        try {                                                            \
            auto _this = node::ObjectWrap::Unwrap<client>(args.Holder());

#define TO_ASYNC_CALLBACK(callback, ...) \
    std::function<std::invoke_result_t<decltype(callback), __VA_ARGS__>(__VA_ARGS__)>(callback)

template <class T>
class future {
  public:
    future() {}

    T value;

    T get() {
        return value;
    }
};

template <>
class future<void> {
  public:
    future() {}

    void get() {}
};

#define ASYNC_BEGIN(result, ...) \
    future<result> future;

#define ASYNC_RETURN(result) \
    future.value = result;

#define ASYNC_END(...)

#define ASYNC_RESULT \
    future.get()

#define METHOD_RETURN(value) \
    args.GetReturnValue().Set(value);

#define METHOD_END                                                                                     \
    }                                                                                                  \
    catch (svn::svn_type_error & error) {                                                              \
        isolate->ThrowException(v8::Exception::TypeError(v8::New<v8::String>(isolate, error.what()))); \
    }                                                                                                  \
    catch (svn::svn_error & raw_error) {                                                               \
        auto error = copy_error(isolate, raw_error);                                                   \
        isolate->ThrowException(error);                                                                \
    }                                                                                                  \
    }

#include "client_template.hpp"
