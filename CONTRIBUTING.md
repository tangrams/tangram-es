# Contributing to Tangram ES

The easiest way to help out is to submit bug reports and feature requests on our [issues](https://github.com/tangrams/tangram-es/issues) page.

When submitting a bug report, please include:

 - The device and operating system version that produced the bug
 - The version or commit of Tangram ES that produced the bug
 - Steps required to recreate the issue
 - What happened
 - What you expected to happen

Thanks!

## Building

If you'd like to contribute code to the project or just make changes for fun, you'll need to make sure your development environment is set up correctly.

 - [Developing for Mac OS X](platforms/osx/README.md#setup)
 - [Developing for Ubuntu Linux](platforms/linux/README.md#setup)
 - [Developing for Android](platforms/android/README.md#setup)
 - [Developing for iOS](platforms/ios/README.md#setup)
 - [Developing for Raspberry Pi](platforms/rpi/README.md#setup)

## Development

The tangram-es project is divided into two parts: a portable C++14 [core library](core) and the various [platform interfaces](platforms).

To develop for the core library, it is usually easiest to build and test your changes using either the Mac OS X or Ubuntu desktop targets. These targets are the fastest and easiest to deploy and debug.

## Code Style

In general, code changes should follow the style of the surrounding code.

When in doubt, you can use the provided clang-format style file for automatic styling.

Install clang-format (available through brew or apt-get):
```
brew install clang-format
```
or
```
sudo apt-get install clang-format
```

Run clang-format with specified style (use -i to modify the contents of the specified file):
```
clang-format -i -style=file [file]
```
