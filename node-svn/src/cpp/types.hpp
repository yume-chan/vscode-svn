#pragma once

#include <cstdint>

#include "svn_type_error.hpp"

namespace svn {
enum class depth {
    /* The order of these depths is important: the higher the number,
       the deeper it descends.  This allows us to compare two depths
       numerically to decide which should govern. */

    /** Depth undetermined or ignored.  In some contexts, this means the
        client should choose an appropriate default depth.  The server
        will generally treat it as #infinity. */
    unknown = -2,

    /** Exclude (i.e., don't descend into) directory D.
        @note In Subversion 1.5, exclude is *not* supported
        anywhere in the client-side (libsvn_wc/libsvn_client/etc) code;
        it is only supported as an argument to set_path functions in the
        ra and repos reporters.  (This will enable future versions of
        Subversion to run updates, etc, against 1.5 servers with proper
        exclude behavior, once we get a chance to implement
        client-side support for exclude.)
    */
    exclude = -1,

    /** Just the named directory D, no entries.  Updates will not pull in
        any files or subdirectories not already present. */
    empty = 0,

    /** D + its file children, but not subdirs.  Updates will pull in any
        files not already present, but not subdirectories. */
    files = 1,

    /** D + immediate children (D and its entries).  Updates will pull in
        any files or subdirectories not already present; those
        subdirectories' this_dir entries will have depth-empty. */
    immediates = 2,

    /** D + all descendants (full recursion from D).  Updates will pull
        in any files or subdirectories not already present; those
        subdirectories' this_dir entries will have depth-infinity.
        Equivalent to the pre-1.5 default update behavior. */
    infinity = 3
};

enum class node_kind {
    /** absent */
    none,

    /** regular file */
    file,

    /** directory */
    dir,

    /** something's here, but we don't know what */
    unknown,
};

enum class status_kind {
    /** does not exist */
    none = 1,

    /** is not a versioned thing in this wc */
    unversioned,

    /** exists, but uninteresting */
    normal,

    /** is scheduled for addition */
    added,

    /** under v.c., but is missing */
    missing,

    /** scheduled for deletion */
    deleted,

    /** was deleted and then re-added */
    replaced,

    /** text or props have been modified */
    modified,

    /** local mods received repos mods (### unused) */
    merged,

    /** local mods received conflicting repos mods */
    conflicted,

    /** is unversioned but configured to be ignored */
    ignored,

    /** an unversioned resource is in the way of the versioned resource */
    obstructed,

    /** an unversioned directory path populated by an svn:externals
    property; this status is not used for file externals */
    external,

    /** a directory doesn't contain a complete entries list */
    incomplete
};

/**
 * A lock object, for client & server to share.
 *
 * A lock represents the exclusive right to add, delete, or modify a
 * path.  A lock is created in a repository, wholly controlled by the
 * repository.  A "lock-token" is the lock's UUID, and can be used to
 * learn more about a lock's fields, and or/make use of the lock.
 * Because a lock is immutable, a client is free to not only cache the
 * lock-token, but the lock's fields too, for convenience.
 *
 * Note that the 'is_dav_comment' field is wholly ignored by every
 * library except for mod_dav_svn.  The field isn't even marshalled
 * over the network to the client.  Assuming lock structures are
 * created with apr_pcalloc(), a default value of 0 is universally safe.
 *
 * @note in the current implementation, only files are lockable.
 *
 * @since New in 1.2.
 */
struct lock {
    /** the path this lock applies to */
    const char* path;

    /** unique URI representing lock */
    const char* token;

    /** the username which owns the lock */
    const char* owner;

    /** (optional) description of lock  */
    const char* comment;

    /** was comment made by generic DAV client? */
    bool is_dav_comment;

    /** when lock was made */
    int64_t creation_date;

    /** (optional) when lock will expire;
        If value is 0, lock will never expire. */
    int64_t expiration_date;
};

/**
  * Structure for holding the "status" of a working copy item.
  *
  * @note Fields may be added to the end of this structure in future
  * versions.  Therefore, to preserve binary compatibility, users
  * should not directly allocate structures of this type.
  *
  * @since New in 1.7.
  */
struct status {
    /** The kind of node as recorded in the working copy */
    node_kind kind;

    /** The absolute path to the node */
    const char* local_abspath;

    /** The actual size of the working file on disk, or SVN_INVALID_FILESIZE
      * if unknown (or if the item isn't a file at all). */
    int64_t filesize;

    /** If the path is under version control, versioned is TRUE, otherwise
      * FALSE. */
    bool versioned;

    /** Set to TRUE if the node is the victim of some kind of conflict. */
    bool conflicted;

    /** The status of the node, based on the restructuring changes and if the
      * node has no restructuring changes the text and prop status. */
    status_kind node_status;

    /** The status of the text of the node, not including restructuring changes.
      * Valid values are: none, normal,
      * modified and conflicted. */
    status_kind text_status;

    /** The status of the node's properties.
      * Valid values are: none, normal,
      * modified and conflicted. */
    status_kind prop_status;

    /** A node can be 'locked' if a working copy update is in progress or
      * was interrupted. */
    bool wc_is_locked;

    /** A file or directory can be 'copied' if it's scheduled for
      * addition-with-history (or part of a subtree that is scheduled as such.).
      */
    bool copied;

    /** The URL of the repository root. */
    const char* repos_root_url;

    /** The UUID of the repository */
    const char* repos_uuid;

    /** The in-repository path relative to the repository root. */
    const char* repos_relpath;

    /** Base revision. */
    int32_t revision;

    /** Last revision this was changed */
    int32_t changed_rev;

    /** Date of last commit. */
    int64_t changed_date;

    /** Last commit author of this item */
    const char* changed_author;

    /** A file or directory can be 'switched' if the switch command has been
      * used.  If this is TRUE, then file_external will be FALSE.
      */
    bool switched;

    /** If the item is a file that was added to the working copy with an
      * svn:externals; if file_external is TRUE, then switched is always
      * FALSE.
      */
    bool file_external;

    /** The locally present lock. (Values of path, token, owner, comment and
      * are available if a lock is present) */
    const lock* local_lock;

    /** Which changelist this item is part of, or NULL if not part of any. */
    const char* changelist;

    /** The depth of the node as recorded in the working copy
      * (#svn_depth_unknown for files or when no depth is recorded) */
    depth node_depth;

    /**
      * @defgroup ood WC out-of-date info from the repository
      * @{
      *
      * When the working copy item is out-of-date compared to the
      * repository, the following fields represent the state of the
      * youngest revision of the item in the repository.  If the working
      * copy is not out of date, the fields are initialized as described
      * below.
      */

    /** Set to the node kind of the youngest commit, or #svn_node_none
      * if not out of date. */
    node_kind ood_kind;

    /** The status of the node, based on the text status if the node has no
      * restructuring changes */
    status_kind repos_node_status;

    /** The node's text status in the repository. */
    status_kind repos_text_status;

    /** The node's property status in the repository. */
    status_kind repos_prop_status;

    /** The node's lock in the repository, if any. */
    const lock* repos_lock;

    /** Set to the youngest committed revision, or #SVN_INVALID_REVNUM
      * if not out of date. */
    int32_t ood_changed_rev;

    /** Set to the most recent commit date, or @c 0 if not out of date. */
    int64_t ood_changed_date;

    /** Set to the user name of the youngest commit, or @c NULL if not
      * out of date or non-existent.  Because a non-existent @c
      * svn:author property has the same behavior as an out-of-date
      * working copy, examine @c ood_changed_rev to determine whether
      * the working copy is out of date. */
    const char* ood_changed_author;

    /** Set to the local absolute path that this node was moved from, if this
      * file or directory has been moved here locally and is the root of that
      * move. Otherwise set to NULL.
      *
      * This will be NULL for moved-here nodes that are just part of a subtree
      * that was moved along (and are not themselves a root of a different move
      * operation).
      *
      * @since New in 1.8. */
    const char* moved_from_abspath;

    /** Set to the local absolute path that this node was moved to, if this file
      * or directory has been moved away locally and corresponds to the root
      * of the destination side of the move. Otherwise set to NULL.
      *
      * Note: Saying just "root" here could be misleading. For example:
      *   svn mv A AA;
      *   svn mv AA/B BB;
      * creates a situation where A/B is moved-to BB, but one could argue that
      * the move source's root actually was AA/B. Note that, as far as the
      * working copy is concerned, above case is exactly identical to:
      *   svn mv A/B BB;
      *   svn mv A AA;
      * In both situations, @a moved_to_abspath would be set for nodes A (moved
      * to AA) and A/B (moved to BB), only.
      *
      * This will be NULL for moved-away nodes that were just part of a subtree
      * that was moved along (and are not themselves a root of a different move
      * operation).
      *
      * @since New in 1.8. */
    const char* moved_to_abspath;
};

/**
  * Various types of checksums.
  *
  * @since New in 1.6.
  */
enum class checksum_kind {
    /** The checksum is (or should be set to) an MD5 checksum. */
    md5,

    /** The checksum is (or should be set to) a SHA1 checksum. */
    sha1,

    /** The checksum is (or should be set to) a FNV-1a 32 bit checksum,
    * in big endian byte order.
    * @since New in 1.9. */
    fnv1a_32,

    /** The checksum is (or should be set to) a modified FNV-1a 32 bit,
    * in big endian byte order.
    * @since New in 1.9. */
    fnv1a_32x4
};

/**
* A generic checksum representation.
*
* @since New in 1.6.
*/
struct checksum {
    /** The bytes of the checksum. */
    const unsigned char* digest;

    /** The type of the checksum.  This should never be changed by consumers
    of the APIs. */
    checksum_kind kind;
};

/**
 * This struct contains information about a working copy node.
 *
 * @note Fields may be added to the end of this structure in future
 * versions.  Therefore, users shouldn't allocate structures of this
 * type, to preserve binary compatibility.
 *
 * @since New in 1.7.
 */
struct working_copy_info {
    /** If copied, the URL from which the copy was made, else @c NULL. */
    const char* copyfrom_url;

    /** If copied, the revision from which the copy was made,
      * else #SVN_INVALID_REVNUM. */
    int32_t copyfrom_rev;

    /** The checksum of the node, if it is a file. */
    const checksum* node_checksum;

    /** A changelist the item is in, @c NULL if this node is not in a
      * changelist. */
    const char* changelist;

    /** The depth of the item, see #svn_depth_t */
    depth node_depth;

    /**
      * The size of the file after being translated into its local
      * representation, or #SVN_INVALID_FILESIZE if unknown.
      * Not applicable for directories.
      */
    int64_t recorded_size;

    /**
      * The time at which the file had the recorded size recorded_size and was
      * considered unmodified. */
    int64_t recorded_time;

    ///** Array of const svn_wc_conflict_description2_t * which contains info
    //  * on any conflict of which this node is a victim. Otherwise NULL.  */
    //const apr_array_header_t* conflicts;

    /** The local absolute path of the working copy root.  */
    const char* wcroot_abspath;

    /** The path the node was moved from, if it was moved here. Else NULL.
      * @since New in 1.8. */
    const char* moved_from_abspath;

    /** The path the node was moved to, if it was moved away. Else NULL.
      * @since New in 1.8. */
    const char* moved_to_abspath;
};

struct info {
    /** Where the item lives in the repository. */
    const char* URL;

    /** The revision of the object.  If the target is a working-copy
    * path, then this is its current working revnum.  If the target
    * is a URL, then this is the repos revision that it lives in. */
    int32_t rev;

    /** The root URL of the repository. */
    const char* repos_root_URL;

    /** The repository's UUID. */
    const char* repos_UUID;

    /** The node's kind. */
    node_kind kind;

    /** The size of the file in the repository (untranslated,
    * e.g. without adjustment of line endings and keyword
    * expansion). Only applicable for file -- not directory -- URLs.
    * For working copy paths, @a size will be #SVN_INVALID_FILESIZE. */
    int64_t size;

    /** The last revision in which this object changed. */
    int32_t last_changed_rev;

    /** The date of the last_changed_rev. */
    int64_t last_changed_date;

    /** The author of the last_changed_rev. */
    const char* last_changed_author;

    /** An exclusive lock, if present.  Could be either local or remote. */
    const lock* node_lock;

    /** Possible information about the working copy, NULL if not valid. */
    const working_copy_info* wc_info;
};

/**
* Various ways of specifying revisions.
*
* @note
* In contexts where local mods are relevant, the `working' kind
* refers to the uncommitted "working" revision, which may be modified
* with respect to its base revision.  In other contexts, `working'
* should behave the same as `committed' or `current'.
*/
enum class revision_kind {
    /** No revision information given. */
    unspecified,

    /** revision given as number */
    number,

    /** revision given as date */
    date,

    /** rev of most recent change */
    committed,

    /** (rev of most recent change) - 1 */
    previous,

    /** .svn/entries current revision */
    base,

    /** current, plus local mods */
    working,

    /** repository youngest */
    head
};

struct revision {
    revision(revision_kind kind) {
        switch (kind) {
            case revision_kind::unspecified:
            case revision_kind::committed:
            case revision_kind::previous:
            case revision_kind::base:
            case revision_kind::working:
            case revision_kind::head:
                this->kind = kind;
                break;
            default:
                throw svn_type_error("");
        }
    }

    revision(int32_t number)
        : kind(revision_kind::number)
        , number(number) {}

    revision(int64_t date)
        : kind(revision_kind::date)
        , date(date) {}

    revision_kind kind;

    union {
        /** The revision number */
        int32_t number;

        /** the date of the revision */
        int64_t date;
    };
};

struct commit_info {
    /** just-committed revision. */
    int32_t revision;

    /** server-side date of the commit. */
    const char* date;

    /** author of the commit. */
    const char* author;

    /** error message from post-commit hook, or NULL. */
    const char* post_commit_error;

    /** repository root, may be @c NULL if unknown.
    @since New in 1.7. */
    const char* repos_root;
};
} // namespace svn
