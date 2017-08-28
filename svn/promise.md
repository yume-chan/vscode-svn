# How to make an async function in C++

(For JavaScript programmers without C++ experience)

(On Windows)

- [How to make an async function in C++](#how-to-make-an-async-function-in-c)
    - [Basic of native addon](#basic-of-native-addon)
    - [Basic types of V8](#basic-types-of-v8)
    - [More basic types of V8](#more-basic-types-of-v8)
    - [Execution of a normal function](#execution-of-a-normal-function)
    - [Execution of an async function](#execution-of-an-async-function)
        - [Use libuv to execute method on other threads](#use-libuv-to-execute-method-on-other-threads)
        - [Pass the `Promise::Resolver` to `after_work_cb`](#pass-the-`promiseresolver`-to-`afterworkcb`)
    - [Compiling](#compiling)
    - [Debugging](#debugging)

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
            "target_name": "svn",
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

(Applies to V8 5.8.282 (node8.0.0), [online documentation](https://v8docs.nodesource.com/node-8.0/))

Note: V8 will frequently introduce breaking changes!

* `v8::Isolate` an instance of V8 engine.

* `v8::Local<T>` a smart pointer for V8 objects. It is valid only in its own method. When the method finished, V8 will delete the object.
* `v8::Persistent<T>` a normal pointer for V8 objects. You need to delete it when you finished using it.

Note: `::` is used to static members, `.` can only get instance members in C++

**Why does V8 need its own pointers?**

Objects in V8 are tracked by garbage collector. garbage collector can move, or remove objects when it think it should.

So their C++ pointers may change, or become invaild anytime in execution.

So you cannot use ordinal C++ pointers.

## More basic types of V8

* `v8::Value` the base class of all V8 objects.
* `v8::String` JavaScript `String` type.
* `v8::FunctionCallbackInfo<T>` JavaScript `arguments` type.
* `v8::Promise` JavaScript `Promise` type.
* `v8::Promise::Resolver` a type used to `resolve` or `reject` a `v8::Promise`.

## Execution of a normal function

To create a C++ method that can be used in JavaScript, you need to declare it as `void Method(const FunctionCallbackInfo<Value>& args)`.

Note: The grammar is:

````plaintext
    void         Method    (   const FunctionCallbackInfo<Value>&        args       )
      ↑            ↑       ↑                    ↑                         ↑         ↑
return type   method name  (              parameter type            parameter name  )
````

To create a string, use `auto value = String::NewFromUtf8(isolate, str, NewStringType::kNormal).ToLocalChecked()`.

Note: `isolate` can be retrieved from `args.GetIsolate()`

Note: `auto` is like `var` in JavaScript

To return a value, use `args.GetReturnValue().Set(value)`.

## Execution of an async function

To create a `Promise`, create a `Promise::Resolver` first.

```` C++
auto resolver = Promise::Resolver::New(context).ToLocalChecked();
args.GetReturnValue().Set(resolver->GetPromise());
````

Note: `->` is used to get pointer member, if the type has a `*` at last, you need `->`, not `.`.

Note: **MAYBE** `context` is from `isolate->GetCurrentContext();`

### Use libuv to execute method on other threads

````C++
int uv_queue_work(uv_loop_t* loop, uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb)
````

`work_cb` will run on a pooled thread, `after_work_cb` will run on the loop thread.

**MAYBE** only `after_work_cb` can use V8 things, call V8 methods in `work_cb` will cause exception, or even crash without any exception.

So we need to do CPU-bound work in `work_cb`, then `resolve` the `Promise` in `after_work_cb`.

For simplify the work with C function pointers (`work_cb` and `after_work_cb` parameters), I have made a wrapper to make it accept lambda, or so called anonymous function in JavaScript world.

````C++
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
````

Note: `std::function` is a generic function pointer, definitely better than C function pointers.

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

To resolve, use `resolver->Resolve(context, result)`, to reject, use `resolver->Reject(context, exception);`.

Note: To create an exception, use `Exception::Error(String::NewFromUtf8(isolate, message, NewStringType::kNormal).ToLocalChecked())`, there are more error types in `Exception`.

## Compiling

```` shell
node-gyp configure
node-gyp build
````

## Debugging

Highly-recommend Visual Studio!

Run `node-gyp configure`, then you can open `binding.sln` in `build` folder with Visual Studio, it's powerful.

To add or remove files, close solution in Visual Studio, edit `binding.gyp`, run `node-gyp configure` again, then open in Visual Studio again.
