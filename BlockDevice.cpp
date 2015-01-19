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
		if (_cache[i].flags & CACHE_VALID) {
			if (_cache[i].flags & CACHE_DIRTY) {
				switchOnActivityLED();

				if (writeBlockToDisk(_cache[i].blockno, _cache[i].data)) {
					_cache[i].flags &= ~CACHE_DIRTY;
				}

				switchOffActivityLED();
			}
		}
	}
}


bool BlockDevice::readBlock(uint32_t block, uint8_t *data) {
	// Is it in the cache already?
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_cache[i].flags & CACHE_VALID) {
			if (_cache[i].blockno == block) {
				memcpy(data, _cache[i].data, 512);
				_cache[i].last_millis = millis();
				_cache[i].hit_count++;
				_cacheHit++;
				return true;
			}
		}
	}

	_cacheMiss++;

	// Find an empty cache slot for the block
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (!(_cache[i].flags & CACHE_VALID)) {
			switchOnActivityLED();

			if (!readBlockFromDisk(block, _cache[i].data)) {
				switchOffActivityLED();
				return false;
			}

			switchOffActivityLED();
			_cache[i].blockno = block;
			_cache[i].last_millis = millis();
			_cache[i].flags = CACHE_VALID;
			_cache[i].hit_count = 0;
			memcpy(data, _cache[i].data, 512);
			return true;
		}
	}

	// No room = let's expire the oldest entry and use that.
	uint32_t max_entry = findExpirableEntry();

	// If the found block is dirty then flush it to disk
	if (_cache[max_entry].flags & CACHE_DIRTY) {
		switchOnActivityLED();
		writeBlockToDisk(_cache[max_entry].blockno, _cache[max_entry].data);
		switchOffActivityLED();
	}

	switchOnActivityLED();

	if (!readBlockFromDisk(block, _cache[max_entry].data)) {
		switchOffActivityLED();
		return false;
	}

	switchOffActivityLED();
	_cache[max_entry].blockno = block;
	_cache[max_entry].last_millis = millis();
	_cache[max_entry].flags = CACHE_VALID;
	_cache[max_entry].hit_count = 0;
	memcpy(data, _cache[max_entry].data, 512);
	return true;
}

bool BlockDevice::writeBlock(uint32_t block, uint8_t *data) {
	// First let's look for the block in the cache
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (_cache[i].flags & CACHE_VALID) {
			if (_cache[i].blockno == block) {
				memcpy(_cache[i].data, data, 512);
				_cache[i].flags |= CACHE_DIRTY;
				_cache[i].last_millis = millis();
				_cache[i].hit_count++;

				if (_cacheMode == CACHE_WRITETHROUGH) {
					switchOnActivityLED();

					if (!writeBlockToDisk(_cache[i].blockno, _cache[i].data)) {
						switchOffActivityLED();
						return false;
					}

					switchOffActivityLED();
					_cache[i].flags &= ~CACHE_DIRTY;
				}

				_cacheHit++;
				return true;
			}
		}
	}

	_cacheMiss++;

	// Not found in the cache, so let's find room for it
	for (int i = 0; i < CACHE_SIZE; i++) {
		if (!(_cache[i].flags & CACHE_VALID)) {
			_cache[i].blockno = block;
			_cache[i].flags = CACHE_VALID | CACHE_DIRTY;
			_cache[i].last_millis = millis();
			_cache[i].hit_count = 0;
			memcpy(_cache[i].data, data, 512);

			if (_cacheMode == CACHE_WRITETHROUGH) {
				switchOnActivityLED();

				if (!writeBlockToDisk(_cache[i].blockno, _cache[i].data)) {
					switchOffActivityLED();
					return false;
				}

				switchOffActivityLED();
				_cache[i].flags &= ~CACHE_DIRTY;
			}

			return true;
		}
	}

	uint32_t max_entry = findExpirableEntry();

	// If the found block is dirty then flush it to disk
	if (_cache[max_entry].flags & CACHE_DIRTY) {
		switchOnActivityLED();
		writeBlockToDisk(_cache[max_entry].blockno, _cache[max_entry].data);
		switchOffActivityLED();
	}

	_cache[max_entry].blockno = block;
	_cache[max_entry].last_millis = millis();
	_cache[max_entry].flags = CACHE_VALID | CACHE_DIRTY;
	_cache[max_entry].hit_count = 0;
	memcpy(_cache[max_entry].data, data, 512);

	if (_cacheMode == CACHE_WRITETHROUGH) {
		switchOnActivityLED();

		if (!writeBlockToDisk(_cache[max_entry].blockno, _cache[max_entry].data)) {
			switchOffActivityLED();
			return false;
		}

		switchOffActivityLED();
		_cache[max_entry].flags &= ~CACHE_DIRTY;
	}

	return true;
}

void BlockDevice::setCacheMode(uint8_t mode) {
	_cacheMode = mode;

	if (_cacheMode == CACHE_WRITETHROUGH) {
		sync();
	}
}


uint32_t BlockDevice::findExpirableEntry() {
	uint32_t leastUsedCount = 0xFFFFFFFFUL;
	uint32_t oldestUsedTime = 0;
	uint32_t oldestUsedID = 0;

	// First scan through and find the least used count.
	for (int i = 0; i < SD_CACHE_SIZE; i++) {
		if (_cache[i].hit_count < leastUsedCount) {
			leastUsedCount = _cache[i].hit_count;
		}
	}

	uint32_t now = millis();

	// Now scan through and find the one with that count that is oldest.
	for (int i = 0; i < SD_CACHE_SIZE; i++) {
		if (_cache[i].hit_count == leastUsedCount) {
			if (now - _cache[i].last_millis > oldestUsedTime) {
				oldestUsedTime = now - _cache[i].last_millis;
				oldestUsedID = i;
			}
		}
	}

//	Serial.print("Expiring entry ");
//	Serial.print(oldestUsedID);
//	Serial.print(" with use count ");
//	Serial.print(leastUsedCount);
//	Serial.print(" and time difference ");
//	Serial.println(oldestUsedTime);
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
	Serial.println("Cache data:");
	Serial.println("ID     Block  Flags  Count  Time");
	char temp[80];
	uint32_t max_hit = 0;

	for (int i = 0; i < SD_CACHE_SIZE; i++) {
		if (_cache[i].hit_count > max_hit) {
			max_hit = _cache[i].hit_count;
		}
	}

	uint32_t now = millis();

	for (int hit = max_hit; hit >= 0; hit--) {
		for (int i = 0; i < SD_CACHE_SIZE; i++) {
			if (_cache[i].hit_count == (uint32_t)hit) {
				sprintf(temp, "%2d  %8lu  %02x     %5lu  %lu",
				        i,
				        _cache[i].blockno,
				        (unsigned int)_cache[i].flags,
				        _cache[i].hit_count,
				        (now - _cache[i].last_millis)
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

bool BlockDevice::loadPartitionTable() {
	uint8_t buffer[512];

	if (!readBlock(0, buffer)) {
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
