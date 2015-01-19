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
Caching is also implemented at this level.

Filesystems translate the blocks into actual files and directories.
Initially only the FAT filesystem is supported, but there are plans
for future filesystems.

Files are the actual contents of the filesystem.  As well as being
a Stream class for familiar Arduino-like access, they also emulate
to a large extent the workings and interface of the Java *File* class.

All access to the content of the filesystem is through File objects.

Advanced caching means high throughput.  Initial tests of reading of
large amounts of data have been most impressive.  Reading of a 100MB
file in 10KB chunks, and in 100 byte chunks, is much much faster
than the traditional SD library:

    10240 byte block reads

    File size: 104857600
    Read time: 113399ms
    Read speed: 924678.35 Bps

    Cache hits: 3
    Cache misses: 204820
    Cache percent: 0%

    Cache data:
    ID     Block  Flags  Count  Time
     2       650  01         2  113413.0
     1       137  01         1  113694.0
     0    712969  01         0  12.0
     3    712963  01         0  16.0
     4    712964  01         0  16.0
     5    712967  01         0  14.0
     6    712962  01         0  17.0
     7    712965  01         0  15.0
     8    712966  01         0  14.0
     9    712968  01         0  13.0

    100 byte block reads

    File size: 104857600
    Read time: 125620ms
    Read speed: 834720.59 Bps

    Cache hits: 1040387
    Cache misses: 204820
    Cache percent: 83%

    Cache data:
    ID     Block  Flags  Count  Time
     0    712928  01         6  37.0
     1    712878  01         6  64.0
     2    712911  01         6  46.0
     3    712961  01         6  18.0
     4    712886  01         6  60.0
     5    712903  01         6  50.0
     7    712936  01         6  32.0
     8    712953  01         6  23.0
     9    712861  01         6  73.0
     6    712969  01         5  13.0

