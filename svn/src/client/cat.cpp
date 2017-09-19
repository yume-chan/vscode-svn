#include "client.hpp"
#include "pool_ptr.hpp"
#include "revision.hpp"

namespace Svn
{
void free_buffer(char *data, void *hint)
{
    auto pool = static_cast<apr_pool_t *>(hint);
    apr_pool_destroy(pool);
}

struct CatOptions
{
    svn_opt_revision_t pegRevision;
    svn_opt_revision_t revision;
    bool expandKeywords;
};

Util_Method(Client::Cat)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string"));

    Util_PreparePool();

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));
    const char *path = Util_ToAprString(arg);
    Util_RejectIf(path == nullptr, Util_Error(Error, "Argument \"path\" must be a string without null bytes"));
    Util_CheckAbsolutePath(path);

    auto options = Util_AprAllocType(CatOptions);
    options->pegRevision = svn_opt_revision_t{svn_opt_revision_unspecified};
    options->revision = svn_opt_revision_t{svn_opt_revision_unspecified};
    options->expandKeywords = false;

    arg = args[1];
    if (arg->IsObject())
    {
        auto object = arg.As<Object>();

        options->pegRevision = ParseRevision(isolate, context, Util_GetProperty(object, "pegRevision"), svn_opt_revision_unspecified);
        options->revision = ParseRevision(isolate, context, Util_GetProperty(object, "revision"), svn_opt_revision_unspecified);

        auto expandKeywords = Util_GetProperty(object, "expandKeywords");
        if (expandKeywords->IsBoolean())
            options->expandKeywords = expandKeywords->BooleanValue();
    }

    // Create a sub pool for `buffer`
    // To create node::Buffer without copy
    apr_pool_t *buffer_pool;
    apr_pool_create(&buffer_pool, client->pool);
    auto buffer = svn_stringbuf_create_empty(buffer_pool);

    auto work = [buffer, pool, path, options, client]() -> svn_error_t * {
        apr_hash_t *props;

        auto stream = svn_stream_from_stringbuf(buffer, pool.get());

        apr_pool_t *scratch_pool;
        apr_pool_create(&scratch_pool, pool.get());

        SVN_ERR(svn_client_cat3(&props,                  // props
                                stream,                  // out
                                path,                    // path_or_url
                                &options->pegRevision,   // peg_revision
                                &options->revision,      // revision
                                options->expandKeywords, // expand_keywords
                                client->context,         // ctx
                                pool.get(),              // result_pool
                                scratch_pool));          // scratch_pool

        return nullptr;
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, buffer, buffer_pool](svn_error_t *error) -> void {
        HandleScope scope(isolate);
        auto context = isolate->GetCallingContext();

        auto resolver = _resolver->Get(isolate);
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        auto result = node::Buffer::New(isolate,      // isolate
                                        buffer->data, // data
                                        buffer->len,  // length
                                        free_buffer,  // callback
                                        buffer_pool)  // hint
                          .ToLocalChecked();
        resolver->Resolve(context, result);
    };

    RunAsync();
}
Util_MethodEnd;
}
