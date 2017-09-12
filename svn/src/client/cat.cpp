#include "client.hpp"
#include "pool_ptr.hpp"

namespace Svn
{
void free_buffer(char *data, void *hint)
{
    auto pool = static_cast<apr_pool_t *>(hint);
    apr_pool_destroy(pool);
}

Util_Method(Client::Cat)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string"));

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));
    auto path = make_shared<string>(to_string(arg));
    Util_RejectIf(Util::ContainsNull(*path), Util_Error(Error, "Argument \"path\" must be a string without null bytes"));

    Util_PreparePool();

    // Create a sub pool for `buffer`
    // To create node::Buffer without copy
    apr_pool_t *buffer_pool;
    apr_pool_create(&buffer_pool, client->pool);
    auto buffer = svn_stringbuf_create_empty(buffer_pool);

    auto _error = make_shared<svn_error_t *>();
    auto work = [buffer, pool, path, client, _error]() -> void {
        apr_hash_t *props;

        auto stream = svn_stream_from_stringbuf(buffer, pool.get());

        svn_opt_revision_t revision{svn_opt_revision_head};

        apr_pool_t *scratch_pool;
        apr_pool_create(&scratch_pool, pool.get());

        *_error = svn_client_cat3(&props,          // props
                                  stream,          // out
                                  path->c_str(),   // path_or_url
                                  &revision,       // peg_revision
                                  &revision,       // revision
                                  false,           // expand_keywords
                                  client->context, // ctx
                                  pool.get(),      // result_pool
                                  scratch_pool);   // scratch_pool
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, _error, buffer, buffer_pool]() -> void {
        auto context = isolate->GetCallingContext();
        HandleScope scope(isolate);

        auto resolver = _resolver->Get(isolate);

        auto error = *_error;
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        auto result = node::Buffer::New(isolate,
                                        buffer->data,
                                        buffer->len,
                                        free_buffer,
                                        buffer_pool)
                          .ToLocalChecked();
        resolver->Resolve(context, result);
    };

    Util_RejectIf(Util::QueueWork(uv_default_loop(), move(work), move(after_work)), Util_Error(Error, "Failed to start async work"));
}
Util_MethodEnd;
}
