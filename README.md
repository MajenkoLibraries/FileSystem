FileSystem
==========

Advanced filesystem storage and interface classes for chipKIT and
other PIC32 boards.

# WORK IN PROGRESS

# NOT READY FOR USE

# LOTS AND LOTS STILL TO DO TO MAKE IT EVEN BEGIN TO WORK

This library will provide a fully expandable filesystem interface
for the chipKIT and other PIC32 boards.  It breaks the system into
three logical blocks:

* Block Devices
* Filesystems
* Files

Block devices are responsible for interacting with the physical
devices at a block level. They abstract the device into a logical
set of 512 byte blocks which are read and written as atomic items.
Caching can also be implemented at this level.

Filesystems translate the blocks into actual files and directories.
Initially only the FAT filesystem is supported, but there are plans
for future filesystems.

Files are the actual contents of the filesystem.  As well as being
a Stream class for familiar Arduino-like access, they also emulate
to a large extent the workings and interface of the Java *File* class.

All access to the content of the filesystem is through File objects.
