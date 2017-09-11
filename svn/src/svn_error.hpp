#ifndef NODE_SVN_ERROR_H
#define NODE_SVN_ERROR_H

#include <node.h>

#include <svn_error.h>

using v8::AccessControl;
using v8::Context;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Integer;
using v8::Isolate;
using v8::MaybeLocal;
using v8::Local;
using v8::NewStringType;
using v8::Object;
using v8::Persistent;
using v8::PropertyAttribute;
using v8::String;
using v8::Undefined;
using v8::Value;

namespace Svn
{
namespace SvnError
{
void Init(Local<Object> exports, Isolate *isolate, Local<Context> context);
Local<Value> New(Isolate *isolate, Local<Context> context, int code, const char *message, Local<Value> &child);
Local<Value> New(Isolate *isolate, Local<Context> context, svn_error_t *error);
}
}

#endif
