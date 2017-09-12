#ifndef NODE_SVN_ERROR_H
#define NODE_SVN_ERROR_H

#include <node.h>

#include <svn_error.h>

#include "v8.hpp"

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
