#include "depth.hpp"

#include <cpp/types.hpp>

#define InternalizedString(value) \
    v8::New<v8::String>(isolate, value, v8::NewStringType::kInternalized, sizeof(value) - 1)

#define SET_ENUM(target, prefix, name)                                                                                   \
    {                                                                                                                    \
        auto key   = InternalizedString(#name);                                                                          \
        auto value = static_cast<int32_t>(prefix::name);                                                                 \
        target->DefineOwnProperty(context,                                                                               \
                                  key,                                                                                   \
                                  v8::New<v8::Integer>(isolate, value),                                                  \
                                  v8::PropertyAttributeEx::ReadOnlyDontDelete);                                          \
        target->DefineOwnProperty(context,                                                                               \
                                  v8::New<v8::String>(isolate, std::to_string(value), v8::NewStringType::kInternalized), \
                                  key,                                                                                   \
                                  v8::PropertyAttributeEx::ReadOnlyDontDelete);                                          \
    }

#define SetReadOnly(object, name, value)                  \
    (object)->DefineOwnProperty(context,                  \
                                InternalizedString(name), \
                                value,                    \
                                v8::PropertyAttributeEx::ReadOnlyDontDelete)

#define SET_DEPTH(name) SET_ENUM(object, svn::depth, name)

namespace node {
namespace depth {
void init(v8::Local<v8::Object>   exports,
          v8::Isolate*            isolate,
          v8::Local<v8::Context>& context) {
    auto object = v8::New<v8::Object>(isolate);

    SET_DEPTH(unknown);
    SET_DEPTH(empty);
    SET_DEPTH(files);
    SET_DEPTH(immediates);
    SET_DEPTH(infinity);

    SetReadOnly(exports, "Depth", object);
}
} // namespace depth
} // namespace node
