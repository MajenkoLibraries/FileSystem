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

File::~File() {
	flush();
}

File::File(FileSystem *fs, uint32_t parent, uint32_t child, bool isValid) {
	_fs = fs;
	_parent = parent;
	_inode = child;	

	_size = _fs->getInodeSize(_parent, _inode);
	_position = 0;
	_posInode = _inode;
	_isValid = isValid;
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

File::operator bool() {
	return _isValid;
}
