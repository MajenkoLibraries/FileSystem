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

/*! The SPIFlash class implements the BlockDevice interface to provide access to 
 *  a SPI connected Flash chip.
 */

#ifndef _SPIFLASH_H
#define _SPIFLASH_H

#include <FileSystem.h>
#include <DSPI.h>

class SPIFlash : public BlockDevice {
private:
	DSPI 		*_spi;
	int 		_cs;

    // This is the number of "pages" in the flash.
	size_t 		_sectors;
    // The total size is the number of pages multiplied by the size of a page.

	
	void 		initializeSPIInterface();
	void		deselectChip();
	void		selectChip();

	bool		readBlockFromDisk(uint32_t blockno, uint8_t *data);
	bool		writeBlockToDisk(uint32_t blockno, uint8_t *data);
	
	int 		command(uint32_t cmd, uint32_t addr);

    void        waitReady();
	
public:
				SPIFlash(DSPI &spi, int cs);
		
	bool 		initialize();
	bool 		eject();
	bool 		insert();
	
	size_t 	getCapacity() { return _sectors; }
};

#endif
