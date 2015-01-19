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
    Read time: 113019ms
    Read speed: 927787.36 Bps

    Cache hits: 3
    Cache misses: 204820
    Cache percent: 0%

    Cache data:
    ID     Block  Flags  Count  Time
     2       650  01         2  113034
     1       137  01         1  113315
     0    712966  01         0  15
     3    712969  01         0  13
     4    712962  01         0  18
     5    712964  01         0  16
     6    712967  01         0  14
     7    712968  01         0  13
     8    712965  01         0  16
     9    712963  01         0  17
    

    100 byte block reads

    File size: 104857600
    Read time: 123667ms
    Read speed: 847902.84 Bps

    Cache hits: 1040387
    Cache misses: 204820
    Cache percent: 83%

    Cache data:
    ID     Block  Flags  Count  Time
     1    712953  01         6  23
     2    712778  01         6  118
     3    712686  01         6  169
     5    712711  01         6  154
     6    712853  01         6  77
     7    712811  01         6  100
     8    712736  01         6  141
     9    712903  01         6  50
     0    712968  01         5  13
     4    712969  01         5  13

Even single byte sequential reads are quite nippy, thanks to the multi-layer
cache system:


    Single byte read:
    
    File size: 104857600
    Read time: 206792ms
    Read speed: 507067.97 Bps

    Cache hits: 3
    Cache misses: 1621
    Cache percent: 0%

    Cache data:
    ID     Block  Flags  Count  Time
     2       650  01         2  206806
     1       137  01         1  207087
     0    712714  01         0  338
     3    712842  01         0  207
     4    712970  01         0  77
     5    712074  01         0  988
     6    712202  01         0  858
     7    712330  01         0  728
     8    712458  01         0  598
     9    712586  01         0  468
