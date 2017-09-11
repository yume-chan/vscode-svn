#ifndef NODE_SVN_ERROR_H
#define NODE_SVN_ERROR_H

#include <node.h>

namespace Svn
{
namespace SvnError
{
void Init(v8::Local<v8::Object> exports, v8::Isolate *isolate, v8::Local<v8::Context> context);
v8::Local<v8::Value> New(v8::Isolate *isolate, v8::Local<v8::Context> context, int code, const char *message);
}
}

#endif
