#pragma once

#include "svn_error.hpp"

namespace svn {
class svn_type_error : public svn_error {
  public:
    explicit svn_type_error(const char* what)
        : svn_error(-1, what){};
};
} // namespace svn
