#include "client.hpp"

#include <utility>

#include <apr_pools.h>

#include <svn_client.h>
#include <svn_path.h>

#include <cpp/svn_type_error.hpp>

static svn::svn_error* copy_error(svn_error_t* error) {
    const auto buffer_size = 100;
    char       buffer[buffer_size];
    auto       message = svn_err_best_message(error, buffer, buffer_size);

    return new svn::svn_error(error->apr_err,
                              message,
                              error->child ? copy_error(error->child) : nullptr,
                              error->file ? std::string(error->file) : std::string(),
                              error->line);
}

static svn::svn_error copy_error(svn_error_t& error) {
    const auto buffer_size = 100;
    char       buffer[buffer_size];
    auto       message = svn_err_best_message(&error, buffer, buffer_size);

    return svn::svn_error(error.apr_err,
                          message,
                          error.child ? copy_error(error.child) : nullptr,
                          error.file ? std::string(error.file) : std::string(),
                          error.line);
}

static void throw_error(svn_error_t* error) {
    auto purged = svn_error_purge_tracing(error);
    auto result = copy_error(*purged);
    svn_error_clear(error);
    throw result;
}

static void check_result(apr_status_t status) {
    if (status != 0)
        throw_error(svn_error_create(status, nullptr, nullptr));
}

static void check_result(svn_error_t* result) {
    if (result != nullptr)
        throw_error(result);
}

static svn_error_t* throw_on_malfunction(svn_boolean_t can_return,
                                         const char*   file,
                                         int           line,
                                         const char*   expr) {
    throw_error(svn_error_raise_on_malfunction(true, file, line, expr));
    return nullptr;
}

static void check_string(const std::string& value) {
    if (value.size() == 0)
        throw svn::svn_type_error("");

    if (value.find('\0') != std::string::npos)
        throw svn::svn_type_error("");
}

static const char* convert_string(const std::string& value) {
    check_string(value);
    return value.c_str();
}

static const char* convert_path(const std::string& value,
                                apr_pool_t*        pool) {
    auto raw = convert_string(value);

    if (!svn_path_is_url(raw))
        check_result(svn_dirent_get_absolute(&raw, raw, pool));

    return raw;
}

static const apr_array_header_t* convert_vector(const svn::string_vector& value,
                                                apr_pool_t*               pool,
                                                bool                      allowEmpty,
                                                bool                      isPath) {
    if (value.size() == 0) {
        if (allowEmpty)
            return nullptr;
        else
            throw svn::svn_type_error("");
    }

    auto result = apr_array_make(pool, static_cast<int>(value.size()), sizeof(const char*));

    for (auto item : value) {
        auto converted                      = isPath ? convert_path(item, pool) : convert_string(item);
        APR_ARRAY_PUSH(result, const char*) = converted;
    }

    return result;
}

static const apr_array_header_t* convert_vector(const std::string& value,
                                                apr_pool_t*        pool) {
    auto result = apr_array_make(pool, 1, sizeof(const char*));

    auto raw_path                       = convert_path(value, pool);
    APR_ARRAY_PUSH(result, std::string) = raw_path;

    return result;
}

static apr_hash_t* convert_map(const svn::string_map& map, apr_pool_t* pool) {
    if (map.size() == 0)
        return nullptr;

    auto result = apr_hash_make(pool);

    for (auto pair : map) {
        auto key     = pair.first;
        auto raw_key = apr_pmemdup(pool, key.data(), key.size() + 1);

        auto value     = pair.second;
        auto duplicate = static_cast<const char*>(apr_pmemdup(pool, value.data(), value.size() + 1));
        auto raw_value = new svn_string_t{duplicate, value.size()};

        apr_hash_set(result, raw_key, key.size(), raw_value);
    }

    return result;
}

static std::unique_ptr<apr_pool_t, decltype(&apr_pool_destroy)> create_pool(apr_pool_t* parent) {
    apr_pool_t* result;
    check_result(apr_pool_create_ex(&result, parent, nullptr, nullptr));

    return std::unique_ptr<apr_pool_t, decltype(&apr_pool_destroy)>(result, apr_pool_destroy);
}

template <class T>
struct baton_wrapper {
    baton_wrapper(const T& value)
        : value(value) {}

    const T& value;
};

static svn_error_t* get_commit_message(const char**              log_msg,
                                       const char**              tmp_file,
                                       const apr_array_header_t* commit_items,
                                       void*                     raw_baton,
                                       apr_pool_t*               pool) {
    auto message_baton = static_cast<baton_wrapper<std::string>*>(raw_baton);
    *log_msg           = static_cast<const char*>(apr_pmemdup(pool, message_baton->value.c_str(), message_baton->value.size()));
    return nullptr;
}

namespace svn {
client::client() {
    apr_initialize();
    check_result(apr_pool_create_ex(&_pool, nullptr, nullptr, nullptr));
    check_result(svn_client_create_context2(&_context, nullptr, _pool));

    svn_error_set_malfunction_handler(throw_on_malfunction);

    auto providers = apr_array_make(_pool, 10, sizeof(svn_auth_provider_object_t*));

    svn_auth_provider_object_t* provider;
    svn_auth_get_simple_provider2(&provider, nullptr, nullptr, _pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t*) = provider;

    svn_auth_get_username_provider(&provider, _pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t*) = provider;

    svn_auth_baton_t* auth_baton;
    svn_auth_open(&auth_baton, providers, _pool);
    _context->auth_baton = auth_baton;

    const char* path;
    check_result(svn_config_get_user_config_path(&path, nullptr, nullptr, _pool));
    svn_auth_set_parameter(_context->auth_baton, SVN_AUTH_PARAM_CONFIG_DIR, path);

    _context->log_msg_func3 = get_commit_message;
}

client::client(client&& other)
    : _pool(std::exchange(other._pool, nullptr))
    , _context(std::exchange(other._context, nullptr)) {
}

client& client::operator=(client&& other) {
    if (this != &other) {
        if (_pool != nullptr) {
            apr_pool_destroy(_pool);
            apr_terminate();
        }

        _pool    = std::exchange(other._pool, nullptr);
        _context = std::exchange(other._context, nullptr);
    }
    return *this;
}

client::~client() {
    if (_pool != nullptr) {
        apr_pool_destroy(_pool);
        apr_terminate();
    }
}

void client::add_to_changelist(const std::string&   path,
                               const std::string&   changelist,
                               depth                depth,
                               const string_vector& changelists) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths       = convert_vector(path, pool);
    auto raw_changelist  = convert_string(changelist);
    auto raw_changelists = convert_vector(changelists, pool, true, false);

    check_result(svn_client_add_to_changelist(raw_paths,
                                              raw_changelist,
                                              static_cast<svn_depth_t>(depth),
                                              raw_changelists,
                                              _context,
                                              pool));
}

void client::add_to_changelist(const string_vector& paths,
                               const std::string&   changelist,
                               depth                depth,
                               const string_vector& changelists) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths       = convert_vector(paths, pool, false, true);
    auto raw_changelist  = convert_string(changelist);
    auto raw_changelists = convert_vector(changelists, pool, true, false);

    check_result(svn_client_add_to_changelist(raw_paths,
                                              raw_changelist,
                                              static_cast<svn_depth_t>(depth),
                                              raw_changelists,
                                              _context,
                                              pool));
}

static svn_error_t* invoke_get_changelists(void*       raw_baton,
                                           const char* path,
                                           const char* changelist,
                                           apr_pool_t* pool) {
    auto callback_baton = static_cast<baton_wrapper<client::get_changelists_callback>*>(raw_baton);
    callback_baton->value(path, changelist);
    return nullptr;
}

void client::get_changelists(const std::string&              path,
                             const get_changelists_callback& callback,
                             depth                           depth,
                             const string_vector&            changelists) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_path        = convert_path(path, pool);
    auto raw_changelists = convert_vector(changelists, pool, true, false);
    auto callback_baton  = std::make_unique<baton_wrapper<get_changelists_callback>>(callback);

    check_result(svn_client_get_changelists(raw_path,
                                            raw_changelists,
                                            static_cast<svn_depth_t>(depth),
                                            invoke_get_changelists,
                                            callback_baton.get(),
                                            _context,
                                            pool));
}

void client::remove_from_changelists(const std::string&   path,
                                     depth                depth,
                                     const string_vector& changelists) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths       = convert_vector(path, pool);
    auto raw_changelists = convert_vector(changelists, pool, true, false);

    check_result(svn_client_remove_from_changelists(raw_paths,
                                                    static_cast<svn_depth_t>(depth),
                                                    raw_changelists,
                                                    _context,
                                                    pool));
}

void client::remove_from_changelists(const string_vector& paths,
                                     depth                depth,
                                     const string_vector& changelists) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths       = convert_vector(paths, pool, false, true);
    auto raw_changelists = convert_vector(changelists, pool, true, false);

    check_result(svn_client_remove_from_changelists(raw_paths,
                                                    static_cast<svn_depth_t>(depth),
                                                    raw_changelists,
                                                    _context,
                                                    pool));
}

void client::add(const std::string& path,
                 depth              depth,
                 bool               force,
                 bool               no_ignore,
                 bool               no_autoprops,
                 bool               add_parents) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_path = convert_path(path, pool);

    check_result(svn_client_add5(raw_path,
                                 static_cast<svn_depth_t>(depth),
                                 force,
                                 no_ignore,
                                 no_autoprops,
                                 add_parents,
                                 _context,
                                 pool));
}

svn_error_t* invoke_cat_callback(void*       raw_baton,
                                 const char* data,
                                 apr_size_t* len) {
    auto callback_baton = static_cast<baton_wrapper<client::cat_callback>*>(raw_baton);
    callback_baton->value(data, *len);

    return nullptr;
}

static svn_opt_revision_t* convert_revision(const revision& value) {
    auto result        = new svn_opt_revision_t();
    result->kind       = static_cast<svn_opt_revision_kind>(value.kind);
    result->value.date = static_cast<apr_time_t>(value.date);
    return result;
}

string_map client::cat(const std::string&  path,
                       const cat_callback& callback,
                       const revision&     peg_revision,
                       const revision&     revision,
                       bool                expand_keywords) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    apr_hash_t* raw_properties;

    auto raw_path         = convert_path(path, pool);
    auto raw_callback     = std::make_unique<baton_wrapper<cat_callback>>(callback);
    auto raw_peg_revision = convert_revision(peg_revision);
    auto raw_revision     = convert_revision(revision);

    auto stream = svn_stream_create(raw_callback.get(), pool);
    svn_stream_set_write(stream, invoke_cat_callback);

    auto scratch_pool_ptr = create_pool(_pool);
    auto scratch_pool     = scratch_pool_ptr.get();

    check_result(svn_client_cat3(&raw_properties,
                                 stream,
                                 raw_path,
                                 raw_peg_revision,
                                 raw_revision,
                                 expand_keywords,
                                 _context,
                                 pool,
                                 scratch_pool));

    string_map result;

    apr_hash_index_t* index;
    const char*       key;
    size_t            key_size;
    svn_string_t*     value;
    for (index = apr_hash_first(_pool, raw_properties); index; index = apr_hash_next(index)) {
        apr_hash_this(index, reinterpret_cast<const void**>(&key), reinterpret_cast<apr_ssize_t*>(&key_size), reinterpret_cast<void**>(&value));

        result.emplace(std::piecewise_construct,
                       std::forward_as_tuple(key, key_size),
                       std::forward_as_tuple(value->data, value->len));
    }

    return result;
}

cat_result client::cat(const std::string& path,
                       const revision&    peg_revision,
                       const revision&    revision,
                       bool               expand_keywords) const {
    auto content  = std::vector<char>();
    auto callback = [&content](const char* data, size_t length) -> void {
        auto end = data + length;
        content.insert(content.end(), data, end);
    };

    auto properties = cat(path, callback, peg_revision, revision, expand_keywords);

    return cat_result{content, properties};
}

int32_t client::checkout(const std::string& url,
                         const std::string& path,
                         const revision&    peg_revision,
                         const revision&    revision,
                         depth              depth,
                         bool               ignore_externals,
                         bool               allow_unver_obstructions) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_url          = convert_string(url);
    auto raw_path         = convert_path(path, pool);
    auto raw_peg_revision = convert_revision(peg_revision);
    auto raw_revision     = convert_revision(revision);

    int32_t result_rev;

    check_result(svn_client_checkout3(reinterpret_cast<svn_revnum_t*>(&result_rev),
                                      raw_url,
                                      raw_path,
                                      raw_peg_revision,
                                      raw_revision,
                                      static_cast<svn_depth_t>(depth),
                                      ignore_externals,
                                      allow_unver_obstructions,
                                      _context,
                                      pool));

    return result_rev;
}

static commit_info* copy_commit_info(const svn_commit_info_t* raw) {
    if (raw == nullptr)
        return nullptr;

    auto result               = new commit_info();
    result->author            = raw->author;
    result->date              = raw->date;
    result->post_commit_error = raw->post_commit_err;
    result->repos_root        = raw->repos_root;
    result->revision          = static_cast<int32_t>(raw->revision);

    return result;
}

static svn_error_t* invoke_commit(const svn_commit_info_t* commit_info,
                                  void*                    raw_baton,
                                  apr_pool_t*              raw_pool) {
    auto callback_baton = static_cast<baton_wrapper<client::commit_callback>*>(raw_baton);
    callback_baton->value(copy_commit_info(commit_info));
    return nullptr;
}

void client::commit(const std::string&     path,
                    const std::string&     message,
                    const commit_callback& callback,
                    depth                  depth,
                    const string_vector&   changelists,
                    const string_map&      revprop_table,
                    bool                   keep_locks,
                    bool                   keep_changelists,
                    bool                   commit_as_operations,
                    bool                   include_file_externals,
                    bool                   include_dir_externals) const {
    commit(string_vector{path},
           message,
           callback,
           depth,
           changelists,
           revprop_table,
           keep_locks,
           keep_changelists,
           commit_as_operations,
           include_file_externals,
           include_dir_externals);
}

void client::commit(const string_vector&   paths,
                    const std::string&     message,
                    const commit_callback& callback,
                    depth                  depth,
                    const string_vector&   changelists,
                    const string_map&      revprop_table,
                    bool                   keep_locks,
                    bool                   keep_changelists,
                    bool                   commit_as_operations,
                    bool                   include_file_externals,
                    bool                   include_dir_externals) const {
    check_string(message);
    auto message_baton       = std::make_unique<baton_wrapper<std::string>>(message);
    _context->log_msg_baton3 = message_baton.get();

    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths       = convert_vector(paths, pool, false, true);
    auto raw_callback    = std::make_unique<baton_wrapper<commit_callback>>(callback);
    auto raw_changelists = convert_vector(changelists, pool, true, false);
    auto raw_props       = convert_map(revprop_table, _pool);

    check_result(svn_client_commit6(raw_paths,
                                    static_cast<svn_depth_t>(depth),
                                    keep_locks,
                                    keep_changelists,
                                    commit_as_operations,
                                    include_file_externals,
                                    include_dir_externals,
                                    raw_changelists,
                                    raw_props,
                                    invoke_commit,
                                    raw_callback.get(),
                                    _context,
                                    pool));
}

static lock* copy_lock(const svn_lock_t* raw) {
    if (raw == nullptr)
        return nullptr;

    auto result             = new lock();
    result->comment         = raw->comment;
    result->creation_date   = raw->creation_date;
    result->expiration_date = raw->expiration_date;
    result->is_dav_comment  = raw->is_dav_comment;
    result->owner           = raw->owner;
    result->path            = raw->path;
    result->token           = raw->token;

    return result;
}

static checksum* copy_checksum(const svn_checksum_t* raw) {
    if (raw == nullptr)
        return nullptr;

    auto result    = new checksum();
    result->digest = raw->digest;
    result->kind   = static_cast<checksum_kind>(raw->kind);

    return result;
}

static working_copy_info* copy_working_copy_info(const svn_wc_info_t* raw) {
    if (raw == nullptr)
        return nullptr;

    auto result                = new working_copy_info();
    result->changelist         = raw->changelist;
    result->node_checksum      = copy_checksum(raw->checksum);
    result->copyfrom_rev       = raw->copyfrom_rev;
    result->copyfrom_url       = raw->copyfrom_url;
    result->node_depth         = static_cast<depth>(raw->depth);
    result->moved_from_abspath = raw->moved_from_abspath;
    result->moved_to_abspath   = raw->moved_to_abspath;
    result->recorded_size      = raw->recorded_size;
    result->recorded_time      = raw->recorded_time;
    result->wcroot_abspath     = raw->wcroot_abspath;

    return result;
}

static info* copy_info(const svn_client_info2_t* raw) {
    if (raw == nullptr)
        return nullptr;

    auto result                 = new info();
    result->kind                = static_cast<node_kind>(raw->kind);
    result->last_changed_author = raw->last_changed_author;
    result->last_changed_date   = raw->last_changed_date;
    result->last_changed_rev    = raw->last_changed_rev;
    result->node_lock           = copy_lock(raw->lock);
    result->repos_root_URL      = raw->repos_root_URL;
    result->repos_UUID          = raw->repos_UUID;
    result->rev                 = raw->rev;
    result->size                = raw->size;
    result->URL                 = raw->URL;
    result->wc_info             = copy_working_copy_info(raw->wc_info);

    return result;
}

static svn_error_t* invoke_info(void*                     raw_baton,
                                const char*               path,
                                const svn_client_info2_t* raw_info,
                                apr_pool_t*               raw_scratch_pool) {
    auto callback_baton = static_cast<baton_wrapper<client::info_callback>*>(raw_baton);
    callback_baton->value(path, copy_info(raw_info));
    return nullptr;
}

void client::info(const std::string&   path,
                  const info_callback& callback,
                  const revision&      peg_revision,
                  const revision&      revision,
                  depth                depth,
                  bool                 fetch_excluded,
                  bool                 fetch_actual_only,
                  bool                 include_externals,
                  const string_vector& changelists) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_path         = convert_path(path, pool);
    auto raw_callback     = std::make_unique<baton_wrapper<info_callback>>(callback);
    auto raw_peg_revision = convert_revision(peg_revision);
    auto raw_revision     = convert_revision(revision);
    auto raw_changelists  = convert_vector(changelists, pool, true, false);

    check_result(svn_client_info4(raw_path,
                                  raw_peg_revision,
                                  raw_revision,
                                  static_cast<svn_depth_t>(depth),
                                  fetch_excluded,
                                  fetch_actual_only,
                                  include_externals,
                                  raw_changelists,
                                  invoke_info,
                                  raw_callback.get(),
                                  _context,
                                  pool));
}

void client::remove(const std::string&     path,
                    const remove_callback& callback,
                    bool                   force,
                    bool                   keep_local,
                    const string_map&      revprop_table) const {
    remove(string_vector{path},
           callback,
           force,
           keep_local,
           revprop_table);
}

void client::remove(const string_vector&   paths,
                    const remove_callback& callback,
                    bool                   force,
                    bool                   keep_local,
                    const string_map&      revprop_table) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths    = convert_vector(paths, pool, false, true);
    auto raw_callback = std::make_unique<baton_wrapper<remove_callback>>(callback);
    auto raw_props    = convert_map(revprop_table, _pool);

    check_result(svn_client_delete4(raw_paths,
                                    force,
                                    keep_local,
                                    raw_props,
                                    invoke_commit,
                                    raw_callback.get(),
                                    _context,
                                    pool));
}

void client::revert(const std::string&   path,
                    depth                depth,
                    const string_vector& changelists,
                    bool                 clear_changelists,
                    bool                 metadata_only) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths       = convert_vector(path, pool);
    auto raw_changelists = convert_vector(changelists, pool, true, false);

    check_result(svn_client_revert3(raw_paths,
                                    static_cast<svn_depth_t>(depth),
                                    raw_changelists,
                                    clear_changelists,
                                    metadata_only,
                                    _context,
                                    pool));
}

void client::revert(const string_vector& paths,
                    depth                depth,
                    const string_vector& changelists,
                    bool                 clear_changelists,
                    bool                 metadata_only) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths       = convert_vector(paths, pool, false, true);
    auto raw_changelists = convert_vector(changelists, pool, true, false);

    check_result(svn_client_revert3(raw_paths,
                                    static_cast<svn_depth_t>(depth),
                                    raw_changelists,
                                    clear_changelists,
                                    metadata_only,
                                    _context,
                                    pool));
}

static status* copy_status(const svn_client_status_t* raw) {
    if (raw == nullptr)
        return nullptr;

    auto result                = new status();
    result->changed_author     = raw->changed_author;
    result->changed_date       = raw->changed_date;
    result->changed_rev        = raw->changed_rev;
    result->changelist         = raw->changelist;
    result->conflicted         = raw->conflicted;
    result->copied             = raw->copied;
    result->node_depth         = static_cast<depth>(raw->depth);
    result->filesize           = raw->filesize;
    result->file_external      = raw->file_external;
    result->kind               = static_cast<node_kind>(raw->kind);
    result->local_abspath      = raw->local_abspath;
    result->local_lock         = copy_lock(raw->lock);
    result->moved_from_abspath = raw->moved_from_abspath;
    result->moved_to_abspath   = raw->moved_to_abspath;
    result->node_status        = static_cast<status_kind>(raw->node_status);
    result->ood_changed_author = raw->ood_changed_author;
    result->ood_changed_date   = raw->ood_changed_date;
    result->ood_changed_rev    = raw->ood_changed_rev;
    result->ood_kind           = static_cast<node_kind>(raw->ood_kind);
    result->prop_status        = static_cast<status_kind>(raw->prop_status);
    result->repos_lock         = copy_lock(raw->repos_lock);
    result->repos_node_status  = static_cast<status_kind>(raw->repos_node_status);
    result->repos_prop_status  = static_cast<status_kind>(raw->repos_prop_status);
    result->repos_relpath      = raw->repos_relpath;
    result->repos_root_url     = raw->repos_root_url;
    result->repos_text_status  = static_cast<status_kind>(raw->repos_text_status);
    result->repos_uuid         = raw->repos_uuid;
    result->revision           = raw->revision;
    result->switched           = raw->switched;
    result->text_status        = static_cast<status_kind>(raw->text_status);
    result->versioned          = raw->versioned;
    result->wc_is_locked       = raw->wc_is_locked;

    return result;
}

static svn_error_t* invoke_status(void*                      raw_baton,
                                  const char*                path,
                                  const svn_client_status_t* raw_status,
                                  apr_pool_t*                raw_scratch_pool) {
    auto callback_baton = static_cast<baton_wrapper<client::status_callback>*>(raw_baton);
    callback_baton->value(path, copy_status(raw_status));
    return nullptr;
}

int32_t client::status(const std::string&     path,
                       const status_callback& callback,
                       const revision&        revision,
                       depth                  depth,
                       bool                   get_all,
                       bool                   check_out_of_date,
                       bool                   check_working_copy,
                       bool                   no_ignore,
                       bool                   ignore_externals,
                       bool                   depth_as_sticky,
                       const string_vector&   changelists) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_path        = convert_path(path, pool);
    auto raw_callback    = std::make_unique<baton_wrapper<status_callback>>(callback);
    auto raw_revision    = convert_revision(revision);
    auto raw_changelists = convert_vector(changelists, pool, true, false);

    int32_t result_rev;

    check_result(svn_client_status6(reinterpret_cast<svn_revnum_t*>(&result_rev),
                                    _context,
                                    raw_path,
                                    raw_revision,
                                    static_cast<svn_depth_t>(depth),
                                    get_all,
                                    check_out_of_date,
                                    check_working_copy,
                                    no_ignore,
                                    ignore_externals,
                                    depth_as_sticky,
                                    raw_changelists,
                                    invoke_status,
                                    raw_callback.get(),
                                    pool));

    return result_rev;
}

int32_t client::update(const std::string& path,
                       const revision&    revision,
                       depth              depth,
                       bool               depth_is_sticky,
                       bool               ignore_externals,
                       bool               allow_unver_obstructions,
                       bool               adds_as_modification,
                       bool               make_parents) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths    = convert_vector(path, pool);
    auto raw_revision = convert_revision(revision);

    apr_array_header_t* raw_result_revs;

    check_result(svn_client_update4(&raw_result_revs,
                                    raw_paths,
                                    raw_revision,
                                    static_cast<svn_depth_t>(depth),
                                    depth_is_sticky,
                                    ignore_externals,
                                    allow_unver_obstructions,
                                    adds_as_modification,
                                    make_parents,
                                    _context,
                                    pool));

    return APR_ARRAY_IDX(raw_result_revs, 0, int32_t);
}

std::vector<int32_t> client::update(const string_vector& paths,
                                    const revision&      revision,
                                    depth                depth,
                                    bool                 depth_is_sticky,
                                    bool                 ignore_externals,
                                    bool                 allow_unver_obstructions,
                                    bool                 adds_as_modification,
                                    bool                 make_parents) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_paths    = convert_vector(paths, pool, false, true);
    auto raw_revision = convert_revision(revision);

    apr_array_header_t* raw_result_revs;

    check_result(svn_client_update4(&raw_result_revs,
                                    raw_paths,
                                    raw_revision,
                                    static_cast<svn_depth_t>(depth),
                                    depth_is_sticky,
                                    ignore_externals,
                                    allow_unver_obstructions,
                                    adds_as_modification,
                                    make_parents,
                                    _context,
                                    pool));

    auto result = std::vector<int32_t>(raw_result_revs->nelts);
    for (int i = 0; i < raw_result_revs->nelts; i++)
        result.push_back(APR_ARRAY_IDX(raw_result_revs, i, int32_t));
    return result;
}

std::string client::get_working_copy_root(const std::string& path) const {
    auto pool_ptr = create_pool(_pool);
    auto pool     = pool_ptr.get();

    auto raw_path = convert_path(path, pool);

    const char* raw_result;

    check_result(svn_client_get_wc_root(&raw_result, raw_path, _context, pool, pool));

    return std::string(raw_result);
}
} // namespace svn
