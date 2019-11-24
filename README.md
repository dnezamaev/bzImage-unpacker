# bzImage-unpacker
Unpacks bzImage file to get uncompressed vmlinux kernel.

## What is it?
This is small tool to unpack bzImage file. In fact it does same thing as [this script](https://github.com/torvalds/linux/blob/master/scripts/extract-vmlinux) (and script even does more!). It may be useful if you want to run it inside your c/c++/c# code or unpack the file on Windows.

## How to use it?

### Linux 
Make sure zlib package installed (it is on many popular distros by default), otherwise it will not work.

In terminal or command line enter following.

`./unpack-bzimage <path_to_file>`

### Windows
Make sure .Net Framework 2.0 and newer installed.

In command line enter following.

`bzImageUnpacker.exe <path_to_file>`

## How it works?
To understand it please read about bzImage file format.
Here is [wiki page](https://en.wikipedia.org/wiki/Vmlinux#bzImage).
In few words it contains the gzipped vmlinux file inside, 
which starts with GZIP magic bytes 1F 8B and compression method 08.
This tool finds such signature and tries to unpack compressed content 
until success or writes it fails.

## How to build?
On Linux prerequirments are:

* zlib
* gcc or g++
* make

When you done just make it =)

On Windows there is project for MSVS 2015, but you can build it on almost any version (or even without it, there just two .cs files). It should be fine for .Net Framework 2.0 and newer.
