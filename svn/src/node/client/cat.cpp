#include "client.hpp"
#include "revision.hpp"

namespace Svn
{
void free_buffer(char *data, void *hint)
{
    /*auto pool = static_cast<apr_pool_t *>(hint);
    apr_pool_destroy(pool);*/
}

struct CatOptions
{
    svn_opt_revision_t pegRevision;
    svn_opt_revision_t revision;
    bool expandKeywords;
};

V8_METHOD_BEGIN(Client::Cat)
{
    auto resolver = Util_NewMaybe(Promise::Resolver);
    Util_Return(resolver->GetPromise());

    Util_RejectIf(args.Length() == 0, Util_Error(TypeError, "Argument \"path\" must be a string"));

    auto arg = args[0];
    Util_RejectIf(!arg->IsString(), Util_Error(TypeError, "Argument \"path\" must be a string"));

    auto path = Util::to_string(arg);

    auto options = make_shared<CatOptions>();
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

    auto client = ObjectWrap::Unwrap<Client>(args.Holder());

    // Create a sub pool for `buffer`
    // To create node::Buffer without copy
    auto buffer_pool = make_shared<apr::pool>();
    auto buffer = make_shared<svn::stringbuffer>(buffer_pool);
    auto stream = make_shared<svn::stream>(buffer);

    auto work = [path, stream, options, client]() -> svn_error_t * {
        try
        {
            apr_hash_t *props;

             client->cat(path,
                &props,
                stream,
                options->pegRevision,
                options->revision,
                options->expandKeywords);
        }
        catch (svn_error_t *ex)
        {
            return ex;
        }
    };

    auto _resolver = Util_SharedPersistent(Promise::Resolver, resolver);
    auto after_work = [isolate, _resolver, buffer, buffer_pool](svn_error_t *error) -> void {
        HandleScope scope(isolate);
        auto context = isolate->GetCallingContext();

        auto resolver = _resolver->Get(isolate);
        Util_RejectIf(error != SVN_NO_ERROR, SvnError::New(isolate, context, error));

        auto result = node::Buffer::New(isolate,                                // isolate
                                        buffer->data(),                         // data
                                        buffer->length(),                       // length
                                        free_buffer,                            // callback
                                        new shared_ptr<apr::pool>(buffer_pool)) // hint
                          .ToLocalChecked();
        resolver->Resolve(context, result);
    };

    RunAsync();
}
V8_METHOD_END;
}
