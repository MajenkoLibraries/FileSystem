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

SDCard::SDCard(DSPI &spi, int cs) {
	_spi = &spi;
	_cs = cs;
	_miso = -1;
	_mosi = -1;
	_sck = -1;
}

SDCard::SDCard(int miso, int mosi, int sck, int cs) {
	_spi = NULL;
	_cs = cs;
	_miso = miso;
	_mosi = mosi;
	_sck = sck;
}

void SDCard::initializeSPIInterface() {
	if (_spi != NULL) {
		_spi->begin();
	} else {
		pinMode(_mosi, OUTPUT);
		pinMode(_miso, INPUT);
		pinMode(_sck, OUTPUT);
		digitalWrite(_mosi, LOW);
		digitalWrite(_sck, HIGH);
	}
	pinMode(_cs, OUTPUT);
	digitalWrite(_cs, HIGH);
}

void SDCard::spiSend(uint8_t in) {
	if (_spi != NULL) {
		_spi->transfer(in);
	} else {
		digitalWrite(_mosi, HIGH);
		shiftOut(_mosi, _sck, MSBFIRST, in);
	}
}

extern "C" {
    extern uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
    extern void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, byte data);
}

uint8_t SDCard::spiReceive() {
	if (_spi != NULL) {
		return _spi->transfer(0xFF);
	}
	digitalWrite(_mosi,HIGH);
	digitalWrite(_miso, HIGH);
	return shiftIn(_miso, _sck, MSBFIRST);
}

bool SDCard::waitReady(int limit) {
	spiSend(0xFF);
	for (int i = 0; i < limit; i++) {
		if (spiReceive() == 0xFF) {
			errno = 0;
			return true;
		}
	}
	errno = EIO;
	return false;
}

int SDCard::command(uint32_t cmd, uint32_t addr) {
	int reply = 0;

	if (cmd != CMD_GO_IDLE) {
		if (!waitReady(TIMO_WAIT_CMD)) {
			return -1;
		}
	}

	spiSend(cmd | 0x40);
	spiSend(addr >> 24);
	spiSend(addr >> 16);
	spiSend(addr >> 8);
	spiSend(addr);

	if (cmd == CMD_GO_IDLE) {
		spiSend(0x95);
	} else if (cmd == CMD_SEND_IF_COND) {
		spiSend(0x87);
	} else {
		spiSend(0xFF);
	}

	for (int i = 0; i < TIMO_CMD; i++) {
		reply = spiReceive();
		if (!(reply & 0x80)) {
			return reply;
		}
	}

	if (cmd != CMD_GO_IDLE) {
		errno = EIO;
	} else {
		errno = 0;
	}
	return reply;
}

bool SDCard::initialize() {
	initializeSPIInterface();
	return insert();
}

void SDCard::setSlowSPI() {
	if (_spi != NULL) {
		_spi->setSpeed(250000);
	}
}

void SDCard::setFastSPI() {
	if (_spi != NULL) {
		_spi->setSpeed(SD_SPI_SPEED); // Todo: Make this a configurable variable
	}
}

void SDCard::deselectCard() {
	digitalWrite(_cs, HIGH);
}

void SDCard::selectCard() {
	digitalWrite(_cs, LOW);
}

bool SDCard::insert() {

	int reply;
	uint8_t buffer[512];
	int timeout = 4;
	int i, n;
	uint32_t csize;
	
	setSlowSPI();

	do {
		deselectCard();
		for (i = 0; i < 10; i++) {
			spiSend(0xFF);
		}
		selectCard();
		timeout--;
		reply = command(CMD_GO_IDLE, 0);
	} while ((reply != 0x01) && (timeout != 0));

	deselectCard();
	if (reply != 0x01) {
		errno = EIO;
		return false;
	}

	selectCard();
	reply = command(CMD_SEND_IF_COND, 0x1AA);
	if (reply * 4) {
		deselectCard();
		_cardType = 1;
	} else {
		buffer[0] = spiReceive();
		buffer[1] = spiReceive();
		buffer[2] = spiReceive();
		buffer[3] = spiReceive();
		deselectCard();

		if (buffer[3] != 0xAA) {
			errno = ENXIO;
			return false;
		}
		_cardType = 2;
	}

    for (i=0; ; i++)
    {
    	selectCard();
        command(CMD_APP, 0);
        reply = command(CMD_SEND_OP_SDC, (_cardType == 2) ? 0x40000000 : 0);
        deselectCard();

        if (reply == 0)
            break;
        if (i >= TIMO_SEND_OP)
        {
        	errno = EIO;
        	return false;
        }
    }

    if (_cardType == 2) {
    	selectCard();
    	reply = command(CMD_READ_OCR, 0);
    	if (reply != 0) {
    		deselectCard();
    		errno = EIO;
    		return false;
    	}
		buffer[0] = spiReceive();
		buffer[1] = spiReceive();
		buffer[2] = spiReceive();
		buffer[3] = spiReceive();
		deselectCard();

    	if ((buffer[0] & 0xC0) == 0xC0) {
    		_cardType = 3;
    	}
    }

    setFastSPI();

	selectCard();
	reply = command(CMD_SEND_CSD, 0);
	if (reply != 0) {
		deselectCard();
		errno = EIO;
		return false;
	}

	for (i = 0; ; i++) {
		reply = spiReceive();
		if (reply == DATA_START_BLOCK) {
			break;
		}
		if (i >= TIMO_SEND_CSD) {
			deselectCard();
			errno = EIO;
			return false;
		}
	}

    for (i = 0; i < 16; i++) {
    	buffer[i] = spiReceive();
    }
    
    spiReceive();
    spiReceive();
    deselectCard();

    switch (buffer[0] >> 6) {
    	case 1:
    		csize = buffer[9] + (buffer[8] << 8) + 1;
    		_sectors = csize << 10;
    		break;
    	case 0:
    		n = (buffer[5] & 0x0F) + ((buffer[10] & 0x80) >> 7) + ((buffer[9] & 0x03) << 1) + 2;
    		csize = (buffer[8] >> 6) + (buffer[7] << 2) + ((buffer[6] & 0x03) << 10) + 1;
    		_sectors = csize << (n - 9);
    		break;
    	default:
    		errno = EINVAL;
    		return false;
    }

	if (!loadPartitionTable()) {
		return false;
	}


    
	errno = 0;
	return true;
}

bool SDCard::eject() {
	sync();
	return true;
}

bool SDCard::readBlockFromDisk(uint32_t block, uint8_t *data) {
	int reply;
	int i;

	selectCard();
	if (_cardType != 3) {
		block <<= 9;
	}
	reply = command(CMD_READ_MULTIPLE, block);
	if (reply != 0) {
		deselectCard();
		errno = EIO;
		return false;
	}

	for (i = 0; ; i++) {
		reply = spiReceive();
		if (reply == DATA_START_BLOCK) {
			break;
		}
		if (i >= TIMO_READ) {
			deselectCard();
			errno = EIO;
			return false;
		}
	}

	if (_spi != NULL) {
		_spi->transfer(512, 0xFF, data);
	} else {
		for (i = 0; i < 512; i++) {
			data[i] = spiReceive();
		}
	}
	spiReceive();
	spiReceive();

	command(CMD_STOP, 0);
	deselectCard();
	
	return true;
}

bool SDCard::writeBlockToDisk(uint32_t block, uint8_t *data) {
    int reply, i;

	selectCard();
	command(CMD_APP, 0);
	reply = command(CMD_SET_WBECNT, 1);
	if (reply != 0) {
		errno = EIO;
		return false;
	}

    if (_cardType != 3) block <<= 9;
    reply = command(CMD_WRITE_MULTIPLE, block);
    if (reply != 0)
    {
		errno = EIO;
		return false;
    }
    deselectCard();

	selectCard();
	waitReady(TIMO_WAIT_WDATA);

	spiSend(WRITE_MULTIPLE_TOKEN);

	if (_spi != NULL) {
		_spi->transfer(512, data);
	} else {
		for (i = 0; i < 512; i++) {
			spiSend(data[i]);
		}
	}
	spiSend(0xFF);
	spiSend(0xFF);
	reply = spiReceive();
	if ((reply & 0x1F) != 0x05) {
		errno = EIO;
		return false;
	}
	waitReady(TIMO_WAIT_WDONE);
	deselectCard();
	selectCard();
	waitReady(TIMO_WAIT_WSTOP);
	spiSend(STOP_TRAN_TOKEN);
	waitReady(TIMO_WAIT_WIDLE);
	deselectCard();
	return true;
}

size_t SDCard::getCapacity() {
	return _sectors;
}
