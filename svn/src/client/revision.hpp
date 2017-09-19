#include <apr_pools.h>
#include <svn_opt.h>

#include "client.hpp"

namespace Svn
{
inline svn_opt_revision_t ParseRevision(Isolate *isolate, Local<Context> &context, Local<Value> &value, enum svn_opt_revision_kind default)
{
    svn_opt_revision_t result;

    if (value->IsInt32())
    {
        auto kind = static_cast<enum svn_opt_revision_kind>(value->IntegerValue());
        switch (kind)
        {
        case svn_opt_revision_unspecified:
        case svn_opt_revision_committed:
        case svn_opt_revision_previous:
        case svn_opt_revision_base:
        case svn_opt_revision_working:
        case svn_opt_revision_head:
            result.kind = kind;
            break;
        case svn_opt_revision_number:
        case svn_opt_revision_date:
        default:
            result.kind = default;
            break;
        }
        return result;
    }

    if (value->IsObject())
    {
        auto object = value->ToObject();

        auto date = Util_GetProperty(object, "date");
        if (date->IsInt32())
        {
            result.kind = svn_opt_revision_date;
            result.value.date = static_cast<apr_time_t>(date->Int32Value() * 1000);
            return result;
        }

        auto number = Util_GetProperty(object, "number");
        if (number->IsInt32())
        {
            result.kind = svn_opt_revision_number;
            result.value.number = static_cast<svn_revnum_t>(date->Int32Value());
            return result;
        }
    }

    result.kind = default;
    return result;
}
}
