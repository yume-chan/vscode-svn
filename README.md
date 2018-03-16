# vscode-svn

[![Visual Studio Marketplace](https://img.shields.io/vscode-marketplace/v/simonchan.vscode-svn.svg)](https://marketplace.visualstudio.com/items?itemName=simonchan.vscode-svn)
[![Build status](https://ci.appveyor.com/api/projects/status/2i0hcx8jhr74d7t5/branch/master?svg=true)](https://ci.appveyor.com/project/yume-chan/vscode-svn/branch/master)
[![GitHub issues](https://img.shields.io/github/issues/yume-chan/vscode-svn.svg)](https://github.com/yume-chan/vscode-svn/issues)
[![Greenkeeper badge](https://badges.greenkeeper.io/yume-chan/vscode-svn.svg)](https://greenkeeper.io/)

**Work In Progress**

An experimental extension to provide Subversion (SVN) support for Visual Studio Code.

- [vscode-svn](#vscode-svn)
    - [node-svn](#node-svn)
    - [Platform](#platform)
    - [Install from `vsix` package](#install-from-vsix-package)
    - [Why not the Subversion executable](#why-not-the-subversion-executable)
    - [Building](#building)
    - [Issues](#issues)
    - [Contribution](#contribution)

## node-svn

This project uses on my [node-svn](https://github.com/yume-chan/node-svn) project, which is a Node Native Addon that wraps the Subversion library.

## Platform

The published package only supports Windows x64.

For Windows x86 package, you can use [this](https://ci.appveyor.com/api/buildjobs/09qln5lubggj3w9r/artifacts/vsix%2Fvscode-svn-0.0.9-ia32.vsix) package.

I have problem compiling node-svn on Linux, macOS. Please check [node-svn issues](https://github.com/yume-chan/node-svn/issues) if you want to help me.

## Install from `vsix` package

1. Uninstall your current version. (You may see an error says permission denied, don't worry, just restart your Visual Studio Code)
1. Switch to Extensions tab.
1. Click on `...`, select `Install from VSIX...` menu item.
1. Pick your downloaded `vsix` package.

## Why not the Subversion executable

like the Git extension?

1. svn.exe doesn't support Unicode on Windows, it cannot take any Unicode arguments or output any Unicode characters to Node.js.
1. Subversion doesn't have a "stage" list, so if you want to commit only some of your changes, you need either:
    1. Add there files to a changelist. While each file can only be in one changelist, this way doesn't work if you already uses changelists to manage your files.
    1. Pass a file list to the command, this is blocked by problem 1.

## Building

```` shell
git clone https://github.com/yume-chan/vscode-svn.git
cd vscode-svn

npm install
````

See [node-svn README](https://github.com/yume-chan/node-svn#readme) for more information.

## Issues

This project is currently at very early stage, so there will be more bugs than features. Please report them [here](https://github.com/yume-chan/vscode-data) and waiting for them to vanish.

## Contribution

Currently there is only one person (me) working on this project (part-time). If you interest in, please help!
