/** @file */

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

/*! The partition structure describes the data stored in the
 * partition tablein the MBR.
 */
struct partition {
#define P_ACTIVE 0x80
	uint8_t status;
	uint8_t start[3];
	uint8_t type;
	uint8_t end[3];
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
} __attribute__((packed));

/** \addtogroup cache Cache System 
 *  @{
 */
 
/** @name Flags
 *  Contents of the `flags` parameter of a cache block.
 */
///@{ 
/*! A cache block has valid data in it */
#define CACHE_VALID 	0x01
/*! A cache block is dirty */
#define CACHE_DIRTY 	0x02
/*! A cache block is locked in core */
#define CACHE_LOCKED 	0x04
/*! A cache block may expire (not currently used) */
#define CACHE_EXPIRE	0x08
///@}

/** @name Mode
 *  Mode to operate the cache system in.
 */
///@{ 
/*! Enable write-back mode (the default) */
#define CACHE_WRITEBACK 	0x00
/*! Enable write-through mode */
#define CACHE_WRITETHROUGH 	0x01
///@}


/*! Number of entries in the cache */


#if RAMEND < 32768
# define CACHE_SIZE 1
#elif RAMEND < 65536
# define CACHE_SIZE 2
#elif RAMEND < 131072
# define CACHE_SIZE 4
#else
# define CACHE_SIZE 8
#endif

/** @} */

/*! Maximum directory depth when parsing a path */
#define MAX_DEPTH 20

/*! Storage block for a cache entry. */ 
struct cache {
	/*! Absolute block number */
	uint32_t blockno;
	/*! Last access time in milliseconds */
	uint32_t last_millis;
	/*! Number of times block has been hit */
	uint32_t hit_count;
	/*! Flags associated with this cache block */
	uint32_t flags;
	/*! The actual block data */
	uint8_t *data; //[512];
};
///@}


/*!
  * The BlockDevice class is an interface class defining the methods used
  * to access a block device - reading blocks, writing blocks, and initializing
  * the device itself.  Not a lot more than that really.
  */

class BlockDevice {

private:
	uint32_t _cacheHit;
	uint32_t _cacheMiss;
	uint8_t _cacheMode;

	uint8_t _activityLED;
	boolean _haveActivityLED;

	uint32_t findExpirableEntry(struct cache *);
	struct cache _dataCache[CACHE_SIZE];
	struct cache _systemCache[CACHE_SIZE];
	struct partition _partitions[4];



protected:
	void switchOnActivityLED();
	void switchOffActivityLED();
	virtual bool readBlockFromDisk(uint32_t blockno, uint8_t *data) = 0;
	virtual bool writeBlockToDisk(uint32_t block, uint8_t *data) = 0;
	bool loadPartitionTable();
    void initCacheBlocks();

    size_t _blockSize;

public:

	/*! Read a single block of data.  Block can come from the cache or from
	 *  the backing store. It's cached if not already in the cache.
	 */
	bool readBlock(uint32_t blockno, uint8_t *data);
	bool readSystemBlock(uint32_t blockno, uint8_t *data);

	/*! Write a single block of data.  It caches the data. If write-through
	 *  caching is enabled the data is also flushed immediately to the backing store.
	 */
	bool writeBlock(uint32_t blockno, uint8_t *data);
	bool writeSystemBlock(uint32_t blockno, uint8_t *data);

	/*! Read a single block of data within a partition.
	 */
	bool readRelativeBlock(uint8_t partition, uint32_t blockno, uint8_t *data);
	bool readRelativeSystemBlock(uint8_t partition, uint32_t blockno, uint8_t *data);

	/*! Write a single block of data within a partition.
	 */
	bool writeRelativeBlock(uint8_t partition, uint32_t blockno, uint8_t *data);
	bool writeRelativeSystemBlock(uint8_t partition, uint32_t blockno, uint8_t *data);

	/*! Performs any configuration of the device.  Returns a simple true/false
	 * bool on success or failure.  Sets errno accordingly.
	 */
	virtual bool initialize() = 0;

	/*! On devices that support removable media this command will flush any
	 * buffers or caches and allow the media to be removed cleanly.  Returns
	 * true/false bool on success or failure.  Sets errno accordingly.
	 */
	virtual bool eject() = 0;

	/*! On devices that support removable media this command will re-attach
	 * and re-initialize the media ready for access.  Returns true/false bool
	 * on success or failure.  Sets errno accordingly.
	 */
	virtual bool insert() = 0;

	/*! This function flushes any cached data to the block device.
	 */
	void sync();

	/*! Returns the number of sectors on the device
	 */
	virtual size_t getCapacity() = 0;

	/*! Set the cache mode. Select between CACHE_WRITEBACK and
	 *  CACHE_WRITETHROUGH.
	 *
	 *  Writeback caching only writes the data to the backing store
	 *  when the cache block needs to be re-used, or when a manual
	 *  sync() is called.  This is by far the fastest and kindest to
	 *  the flash device, but risks losing data on premature ejection or
	 *  power loss.
	 *
	 *  Writethough however writes the data to the backing store as
	 *  soon as it is placed in the cache.  Somewhat slower and causes
	 *  more writes to the flash (which shortens its lifetime) but greatly
	 *  reduces the chances of data loss
	 */
	virtual void setCacheMode(uint8_t cacheMode);

	/*! Connect an activity LED into the block device driver. Turns on
	 *  when a physical block read or write starts, turns off again
	 *  afterwards.  Just pass a pin number, the driver does the rest.
	 */
	void attachActivityLED(uint8_t pin);

	/*! Display some cache statistics - number of hits vs misses, contents
	 *  of cache, etc.
	 */
	virtual void printCacheStats();

    /*! Return the size of one sector - the smallest size that can be
     *  worked with on the device
     */

    virtual size_t getSectorSize();
};

/*! The FileSystem class is an interface class which defines the functions
 *  used to access a filesystem.  It implements a subset of the POSIX functions
 *  for accessing files and directories.
 */

class FileSystem;

class File : public Stream {
private:
	uint32_t	_parent;
	uint32_t	_inode;
	uint32_t	_position;
	FileSystem 	*_fs;
	uint32_t 	_size;
	uint32_t 	_posInode;
	bool		_isValid;

public:
	// Stream interface functions
	int 	read();
	size_t	readBytes(char *, size_t);

	size_t 	write(uint8_t c) { return 0; }
	int		available();
	int		peek() { return 0; }
	void	flush();

    void seek(uint32_t pos) { _position = pos; }

	operator bool();
//    File & operator =(const File &other);

	// Constructors
	File(FileSystem *fs, uint32_t parent, uint32_t child, bool);
    File() { _isValid = false; };
	~File();
	uint32_t length() { return _size; }
    void close() {}
};


class FileSystem {
protected:
	File	*_root;
	BlockDevice *_dev;

public:
	virtual bool begin() = 0;

	virtual int 			parsePath(String const &path, char **parts) { return parsePath(path.c_str(), parts); }
	virtual int 			parsePath(const char *path, char **parts);

	virtual uint32_t		getInode(const char *path) { return getInode(0, path, NULL); }
	virtual uint32_t 		getInode(uint32_t parent, const char *path) { return getInode(0, path, NULL); }
	virtual uint32_t		getInode(uint32_t parent, const char *path, uint32_t *ancestor) = 0;
	virtual uint32_t		getNextInode(uint32_t inode) = 0;

	virtual uint32_t		getInodeSize(uint32_t parent, uint32_t child);
	virtual int				readFileByte(uint32_t start, uint32_t offset);
	virtual int				readClusterByte(uint32_t start, uint32_t offset);
	virtual uint32_t		readFileBytes(uint32_t start, uint32_t offset, uint8_t *buffer, uint32_t len);
	virtual uint32_t		readClusterBytes(uint32_t start, uint32_t offset, uint8_t *buffer, uint32_t len);
	virtual uint32_t		getClusterSize() = 0;

	virtual File			open(const char *filename) = 0;
	virtual void			sync();
};


#include <SDCard.h>
#include <SPIFlash.h>
#include <Fat.h>

#endif
