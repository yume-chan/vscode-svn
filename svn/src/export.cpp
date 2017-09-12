#include "v8.hpp"
#include "client.hpp"
#include "svn_error.hpp"

namespace Svn
{
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

// Util_Method(Test)
// {
// 	const char *c;
// 	vector<string> strings;

// 	{
// 		auto s = std::string("Hello world");
// 		c = s.c_str();
// 		strings.push_back(std::move(s));
// 	}

// 	auto len = strlen(c);
// }
// Util_MethodEnd;

void Init(Local<Object> exports)
{
	auto isolate = exports->GetIsolate();
	auto context = isolate->GetCurrentContext();

	exports->SetAccessor(context,					  // context
						 Util_String("version"),	  // name
						 Version,					  // getter
						 nullptr,					  // setter
						 MaybeLocal<Value>(),		  // data
						 AccessControl::ALL_CAN_READ, // settings
						 ReadOnlyDontDelete);		  // attribute

	// NODE_SET_METHOD(exports, "test", Test);

	Client::Init(exports, isolate, context);
	SvnError::Init(exports, isolate, context);
}

NODE_MODULE(svn, Init)
}
