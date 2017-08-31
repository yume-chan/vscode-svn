#ifndef NODE_SVN_UTILS_H
#define NODE_SVN_UTILS_H

#include <functional>

#include <v8.h>
#include <uv.h>

#include <svn_client.h>

namespace Svn
{
namespace Util
{
int32_t QueueWork(uv_loop_t *loop, std::function<void()> work, std::function<void()> after_work = nullptr);

svn_error_t *SvnStatusCallback(void *baton, const char *path, const svn_client_status_t *status, apr_pool_t *scratch_pool);

inline void Set(v8::Isolate *isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> object, char *name, v8::Local<v8::Value> value)
{
	object->Set(context,
				v8::String::NewFromUtf8(isolate, name, v8::NewStringType::kNormal).ToLocalChecked(),
				value);
}

inline void SetReadOnly(v8::Isolate *isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> object, char *name, v8::Local<v8::Value> value)
{
	object->DefineOwnProperty(context,
							  v8::String::NewFromUtf8(isolate, name, v8::NewStringType::kNormal).ToLocalChecked(),
							  value,
							  (v8::PropertyAttribute)(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete));
}
}
}

#endif
