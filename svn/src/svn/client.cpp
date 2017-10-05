#include <svn_client.h>
#include <svn_io.h>
#include <svn_path.h>

#include "client.hpp"

using std::function;
using std::make_shared;
using std::shared_ptr;

namespace svn {
static void check_result(svn_error_t* result) {
    if (result != nullptr)
        throw result;
}

static const char* convert_string(const std::string& value) {
    if (value.find('\0') != std::string::npos)
        throw std::invalid_argument("Value connot contain null bytes");

    return value.c_str();
}

static const char* convert_path(const std::string& value,
                                const apr::pool&   pool) {
    auto raw = convert_string(value);

    if (!svn_path_is_url(raw))
        check_result(svn_dirent_get_absolute(&raw, raw, pool.get()));

    return raw;
}

static const apr_array_header_t* convert_changelists(const std::vector<std::string>& value,
                                                     const apr::pool&                pool) {
    auto result = apr::array<const char*>(pool, value.size());

    for (auto item = value.begin(); item != value.end(); item++)
        result.push_back(convert_string(*item));

    return result.get();
}

static const apr_array_header_t* convert_paths(const std::string& value,
                                               const apr::pool&   pool) {
    auto result = apr::array<const char*>(pool, 1);

    auto raw_path = convert_path(value, pool);
    result.push_back(raw_path);

    return result.get();
}

static const apr_array_header_t* convert_paths(const std::vector<std::string>& value, const apr::pool& pool) {
    auto result = apr::array<const char*>(pool, value.size());

    for (auto item = value.begin(); item != value.end(); item++)
        result.push_back(convert_path(*item, pool));

    return result.get();
}

client::client()
    : _pool(apr::pool()) {
    check_result(svn_client_create_context2(&_value, nullptr, _pool.get()));

    _value->notify_func2 = [](void*                  baton,
                              const svn_wc_notify_t* notify,
                              apr_pool_t*            pool) -> void {
        auto _this = static_cast<client*>(baton);
        // _this->notify(notify);
    };
    _value->notify_baton2 = this;
}

void client::add_to_changelist(
    const std::string&              path,
    const std::string&              changelist,
    svn_depth_t                     depth,
    const std::vector<std::string>& changelists) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(path, pool);

    auto raw_changelist = convert_string(changelist);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_add_to_changelist(raw_paths,
                                              raw_changelist,
                                              depth,
                                              raw_changelists,
                                              _value,
                                              pool.get()));
}

void client::add_to_changelist(
    const std::vector<std::string>& paths,
    const std::string&              changelist,
    svn_depth_t                     depth,
    const std::vector<std::string>& changelists) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(paths, pool);

    auto raw_changelist = convert_string(changelist);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_add_to_changelist(raw_paths,
                                              raw_changelist,
                                              depth,
                                              raw_changelists,
                                              _value,
                                              pool.get()));
}

static svn_error_t* invoke_get_changelists(void*       baton,
                                           const char* path,
                                           const char* changelist,
                                           apr_pool_t* raw_pool) {
    auto callback = *static_cast<get_changelists_callback*>(baton);
    auto pool     = apr::pool::unmanaged(raw_pool);
    callback(path, changelist, pool);
    return nullptr;
}

void client::get_changelists(const std::string&              path,
                             get_changelists_callback&       callback,
                             const std::vector<std::string>& changelists,
                             svn_depth_t                     depth) const {
    auto pool = _pool.create();

    auto raw_path = convert_path(path, pool);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_get_changelists(raw_path,
                                            raw_changelists,
                                            depth,
                                            invoke_get_changelists,
                                            &callback,
                                            _value,
                                            pool.get()));
}

void client::remove_from_changelists(
    const std::string&              path,
    svn_depth_t                     depth,
    const std::vector<std::string>& changelists) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(path, pool);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_remove_from_changelists(raw_paths,
                                                    depth,
                                                    raw_changelists,
                                                    _value,
                                                    pool.get()));
}

void client::remove_from_changelists(
    const std::vector<std::string>& paths,
    svn_depth_t                     depth,
    const std::vector<std::string>& changelists) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(paths, pool);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_remove_from_changelists(raw_paths,
                                                    depth,
                                                    raw_changelists,
                                                    _value,
                                                    pool.get()));
}

void client::add(const std::string& path,
                 svn_depth_t        depth,
                 bool               force,
                 bool               no_ignore,
                 bool               no_autoprops,
                 bool               add_parents) const {
    auto pool = _pool.create();

    auto raw_path = convert_path(path, pool);

    check_result(svn_client_add5(raw_path,
                                 depth,
                                 force,
                                 no_ignore,
                                 no_autoprops,
                                 add_parents,
                                 _value,
                                 pool.get()));
}

svn_error_t*
invoke_cat_callback(void* baton, const char* data, apr_size_t* len) {
    auto callback = *static_cast<cat_callback*>(baton);
    callback(data, *len);

    return nullptr;
}

void client::cat(const std::string&        path,
                 apr_hash_t**              props,
                 cat_callback              callback,
                 const svn_opt_revision_t& peg_revision,
                 const svn_opt_revision_t& revision,
                 bool                      expand_keywords) const {
    auto pool = _pool.create();

    auto stream = pool.alloc<svn_stream_t>();
    svn_stream_set_baton(stream, &callback);
    svn_stream_set_write(stream, invoke_cat_callback);

    auto raw_path = convert_path(path, pool);

    auto scratch_pool = pool.create();

    check_result(svn_client_cat3(props,
                                 stream,
                                 raw_path,
                                 &peg_revision,
                                 &revision,
                                 expand_keywords,
                                 _value,
                                 pool.get(),
                                 scratch_pool.get()));
}

svn_revnum_t client::checkout(const std::string&        url,
                              const std::string&        path,
                              const svn_opt_revision_t& peg_revision,
                              const svn_opt_revision_t& revision,
                              svn_depth_t               depth,
                              bool                      ignore_externals,
                              bool                      allow_unver_obstructions) const {
    auto pool = _pool.create();

    auto raw_path = convert_path(path, pool);

    auto result_rev = 0L;

    check_result(svn_client_checkout3(&result_rev,
                                      url.c_str(),
                                      raw_path,
                                      &peg_revision,
                                      &revision,
                                      depth,
                                      ignore_externals,
                                      allow_unver_obstructions,
                                      _value,
                                      pool.get()));

    return result_rev;
}

static svn_error_t* invoke_commit(const svn_commit_info_t* commit_info,
                                  void*                    baton,
                                  apr_pool_t*              raw_pool) {
    auto callback = *static_cast<commit_callback*>(baton);
    auto pool     = apr::pool::unmanaged(raw_pool);
    callback(commit_info, pool);
    return nullptr;
}

void client::commit(const std::string&              path,
                    const std::string&              message,
                    commit_callback&                callback,
                    svn_depth_t                     depth,
                    const std::vector<std::string>& changelists,
                    apr_hash_t*                     revprop_table,
                    bool                            keep_locks,
                    bool                            keep_changelists,
                    bool                            commit_as_aperations,
                    bool                            include_file_externals,
                    bool                            include_dir_externals) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(path, pool);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_commit6(raw_paths,
                                    depth,
                                    keep_locks,
                                    keep_changelists,
                                    commit_as_aperations,
                                    include_file_externals,
                                    include_dir_externals,
                                    raw_changelists,
                                    revprop_table,
                                    invoke_commit,
                                    &callback,
                                    _value,
                                    pool.get()));
}

void client::commit(const std::vector<std::string>& paths,
                    const std::string&              message,
                    commit_callback&                callback,
                    svn_depth_t                     depth,
                    const std::vector<std::string>& changelists,
                    apr_hash_t*                     revprop_table,
                    bool                            keep_locks,
                    bool                            keep_changelists,
                    bool                            commit_as_aperations,
                    bool                            include_file_externals,
                    bool                            include_dir_externals) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(paths, pool);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_commit6(raw_paths,
                                    depth,
                                    keep_locks,
                                    keep_changelists,
                                    commit_as_aperations,
                                    include_file_externals,
                                    include_dir_externals,
                                    raw_changelists,
                                    revprop_table,
                                    invoke_commit,
                                    &callback,
                                    _value,
                                    pool.get()));
}

static svn_error_t* invoke_info(void*                     baton,
                                const char*               path,
                                const svn_client_info2_t* info,
                                apr_pool_t*               raw_scratch_pool) {
    auto callback     = *static_cast<info_callback*>(baton);
    auto scratch_pool = apr::pool::unmanaged(raw_scratch_pool);
    callback(path, info, scratch_pool);
    return nullptr;
}

void client::info(const std::string&              path,
                  info_callback&                  callback,
                  const svn_opt_revision_t&       peg_revision,
                  const svn_opt_revision_t&       revision,
                  svn_depth_t                     depth,
                  bool                            fetch_excluded,
                  bool                            fetch_actual_only,
                  bool                            include_externals,
                  const std::vector<std::string>& changelists) const {
    auto pool = _pool.create();

    auto raw_path = convert_path(path, pool);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_info4(raw_path,
                                  &peg_revision,
                                  &revision,
                                  depth,
                                  fetch_excluded,
                                  fetch_actual_only,
                                  include_externals,
                                  raw_changelists,
                                  invoke_info,
                                  &callback,
                                  _value,
                                  pool.get()));
}

void client::remove(const std::string& path,
                    remove_callback&   callback,
                    bool               force,
                    bool               keep_local,
                    apr_hash_t*        revprop_table) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(path, pool);

    check_result(svn_client_delete4(raw_paths,
                                    force,
                                    keep_local,
                                    revprop_table,
                                    invoke_commit,
                                    &callback,
                                    _value,
                                    pool.get()));
}

void client::remove(const std::vector<std::string>& paths,
                    remove_callback&                callback,
                    bool                            force,
                    bool                            keep_local,
                    apr_hash_t*                     revprop_table) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(paths, pool);

    check_result(svn_client_delete4(raw_paths,
                                    force,
                                    keep_local,
                                    revprop_table,
                                    invoke_commit,
                                    &callback,
                                    _value,
                                    pool.get()));
}

void client::revert(const std::string&              path,
                    svn_depth_t                     depth,
                    const std::vector<std::string>& changelists,
                    bool                            clear_changelists,
                    bool                            metadata_only) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(path, pool);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_revert3(raw_paths,
                                    depth,
                                    raw_changelists,
                                    clear_changelists,
                                    metadata_only,
                                    _value,
                                    pool.get()));
}

void client::revert(const std::vector<std::string>& paths,
                    svn_depth_t                     depth,
                    const std::vector<std::string>& changelists,
                    bool                            clear_changelists,
                    bool                            metadata_only) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(paths, pool);

    auto raw_changelists = convert_changelists(changelists, pool);

    check_result(svn_client_revert3(raw_paths,
                                    depth,
                                    raw_changelists,
                                    clear_changelists,
                                    metadata_only,
                                    _value,
                                    pool.get()));
}

static svn_error_t* invoke_status(void*                      baton,
                                  const char*                path,
                                  const svn_client_status_t* status,
                                  apr_pool_t*                raw_scratch_pool) {
    auto callback     = *static_cast<status_callback*>(baton);
    auto scratch_pool = apr::pool::unmanaged(raw_scratch_pool);
    callback(path, status, scratch_pool);
    return nullptr;
}

svn_revnum_t client::status(const std::string&              path,
                            status_callback&                callback,
                            const svn_opt_revision_t&       revision,
                            svn_depth_t                     depth,
                            bool                            get_all,
                            bool                            check_out_of_date,
                            bool                            check_working_copy,
                            bool                            no_ignore,
                            bool                            ignore_externals,
                            bool                            depth_as_sticky,
                            const std::vector<std::string>& changelists) const {
    auto pool = _pool.create();

    auto raw_path = convert_path(path, pool);

    auto raw_changelists = convert_changelists(changelists, pool);

    auto result_rev = 0L;

    check_result(svn_client_status6(&result_rev,
                                    _value,
                                    raw_path,
                                    &revision,
                                    depth,
                                    get_all,
                                    check_out_of_date,
                                    check_working_copy,
                                    no_ignore,
                                    ignore_externals,
                                    depth_as_sticky,
                                    raw_changelists,
                                    invoke_status,
                                    &callback,
                                    pool.get()));

    return result_rev;
}

std::vector<svn_revnum_t> client::update(const std::string&        path,
                                         const svn_opt_revision_t& revision,
                                         svn_depth_t               depth,
                                         bool                      depth_is_sticky,
                                         bool                      ignore_externals,
                                         bool                      allow_unver_obstructions,
                                         bool                      adds_as_modification,
                                         bool                      make_parents) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(path, pool);

    apr_array_header_t* raw_result_revs;

    check_result(svn_client_update4(&raw_result_revs,
                                    raw_paths,
                                    &revision,
                                    depth,
                                    depth_is_sticky,
                                    ignore_externals,
                                    allow_unver_obstructions,
                                    adds_as_modification,
                                    make_parents,
                                    _value,
                                    pool.get()));

    auto result_revs = apr::array<svn_revnum_t>(raw_result_revs);

    auto result = std::vector<svn_revnum_t>(1);
    result.push_back(result_revs[0]);
    return result;
}

std::vector<svn_revnum_t> client::update(const std::vector<std::string>& paths,
                                         const svn_opt_revision_t&       revision,
                                         svn_depth_t                     depth,
                                         bool                            depth_is_sticky,
                                         bool                            ignore_externals,
                                         bool                            allow_unver_obstructions,
                                         bool                            adds_as_modification,
                                         bool                            make_parents) const {
    auto pool = _pool.create();

    auto raw_paths = convert_paths(paths, pool);

    apr_array_header_t* raw_result_revs;

    check_result(svn_client_update4(&raw_result_revs,
                                    raw_paths,
                                    &revision,
                                    depth,
                                    depth_is_sticky,
                                    ignore_externals,
                                    allow_unver_obstructions,
                                    adds_as_modification,
                                    make_parents,
                                    _value,
                                    pool.get()));

    auto result_revs = apr::array<svn_revnum_t>(raw_result_revs);

    auto result = std::vector<svn_revnum_t>(result_revs.size());
    for (auto item = result_revs.begin(); item != result_revs.end(); item++)
        result.push_back(*item);
    return result;
}

void client::auth_baton(svn_auth_baton_t* baton) {
    _value->auth_baton = baton;
}
} // namespace svn
