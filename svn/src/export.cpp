#include <svn_version.h>

#include "v8.hpp"

#include "client.hpp"
#include "svn_error.hpp"

#define InternalizedString(value) v8::New<String>(isolate, value, NewStringType::kInternalized, sizeof(value) - 1)

#define SetReadOnly(object, name, value)                   \
	(object)->DefineOwnProperty(context,                   \
								InternalizedString(#name), \
								(value),                   \
								ReadOnlyDontDelete)

namespace Svn
{
void Version(Local<Name> property, const PropertyCallbackInfo<Value> &args)
{
	auto isolate = args.GetIsolate();
	auto context = isolate->GetCurrentContext();

	auto version = svn_client_version();

	auto object = Object::New(isolate);
	SetReadOnly(object, major, v8::New<Integer>(isolate, version->major));
	SetReadOnly(object, minor, v8::New<Integer>(isolate, version->minor));
	SetReadOnly(object, patch, v8::New<Integer>(isolate, version->patch));
	args.GetReturnValue().Set(object);
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

	exports->SetAccessor(context,						// context
						 InternalizedString("version"), // name
						 Version,						// getter
						 nullptr,						// setter
						 MaybeLocal<Value>(),			// data
						 AccessControl::ALL_CAN_READ,   // settings
						 ReadOnlyDontDelete);			// attribute

	// NODE_SET_METHOD(exports, "test", Test);

	Client::Init(exports, isolate, context);
	SvnError::Init(exports, isolate, context);
}

NODE_MODULE(svn, Init)
}
