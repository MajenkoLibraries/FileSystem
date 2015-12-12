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
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_entries;
    uint16_t total_sectors;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    union {
        struct {
            uint16_t hidden_sectors_12;
            uint8_t bootstrap_12[480];
        } __attribute__((packed));
        struct {
            uint32_t hidden_sectors_16;
            uint32_t total_sectors_16;
            uint8_t logical_drive_16;
            uint8_t reserved_16;
            uint8_t extended_signature_16;
            uint32_t serial_number_16;
            char label_16[11];
            char fstype_16[8];
            uint8_t bootstrap_16[448];
        } __attribute__((packed));
        struct {
            uint32_t hidden_sectors_32;
            uint32_t total_sectors_32;
            uint32_t sectors_per_fat_32;
            uint16_t mirror_flags_32;
            uint16_t fs_version_32;
            uint32_t root_start_32;
            uint16_t fs_info_sector_32;
            uint16_t backup_boot_32;
            uint8_t reserved_32[12];
            uint8_t logical_drive_32;
            uint8_t reserved2_32;
            uint8_t extended_signature_32;
            uint32_t serial_number_32;
            uint8_t label_32[11];
            uint8_t fstype_32[8];
            uint8_t bootstrap_32[420];
        } __attribute__((packed));
    } __attribute__((packed));
    uint16_t signature;
} __attribute__((packed));

class Fat : public FileSystem {
private:
//	BlockDevice 	*_dev;
	uint8_t 		_part;
	uint8_t 		_type;

	uint32_t 		findDirectoryEntry(uint32_t parent, const char *path);
	uint32_t		_cwd;
	uint8_t			_cachedFat[512];
	uint32_t		_cachedFatNumber;
	uint8_t			_cachedBlock[512];
	uint32_t		_cachedBlockNumber;

	uint32_t 		_root_block;
	uint32_t		_cluster_size;
    uint32_t        _bytes_per_sector;
	uint32_t		_data_start;
    uint32_t        _fat_start;
    uint32_t        _blockSize;

public:
	
					Fat(BlockDevice &dev, uint8_t partition);
	bool 			begin();
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

	uint32_t		getClusterSize() { return _cluster_size * _bytes_per_sector; }

    void dumpBlock(uint8_t *block);
};

#endif
