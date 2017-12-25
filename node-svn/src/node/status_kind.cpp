#include "status_kind.hpp"

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

#define SET_STATUS_KIND(name) SET_ENUM(object, svn::status_kind, name)

namespace node {
namespace status_kind {
void init(v8::Local<v8::Object>   exports,
          v8::Isolate*            isolate,
          v8::Local<v8::Context>& context) {
    auto object = v8::New<v8::Object>(isolate);

    SET_STATUS_KIND(none);
    SET_STATUS_KIND(unversioned);
    SET_STATUS_KIND(normal);
    SET_STATUS_KIND(added);
    SET_STATUS_KIND(missing);
    SET_STATUS_KIND(deleted);
    SET_STATUS_KIND(replaced);
    SET_STATUS_KIND(modified);
    SET_STATUS_KIND(conflicted);
    SET_STATUS_KIND(ignored);
    SET_STATUS_KIND(obstructed);
    SET_STATUS_KIND(external);
    SET_STATUS_KIND(incomplete);

    SetReadOnly(exports, "StatusKind", object);
}
} // namespace status_kind
} // namespace node
