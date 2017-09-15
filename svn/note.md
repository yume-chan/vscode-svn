# How to make an async function in C++

(For JavaScript programmers without C++ experience)

(On Windows)

- [How to make an async function in C++](#how-to-make-an-async-function-in-c)
    - [Basic of native addon](#basic-of-native-addon)
    - [Basic types of V8](#basic-types-of-v8)
    - [More basic types of V8](#more-basic-types-of-v8)
    - [Execution of a normal function](#execution-of-a-normal-function)
    - [Execution of an async function](#execution-of-an-async-function)
        - [Threading in V8](#threading-in-v8)
        - [Use libuv to execute methods non-blocking](#use-libuv-to-execute-methods-non-blocking)
        - [Pass the `Promise::Resolver` to `after_work_cb`](#pass-the-promiseresolver-to-afterworkcb)
        - [Access V8 things in `work_cb`](#access-v8-things-in-workcb)
    - [Configuring](#configuring)
    - [Compiling](#compiling)
    - [Using Visual Studio](#using-visual-studio)
    - [Testing](#testing)

## Basic of native addon

To create a native addon:

1. Create a npm package: `npm init --yes`.
1. Install `node-gyp` globally: `npm i -g node-gyp`.
1. Create a file named `binding.gyp`.
1. Open `binding.gyp`, paste

````JSON
{
    "targets": [
        {
            "target_name": "module_name",
            "include_dirs": [
            ],
            "sources": [
                "src/export.cpp"
            ]
        }
    ]
}
````

See [node docs](https://nodejs.org/api/addons.html) for more setup.

## Basic types of V8

(Applies to V8 5.8.282 for node 8.0.0, see online documentation [here](https://v8docs.nodesource.com/node-8.0/))

Note: V8 will frequently introduce breaking changes!

* `v8::Local<T>` a smart pointer for V8 objects. It is valid only in its own method. When the method finished, V8 will delete the object.
* `v8::Persistent<T>` a normal pointer for V8 objects. You need to delete it when you finished using it.

Note: `::` is used to static members, `.` can only get instance members in C++

**Why does V8 need its own pointers?**

Objects in V8 are tracked by garbage collector. garbage collector can move, or remove objects when it think it should.

So their C++ pointers may change, or become invaild anytime in execution.

So you cannot use ordinal C++ pointers.

## More basic types of V8

* `v8::Isolate` an instance of V8 engine.
* `v8::Value` the base class of all V8 objects.
* `v8::String` represents the `String` type in JavaScript.
* `v8::FunctionCallbackInfo<T>` represents the `arguments` type in JavaScript.
* `v8::Promise` represents the `Promise` type in JavaScript.
* `v8::Promise::Resolver` a type used to `resolve` or `reject` a `v8::Promise`.

## Execution of a normal function

To create a C++ method that can be used in JavaScript, you need to declare it as `void Method(const FunctionCallbackInfo<Value>& args)`.

Note: The grammar is:

````plaintext
    void        Method     (   const FunctionCallbackInfo<Value>&        args       )
      ↑            ↑       ↑                    ↑                         ↑         ↑
return type   method name  (              parameter type            parameter name  )
````

Note: `const` is a part of parameter type. For more information about C++ types, you can see [this](http://en.cppreference.com/w/cpp/language/type).

To create a JavaScript `String`, use

```` C++
auto isolate = args.GetIsolate();
auto chars = "Hello, World";
auto string = String::NewFromUtf8(isolate, chars, NewStringType::kNormal).ToLocalChecked();
````

Note: The type of `chars` is `char *`.

Note: `auto` is like `var` in JavaScript.

To return a value to JavaScript, use

```` C++
args.GetReturnValue().Set(value);

````

Note: The return type of this C++ method is `void`, means it doesn't return anything, so you cannot use `return`.

Similarly, to create a JavaScript `Number`, use

```` C++
auto value1 = 42;
auto number1 = Integer::New(isolate, value1);

auto value2 = 42.0;
auto number2 = Number::New(isolate, value2);
````

Note: `value1` is a `int32_t`, an integer, while `value2` is a `double`, a float number.

Note: `Integer::New` and `Number::New` both create JavaSript `Number`, but from different C++ types.

As you can see, `isolate` is required for almost all V8 operations, so you definitly want to get it at the beginning of every method.

## Execution of an async function

The future of async JavaScript belongs to `Promise`, so I won't talk about callbacks.

To create a `Promise`, create a `Promise::Resolver` instead.

```` C++
auto resolver = Promise::Resolver::New(context).ToLocalChecked();
auto promise = resolver->GetPromise();
args.GetReturnValue().Set(promise);
````

Note: `->` is used to get a pointer's member, `Local<T>` is a pointer, thus you need `->`, not `.` or `::`.

Note: **MAYBE** `context` is from `isolate->GetCurrentContext();`

Also, lots of (and more and more) operations need `context` as well, so you should also get it at the beginning of methods.

### Threading in V8

V8 is single-thread, access V8 things from multiple threads without locks will cause crashes (And more badly, crash with no exception).

Standard embeded V8 should be able to use `v8::Locker` and `v8::Unlocker` to switch the active thread. But I cannot archieve it in Node.js, correct me if it's possible.

### Use libuv to execute methods non-blocking

```` C++
int uv_queue_work(uv_loop_t* loop, uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb)
````

`work_cb` will run on a pooled thread, `after_work_cb` will run on the caller thread.

Note: As we already known, you cannot access V8 things in `work_cb`.

So we need to do CPU-bound work in `work_cb`, then `resolve` the `Promise` in `after_work_cb`.

For simplify the work with C function pointers (`work_cb` and `after_work_cb` parameters), I have made a wrapper to make it accept lambda, or so called anonymous function in JavaScript world.

````C++
using std::function;

template <class T>
class WorkData
{
  public:
    WorkData(function<T()> work, function<void(T)> after_work)
        : work(move(work)),
          after_work(move(after_work)) {}

    WorkData(const WorkData &other) = delete;

    const function<T()> work;
    const function<void(T)> after_work;

    T result;
};

template <class T>
static void invoke_uv_work(uv_work_t *req)
{
    auto data = static_cast<WorkData<T> *>(req->data);
    data->result = data->work();
}

template <class T>
static void invoke_after_uv_work(uv_work_t *req, int status)
{
    auto data = static_cast<WorkData<T> *>(req->data);
    data->after_work(data->result);

    delete data;
    delete req;
}

// @return 0 if success.
template <class T>
inline int32_t QueueWork(uv_loop_t *loop, const function<T()> work, const function<void(T)> after_work)
{
    if (work == nullptr || after_work == nullptr)
        return -1;

    auto req = new uv_work_t;
    req->data = new WorkData<T>(move(work), move(after_work));
    return uv_queue_work(loop, req, invoke_uv_work<T>, invoke_after_uv_work<T>);
}
````

Note: `std::function` is a generic function pointer, definitely better than C function pointers.

Note: It's a templated method, so it can carry one value from `work_cb` to `after_work_cb`.

### Pass the `Promise::Resolver` to `after_work_cb`

As the first chapter says, `Promise::Resolver::New` will create a `Local`, but it's valid only in the method itself, not the `after_work_cb` callback.

To pass it to the callback, we need to convert it to a `Persistent`, and get a `Local` back in the callback.

To create a `Persistent`, use `auto _resolver = new Persistent<Promise::Resolver>(isolate, resolver)`.

To create a lambda for `after_work_cb` and get `resolver` back, use

````C++
auto after_work = [=]() -> void {
    auto context = isolate->GetCallingContext();
    HandleScope scope(isolate);

    auto resolver = _resolver->Get(isolate);
    _resolver->Reset();
    delete _resolver;
};
````

Note: **MAYBE** get `context` from `isolate->GetCallingContext()`.

Note: For more information about Lambda expressions, see also [Lambda expressions on cppreference.com](http://en.cppreference.com/w/cpp/language/lambda).

To resolve a Promise, call `resolver->Resolve(context, result)`. To reject it, call `resolver->Reject(context, exception);`.

Note: To create an exception, use `Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked())`, there are more error types in `Exception`, for example, `TypeError` and `RangeError`.

### Access V8 things in `work_cb`

Normally, you cannot access V8 things in `work_cb`. But if you really want to do it, there is a way.

libuv's `uv_async_t` object can make you wake up the main thread, where V8 is useable. The basic usage is:

```` C++
auto semaphore = new uv_sem_t;
uv_sem_init(semaphore, 0);

auto callback = [=]() -> void {
    // Do things
    semaphore->post();
};

auto invoke_callback = [](uv_async_t *handle) -> void {
    auto callback = static_cast<function<void(void)>>(handle->data);
    callback();
};

auto async = new uv_async_t;
auto loop = uv_default_loop();
uv_async_init(loop, async, invoke_callback);

async->data = new function<void(void)>(callback);
uv_async_send(async);

semaphore->wait();

auto close_cb = [](uv_handle_t* handle) -> void {
    delete reinterpret_cast<uv_async_t *>(handle);
};
uv_close_t(reinterpret_cast<uv_handle_t *>(async), close_cb);
````

As usual, I wrapped this method so I can pass `std::function`s instead of C function pointers.

Use the example above, now I can use V8 things in `work_cb`, without crash my Node.

## Configuring

```` shell
node-gyp configure
````

## Compiling

```` shell
node-gyp build
````

Debug build:

```` shell
node-gyp --debug build
````

To specify which build configuration to use when `require`d, edit the `main` field in `package.json`.

## Using Visual Studio

Highly recommend Visual Studio!

* Developing

    1. Run `node-gyp configure`.
    2. Open `build/binding.sln` in Visual Studio.

    You may add the `.h` files to `binding.gyp`, to make the Visual Studio project contains them.

* Debugging

    1. Change the debug settings:
        * Set executable to the full path of `node`.
        * Set arguments to a test script (`src/index.js` is used for this).
    1. Press <kbd>F5</kbd> to debug.

    Everytime you re-generate the solution with `node-gyp configure`, you need to do `i.` again.

* Adding or removing files

    1. Close solution in Visual Studio.
    1. Edit `sources` in `binding.gyp`.
    1. Run `node-gyp configure` again.
    1. Open `build/binding.sln` in Visual Studio again.

## Testing

I use `mocha` for testing. Run

```` shell
npm test
````

To run tests.
