# Change Log

## [unrealsed]

## 0.0.11 - 2018-4-4

### Added
- Added a new `svn.cleanup` command.

### Fixed
- Fixed a issue that there is no syntax highlight for the old content in diff view.
- Fixed a issue that files that untouched but belongs to some changelists will be shown in `Changes` group.

## 0.0.10 - 2018-2-28

### Added
- Added notice for non-Windows-x64 environments.
- Added support for comparing image files.
- Added support for simple authentication.

### Changed
- Changed to only clear Source Control input box only if commit success.

### Fixed
- Fixed a issue that clicking on a newly added item would comparing it with empty instead of opening it.
- Fixed a issue that clicking on a missing or deleted file would throw an error.

## 0.0.9 - 2018-1-1

### Added
- Added a new `svn.checkout` command.

### Changed
- Changed to use newer version of node-svn, which build all dependencies directly from source code.

### Fixed
- Fixed a issue that `svn.commit` command can randomly fail or appends junk characters to commit message.
- Fixed a issue that `svn.update` command can return an array instead of a number when the parameter is a single url.

## 0.0.8 - 2017-12-12

### Added
- Added progress for `svn.refresh` command.
- Added a new `svn.enabled` setting to disable this extension (globally or workspace-wise).

### Changed
- Changed `svn.show_changes_from` setting to a workspace setting.

### Removed
- Removed internationalization to fix extension page temporarily.

### Fixed
- Fixed a issue that there are two duplicate `svn.update` commands.

## 0.0.7 - 2017-11-29

### Added

- Added a new `svn.show_changes_from` setting to control which changes are included in the Source Control viewlet.
- Added the ability to use credentials in the global auth store for `svn.update` and `svn.commit` commands.

### Fixed
- Fixed a issue that the Source Control viewlet can include unchanged externals.
