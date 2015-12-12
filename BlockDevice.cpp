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

void BlockDevice::attachActivityLED(uint8_t pin) {
	_activityLED = pin;
	_haveActivityLED = true;
	pinMode(_activityLED, OUTPUT);
	digitalWrite(_activityLED, LOW);
}

void BlockDevice::switchOnActivityLED() {
	if (_haveActivityLED) {
		digitalWrite(_activityLED, HIGH);
	}
}

void BlockDevice::switchOffActivityLED() {
	if (_haveActivityLED) {
		digitalWrite(_activityLED, LOW);
	}
}

void BlockDevice::sync() {
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_dataCache[i].flags & CACHE_VALID) {
			if (_dataCache[i].flags & CACHE_DIRTY) {
				switchOnActivityLED();

				if (writeBlockToDisk(_dataCache[i].blockno, _dataCache[i].data)) {
					_dataCache[i].flags &= ~CACHE_DIRTY;
				}

				switchOffActivityLED();
			}
		}
	}
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_systemCache[i].flags & CACHE_VALID) {
			if (_systemCache[i].flags & CACHE_DIRTY) {
				switchOnActivityLED();

				if (writeBlockToDisk(_systemCache[i].blockno, _systemCache[i].data)) {
					_systemCache[i].flags &= ~CACHE_DIRTY;
				}

				switchOffActivityLED();
			}
		}
	}
}


bool BlockDevice::readBlock(uint32_t block, uint8_t *data) {
	// Is it in the cache already?
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_dataCache[i].flags & CACHE_VALID) {
			if (_dataCache[i].blockno == block) {
				memcpy(data, _dataCache[i].data, _blockSize);
				_dataCache[i].last_millis = millis();
				_dataCache[i].hit_count++;
				_cacheHit++;
				return true;
			}
		}
	}

	_cacheMiss++;

	// Find an empty cache slot for the block
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (!(_dataCache[i].flags & CACHE_VALID)) {
			switchOnActivityLED();

			if (!readBlockFromDisk(block, _dataCache[i].data)) {
				switchOffActivityLED();
				return false;
			}

			switchOffActivityLED();
			_dataCache[i].blockno = block;
			_dataCache[i].last_millis = millis();
			_dataCache[i].flags = CACHE_VALID;
			_dataCache[i].hit_count = 0;
			memcpy(data, _dataCache[i].data, _blockSize);
			return true;
		}
	}

	// No room = let's expire the oldest entry and use that.
	uint32_t max_entry = findExpirableEntry(_dataCache);

	// If the found block is dirty then flush it to disk
	if (_dataCache[max_entry].flags & CACHE_DIRTY) {
		switchOnActivityLED();
		writeBlockToDisk(_dataCache[max_entry].blockno, _dataCache[max_entry].data);
		switchOffActivityLED();
	}

	switchOnActivityLED();

	if (!readBlockFromDisk(block, _dataCache[max_entry].data)) {
		switchOffActivityLED();
		return false;
	}

	switchOffActivityLED();
	_dataCache[max_entry].blockno = block;
	_dataCache[max_entry].last_millis = millis();
	_dataCache[max_entry].flags = CACHE_VALID;
	_dataCache[max_entry].hit_count = 0;
	memcpy(data, _dataCache[max_entry].data, _blockSize);
	return true;
}

bool BlockDevice::readSystemBlock(uint32_t block, uint8_t *data) {
	// Is it in the cache already?
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_dataCache[i].flags & CACHE_VALID) {
			if (_systemCache[i].blockno == block) {
				memcpy(data, _systemCache[i].data, _blockSize);
				_systemCache[i].last_millis = millis();
				_systemCache[i].hit_count++;
				_cacheHit++;
				return true;
			}
		}
	}

	_cacheMiss++;

	// Find an empty cache slot for the block
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (!(_systemCache[i].flags & CACHE_VALID)) {
			switchOnActivityLED();

			if (!readBlockFromDisk(block, _systemCache[i].data)) {
				switchOffActivityLED();
				return false;
			}

			switchOffActivityLED();
			_systemCache[i].blockno = block;
			_systemCache[i].last_millis = millis();
			_systemCache[i].flags = CACHE_VALID;
			_systemCache[i].hit_count = 0;
			memcpy(data, _systemCache[i].data, _blockSize);
			return true;
		}
	}

	// No room = let's expire the oldest entry and use that.
	uint32_t max_entry = findExpirableEntry(_systemCache);

	// If the found block is dirty then flush it to disk
	if (_systemCache[max_entry].flags & CACHE_DIRTY) {
		switchOnActivityLED();
		writeBlockToDisk(_systemCache[max_entry].blockno, _systemCache[max_entry].data);
		switchOffActivityLED();
	}

	switchOnActivityLED();

	if (!readBlockFromDisk(block, _systemCache[max_entry].data)) {
		switchOffActivityLED();
		return false;
	}

	switchOffActivityLED();
	_systemCache[max_entry].blockno = block;
	_systemCache[max_entry].last_millis = millis();
	_systemCache[max_entry].flags = CACHE_VALID;
	_systemCache[max_entry].hit_count = 0;
	memcpy(data, _systemCache[max_entry].data, _blockSize);
	return true;
}

bool BlockDevice::writeBlock(uint32_t block, uint8_t *data) {
	// First let's look for the block in the cache
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_dataCache[i].flags & CACHE_VALID) {
			if (_dataCache[i].blockno == block) {
				memcpy(_dataCache[i].data, data, _blockSize);
				_dataCache[i].flags |= CACHE_DIRTY;
				_dataCache[i].last_millis = millis();
				_dataCache[i].hit_count++;

				if (_cacheMode == CACHE_WRITETHROUGH) {
					switchOnActivityLED();

					if (!writeBlockToDisk(_dataCache[i].blockno, _dataCache[i].data)) {
						switchOffActivityLED();
						return false;
					}

					switchOffActivityLED();
					_dataCache[i].flags &= ~CACHE_DIRTY;
				}

				_cacheHit++;
				return true;
			}
		}
	}

	_cacheMiss++;

	// Not found in the cache, so let's find room for it
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (!(_dataCache[i].flags & CACHE_VALID)) {
			_dataCache[i].blockno = block;
			_dataCache[i].flags = CACHE_VALID | CACHE_DIRTY;
			_dataCache[i].last_millis = millis();
			_dataCache[i].hit_count = 0;
			memcpy(_dataCache[i].data, data, _blockSize);

			if (_cacheMode == CACHE_WRITETHROUGH) {
				switchOnActivityLED();

				if (!writeBlockToDisk(_dataCache[i].blockno, _dataCache[i].data)) {
					switchOffActivityLED();
					return false;
				}

				switchOffActivityLED();
				_dataCache[i].flags &= ~CACHE_DIRTY;
			}

			return true;
		}
	}

	uint32_t max_entry = findExpirableEntry(_dataCache);

	// If the found block is dirty then flush it to disk
	if (_dataCache[max_entry].flags & CACHE_DIRTY) {
		switchOnActivityLED();
		writeBlockToDisk(_dataCache[max_entry].blockno, _dataCache[max_entry].data);
		switchOffActivityLED();
	}

	_dataCache[max_entry].blockno = block;
	_dataCache[max_entry].last_millis = millis();
	_dataCache[max_entry].flags = CACHE_VALID | CACHE_DIRTY;
	_dataCache[max_entry].hit_count = 0;
	memcpy(_dataCache[max_entry].data, data, _blockSize);

	if (_cacheMode == CACHE_WRITETHROUGH) {
		switchOnActivityLED();

		if (!writeBlockToDisk(_dataCache[max_entry].blockno, _dataCache[max_entry].data)) {
			switchOffActivityLED();
			return false;
		}

		switchOffActivityLED();
		_dataCache[max_entry].flags &= ~CACHE_DIRTY;
	}

	return true;
}

bool BlockDevice::writeSystemBlock(uint32_t block, uint8_t *data) {
	// First let's look for the block in the cache
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_systemCache[i].flags & CACHE_VALID) {
			if (_systemCache[i].blockno == block) {
				memcpy(_systemCache[i].data, data, _blockSize);
				_systemCache[i].flags |= CACHE_DIRTY;
				_systemCache[i].last_millis = millis();
				_systemCache[i].hit_count++;

				if (_cacheMode == CACHE_WRITETHROUGH) {
					switchOnActivityLED();

					if (!writeBlockToDisk(_systemCache[i].blockno, _systemCache[i].data)) {
						switchOffActivityLED();
						return false;
					}

					switchOffActivityLED();
					_systemCache[i].flags &= ~CACHE_DIRTY;
				}

				_cacheHit++;
				return true;
			}
		}
	}

	_cacheMiss++;

	// Not found in the cache, so let's find room for it
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (!(_systemCache[i].flags & CACHE_VALID)) {
			_systemCache[i].blockno = block;
			_systemCache[i].flags = CACHE_VALID | CACHE_DIRTY;
			_systemCache[i].last_millis = millis();
			_systemCache[i].hit_count = 0;
			memcpy(_systemCache[i].data, data, _blockSize);

			if (_cacheMode == CACHE_WRITETHROUGH) {
				switchOnActivityLED();

				if (!writeBlockToDisk(_systemCache[i].blockno, _systemCache[i].data)) {
					switchOffActivityLED();
					return false;
				}

				switchOffActivityLED();
				_systemCache[i].flags &= ~CACHE_DIRTY;
			}

			return true;
		}
	}

	uint32_t max_entry = findExpirableEntry(_systemCache);

	// If the found block is dirty then flush it to disk
	if (_systemCache[max_entry].flags & CACHE_DIRTY) {
		switchOnActivityLED();
		writeBlockToDisk(_systemCache[max_entry].blockno, _systemCache[max_entry].data);
		switchOffActivityLED();
	}

	_systemCache[max_entry].blockno = block;
	_systemCache[max_entry].last_millis = millis();
	_systemCache[max_entry].flags = CACHE_VALID | CACHE_DIRTY;
	_systemCache[max_entry].hit_count = 0;
	memcpy(_systemCache[max_entry].data, data, _blockSize);

	if (_cacheMode == CACHE_WRITETHROUGH) {
		switchOnActivityLED();

		if (!writeBlockToDisk(_systemCache[max_entry].blockno, _systemCache[max_entry].data)) {
			switchOffActivityLED();
			return false;
		}

		switchOffActivityLED();
		_systemCache[max_entry].flags &= ~CACHE_DIRTY;
	}

	return true;
}

void BlockDevice::setCacheMode(uint8_t mode) {
	_cacheMode = mode;

	if (_cacheMode == CACHE_WRITETHROUGH) {
		sync();
	}
}


uint32_t BlockDevice::findExpirableEntry(struct cache *cache) {
	uint32_t leastUsedCount = 0xFFFFFFFFUL;
	uint32_t oldestUsedTime = 0;
	uint32_t oldestUsedID = 0;

	// First scan through and find the least used count.
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (cache[i].hit_count < leastUsedCount) {
			leastUsedCount = cache[i].hit_count;
		}
	}

	uint32_t now = millis();

	// Now scan through and find the one with that count that is oldest.
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (cache[i].hit_count == leastUsedCount) {
			if (now - cache[i].last_millis > oldestUsedTime) {
				oldestUsedTime = now - cache[i].last_millis;
				oldestUsedID = i;
			}
		}
	}

	return oldestUsedID;
}

void BlockDevice::printCacheStats() {
	Serial.print("Cache hits: ");
	Serial.println(_cacheHit);
	Serial.print("Cache misses: ");
	Serial.println(_cacheMiss);
	Serial.print("Cache percent: ");
	Serial.print((_cacheHit * 100) / (_cacheHit + _cacheMiss));
	Serial.println("%");
	Serial.println();
	Serial.println("Data cache:");
	Serial.println("ID     Block  Flags  Count  Time");
	char temp[80];
	uint32_t max_hit = 0;

	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_dataCache[i].hit_count > max_hit) {
			max_hit = _dataCache[i].hit_count;
		}
	}

	uint32_t now = millis();

	for (int hit = max_hit; hit >= 0; hit--) {
		for (int i = 0; i < CACHE_SIZE; i++) {
			if (_dataCache[i].hit_count == (uint32_t)hit) {
				sprintf(temp, "%2d  %8lu  %02x     %5lu  %lu",
				        i,
				        _dataCache[i].blockno,
				        (unsigned int)_dataCache[i].flags,
				        _dataCache[i].hit_count,
				        (now - _dataCache[i].last_millis)
				       );
				Serial.println(temp);
			}
		}
	}
	Serial.println();
	Serial.println("System cache:");
	Serial.println("ID     Block  Flags  Count  Time");
	max_hit = 0;

	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_systemCache[i].hit_count > max_hit) {
			max_hit = _systemCache[i].hit_count;
		}
	}

	now = millis();

	for (int hit = max_hit; hit >= 0; hit--) {
		for (int i = 0; i < CACHE_SIZE; i++) {
			if (_systemCache[i].hit_count == (uint32_t)hit) {
				sprintf(temp, "%2d  %8lu  %02x     %5lu  %lu",
				        i,
				        _systemCache[i].blockno,
				        (unsigned int)_systemCache[i].flags,
				        _systemCache[i].hit_count,
				        (now - _systemCache[i].last_millis)
				       );
				Serial.println(temp);
			}
		}
	}
}

bool BlockDevice::readRelativeBlock(uint8_t partition, uint32_t block, uint8_t *data) {
	uint32_t offset = _partitions[partition & 0x03].lbastart;
	uint32_t size = _partitions[partition & 0x03].lbalength;

	if (offset > getCapacity()) {
		errno = EINVAL;
		return false;
	}

	if (block > size) {
		errno = EINVAL;
		return false;
	}

	return readBlock(offset + block, data);
}

bool BlockDevice::readRelativeSystemBlock(uint8_t partition, uint32_t block, uint8_t *data) {
	uint32_t offset = _partitions[partition & 0x03].lbastart;
	uint32_t size = _partitions[partition & 0x03].lbalength;

	if (offset > getCapacity()) {
		errno = EINVAL;
		return false;
	}

	if (block > size) {
		errno = EINVAL;
		return false;
	}

	return readSystemBlock(offset + block, data);
}

bool BlockDevice::writeRelativeBlock(uint8_t partition, uint32_t block, uint8_t *data) {
	uint8_t offset = _partitions[partition & 0x03].lbastart;
	uint8_t size = _partitions[partition & 0x03].lbalength;

	if (offset > getCapacity()) {
		errno = EINVAL;
		return false;
	}

	if (block > size) {
		errno = EINVAL;
		return false;
	}

	return writeBlock(offset + block, data);
}

bool BlockDevice::writeRelativeSystemBlock(uint8_t partition, uint32_t block, uint8_t *data) {
	uint8_t offset = _partitions[partition & 0x03].lbastart;
	uint8_t size = _partitions[partition & 0x03].lbalength;

	if (offset > getCapacity()) {
		errno = EINVAL;
		return false;
	}

	if (block > size) {
		errno = EINVAL;
		return false;
	}

	return writeSystemBlock(offset + block, data);
}

bool BlockDevice::loadPartitionTable() {
	uint8_t buffer[_blockSize];

	if (!readSystemBlock(0, buffer)) {
		errno = EIO;
		return false;
	}

	struct mbr *mbr = (struct mbr *)buffer;

	memcpy(_partitions, mbr->partitions, sizeof(struct partition) * 4);

	if (_partitions[0].lbastart > getCapacity()) {
		errno = ENODEV;
		return false;
	}
	return true;
}

size_t BlockDevice::getSectorSize() {
    return _blockSize;
}

void BlockDevice::initCacheBlocks() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (_dataCache[i].data == NULL) {
            _dataCache[i].data = (uint8_t *)malloc(_blockSize);
        }
        if (_systemCache[i].data == NULL) {
            _systemCache[i].data = (uint8_t *)malloc(_blockSize);
        }
    }
}
