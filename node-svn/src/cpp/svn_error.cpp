#include "svn_error.hpp"

#include <memory>

namespace svn {
svn_error::svn_error(int         code,
                     const char* what,
                     svn_error*  child,
                     std::string file,
                     int         line) noexcept
    : std::runtime_error(what)
    , code(code)
    , child(child)
    , file(file)
    , line(line) {}

svn_error::svn_error(const svn_error& other) noexcept
    : std::runtime_error(other)
    , code(other.code)
    , child(other.child != nullptr ? new svn_error(*other.child) : nullptr)
    , file(other.file)
    , line(other.line) {}

svn_error::svn_error(svn_error&& other) noexcept
    : std::runtime_error(other)
    , code(other.code)
    , child(other.child)
    , file(std::move(other.file))
    , line(other.line) {
    other.child = nullptr;
}

svn_error::~svn_error() {
    delete child;
}
} // namespace svn
