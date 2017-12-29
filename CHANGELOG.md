# Change Log

## [Unreleased]

### Added
- New `svn.checkout` command.

### Changed
- Use build-svn branch of node-svn, which build all subversion dependencies and components from source code.

### Fixed
- `svn.commit` randomly fail or append junk characters to commit message.

## 0.0.8 - 2017-12-12

### Added
- Progress for `svn.refresh` command.
- New `svn.enabled` setting to disable it (globally or workspace-wise).

### Changed
- `svn.show_changes_from` is a workspace setting now.

### Removed
- Temporarily removed internationalization to fix extension page.

### Fixed
- Removed duplicate `svn.update` command.

## 0.0.7 - 2017-11-29
### Added

- New `svn.show_changes_from` setting to determine which changes are included in Source Control panel.
- `svn.update` and `svn.commit` with credentials in the global auth store.

### Fixed
- Don't include unchanged externals in Source Control panel.
