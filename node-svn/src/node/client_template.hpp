#include <cstring>

#include <node_buffer.h>

#include <cpp/client.hpp>
#include <cpp/svn_type_error.hpp>

#include <node/v8.hpp>

#define InternalizedString(value) \
    v8::New<v8::String>(isolate, value, v8::NewStringType::kInternalized, sizeof(value) - 1)

static v8::Local<v8::Value> copy_error(v8::Isolate* isolate, svn::svn_error& raw_error) {
    auto error = v8::Exception::Error(v8::New<v8::String>(isolate, raw_error.what()));
    if (raw_error.child != nullptr)
        error.As<v8::Object>()->Set(InternalizedString("child"), copy_error(isolate, *raw_error.child));
    return error;
}

static std::string convert_string(const v8::Local<v8::Value>& value) {
    if (!value->IsString())
        throw svn::svn_type_error("");

    v8::String::Utf8Value utf8(value);
    auto                  length = static_cast<size_t>(utf8.length());

    if (std::strlen(*utf8) != length)
        throw svn::svn_type_error("");

    return std::string(*utf8, length);
}

static std::vector<std::string> convert_array(const v8::Local<v8::Value>& value, bool allowEmpty) {
    if (value->IsUndefined()) {
        if (allowEmpty)
            return std::vector<std::string>();
        else
            throw svn::svn_type_error("");
    }

    if (value->IsString())
        return std::vector<std::string>{convert_string(value)};

    if (value->IsArray()) {
        auto array  = value.As<v8::Array>();
        auto length = array->Length();
        auto result = std::vector<std::string>();
        for (uint32_t i = 0; i < length; i++) {
            auto item = array->Get(i);
            result.push_back(std::move(convert_string(item)));
        }
        return result;
    }

    throw svn::svn_type_error("");
}

static v8::Local<v8::Object> convert_options(const v8::Local<v8::Value> options) {
    if (options->IsUndefined())
        return v8::Local<v8::Object>();

    if (options->IsObject())
        return options.As<v8::Object>();

    throw svn::svn_type_error("");
}

static svn::revision convert_revision(v8::Isolate*                 isolate,
                                      const v8::Local<v8::Object>& options,
                                      const char*                  key,
                                      svn::revision_kind           defaultValue) {
    if (options.IsEmpty())
        return svn::revision{defaultValue};

    auto value = options->Get(v8::New<v8::String>(isolate, key, v8::NewStringType::kInternalized));
    if (value->IsUndefined())
        return svn::revision{defaultValue};

    if (value->IsNumber()) {
        auto simple = static_cast<svn::revision_kind>(value->Int32Value());
        switch (simple) {
            case svn::revision_kind::unspecified:
            case svn::revision_kind::committed:
            case svn::revision_kind::previous:
            case svn::revision_kind::base:
            case svn::revision_kind::working:
            case svn::revision_kind::head:
                return svn::revision{simple};
            case svn::revision_kind::number:
            case svn::revision_kind::date:
            default:
                throw svn::svn_type_error("");
        }
    }

    if (value->IsObject()) {
        auto object = value.As<v8::Object>();
        auto number = object->Get(v8::New<v8::String>(isolate, "number", v8::NewStringType::kInternalized));
        if (!number->IsUndefined()) {
            if (!number->IsNumber())
                throw svn::svn_type_error("");

            return svn::revision(number->Int32Value());
        }

        auto date = object->Get(v8::New<v8::String>(isolate, "date", v8::NewStringType::kInternalized));
        if (!date->IsUndefined()) {
            if (!date->IsNumber())
                throw svn::svn_type_error("");

            return svn::revision(date->IntegerValue());
        }
    }

    throw svn::svn_type_error("");
}

static svn::depth convert_depth(v8::Isolate*                 isolate,
                                const v8::Local<v8::Object>& options,
                                const char*                  key,
                                svn::depth                   defaultValue) {
    if (options.IsEmpty())
        return defaultValue;

    auto value = options->Get(v8::New<v8::String>(isolate, key, v8::NewStringType::kInternalized));
    if (value->IsUndefined())
        return defaultValue;

    if (value->IsNumber())
        return static_cast<svn::depth>(value->Int32Value());

    throw svn::svn_type_error("");
}

static bool convert_boolean(v8::Isolate*                 isolate,
                            const v8::Local<v8::Object>& options,
                            const char*                  key,
                            bool                         defaultValue) {
    if (options.IsEmpty())
        return defaultValue;

    auto value = options->Get(v8::New<v8::String>(isolate, key, v8::NewStringType::kInternalized));
    if (value->IsUndefined())
        return defaultValue;

    if (value->IsBoolean())
        return value->BooleanValue();

    throw svn::svn_type_error("");
}

static std::vector<std::string> convert_array(v8::Isolate*          isolate,
                                              v8::Local<v8::Object> options,
                                              const char*           key) {
    if (options.IsEmpty())
        return std::vector<std::string>();

    auto value = options->Get(v8::New<v8::String>(isolate, key, v8::NewStringType::kInternalized));
    if (value->IsUndefined())
        return std::vector<std::string>();

    return convert_array(value, true);
}

static void buffer_free_pointer(char*, void* hint) {
    delete static_cast<std::vector<char>*>(hint);
}

static v8::Local<v8::Object> buffer_from_vector(v8::Isolate* isolate, std::vector<char>& vector) {
    auto pointer = new std::vector<char>(std::move(vector));
    return node::Buffer::New(isolate,
                             pointer->data(),
                             pointer->size(),
                             buffer_free_pointer,
                             pointer)
        .ToLocalChecked();
}

#define STRINGIFY_INTERNAL(X) #X
#define STRINGIFY(X) STRINGIFY_INTERNAL(X)

#define SetReadOnly(object, name, value)                  \
    (object)->DefineOwnProperty(context,                  \
                                InternalizedString(name), \
                                value,                    \
                                v8::PropertyAttributeEx::ReadOnlyDontDelete)

#define SetPrototypeMethod(signature, prototype, name, callback, length)                     \
    /* Add a scope to hide extra variables */                                                \
    {                                                                                        \
        auto function = v8::FunctionTemplate::New(isolate,                /* isolate */      \
                                                  callback,               /* callback */     \
                                                  v8::Local<v8::Value>(), /* data */         \
                                                  signature,              /* signature */    \
                                                  length);                /* length */       \
        function->RemovePrototype();                                                         \
        prototype->Set(InternalizedString(name), function, v8::PropertyAttribute::DontEnum); \
    }

#define CONVERT_OPTIONS_AND_CALLBACK(index)                \
    v8::Local<v8::Object>   options;                       \
    v8::Local<v8::Function> raw_callback;                  \
    if (args[index]->IsFunction()) {                       \
        raw_callback = args[index].As<v8::Function>();     \
    } else if (args[index + 1]->IsFunction()) {            \
        options      = convert_options(args[index]);       \
        raw_callback = args[index + 1].As<v8::Function>(); \
    } else {                                               \
        throw svn::svn_type_error("");                     \
    }                                                      \
    auto _raw_callback = std::make_shared<v8::Global<v8::Function>>(isolate, raw_callback);

namespace node {
void CLASS_NAME::init(v8::Local<v8::Object>   exports,
                      v8::Isolate*            isolate,
                      v8::Local<v8::Context>& context) {
    auto client    = v8::New<v8::FunctionTemplate>(isolate, create_instance);
    auto signature = v8::Signature::New(isolate, client);

    client->SetClassName(InternalizedString(EXPORT_NAME));
    client->ReadOnlyPrototype();

    client->InstanceTemplate()->SetInternalFieldCount(1);

    auto prototype = client->PrototypeTemplate();
    SetPrototypeMethod(signature, prototype, "add_to_changelist", add_to_changelist, 2);
    SetPrototypeMethod(signature, prototype, "get_changelists", get_changelists, 2);
    SetPrototypeMethod(signature, prototype, "remove_from_changelists", remove_from_changelists, 2);

    SetPrototypeMethod(signature, prototype, "add", add, 1);
    SetPrototypeMethod(signature, prototype, "cat", cat, 1);
    SetPrototypeMethod(signature, prototype, "checkout", checkout, 2);
    SetPrototypeMethod(signature, prototype, "commit", commit, 3);
    SetPrototypeMethod(signature, prototype, "info", info, 2);
    SetPrototypeMethod(signature, prototype, "remove", remove, 2);
    SetPrototypeMethod(signature, prototype, "revert", revert, 1);
    SetPrototypeMethod(signature, prototype, "status", status, 2);
    SetPrototypeMethod(signature, prototype, "update", update, 1);

    SetPrototypeMethod(signature, prototype, "get_working_copy_root", get_working_copy_root, 1);

    SetReadOnly(exports, EXPORT_NAME, client->GetFunction());
}

void CLASS_NAME::create_instance(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        auto isolate = args.GetIsolate();
        auto message = "Class constructor " STRINGIFY(CLASS_NAME) " cannot be invoked without 'new'";
        isolate->ThrowException(v8::Exception::TypeError(v8::New<v8::String>(isolate, message)));
        return;
    }

    auto result = new CLASS_NAME();
    result->Wrap(args.This());
}

METHOD_BEGIN(add_to_changelist)
    auto paths      = convert_array(args[0], false);
    auto changelist = convert_string(args[1]);

    auto options     = convert_options(args[2]);
    auto depth       = convert_depth(isolate, options, "depth", svn::depth::infinity);
    auto changelists = convert_array(isolate, options, "changelists");

    ASYNC_BEGIN(void, paths, changelist, depth, changelists)
        _this->_client->add_to_changelist(paths, changelist, depth, changelists);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate))
METHOD_END

METHOD_BEGIN(get_changelists)
    auto path = convert_string(args[0]);

    CONVERT_OPTIONS_AND_CALLBACK(1)

    auto _callback = [isolate, _raw_callback](const char* path, const char* changelist) -> void {
        v8::HandleScope scope(isolate);

        const auto           argc       = 2;
        v8::Local<v8::Value> argv[argc] = {
            v8::New<v8::String>(isolate, path),
            v8::New<v8::String>(isolate, changelist)};

        auto callback = _raw_callback->Get(isolate);
        callback->Call(v8::Undefined(isolate), argc, argv);
    };
    auto callback = TO_ASYNC_CALLBACK(_callback, const char*, const char*);

    auto depth       = convert_depth(isolate, options, "depth", svn::depth::infinity);
    auto changelists = convert_array(isolate, options, "changelists");

    ASYNC_BEGIN(void, path, callback, depth, changelists)
        _this->_client->get_changelists(path, callback, depth, changelists);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate));
METHOD_END

METHOD_BEGIN(remove_from_changelists)
    auto paths = convert_array(args[0], false);

    auto options     = convert_options(args[1]);
    auto depth       = convert_depth(isolate, options, "depth", svn::depth::infinity);
    auto changelists = convert_array(isolate, options, "changelists");

    ASYNC_BEGIN(void, paths, depth, changelists)
        _this->_client->remove_from_changelists(paths, depth, changelists);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate));
METHOD_END

METHOD_BEGIN(add)
    auto path = convert_string(args[0]);

    auto options = convert_options(args[1]);
    auto depth   = convert_depth(isolate, options, "depth", svn::depth::infinity);

    ASYNC_BEGIN(void, path, depth)
        _this->_client->add(path, depth);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate));
METHOD_END

METHOD_BEGIN(cat)
    auto path = convert_string(args[0]);

    auto options      = convert_options(args[1]);
    auto peg_revision = convert_revision(isolate, options, "peg_revision", svn::revision_kind::working);
    auto revision     = convert_revision(isolate, options, "revision", svn::revision_kind::working);

    ASYNC_BEGIN(svn::cat_result, path, peg_revision, revision)
        ASYNC_RETURN(_this->_client->cat(path, peg_revision, revision));
    ASYNC_END()

    auto raw_result = ASYNC_RESULT;

    auto result = v8::New<v8::Object>(isolate);
    result->Set(InternalizedString("content"), buffer_from_vector(isolate, raw_result.content));

    auto properties = v8::New<v8::Object>(isolate);
    for (auto pair : raw_result.properties) {
        properties->Set(v8::New<v8::String>(isolate, pair.first), v8::New<v8::String>(isolate, pair.second));
    }
    result->Set(InternalizedString("properties"), properties);

    METHOD_RETURN(result);
METHOD_END

METHOD_BEGIN(checkout)
    auto url  = convert_string(args[0]);
    auto path = convert_string(args[1]);

    auto options      = convert_options(args[2]);
    auto peg_revision = convert_revision(isolate, options, "peg_revision", svn::revision_kind::working);
    auto revision     = convert_revision(isolate, options, "revision", svn::revision_kind::working);
    auto depth        = convert_depth(isolate, options, "depth", svn::depth::infinity);

    ASYNC_BEGIN(void, url, path, peg_revision, revision, depth)
        _this->_client->checkout(url, path, peg_revision, revision, depth);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate));
METHOD_END

static svn::client::commit_callback convert_commit_callback(v8::Isolate* isolate, const v8::Local<v8::Value>& value) {
    if (!value->IsFunction())
        throw svn::svn_type_error("");

    auto raw_callback  = value.As<v8::Function>();
    auto _raw_callback = std::make_shared<v8::Global<v8::Function>>(isolate, raw_callback);
    auto _callback     = [isolate, _raw_callback](const svn::commit_info* raw_info) -> void {
        v8::HandleScope scope(isolate);

        auto info = v8::New<v8::Object>(isolate);
        info->Set(InternalizedString("author"), v8::New<v8::String>(isolate, raw_info->author));
        info->Set(InternalizedString("date"), v8::New<v8::String>(isolate, raw_info->date));
        info->Set(InternalizedString("repos_root"), v8::New<v8::String>(isolate, raw_info->repos_root));
        info->Set(InternalizedString("revision"), v8::New<v8::Integer>(isolate, raw_info->revision));

        if (raw_info->post_commit_error != nullptr)
            info->Set(InternalizedString("post_commit_error"), v8::New<v8::String>(isolate, raw_info->post_commit_error));

        const auto           argc       = 1;
        v8::Local<v8::Value> argv[argc] = {info};

        auto callback = _raw_callback->Get(isolate);
        callback->Call(v8::Undefined(isolate), argc, argv);
    };

    return TO_ASYNC_CALLBACK(_callback, const svn::commit_info*);
}

METHOD_BEGIN(commit)
    auto paths    = convert_array(args[0], false);
    auto message  = convert_string(args[1]);
    auto callback = convert_commit_callback(isolate, args[2]);

    ASYNC_BEGIN(void, paths, message, callback)
        _this->_client->commit(paths, message, callback);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate));
METHOD_END

static v8::Local<v8::Value> copy_int64(v8::Isolate* isolate, int64_t value) {
    if (value <= INT32_MAX)
        return v8::New<v8::Integer>(isolate, static_cast<int32_t>(value));
    else
        return v8::New<v8::String>(isolate, std::to_string(value));
}

METHOD_BEGIN(info)
    auto path = convert_string(args[0]);

    CONVERT_OPTIONS_AND_CALLBACK(1)

    auto _callback = [isolate, _raw_callback](const char* path, const svn::info* raw_info) -> void {
        v8::HandleScope scope(isolate);

        auto info = v8::New<v8::Object>(isolate);
        info->Set(InternalizedString("path"), v8::New<v8::String>(isolate, path));
        info->Set(InternalizedString("kind"), v8::New<v8::Integer>(isolate, static_cast<int32_t>(raw_info->kind)));
        info->Set(InternalizedString("last_changed_author"), v8::New<v8::String>(isolate, raw_info->last_changed_author));
        info->Set(InternalizedString("last_changed_date"), copy_int64(isolate, raw_info->last_changed_date));
        info->Set(InternalizedString("last_changed_rev"), v8::New<v8::Integer>(isolate, static_cast<int32_t>(raw_info->last_changed_rev)));
        info->Set(InternalizedString("repos_root_url"), v8::New<v8::String>(isolate, raw_info->repos_root_URL));
        info->Set(InternalizedString("url"), v8::New<v8::String>(isolate, raw_info->URL));

        const auto           argc       = 1;
        v8::Local<v8::Value> argv[argc] = {info};

        auto callback = _raw_callback->Get(isolate);
        callback->Call(v8::Undefined(isolate), argc, argv);
    };
    auto callback = TO_ASYNC_CALLBACK(_callback, const char*, const svn::info*);

    auto peg_revision = convert_revision(isolate, options, "peg_revision", svn::revision_kind::working);
    auto revision     = convert_revision(isolate, options, "revision", svn::revision_kind::working);
    auto depth        = convert_depth(isolate, options, "depth", svn::depth::empty);

    ASYNC_BEGIN(void, path, callback, peg_revision, revision, depth)
        _this->_client->info(path, callback, peg_revision, revision, depth);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate));
METHOD_END

METHOD_BEGIN(remove)
    auto paths    = convert_array(args[0], false);
    auto callback = convert_commit_callback(isolate, args[1]);

    ASYNC_BEGIN(void, paths, callback)
        _this->_client->remove(paths, callback);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate));
METHOD_END

METHOD_BEGIN(revert)
    auto paths = convert_array(args[0], false);

    ASYNC_BEGIN(void, paths)
        _this->_client->revert(paths);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate));
METHOD_END

static v8::Local<v8::Value> copy_string(v8::Isolate* isolate, const char* value) {
    if (value == nullptr)
        return v8::Undefined(isolate);
    return v8::New<v8::String>(isolate, value);
}

METHOD_BEGIN(status)
    auto path = convert_string(args[0]);

    CONVERT_OPTIONS_AND_CALLBACK(1)

    auto _callback = [isolate, _raw_callback](const char* path, const svn::status* raw_info) -> void {
        v8::HandleScope scope(isolate);

        auto info = v8::New<v8::Object>(isolate);
        info->Set(InternalizedString("path"), v8::New<v8::String>(isolate, path));
        info->Set(InternalizedString("changelist"), copy_string(isolate, raw_info->changelist));
        info->Set(InternalizedString("changed_author"), copy_string(isolate, raw_info->changed_author));
        info->Set(InternalizedString("changed_date"), copy_int64(isolate, raw_info->changed_date));
        info->Set(InternalizedString("changed_rev"), v8::New<v8::Integer>(isolate, raw_info->changed_rev));
        info->Set(InternalizedString("conflicted"), v8::New<v8::Boolean>(isolate, raw_info->conflicted));
        info->Set(InternalizedString("copied"), v8::New<v8::Boolean>(isolate, raw_info->copied));
        info->Set(InternalizedString("depth"), v8::New<v8::Integer>(isolate, static_cast<int32_t>(raw_info->node_depth)));
        info->Set(InternalizedString("file_external"), v8::New<v8::Boolean>(isolate, raw_info->file_external));
        info->Set(InternalizedString("kind"), v8::New<v8::Integer>(isolate, static_cast<int32_t>(raw_info->kind)));
        info->Set(InternalizedString("node_status"), v8::New<v8::Integer>(isolate, static_cast<int32_t>(raw_info->node_status)));
        info->Set(InternalizedString("prop_status"), v8::New<v8::Integer>(isolate, static_cast<int32_t>(raw_info->prop_status)));
        info->Set(InternalizedString("revision"), v8::New<v8::Integer>(isolate, static_cast<int32_t>(raw_info->revision)));
        info->Set(InternalizedString("text_status"), v8::New<v8::Integer>(isolate, static_cast<int32_t>(raw_info->text_status)));
        info->Set(InternalizedString("versioned"), v8::New<v8::Boolean>(isolate, raw_info->versioned));

        const auto           argc       = 1;
        v8::Local<v8::Value> argv[argc] = {info};

        auto callback = _raw_callback->Get(isolate);
        callback->Call(v8::Undefined(isolate), argc, argv);
    };
    auto callback = TO_ASYNC_CALLBACK(_callback, const char*, const svn::status*);

    auto revision         = convert_revision(isolate, options, "revision", svn::revision_kind::working);
    auto depth            = convert_depth(isolate, options, "depth", svn::depth::infinity);
    auto ignore_externals = convert_boolean(isolate, options, "ignore_externals", false);

    ASYNC_BEGIN(void, path, callback, revision, depth, ignore_externals)
        _this->_client->status(path, callback, revision, depth, false, false, true, false, ignore_externals);
    ASYNC_END()

    ASYNC_RESULT;
    METHOD_RETURN(v8::Undefined(isolate));
METHOD_END

METHOD_BEGIN(update)
    auto paths = convert_array(args[0], false);

    ASYNC_BEGIN(std::vector<int32_t>, paths)
        ASYNC_RETURN(_this->_client->update(paths));
    ASYNC_END(args)

    if (args[0]->IsString()) {
        auto result = v8::New<v8::Integer>(isolate, ASYNC_RESULT[0]);
        METHOD_RETURN(result);
    } else {
        auto vector = ASYNC_RESULT;
        auto result = v8::New<v8::Array>(isolate, static_cast<int32_t>(vector.size()));
        for (uint32_t i = 0; i < vector.size(); i++)
            result->Set(i, v8::New<v8::Integer>(isolate, vector[i]));
        METHOD_RETURN(result);
    }
METHOD_END

METHOD_BEGIN(get_working_copy_root)
    auto path = convert_string(args[0]);

    ASYNC_BEGIN(std::string, path)
        ASYNC_RETURN(_this->_client->get_working_copy_root(path));
    ASYNC_END()

    auto result = v8::New<v8::String>(isolate, ASYNC_RESULT);
    METHOD_RETURN(result);
METHOD_END

CLASS_NAME::CLASS_NAME()
    : _client(new svn::client()) {}

CLASS_NAME::~CLASS_NAME() {}

} // namespace node
