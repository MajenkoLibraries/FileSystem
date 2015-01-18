/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*! Advanced SD Library
 *
 * This is a complete re-write of the SD library from the ground up, specifically
 * for the PIC32 class of microcontrollers.
 *
 * It abstracts the SD card from the filesystem, creating the concept of the
 * "block device" driver onto which is overlaid the FAT filesystem.
 *
 */

#ifndef _ADVANCEDSD_H
#define _ADVANCEDSD_H

#if (ARDUINO >= 100)
# include <Arduino.h>
#else
# include <WProgram.h>
#endif

#include <errno.h>

/*! The CSH structure defines a disk location based on the cylinder, 
 * head and sector location.
 */
struct chs { 
    uint8_t head;
    struct {
        unsigned cylhigh:2;
        unsigned sector:6;
    } __attribute__((packed));
    uint8_t cyllow;
}__attribute__((packed));

/*! The partition structure describes the data stored in the
 * partition tablein the MBR.
 */
struct partition {
#define P_ACTIVE 0x80
    uint8_t status;
    struct chs start;
    uint8_t type;
    struct chs end;
    uint32_t lbastart;
    uint32_t lbalength;
};


/*! The mbr structore describes the contents of a Master Boot Record.
 * 
 * The MBR not only contains the code (in two sections) for booting a disk, but
 * also the partition table containing the four primary partitions.
 */
struct mbr {
    uint8_t bootstrap1[218];
    uint16_t pad0000;
    uint8_t biosdrive;
    uint8_t secs;
    uint8_t mins;
    uint8_t hours;
    uint8_t bootstrap2[216];
    uint32_t sig;
    uint16_t pad0001;
    struct partition partitions[4];
    uint16_t bootsig;
}__attribute__((packed));

/*!
  * The BlockDevice class is an interface class defining the methods used
  * to access a block device - reading blocks, writing blocks, and initializing
  * the device itself.  Not a lot more than that really.
  */

class BlockDevice {
public:
	/*! Read a single block of data.
	 *
	 * *maxlen* is the maximum amount of data to read into *data,
	 * but it will never read past the end of the current block.
	 * Returns the number of bytes actually read.
	 */
	virtual size_t readBlock(uint32_t blockno, uint8_t *data, uint32_t maxlen) = 0;

	/*! Write a single block of data.
	 *
	 * *maxlen* is the maximum amount of data to write from *data,
	 * but it will never write past the end of the current block.
	 * Return sthe number of bytes actually written.
	 */
	virtual size_t writeBlock(uint32_t blockno, uint8_t *data, uint32_t maxlen) = 0;

	/*! Read a single block of data within a partition.
	 *
	 * *maxlen* is the maximum amount of data to read into *data,
	 * but it will never read past the end of the current block.
	 * Returns the number of bytes actually read.
	 */
	virtual size_t readRelativeBlock(uint8_t partition, uint32_t blockno, uint8_t *data, uint32_t maxlen) = 0;

	/*! Write a single block of data within a partition.
	 *
	 * *maxlen* is the maximum amount of data to write from *data,
	 * but it will never write past the end of the current block.
	 * Return sthe number of bytes actually written.
	 */
	virtual size_t writeRelativeBlock(uint8_t partition, uint32_t blockno, uint8_t *data, uint32_t maxlen) = 0;

	/*! Initialize the device.
	 *
	 * Performs any configuration of the device.  Returns a simple true/false
	 * bool on success or failure.  Sets errno accordingly.
	 */
	virtual bool initialize() = 0;

	/*! Eject the medium
	 *
	 * On devices that support removable media this command will flush any
	 * buffers or caches and allow the media to be removed cleanly.  Returns
	 * true/false bool on success or failure.  Sets errno accordingly.
	 */
	virtual bool eject() = 0;

	/*! Insert medium
	 *
	 * On devices that support removable media this command will re-attach
	 * and re-initialize the media ready for access.  Returns true/false bool
	 * on success or failure.  Sets errno accordingly.
	 */
	virtual bool insert() = 0;

	/*! Flush any caches
	 *
	 * This function flushes any cached data to the block device.
	 */
	virtual void sync() = 0;

	/*! Get capacity in sectors
	 * 
	 * Returns the number of blocks on the device
	 */
	 virtual size_t getCapacity() = 0;
};

/*! The FileSystem class is an interface class which defines the functions
 *  used to access a filesystem.  It implements a subset of the POSIX functions
 *  for accessing files and directories.
 */

class FileSystem {
public:
	virtual bool begin() = 0;

	virtual int open(const char *pathname, int flags) = 0;
	virtual int open(const char *pathname, int flags, mode_t mode) = 0;
	virtual int creat(const char *pathname, int flags, mode_t mode) = 0;
	virtual int openat(int dirfd, const char *pathname, int flags) = 0;
	virtual int openat(int dirfd, const char *pathname, int flags, mode_t mode) = 0;

	virtual int close(int fd) = 0;
	virtual size_t write(int fd, const void *buf, size_t count) = 0;
	virtual size_t read(int fd, void *buf, size_t count) = 0;

	virtual int access(const char *pathname, int mode) = 0;
	virtual int accessat(int dirfd, const char *pathname, int mode) = 0;
	virtual int chdir(const char *path) = 0;
	virtual int unlink(const char *pathname) = 0;
	virtual int unlinkar(int dirfd, const char *pathname) = 0;
	virtual int rmdir(const char *pathname) = 0;
	virtual int rmdirat(int dirfd, const char *pathname) = 0;
	virtual int mkdir(const char *pathname, mode_t mode) = 0;
	virtual int mkdirat(int dirfd, const char *pathname, mode_t mode) = 0;
	virtual int rename(const char *oldpath, const char *newpath) = 0;
	virtual int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath) = 0;
};

#include <SDCard.h>
#include <Fat.h>

#endif
