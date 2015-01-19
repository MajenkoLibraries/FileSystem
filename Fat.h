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

#ifndef _FAT_H
#define _FAT_H

#include <FileSystem.h>


#define ATTR_READONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

#define ATTR_LFN (ATTR_VOLUME | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_READONLY)


struct fat_dirent {
	uint8_t filename[8];
	uint8_t extension[3];
	uint8_t attribs;
	uint8_t reserved;
	uint8_t create_tenths;
	uint16_t create_time;
	uint16_t update_date;
	uint16_t access_date;
	uint16_t cluster_high;
	uint16_t write_time;
	uint16_t write_date;
	uint16_t cluster_low;
	uint32_t size;
} __attribute__((packed));

struct fat_lfnent{
	uint8_t ordinal;
	uint16_t lfn1[5];
	uint8_t attribs;
	uint8_t reserved;
	uint8_t checksum;
	uint16_t lfn2[6];
	uint16_t zero;
	uint16_t lfn3[2];
} __attribute__((packed));

struct bootblock {
	uint8_t bs_start[3];
	uint8_t mfg_desc[8];
	uint16_t blocksize;
	uint8_t cluster_size;
	uint16_t reserved;
	uint8_t nfats;
	uint16_t nroot;
	uint16_t size_small;
	uint8_t descriptor;
	uint16_t fat_size;
	uint16_t bpt;
	uint16_t heads;
	uint32_t hidden;
	uint32_t size_big;
	uint16_t drive;
	uint8_t ext_sig;
	uint32_t serial;
	uint8_t label[11];
	uint8_t ident[8];
	uint8_t bs_prog[0x1c0];
	uint16_t signature;
} __attribute__((packed));

class Fat : public FileSystem {
private:
	BlockDevice 	*_dev;
	uint8_t 		_part;
	uint8_t 		_type;

	uint32_t 		findDirectoryEntry(uint32_t parent, const char *path);
	uint32_t		_cwd;

public:
	uint32_t 		_root_block;
	uint32_t		_cluster_size;
	uint32_t		_data_start;
	
	Fat(BlockDevice &dev, uint8_t partition);
	bool begin();
	uint32_t		getInode(const char *path) { return getInode(0, path, NULL); }
	uint32_t 		getInode(uint32_t parent, const char *path) { return getInode(0, path, NULL); }
	uint32_t 		getInode(uint32_t parent, const char *path, uint32_t *ancestor);
	
	uint32_t		getNextInode(uint32_t inode);

	File			open(const char *filename);
	uint32_t		getInodeSize(uint32_t parent, uint32_t child);

	int				readFileByte(uint32_t start, uint32_t offset);
	int				readClusterByte(uint32_t start, uint32_t offset);
	uint32_t		readFileBytes(uint32_t start, uint32_t offset, uint8_t *buffer, uint32_t len);
	uint32_t		readClusterBytes(uint32_t start, uint32_t offset, uint8_t *buffer, uint32_t len);

	uint32_t		getClusterSize() { return _cluster_size * 512; }
};

#endif
