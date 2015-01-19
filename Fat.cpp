#include <FileSystem.h>


Fat::Fat(BlockDevice &dev, uint8_t partition) {
	_dev = &dev;
	_part = partition & 0x03;
	_type = 0;
	_cwd = 0;
}

bool Fat::begin() {
	if (!_dev->initialize()) {
		return false;
	}

	uint8_t buffer[512];

	struct bootblock *bb = (struct bootblock *)buffer;

	if (!_dev->readRelativeBlock(_part, 0, buffer)) {
		errno = -10;
		return false;
	}

	if (!strncmp((const char *)bb->ident, "FAT16", 5)) {
		Serial.println("It's fat16");
		_type = 16;
	} else 	if (!strncmp((const char *)bb->ident, "FAT32", 5)) {
		Serial.println("It's fat32");
		_type = 32;
	} else {
		Serial.println("It's not identified");
		errno = -20; //EINVAL;
		return false;
	}

	_root_block = (bb->nfats * bb->fat_size) + 1;
	_cluster_size = bb->cluster_size;
	_data_start = _root_block + ((bb->nroot * sizeof(struct fat_dirent)) / 512);
	return true;
}

uint32_t Fat::findDirectoryEntry(uint32_t parent, const char *path) {
	uint8_t block[512];
	uint32_t offset = 0;

	Serial.print("findDirectoryEntry(");
	Serial.print(parent);
	Serial.print(", ");
	Serial.print(path);
	Serial.println(");");
	
	if (parent == 0) {
		offset = _root_block;
	} else {
		offset = _data_start + ((parent - 2) * _cluster_size);
	}

	char lfn[256];
	bool has_lfn = false;
	bool done = false;
	while (!done) {
		if (!_dev->readRelativeBlock(0, offset, block)) {
			return 0;
		}
		struct fat_dirent *p = (struct fat_dirent *)block;
		for (uint32_t i = 0; i < 512 / sizeof(struct fat_dirent); i++) {
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
				if (!strcmp(fn, path)) {
					return cluster;
				}
			}
		}
		offset++;
	}	
	return 0;	
}

uint32_t Fat::getInode(uint32_t parent, const char *path, uint32_t *ancestor) {
	char *parts[MAX_DEPTH];
	int len = FileSystem::parsePath(path, parts);
	Serial.print("Path length: ");
	Serial.println(len);
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
		inodesPerBlock = 512 / 4;		
	} else {
		inodesPerBlock = 512 / 2;
	}

	uint32_t block = inode / inodesPerBlock;
	uint32_t inner = inode % inodesPerBlock;
	
	uint8_t data[512];

	if (!_dev->readRelativeBlock(_part, 1 + block, data)) {
		return 0;
	}
	uint32_t nextInode = 0;
	if (_type == 32) {
		nextInode = data[(inner * 4)] | (data[(inner * 4)+1] << 8) | (data[(inner * 4)+2] << 16) | (data[(inner * 4)+3] << 24);
	} else {
		nextInode = data[(inner * 2)] | (data[(inner * 2)+1] << 8);
	}
	return nextInode;
}

File Fat::open(const char *filename) {
	uint32_t inode;
	uint32_t parent;

	inode = getInode(_cwd, filename, &parent);
	Serial.print("Open file inode: ");
	Serial.println(inode);
	return File(this, parent, inode);
}

uint32_t Fat::getInodeSize(uint32_t parent, uint32_t child) {
	uint8_t block[512];
	uint32_t offset = 0;
	if (parent == 0) {
		offset = _root_block;
		Serial.println("File is in root folder");
	} else {
		offset = _data_start + ((parent - 2) * _cluster_size);
		Serial.println("File is in sub folder");
	}

	bool done = false;
	while (!done) {
		if (!_dev->readRelativeBlock(0, offset, block)) {
			return 0;
		}
		struct fat_dirent *p = (struct fat_dirent *)block;
		for (uint32_t i = 0; i < 512 / sizeof(struct fat_dirent); i++) {
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
	Serial.println("Not found?!");
	return 0;	
}

int Fat::readFileByte(uint32_t start, uint32_t offset) {
	uint32_t clusterNumber = offset / (_cluster_size * 512);
	uint32_t clusterOffset = offset % (_cluster_size * 512);
	uint32_t clusterBlock = clusterOffset / 512;
	uint32_t blockOffset = clusterOffset % 512;

	uint32_t inode = start;
	for (uint32_t i = 0; i < clusterNumber; i++) {
		inode = getNextInode(inode);
	}

	uint8_t data[512];

	_dev->readRelativeBlock(_part, (inode - 2) * _cluster_size + clusterBlock + _data_start, data);
	return data[blockOffset];
}

int Fat::readClusterByte(uint32_t inode, uint32_t offset) {
	uint32_t clusterBlock = offset / 512;
	uint32_t blockOffset = offset % 512;

	uint8_t data[512];

	_dev->readRelativeBlock(_part, (inode - 2) * _cluster_size + clusterBlock + _data_start, data);
	return data[blockOffset];
}

uint32_t Fat::readFileBytes(uint32_t start, uint32_t offset, uint8_t *buffer, uint32_t len) {


	uint32_t currentBlock = 0xFFFFFFFFUL;
	uint8_t data[512];
	uint32_t numRead = 0;

	uint32_t clusterNumber = (offset) / (_cluster_size * 512);
	uint32_t clusterOffset = (offset) % (_cluster_size * 512);
	uint32_t clusterBlock = clusterOffset / 512;
	uint32_t blockOffset = clusterOffset % 512;
	uint32_t relativeBlock = clusterNumber * _cluster_size + clusterBlock;

	uint32_t inode = start;

	uint32_t ts_start = millis();
	for (uint32_t j = 0; j < clusterNumber; j++) {
		inode = getNextInode(inode);
	}
	uint32_t currentCluster = clusterNumber;
	uint32_t ts_end = millis();
	Serial.print("Get Next Cluster loop took ");
	Serial.print(ts_end - ts_start);
	Serial.println("ms");
	
	for (uint32_t i = 0; i < len; i++) {
		clusterNumber = (offset + i) / (_cluster_size * 512);
		clusterOffset = (offset + i) % (_cluster_size * 512);
		clusterBlock = clusterOffset / 512;
		blockOffset = clusterOffset % 512;
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
	uint8_t data[512];
	uint32_t numRead = 0;
	
	for (uint32_t i = 0; i < len; i++) {
		uint32_t clusterBlock = (offset + i) / 512;
		uint32_t blockOffset = (offset + i) % 512;
		
		if (currentBlock != clusterBlock) {
			currentBlock = clusterBlock;

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