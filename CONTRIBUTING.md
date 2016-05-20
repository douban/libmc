# How to Contribute

Thanks for using libmc! Please feel free to file an issue
if you have any question or meet any trouble. Pull Requests are
more than welcomed.

## Before you open a Pull Request

### Get some hints on how to build
Before you get started, you may need know some basic steps
on how to
[build the project](https://github.com/douban/libmc/wiki/Build-and-installation#for-libmc-developer) first.


### Change the version definitions before you commit (optional)
To get the exact version of the library you're running, it's
advised (but not mandatory) to change the **version** constants
defined in libmc everytime when you submit a commit.
There's an easy way to enable this behaviour:

    ln -sf ../../misc/update_version.sh .git/hooks/pre-commit

You need to run this command once before you first commit.
