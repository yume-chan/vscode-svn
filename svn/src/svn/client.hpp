#ifndef SVN_CPP_CLIENT_H
#define SVN_CPP_CLIENT_H

#include <functional>
#include <memory>
#include <vector>

#include <svn_opt.h>
#include <svn_types.h>

#include <apr/apr.hpp>

struct svn_auth_baton_t;
struct svn_client_ctx_t;
struct svn_client_status_t;
struct apr_hash_t;
struct svn_wc_notify_t;
struct svn_commit_info_t;
struct svn_client_info2_t;

namespace svn {
using get_changelists_callback = std::function<void(const char*, const char*, const apr::pool&)>;

using cat_callback = std::function<void(const char*, size_t)>;

using commit_callback = std::function<void(const svn_commit_info_t*, const apr::pool&)>;

using info_callback = std::function<void(const char*, const svn_client_info2_t*, const apr::pool&)>;

using remove_callback = std::function<void(const svn_commit_info_t*, const apr::pool&)>;

using status_callback = std::function<void(const char*, const svn_client_status_t*, const apr::pool&)>;

class client : public std::enable_shared_from_this<client> {
  public:
    explicit client();

    void add_to_changelist(const std::string&              path,
                           const std::string&              changelist,
                           svn_depth_t                     depth       = svn_depth_infinity,
                           const std::vector<std::string>& changelists = std::vector<std::string>()) const;
    void add_to_changelist(const std::vector<std::string>& paths,
                           const std::string&              changelist,
                           svn_depth_t                     depth       = svn_depth_infinity,
                           const std::vector<std::string>& changelists = std::vector<std::string>()) const;

    void get_changelists(const std::string&              path,
                         get_changelists_callback&       callback,
                         const std::vector<std::string>& changelists = std::vector<std::string>(),
                         svn_depth_t                     depth       = svn_depth_infinity) const;

    void remove_from_changelists(const std::string&              path,
                                 svn_depth_t                     depth       = svn_depth_infinity,
                                 const std::vector<std::string>& changelists = std::vector<std::string>()) const;
    void remove_from_changelists(const std::vector<std::string>& paths,
                                 svn_depth_t                     depth       = svn_depth_infinity,
                                 const std::vector<std::string>& changelists = std::vector<std::string>()) const;

    void add(const std::string& path,
             svn_depth_t        depth        = svn_depth_infinity,
             bool               force        = true,
             bool               no_ignore    = false,
             bool               no_autoprops = false,
             bool               add_parents  = true) const;

    void cat(const std::string&        path,
             apr_hash_t**              props,
             cat_callback              callback,
             const svn_opt_revision_t& peg_revision    = svn_opt_revision_t{svn_opt_revision_working},
             const svn_opt_revision_t& revision        = svn_opt_revision_t{svn_opt_revision_working},
             bool                      expand_keywords = true) const;

    svn_revnum_t checkout(const std::string&        url,
                          const std::string&        path,
                          const svn_opt_revision_t& peg_revision             = svn_opt_revision_t{svn_opt_revision_working},
                          const svn_opt_revision_t& revision                 = svn_opt_revision_t{svn_opt_revision_working},
                          svn_depth_t               depth                    = svn_depth_infinity,
                          bool                      ignore_externals         = false,
                          bool                      allow_unver_obstructions = false) const;

    void commit(const std::string&              path,
                const std::string&              message,
                commit_callback&                callback,
                svn_depth_t                     depth                  = svn_depth_infinity,
                const std::vector<std::string>& changelists            = std::vector<std::string>(),
                apr_hash_t*                     revprop_table          = nullptr,
                bool                            keep_locks             = true,
                bool                            keep_changelists       = false,
                bool                            commit_as_aperations   = false,
                bool                            include_file_externals = true,
                bool                            include_dir_externals  = true) const;
    void commit(const std::vector<std::string>& paths,
                const std::string&              message,
                commit_callback&                callback,
                svn_depth_t                     depth                  = svn_depth_infinity,
                const std::vector<std::string>& changelists            = std::vector<std::string>(),
                apr_hash_t*                     revprop_table          = nullptr,
                bool                            keep_locks             = true,
                bool                            keep_changelists       = false,
                bool                            commit_as_aperations   = false,
                bool                            include_file_externals = true,
                bool                            include_dir_externals  = true) const;

    void info(const std::string&              path,
              info_callback&                  callback,
              const svn_opt_revision_t&       peg_revision      = svn_opt_revision_t{svn_opt_revision_working},
              const svn_opt_revision_t&       revision          = svn_opt_revision_t{svn_opt_revision_working},
              svn_depth_t                     depth             = svn_depth_infinity,
              bool                            fetch_excluded    = true,
              bool                            fetch_actual_only = true,
              bool                            include_externals = true,
              const std::vector<std::string>& changelists       = std::vector<std::string>()) const;

    void remove(const std::string& path,
                remove_callback&   callback,
                bool               force         = true,
                bool               keep_local    = false,
                apr_hash_t*        revprop_table = nullptr) const;
    void remove(const std::vector<std::string>& paths,
                remove_callback&                callback,
                bool                            force         = true,
                bool                            keep_local    = false,
                apr_hash_t*                     revprop_table = nullptr) const;

    void revert(const std::string&              path,
                svn_depth_t                     depth             = svn_depth_infinity,
                const std::vector<std::string>& changelists       = std::vector<std::string>(),
                bool                            clear_changelists = true,
                bool                            metadata_only     = true) const;
    void revert(const std::vector<std::string>& paths,
                svn_depth_t                     depth             = svn_depth_infinity,
                const std::vector<std::string>& changelists       = std::vector<std::string>(),
                bool                            clear_changelists = true,
                bool                            metadata_only     = true) const;

    svn_revnum_t status(const std::string&              path,
                        status_callback&                callback,
                        const svn_opt_revision_t&       revision           = svn_opt_revision_t{svn_opt_revision_working},
                        svn_depth_t                     depth              = svn_depth_infinity,
                        bool                            get_all            = false,
                        bool                            check_out_of_date  = false,
                        bool                            check_working_copy = true,
                        bool                            no_ignore          = false,
                        bool                            ignore_externals   = true,
                        bool                            depth_as_sticky    = false,
                        const std::vector<std::string>& changelists        = std::vector<std::string>()) const;

    std::vector<svn_revnum_t> update(const std::string&        path,
                                     const svn_opt_revision_t& revision                 = svn_opt_revision_t{svn_opt_revision_working},
                                     svn_depth_t               depth                    = svn_depth_infinity,
                                     bool                      depth_is_sticky          = false,
                                     bool                      ignore_externals         = false,
                                     bool                      allow_unver_obstructions = false,
                                     bool                      adds_as_modification     = false,
                                     bool                      make_parents             = true) const;
    std::vector<svn_revnum_t> update(const std::vector<std::string>& paths,
                                     const svn_opt_revision_t&       revision                 = svn_opt_revision_t{svn_opt_revision_working},
                                     svn_depth_t                     depth                    = svn_depth_infinity,
                                     bool                            depth_is_sticky          = false,
                                     bool                            ignore_externals         = false,
                                     bool                            allow_unver_obstructions = false,
                                     bool                            adds_as_modification     = false,
                                     bool                            make_parents             = true) const;

    void auth_baton(svn_auth_baton_t* baton);

    void set_notify(const std::function<void(svn_wc_notify_t*)>&) const;
    void clear_notify() const;

  private:
    const apr::pool   _pool;
    svn_client_ctx_t* _value;
};
} // namespace node_svn

#endif
