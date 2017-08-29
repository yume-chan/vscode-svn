#include "client.h"
#include "svn_error.h"

namespace Svn
{
using v8::AccessControl;
using v8::Context;
using v8::Integer;
using v8::Local;
using v8::MaybeLocal;
using v8::Name;
using v8::Object;
using v8::PropertyAttribute;
using v8::PropertyCallbackInfo;
using v8::String;
using v8::NewStringType;
using v8::Value;

void Version(Local<Name> property, const PropertyCallbackInfo<Value> &args)
{
	auto isolate = args.GetIsolate();

	auto version = svn_client_version();
	auto oVersion = Object::New(isolate);
	oVersion->Set(String::NewFromUtf8(isolate, "major"), Integer::New(isolate, version->major));
	oVersion->Set(String::NewFromUtf8(isolate, "minor"), Integer::New(isolate, version->minor));
	oVersion->Set(String::NewFromUtf8(isolate, "patch"), Integer::New(isolate, version->patch));
	args.GetReturnValue().Set(oVersion);
}

void Test(const v8::FunctionCallbackInfo<Value> &args)
{
	auto isolate = args.GetIsolate();
	auto context = isolate->GetCurrentContext();

	auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
	auto promise = resolver->GetPromise();
	auto persistent = new v8::Persistent<v8::Promise::Resolver>(isolate, resolver);

	uv_work_t *handle = new uv_work_t;
	handle->data = persistent;
	auto work = [](uv_work_t *handle) -> void {
	};
	auto after_work = [](uv_work_t *handle, int) -> void {
		auto isolate = v8::Isolate::GetCurrent();
		v8::HandleScope scope(isolate);
		auto persistent = static_cast<v8::Persistent<v8::Promise::Resolver> *>(handle->data);

		auto resolver = persistent->Get(isolate);
		resolver->Resolve(String::NewFromUtf8(isolate, "Done"));

		persistent->Reset();
		delete persistent;
	};
	uv_queue_work(uv_default_loop(), handle, work, after_work);

	args.GetReturnValue().Set(promise);
}

void Init(Local<Object> exports)
{
	auto isolate = exports->GetIsolate();
	auto context = isolate->GetCurrentContext();

	exports->SetAccessor(context,																			// context
						 String::NewFromUtf8(isolate, "version", NewStringType::kNormal).ToLocalChecked(),  // name
						 Version,																			// getter
						 nullptr,																			// setter
						 MaybeLocal<Value>(),																// data
						 AccessControl::ALL_CAN_READ,														// settings
						 (PropertyAttribute)(PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete)); // attribute

	NODE_SET_METHOD(exports, "test", Test);

	Client::Init(exports, isolate, context);
	SvnError::Init(exports, isolate, context);
}

NODE_MODULE(svn, Init)
}
