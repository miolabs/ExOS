#ifndef ONFI_H
#define ONFI_H

typedef enum
{
	ONFI_CMD1_READ = 0x00,
	ONFI_CMD1_COPYBACK_READ = 0x00,
	ONFI_CMD1_CHANGE_READ_COL = 0x05,
	ONFI_CMD1_READ_CACHE = 0x31,
	ONFI_CMD1_READ_CACHE_END = 0x3F,
	ONFI_CMD1_BLOCK_ERASE = 0x60,
	ONFI_CMD1_READ_STATUS = 0x70,
	ONFI_CMD1_READ_STATUS_ENH = 0x78,
	ONFI_CMD1_PAGE_PROGRAM = 0x80,
	ONFI_CMD1_COPYBACK_PROGRAM = 0x85,
	ONFI_CMD1_CHANGE_WRITE_COL = 0x85,
	ONFI_CMD1_READ_ID = 0x90,
	ONFI_CMD1_READ_PARAMETER_PAGE = 0xEC,
	ONFI_CMD1_READ_UNIQUE_ID = 0xED,
	ONFI_CMD1_GET_FEATURES = 0xEE,
	ONFI_CMD1_SET_FEATURES = 0xEF,
	ONFI_CMD1_RESET = 0xFF,
} ONFI_CMD1;

typedef enum
{
	ONFI_CMD2_READ = 0x30,
	ONFI_CMD2_COPYBACK_READ = 0x35,
	ONFI_CMD2_CHANGE_READ_COLUMN = 0xE0,
	ONFI_CMD2_READ_CACHE_ENH = 0x31,
	ONFI_CMD2_BLOCK_ERASE = 0xD0,
	ONFI_CMD2_BLOCK_ERASE_INTERLEAVED = 0xD1,
	ONFI_CMD2_PAGE_PROGRAM = 0x10,
	ONFI_CMD2_PAGE_PROGRAM_INTERLEAVED = 0x11,
	ONFI_CMD2_PAGE_CACHE_PROGRAM = 0x15,
	ONFI_CMD2_PAGE_COPYBACK_PROGRAM = 0x10,
	ONFI_CMD2_PAGE_COPYBACK_PROGRAM_INTERLEAVED = 0x11,
} ONFI_CMD2;

typedef struct __attribute__((__packed__))
{
	unsigned char Bytes[2];
} ONFI16;

#define F16TOH(a) (((a).Bytes[1] << 8) | ((a).Bytes[0]))
#define HTOF16(h) { (unsigned char)(h & 0xFF), (unsigned char)(h >> 8) }

typedef struct __attribute__((__packed__))
{
	unsigned char Bytes[4];
} ONFI32;

#define F32TOH(a) ( ((a).Bytes[3] << 24) | ((a).Bytes[2] << 16) | ((a).Bytes[1] << 8) | ((a).Bytes[0]))
#define HTOF32(h) { (unsigned char)(h), (unsigned char)(h >> 8), (unsigned char)(h >> 16), (unsigned char)(h >> 24) }

typedef struct
{
	struct
	{
		unsigned char Signature[4];
		unsigned short Revision;
		union
		{
			unsigned short Features;
			struct
			{
				unsigned BUS16:1;
				unsigned MultipleLUNOperations;
				unsigned NonSequentialPageProgramming;
				unsigned InterleavedOperations;
				unsigned NoEvenOddPageRestriction;
			} Bits;
		};
		unsigned short OptionalCommands;
	} Features;
	struct
	{
		unsigned char DeviceManufacturer[12];
		unsigned char DeviceModel[20];
		unsigned char JEDECManufacturer;
	} Manufacturer;
	struct __attribute__((__packed__))
	{
		ONFI32 DataBytesPerPage;
		ONFI16 SpareBytesPerPage;
		ONFI32 DataBytesPerPartialPage;
		ONFI16 SpareBytesPerPartialPage;
		ONFI32 PagesPerBlock;
		ONFI32 BlocksPerLUN;
		unsigned char LUNs;
		unsigned char AddrCycles;
		unsigned char BitsPerCell;
		ONFI16 MaxBadBlocksPerLUN;
		ONFI16 BlockEndurance;
		unsigned char GuaranteedBeginningBlocks;
		ONFI16 BlockEnduranceForGuaranteedBlocks;
		unsigned char ProgramsPerPage;
		unsigned char PartialProgrammingAttributes;
		unsigned char ECCBits;
		unsigned char InterleavedAddressBits;
		unsigned char InterleavedOperationAttributes;
	} MemoryOrganization;
	struct __attribute__((__packed__))
	{
		unsigned char IOPinCapacitance;
		ONFI16 TimingModeSupport;
		ONFI16 ProgramCacheTimingModeSupport;
		ONFI16 MaxPageProgramTime;
		ONFI16 MaxBlockEraseTime;
		ONFI16 MaxPageReadTime;
		ONFI16 MinChangeColumnSetupTime;
	} ElectricalParameters;
} ONFI_PARAMETER_PAGE;

typedef struct 
{
	unsigned char Id[4];
} ONFI_STATUS;

typedef struct
{
	void (* Cmd)(unsigned char cmd, unsigned addr_len, unsigned char *addr);
	int (* Write)(void *data, unsigned length);
	int (* Read)(void *data, unsigned length);
} ONFI_DRIVER;

int onfi_initialize(const ONFI_DRIVER *driver, ONFI_STATUS *param);

#endif // ONFI_H


