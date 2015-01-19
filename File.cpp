#include <FileSystem.h>

File::~File() {
	flush();
}

File::File(FileSystem *fs, uint32_t parent, uint32_t child) {
	_fs = fs;
	_parent = parent;
	_inode = child;	

	_size = _fs->getInodeSize(_parent, _inode);
	_position = 0;
	_posInode = _inode;
}

int File::read() {
	if (_position >= _size) {
		return -1;
	}

	uint32_t cs = _fs->getClusterSize();
	int c = _fs->readClusterByte(_posInode, _position & cs);
	_position++;
	if ((_position % cs) == 0) {
		_posInode = _fs->getNextInode(_posInode);
	}
	return c;
}

size_t File::readBytes(char *buffer, size_t len) {
	size_t end = _position + len;
	if (end > _size) {
		end = _size;
	}
	len = end - _position;
	uint32_t toRead = len;
	uint32_t cs = _fs->getClusterSize();
	uint32_t totalRead = 0;
	while (toRead > 0) {
		uint32_t chunkLeft = cs - (_position % cs);
		uint32_t thisChunk = min(chunkLeft, toRead);
		uint32_t numRead = _fs->readClusterBytes(_posInode, _position % cs, (uint8_t *)buffer, thisChunk);
		
		_position += numRead;
		if ((_position % cs) == 0) {
			_posInode = _fs->getNextInode(_posInode);
		}
		toRead -= numRead;
		totalRead += numRead;
	}
	return totalRead;
}

void File::flush() { 
	_fs->sync();
}
