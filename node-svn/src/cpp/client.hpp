#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <cpp/types.hpp>

struct apr_pool_t;
struct svn_client_ctx_t;

namespace svn {
using string_vector = std::vector<std::string>;
using string_map    = std::unordered_map<std::string, std::string>;

struct cat_result {
    std::vector<char> content;
    string_map        properties;
};

class client : public std::enable_shared_from_this<client> {
  public:
    using get_changelists_callback = std::function<void(const char*, const char*)>;
    using cat_callback             = std::function<void(const char*, size_t)>;
    using commit_callback          = std::function<void(const commit_info*)>;
    using info_callback            = std::function<void(const char*, const info*)>;
    using remove_callback          = std::function<void(const commit_info*)>;
    using status_callback          = std::function<void(const char*, const status*)>;

    explicit client();
    client(client&&);
    client(const client&) = delete;

    client& operator=(client&&);
    client& operator=(const client&) = delete;

    ~client();

    void add_to_changelist(const std::string&   path,
                           const std::string&   changelist,
                           depth                depth       = depth::infinity,
                           const string_vector& changelists = string_vector()) const;
    void add_to_changelist(const string_vector& paths,
                           const std::string&   changelist,
                           depth                depth       = depth::infinity,
                           const string_vector& changelists = string_vector()) const;

    void get_changelists(const std::string&              path,
                         const get_changelists_callback& callback,
                         depth                           depth       = depth::infinity,
                         const string_vector&            changelists = string_vector()) const;

    void remove_from_changelists(const std::string&   path,
                                 depth                depth       = depth::infinity,
                                 const string_vector& changelists = string_vector()) const;
    void remove_from_changelists(const string_vector& paths,
                                 depth                depth       = depth::infinity,
                                 const string_vector& changelists = string_vector()) const;

    void add(const std::string& path,
             depth              depth        = depth::infinity,
             bool               force        = true,
             bool               no_ignore    = false,
             bool               no_autoprops = false,
             bool               add_parents  = true) const;

    string_map cat(const std::string&  path,
                   const cat_callback& callback,
                   const revision&     peg_revision    = revision(revision_kind::working),
                   const revision&     revision        = revision(revision_kind::working),
                   bool                expand_keywords = true) const;
    cat_result cat(const std::string& path,
                   const revision&    peg_revision    = revision(revision_kind::working),
                   const revision&    revision        = revision(revision_kind::working),
                   bool               expand_keywords = true) const;

    int32_t checkout(const std::string& url,
                     const std::string& path,
                     const revision&    peg_revision             = revision(revision_kind::working),
                     const revision&    revision                 = revision(revision_kind::working),
                     depth              depth                    = depth::infinity,
                     bool               ignore_externals         = false,
                     bool               allow_unver_obstructions = false) const;

    void commit(const std::string&     path,
                const std::string&     message,
                const commit_callback& callback,
                depth                  depth                  = depth::infinity,
                const string_vector&   changelists            = string_vector(),
                const string_map&      revprop_table          = string_map(),
                bool                   keep_locks             = true,
                bool                   keep_changelists       = false,
                bool                   commit_as_operations   = false,
                bool                   include_file_externals = true,
                bool                   include_dir_externals  = true) const;
    void commit(const string_vector&   paths,
                const std::string&     message,
                const commit_callback& callback,
                depth                  depth                  = depth::infinity,
                const string_vector&   changelists            = string_vector(),
                const string_map&      revprop_table          = string_map(),
                bool                   keep_locks             = true,
                bool                   keep_changelists       = false,
                bool                   commit_as_operations   = false,
                bool                   include_file_externals = true,
                bool                   include_dir_externals  = true) const;

    void info(const std::string&   path,
              const info_callback& callback,
              const revision&      peg_revision      = revision(revision_kind::working),
              const revision&      revision          = revision(revision_kind::working),
              depth                depth             = depth::empty,
              bool                 fetch_excluded    = true,
              bool                 fetch_actual_only = true,
              bool                 include_externals = false,
              const string_vector& changelists       = string_vector()) const;

    void remove(const std::string&     path,
                const remove_callback& callback,
                bool                   force         = true,
                bool                   keep_local    = false,
                const string_map&      revprop_table = string_map()) const;
    void remove(const string_vector&   paths,
                const remove_callback& callback,
                bool                   force         = true,
                bool                   keep_local    = false,
                const string_map&      revprop_table = string_map()) const;

    void revert(const std::string&   path,
                depth                depth             = depth::infinity,
                const string_vector& changelists       = string_vector(),
                bool                 clear_changelists = true,
                bool                 metadata_only     = true) const;
    void revert(const string_vector& paths,
                depth                depth             = depth::infinity,
                const string_vector& changelists       = string_vector(),
                bool                 clear_changelists = true,
                bool                 metadata_only     = true) const;

    int32_t status(const std::string&     path,
                   const status_callback& callback,
                   const revision&        revision           = revision(revision_kind::working),
                   depth                  depth              = depth::infinity,
                   bool                   get_all            = false,
                   bool                   check_out_of_date  = false,
                   bool                   check_working_copy = true,
                   bool                   no_ignore          = false,
                   bool                   ignore_externals   = false,
                   bool                   depth_as_sticky    = false,
                   const string_vector&   changelists        = string_vector()) const;

    int32_t              update(const std::string& path,
                                const revision&    revision                 = revision(revision_kind::head),
                                depth              depth                    = depth::infinity,
                                bool               depth_is_sticky          = false,
                                bool               ignore_externals         = false,
                                bool               allow_unver_obstructions = false,
                                bool               adds_as_modification     = false,
                                bool               make_parents             = true) const;
    std::vector<int32_t> update(const string_vector& paths,
                                const revision&      revision                 = revision(revision_kind::head),
                                depth                depth                    = depth::infinity,
                                bool                 depth_is_sticky          = false,
                                bool                 ignore_externals         = false,
                                bool                 allow_unver_obstructions = false,
                                bool                 adds_as_modification     = false,
                                bool                 make_parents             = true) const;

    std::string get_working_copy_root(const std::string& path) const;

  private:
    apr_pool_t*       _pool;
    svn_client_ctx_t* _context;
};
} // namespace svn
