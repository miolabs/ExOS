// LPC2K MCI Peripheral Support
// by Miguel Fides

#include "mci.h"
#include "cpu.h"
#include "dma.h"
#include <support/mci_hal.h>
#include <support/board_hal.h>
#include <kernel/panic.h>
#include <kernel/event.h>

static volatile unsigned long _int_status = 0;
static EXOS_EVENT _status_event;

void hal_mci_initialize()
{
	exos_event_create(&_status_event);

	// peripheral clock selection
	PCLKSEL1bits.PCLK_MCI = 3; // 0 = CCLK/4 (25MHz @100MHz)

	// module initialization
	LPC_SC->PCONP |= PCONP_PCSDC;	// power enable module
	MCIPowerBits.Control = MCI_POWER_OFF;
	for (volatile int time = 0; time < 1000000; time++);

	//NOTE: clk is pclk / (2 * (div + 1)) for low speed, then bypassed for high speed
	MCIClock = (23 & MCIClock_ClkDiv_MASK) | MCIClock_Enable | MCIClock_WideBus;
	for (volatile int time = 0; time < 10000; time++);

	MCICommand = 0;
	MCIDataCtrl = 0;			
	MCIPowerBits.Control = MCI_POWER_UP;	// turns on power on card, but bus remains inactive

	// wait some time for initialization
	for (volatile int time = 0; time < 10000; time++);
	MCIPowerBits.Control = MCI_POWER_ON;	// full enable 

	// register interrupt handler
	MCIClear = MCI_STATUS_TERM_MASK;	// clear termination flags
	MCIMask0 = MCI_STATUS_TERM_MASK;	// setup interrupt mask

	VIC_EnableIRQ(MCI_IRQn);
}

static void _debug(SD_ERROR error)
{
	// debug / breakpoint placeholder
}

void MCI_IRQHandler()
{
	// a termination flag has been asserted
	_int_status |= MCIStatus;
	MCIClear = _int_status & MCIMask0;
	exos_event_set(&_status_event);
}

static inline void _reset_status()
{
	_int_status = 0;
	MCIClear = MCIStatus & MCIMask0;	// clear static bits in status
	exos_event_reset(&_status_event);
}

static inline MCI_STATUS_BITS _wait_status()
{
	if (-1 == exos_event_wait(&_status_event, 1000))	//FIXME: set timeout according (longer) to hw set timeout
		_debug(SD_ERROR_TIMEOUT);
	return *(MCI_STATUS_BITS *)&_int_status;
}

static SD_ERROR _wait_resp(unsigned char cmd, MCI_WAIT_FLAGS flags)
{
	SD_ERROR result = SD_ERROR_TIMEOUT;	// timeout 
	while(1)
	{
		MCI_STATUS_BITS sta = _wait_status();

		if (sta.CmdTimeout || sta.DataTimeout) break;
		if ((flags & MCI_WAIT_CRC) 
			&& (sta.CmdCrcFail || sta.DataCrcFail))
		{
			result = SD_ERROR_BAD_CRC;
			break;
		}
		if ((flags & (MCI_WAIT_RESPONSE | MCI_WAIT_LONGRESP))
			&& (sta.CmdRespEnd || sta.CmdCrcFail))
		{
			if (flags & MCI_WAIT_CMD)	// cmd matching
			{
				unsigned char resp_cmd = MCIRespCmd & 0x3f;
				if ((cmd & 0x3f) != resp_cmd)
				{
					result = SD_ERROR_BAD_CMD;
					break;
				}
			}
			result = SD_OK;
			break;
		}
		else if (sta.CmdSent)
		{
			result = SD_OK;
			break;
		}
	}

	MCICommand = 0;
	MCIArgument = -1;
#ifdef DEBUG
	if (result != SD_OK) _debug(result);
#endif
	return result;
}

void hal_mci_set_speed(int high)
{
	if (high)
		MCIClock |= MCIClock_Bypass; // Enable bypass. MCLK driven to card bus output (MCICLK)
	else 
		MCIClock &= ~MCIClock_Bypass;
}

SD_ERROR hal_mci_send_cmd(unsigned char cmd, unsigned long arg, MCI_WAIT_FLAGS flags)
{
	_reset_status();

	// setup argument and issue command
	MCIArgument = arg;
	int cmd_reg = (cmd & 0x3f) 
		| ((flags & (MCI_WAIT_RESPONSE | MCI_WAIT_LONGRESP)) ? MCICommand_Response : 0)
		| ((flags & MCI_WAIT_LONGRESP) ? MCICommand_LongRsp : 0)
		| MCICommand_Enable;
	MCICommand = cmd_reg;

	SD_ERROR status = _wait_resp(cmd, flags);
	return status;
}

int hal_mci_read_resp(unsigned char *buf, int resp_bytes)
{
	int length = 0;
	unsigned char *resp = (unsigned char *)&MCIResponse0;
	while((resp_bytes - length) >= 4)
	{
		*buf++ = resp[length + 3];
		*buf++ = resp[length + 2];
		*buf++ = resp[length + 1];
		*buf++ = resp[length];
		length += 4;
	}
	return length;
}

unsigned long hal_mci_read_short_resp()
{
	return (unsigned long)MCIResponse0;
}

SD_ERROR hal_mci_read_data_blocks(unsigned char *buf, int count, MCI_BLOCK_SIZE size)
{
	int dma;
	if (dma_alloc_channels(&dma, 1))
	{
		_reset_status();
		
		DMA_CONFIG config = (DMA_CONFIG) {
			.Src = { .Burst = DMA_BURST_8, .Width = DMA_WIDTH_32BIT },
			.Dst = { .Burst = DMA_BURST_32, .Width = DMA_WIDTH_32BIT, .Increment = 1 },
			.Peripheral = DMA_P_MCI, .Flow = DMA_FLOW_P2M_SP };
		dma_channel_enable_fast(dma, (void *)&MCIFIFO, (void *)buf, 0,
			&config, NULL, 0);
		
		MCIDataTimer = 1000000;	// FIXME
		MCIDataLength = (1 << size) * count;
		MCIDataCtrl = MCI_DATACTRL_ENABLE_READ | MCIDataCtrl_DMAEnable |
			((size << MCIDataCtrl_BlockSize_BIT) & MCIDataCtrl_BlockSize_MASK);

		SD_ERROR error = SD_OK;
		while (error == SD_OK)
		{
			MCI_STATUS_BITS sta = _wait_status();
	
			if (sta.DataTimeout) error = SD_ERROR_TIMEOUT;
			else if (sta.DataCrcFail) error = SD_ERROR_BAD_CRC;
			else if (sta.RxOverrun) error = SD_ERROR_OVERFLOW;
			else if (sta.StartBitErr) error = SD_ERROR_BAD_TOKEN;
			if (sta.DataEnd)
			{
				break;
			}
		}

		if (error != SD_OK) 
		{
			MCIDataCtrl = 0;	
#ifdef DEBUG
			_debug(error);
#endif
		}
		dma_free_channel(dma);
		return error;
	}
	kernel_panic(KERNEL_ERROR_NO_HARDWARE_RESOURCES);
}

SD_ERROR hal_mci_write_data_blocks(unsigned char *buf, int count, MCI_BLOCK_SIZE size)
{
	int dma;
	if (dma_alloc_channels(&dma, 1))
	{
		_reset_status();
		
		DMA_CONFIG config = (DMA_CONFIG) {
			.Src = { .Burst = DMA_BURST_32, .Width = DMA_WIDTH_32BIT, .Increment = 1 },
			.Dst = { .Burst = DMA_BURST_8, .Width = DMA_WIDTH_32BIT },
			.Peripheral = DMA_P_MCI, .Flow = DMA_FLOW_M2P_DP };
		dma_channel_enable_fast(dma, (void *)buf, (void *)&MCIFIFO, 0,
			&config, NULL, 0);

		MCIDataTimer = 1000000000; // FIXME: use adjusted time value
		MCIDataLength = count << size;
		MCIDataCtrl = MCI_DATACTRL_ENABLE_WRITE | MCIDataCtrl_DMAEnable |
			((size << MCIDataCtrl_BlockSize_BIT) & MCIDataCtrl_BlockSize_MASK);
		
		SD_ERROR error = SD_OK;
		while (error == SD_OK)
		{
			MCI_STATUS_BITS sta = _wait_status();
	
			// FIXME: detect datatimer timeout 
			if (sta.DataTimeout) error = SD_ERROR_TIMEOUT;
			else if (sta.DataCrcFail) error = SD_ERROR_BAD_CRC;
			else if (sta.TxUnderrun) error = SD_ERROR_OVERFLOW; 
			if (sta.DataEnd)
			{
				if (!sta.DataBlockEnd) error = SD_ERROR_BAD_CRC;
				break;
			}
		}
	
		if (error != SD_OK) 
		{
			MCIDataCtrl = 0;
#ifdef DEBUG
			_debug(error);
#endif
		}
		dma_free_channel(dma);
		return error;
	}
	kernel_panic(KERNEL_ERROR_NO_HARDWARE_RESOURCES);
}



