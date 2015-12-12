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

#include <FileSystem.h>


Fat::Fat(BlockDevice &dev, uint8_t partition) {
	_dev = &dev;
	_part = partition & 0x03;
	_type = 0;
	_cwd = 0;
	_cachedFatNumber = 0xFFFFFFFFUL;
	_cachedBlockNumber = 0xFFFFFFFFUL;
}
void Fat::dumpBlock(uint8_t *data) {
    char temp[32];
    char ascii[32];
    for (int i = 0; i < _blockSize; i++) {
        sprintf(temp, "%02x ", data[i]);
        ascii[i % 16] = (data[i] >= ' ' && data[i] <= 127) ? data[i] : '.';
        ascii[(i % 16)+1] = 0;
        Serial.print(temp);
        if (i % 16 == 15) {
            Serial.print(" ");
            Serial.println(ascii);
        }
    }
}


bool Fat::begin() {
    errno = 0;
	if (!_dev->initialize()) {
		return false;
	}
    _blockSize = _dev->getSectorSize();

	uint8_t buffer[_blockSize];

	struct bootblock *bb = (struct bootblock *)buffer;

	if (!_dev->readRelativeSystemBlock(_part, 0, buffer)) {
		errno = -10;
		return false;
	}

	if (!strncmp((const char *)bb->fstype_16, "FAT16", 5)) {
		_type = 16;
        _root_block = (bb->fat_copies * bb->sectors_per_fat) + 1;
        _fat_start = bb->reserved_sectors;
        _data_start = _root_block + ((bb->root_entries * sizeof(struct fat_dirent)) / _blockSize);
	} else 	if (!strncmp((const char *)bb->fstype_32, "FAT32", 5)) {
        _data_start = bb->reserved_sectors + (bb->fat_copies * bb->sectors_per_fat_32);
        _fat_start = bb->reserved_sectors;
        _root_block = _data_start;
		_type = 32;
	} else {
		errno = -20; //EINVAL;
		return false;
	}

	return true;
}

uint32_t Fat::findDirectoryEntry(uint32_t parent, const char *path) {
	uint8_t block[_blockSize];
	uint32_t offset = 0;

	if (parent == 0) {
		offset = _root_block;
	} else {
		offset = _data_start + ((parent - 2) * _cluster_size);
	}

	char lfn[256];
	bzero(lfn, 256);
	bool has_lfn = false;
	bool done = false;
	while (!done) {
		if (!_dev->readRelativeSystemBlock(_part, offset, block)) {
			return 0;
		}
		struct fat_dirent *p = (struct fat_dirent *)block;
		for (uint32_t i = 0; i < _blockSize / sizeof(struct fat_dirent); i++) {
			if ((p[i].attribs & ATTR_LFN) == ATTR_LFN) {
				struct fat_lfnent *as_lfn = (struct fat_lfnent *)block;
				// Deleted?
				if (as_lfn[i].ordinal & 0x80) {
					continue;
				}
				int chunk = ((as_lfn[i].ordinal & 0x3F) - 1) * 13;
				for (int j = 0; j < 5; j++) {
					lfn[chunk + j] = as_lfn[i].lfn1[j];
				}
				for (int j = 0; j < 6; j++) {
					lfn[chunk + j + 5] = as_lfn[i].lfn2[j];
				}
				lfn[chunk + 11] = as_lfn[i].lfn3[0];
				lfn[chunk + 12] = as_lfn[i].lfn3[1];
				// Last entry?
				if (as_lfn[i].ordinal & 0x40) {
					has_lfn = true;
				}
				continue;
			}
	
			if (p[i].attribs & ATTR_VOLUME) {
				continue;
			}
	
			if (p[i].filename[0] == 0) {
				done = true;
				break;
			}


			uint32_t cluster = (p[i].cluster_high << 16) | p[i].cluster_low;

			if (has_lfn) {
				if (!strcmp(lfn, path)) {
					return cluster;
				}
			} else {
				char fn[12];
                char ext[4];
                fn[0] = p[i].filename[0];
                fn[1] = p[i].filename[1];
                fn[2] = p[i].filename[2];
                fn[3] = p[i].filename[3];
                fn[4] = p[i].filename[4];
                fn[5] = p[i].filename[5];
                fn[6] = p[i].filename[6];
                fn[7] = p[i].filename[7];
                fn[8] = 0;
                ext[0] = p[i].filename[8];
                ext[1] = p[i].filename[9];
                ext[2] = p[i].filename[10];
                ext[3] = 0;

                while ((strlen(ext) > 0) && (fn[strlen(ext)-1] == ' ')) {
                    fn[strlen(ext)-1] = 0;
                }
                while ((strlen(fn) > 0) && (fn[strlen(fn)-1] == ' ')) {
                    fn[strlen(fn)-1] = 0;
                }
                if (strlen(ext) > 0) {
                    strcat(fn, ".");
                    strcat(fn, ext);
                }

				if (!strcmp(fn, path)) {
					return cluster;
				}
			}
		}
		offset++;
	}	
	errno = ENOENT;
	return 0;	
}

uint32_t Fat::getInode(uint32_t parent, const char *path, uint32_t *ancestor) {
	char *parts[MAX_DEPTH];
	int len = FileSystem::parsePath(path, parts);

	if (len == 1) {
		free(parts[0]);
		if (ancestor != NULL) {
			*ancestor = 0;
		}
		return findDirectoryEntry(parent, path);
	}
	uint32_t inode = parent;
	for (int i = 0; i < len; i++) {
		if (ancestor != NULL) {
			*ancestor = inode;
		}
		inode = getInode(inode, parts[i], ancestor);
		if (inode == 0) {
			free(parts[0]);
			return 0;
		}
	}
	
	free(parts[0]);
	return inode;
}

uint32_t Fat::getNextInode(uint32_t inode) {

	uint32_t inodesPerBlock = 0;
	if (_type == 32) {
		inodesPerBlock = _blockSize / 4;		
	} else {
		inodesPerBlock = _blockSize / 2;
	}

	uint32_t block = inode / inodesPerBlock;
	uint32_t inner = inode % inodesPerBlock;

    block += _fat_start;


	if (_cachedFatNumber != (block)) {
		if (!_dev->readRelativeSystemBlock(_part, block, _cachedFat)) {
			_cachedFatNumber = 0xFFFFFFFFUL;
			return 0;
		}
		_cachedFatNumber = (block);
	}
	uint32_t nextInode = 0;
	if (_type == 32) {
		nextInode = _cachedFat[(inner * 4)] | (_cachedFat[(inner * 4)+1] << 8) | (_cachedFat[(inner * 4)+2] << 16) | (_cachedFat[(inner * 4)+3] << 24);
	} else {
		nextInode = _cachedFat[(inner * 2)] | (_cachedFat[(inner * 2)+1] << 8);
	}
	return nextInode;
}

File Fat::open(const char *filename) {
	uint32_t inode;
	uint32_t parent;

	if (filename[0] == '/') {
		inode = getInode(0, filename+1, &parent);
	} else {
		inode = getInode(_cwd, filename+1, &parent);		
	}
	return File(this, parent, inode, inode == 0 ? false : true);
}

uint32_t Fat::getInodeSize(uint32_t parent, uint32_t child) {
	uint8_t block[_blockSize];
	uint32_t offset = 0;
	if (parent == 0) {
		offset = _root_block;
	} else {
		offset = _data_start + ((parent - 2) * _cluster_size);
	}

	bool done = false;
	while (!done) {
		if (!_dev->readRelativeSystemBlock(0, offset, block)) {
			return 0;
		}
		struct fat_dirent *p = (struct fat_dirent *)block;
		for (uint32_t i = 0; i < _blockSize / sizeof(struct fat_dirent); i++) {
			if ((p[i].attribs & ATTR_LFN) == ATTR_LFN) {
				continue;
			}
	
			if (p[i].attribs & ATTR_VOLUME) {
				continue;
			}
	
			if (p[i].filename[0] == 0) {
				done = true;
				break;
			}

			uint32_t cluster = (p[i].cluster_high << 16) | p[i].cluster_low;
			if (cluster == child) {
				return p[i].size;
			}
		}
		offset++;
	}	
	return 0;	
}

int Fat::readFileByte(uint32_t start, uint32_t offset) {
	uint32_t clusterNumber = offset / (_cluster_size * _blockSize);
	uint32_t clusterOffset = offset % (_cluster_size * _blockSize);
	uint32_t clusterBlock = clusterOffset / _blockSize;
	uint32_t blockOffset = clusterOffset % _blockSize;

	uint32_t inode = start;
	for (uint32_t i = 0; i < clusterNumber; i++) {
		inode = getNextInode(inode);
	}

	uint32_t block = (inode - 2) * _cluster_size + clusterBlock + _data_start;

	if (block != _cachedBlockNumber) {
		_dev->readRelativeBlock(_part, block, _cachedBlock);
		_cachedBlockNumber = block;
		
	}

	return _cachedBlock[blockOffset];
}

int Fat::readClusterByte(uint32_t inode, uint32_t offset) {
	uint32_t clusterBlock = offset / _blockSize;
	uint32_t blockOffset = offset % _blockSize;

	uint32_t block = (inode - 2) * _cluster_size + clusterBlock + _data_start;
	if (block != _cachedBlockNumber) {	
		_dev->readRelativeBlock(_part, block, _cachedBlock);
		_cachedBlockNumber = block;
	}
	return _cachedBlock[blockOffset];
}

uint32_t Fat::readFileBytes(uint32_t start, uint32_t offset, uint8_t *buffer, uint32_t len) {

	uint32_t currentBlock = 0xFFFFFFFFUL;
	uint8_t data[_blockSize];
	uint32_t numRead = 0;

	uint32_t clusterNumber = (offset) / (_cluster_size * _blockSize);
	uint32_t clusterOffset = (offset) % (_cluster_size * _blockSize);
	uint32_t clusterBlock = clusterOffset / _blockSize;
	uint32_t blockOffset = clusterOffset % _blockSize;
	uint32_t relativeBlock = clusterNumber * _cluster_size + clusterBlock;

	uint32_t inode = start;

	for (uint32_t j = 0; j < clusterNumber; j++) {
		inode = getNextInode(inode);
	}
	uint32_t currentCluster = clusterNumber;

	for (uint32_t i = 0; i < len; i++) {
		clusterNumber = (offset + i) / (_cluster_size * _blockSize);
		clusterOffset = (offset + i) % (_cluster_size * _blockSize);
		clusterBlock = clusterOffset / _blockSize;
		blockOffset = clusterOffset % _blockSize;
		relativeBlock = clusterNumber * _cluster_size + clusterBlock;
		
		if (currentBlock != relativeBlock) {
			currentBlock = relativeBlock;
			uint32_t inode = start;

			if (currentCluster != clusterNumber) {
				currentCluster++;
				inode = getNextInode(inode);
			}
			uint32_t thisBlock = (inode - 2) * _cluster_size + clusterBlock + _data_start;

			if (!_dev->readRelativeBlock(_part, thisBlock, data)) {
				break;
			}
		}
		numRead++;
		buffer[i] = data[blockOffset];
	}

	return numRead;
}

uint32_t Fat::readClusterBytes(uint32_t inode, uint32_t offset, uint8_t *buffer, uint32_t len) {

	uint32_t currentBlock = 0xFFFFFFFFUL;
	uint32_t numRead = 0;
	
	for (uint32_t i = 0; i < len; i++) {
		uint32_t clusterBlock = (offset + i) / _blockSize;
		uint32_t blockOffset = (offset + i) % _blockSize;
		
		if (currentBlock != clusterBlock) {
			currentBlock = clusterBlock;

			uint32_t thisBlock = (inode - 2) * _cluster_size + clusterBlock + _data_start;
			if (thisBlock != _cachedBlockNumber) {
				if (!_dev->readRelativeBlock(_part, thisBlock, _cachedBlock)) {
					break;
				}
				_cachedBlockNumber = thisBlock;
			}
		}
		numRead++;
		buffer[i] = _cachedBlock[blockOffset];
	}

	return numRead;
}
