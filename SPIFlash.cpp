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
#define DEBUG 0

SPIFlash::SPIFlash(DSPI &spi, int cs) {
	_spi = &spi;
	_cs = cs;
}

void SPIFlash::initializeSPIInterface() {
    _spi->begin();
//    _spi->setSpeed(20000000UL);
	pinMode(_cs, OUTPUT);
	digitalWrite(_cs, HIGH);
}

bool SPIFlash::initialize() {
	initializeSPIInterface();
	return insert();
}

void SPIFlash::deselectChip() {
	digitalWrite(_cs, HIGH);
}

void SPIFlash::selectChip() {
	digitalWrite(_cs, LOW);
}

bool SPIFlash::insert() {

    uint8_t manufacturer;
    uint8_t device_type;
    uint8_t device_id;

    selectChip();
    _spi->transfer(0x9f);
    manufacturer = _spi->transfer(0xFF);
    device_type = _spi->transfer(0xFF);
    device_id = _spi->transfer(0xFF);
    deselectChip();


    switch (manufacturer) {
        case 0xbf: // Microchip
            switch (device_type) {
                case 0x26: // SST26VF064B
                    _sectors = 2048;
                    _blockSize = 4096;
                    break;

                default:
                    _sectors = 1;
                    _blockSize = 512;
                    break;
            }
            break;
        default:
            _sectors = 1;
            _blockSize = 512;
            break;
    }
    initCacheBlocks();


	if (!loadPartitionTable()) {
		return false;
	}
	errno = 0;
	return true;
}

bool SPIFlash::eject() {
	sync();
	return true;
}

bool SPIFlash::readBlockFromDisk(uint32_t block, uint8_t *data) {
    uint32_t startAddress = block * _blockSize;
    selectChip();
    _spi->transfer(0x03);
    _spi->transfer((startAddress >> 16) & 0xFF);
    _spi->transfer((startAddress >> 8) & 0xFF);
    _spi->transfer(startAddress & 0xFF);
    for (int i = 0; i < _blockSize; i++) {
        data[i] = _spi->transfer(0xFF);
    }
    deselectChip();

	return true;
}

bool SPIFlash::writeBlockToDisk(uint32_t block, uint8_t *data) {
    uint32_t startAddress = block * _blockSize;

    selectChip();
    _spi->transfer(0x06);
    deselectChip();

    selectChip();
    _spi->transfer(0x20);
    _spi->transfer((startAddress >> 16) & 0xFF);
    _spi->transfer((startAddress >> 8) & 0xFF);
    _spi->transfer(startAddress & 0xFF);
    deselectChip();
    waitReady();

    for (uint32_t b = 0; b < _blockSize; b += 256) {

        selectChip();
        _spi->transfer(0x06);
        deselectChip();

        selectChip();
        _spi->transfer(0x02);
        _spi->transfer(((startAddress + b) >> 16) & 0xFF);
        _spi->transfer(((startAddress + b) >> 8) & 0xFF);
        _spi->transfer((startAddress + b) & 0xFF);
        for (int i = 0; i < 256; i++) {
            _spi->transfer(data[b + i]);
        }
        deselectChip();
        waitReady();
    }
	return true;
}

void SPIFlash::waitReady() {

    selectChip();
    _spi->transfer(0x05);
    uint8_t status = _spi->transfer(0xFF);
    deselectChip();

    while (status & 0x80) {
        selectChip();
        _spi->transfer(0x05);
        uint8_t status = _spi->transfer(0xFF);
        deselectChip();
    }
}
