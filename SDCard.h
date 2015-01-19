/*! The SDCard class implements the BlockDevice interface to provide access to an
 *  SD card through the SPI interface.
 */

#ifndef _SDCARD_H
#define _SDCARD_H

#include <FileSystem.h>
#include <DSPI.h>

// Number of entries in the cache
// Minimum 1 (reeeeealy slow) - Maximum depends on memory of chip.
// Each entry is 528 bytes in size
#define SD_CACHE_SIZE 10

// How fast to run the SPI port
#define SD_SPI_SPEED 20000000UL



// SD Card Commands

#define CMD_GO_IDLE             0       /* CMD0 */
#define CMD_SEND_OP_MMC         1       /* CMD1 (MMC) */
#define CMD_SEND_IF_COND        8
#define CMD_SEND_CSD            9
#define CMD_SEND_CID            10
#define CMD_STOP                12
#define CMD_SEND_STATUS         13      /* CMD13 */
#define CMD_SET_BLEN            16
#define CMD_READ_SINGLE         17
#define CMD_READ_MULTIPLE       18
#define CMD_SET_BCOUNT          23      /* (MMC) */
#define CMD_SET_WBECNT          23      /* ACMD23 (SDC) */
#define CMD_WRITE_SINGLE        24
#define CMD_WRITE_MULTIPLE      25
#define CMD_SEND_OP_SDC         41      /* ACMD41 (SDC) */
#define CMD_APP                 55      /* CMD55 */
#define CMD_READ_OCR            58

// Timeout values

#define TIMO_WAIT_WDONE 400000
#define TIMO_WAIT_WIDLE 200000
#define TIMO_WAIT_CMD   100000
#define TIMO_WAIT_WDATA 30000
#define TIMO_READ       90000
#define TIMO_SEND_OP    8000
#define TIMO_CMD        7000
#define TIMO_SEND_CSD   6000
#define TIMO_WAIT_WSTOP 5000

#define DATA_START_BLOCK        0xFE    /* start data for single block */
#define STOP_TRAN_TOKEN         0xFD    /* stop token for write multiple */
#define WRITE_MULTIPLE_TOKEN    0xFC    /* start data for write multiple */


class SDCard : public BlockDevice {
private:
	DSPI 		*_spi;
	int 		_cs;
	int 		_miso;
	int			_mosi;
	int 		_sck;

	int			_cardType;
	size_t 		_sectors;

	
	
	void 		initializeSPIInterface();
	void 		spiSend(uint8_t);
	uint8_t 	spiReceive();
	void 		setSlowSPI();
	void		setFastSPI();
	void		deselectCard();
	void		selectCard();

	bool		readBlockFromDisk(uint32_t blockno, uint8_t *data);
	bool		writeBlockToDisk(uint32_t blockno, uint8_t *data);
	
	bool 		waitReady(int limit);
	int 		command(uint32_t cmd, uint32_t addr);


	
public:
				SDCard(DSPI &spi, int cs);
				SDCard(int miso, int mosi, int sck, int cs);
		
	bool 		initialize();
	bool 		eject();
	bool 		insert();
	
	size_t 	getCapacity();
};

#endif
