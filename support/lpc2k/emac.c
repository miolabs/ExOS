// LPC23xx/LPC24xx Ethernet/PHY Support
// by Miguel Fides

#include "emac.h"
#include "emac_mem.h"
#include "pincon.h"
#include "cpu.h"

LPC_EMAC_TypeDef *LPC_EMAC = (LPC_EMAC_TypeDef *)0xFFE00000;

static void *_handler_state;
static void (*_handler)(void *) = (void *)0;

static void _write_phy(PHYREG reg, unsigned short value);
static unsigned short _read_phy(PHYREG reg);
static const PHY_HANDLER _phy = { .Read = _read_phy, .Write = _write_phy };

void _wait(int time)
{
	for (int volatile count = 0; count < time; count++);
}

int emac_initialize(ETH_MAC *mac, void (*handler)(void *), void *state)
{
	_handler = handler;
	_handler_state = state;

	// PCONP enable
	LPC_SC->PCONP |= PCONP_PCENET;
	VIC_DisableIRQ(ENET_IRQn);

	// clock selection
	
	// pinsel for RMII
	PINSEL2bits.P1_0 = 1;	// ENET_TXD0
	PINSEL2bits.P1_1 = 1;	// ENET_TXD1
	PINSEL2bits.P1_4 = 1;	// ENET_TX_EN

	if (LPC_EMAC->Module_ID == OLD_EMAC_MODULE_ID)
	{
		// On Rev '-' P1.6 MUST BE SET (See ERRATA Ethernet.1)
		PINSEL2bits.P1_6 = 1;	// ENET_TX_CLK
	}

	PINSEL2bits.P1_8 = 1;	// ENET_CRS
	PINSEL2bits.P1_9 = 1;	// ENET_RXD0
	PINSEL2bits.P1_10 = 1;	// ENET_RXD1
	PINSEL2bits.P1_14 = 1;	// ENET_RX_ER
	PINSEL2bits.P1_15 = 1;	// ENET_REF_CLK

	PINSEL3bits.P1_16 = 1;	// ENET_MDC
	PINSEL3bits.P1_17 = 1;	// ENET_MDIO

	// module initialization
	LPC_EMAC->MAC1 = MAC1_RESET_TX | MAC1_RESET_MCS_TX | 
		MAC1_RESET_RX | MAC1_RESET_MCS_RX |
		MAC1_SIM_RESET | MAC1_SOFT_RESET;
	LPC_EMAC->Command = MAC_COMMAND_REG_RESET | MAC_COMMAND_TX_RESET | MAC_COMMAND_RX_RESET;

	_wait(1000);

	LPC_EMAC->MAC1 = MAC1_PASS_ALL;
	LPC_EMAC->MAC2 = MAC2_CRC_ENABLE | MAC2_PAD_ENABLE;
	LPC_EMAC->MAXF = ETH_MAX_FRAME_LEN;
	LPC_EMAC->CLRT = MAC_CLRT_DEFAULT;
	LPC_EMAC->IPGR = MAC_IPGR_DEFAULT;

	// initialize RMII interface
	LPC_EMAC->MCFG = MAC_MCFG_CLK_DIV20 | MAC_MCFG_RESET_MII;
	_wait(1000);
	LPC_EMAC->MCFG = MAC_MCFG_CLK_DIV20; // 72MHz / 20 = 3.6MHz?
	LPC_EMAC->Command = MAC_COMMAND_RMII | 
		MAC_COMMAND_PASS_RUNT_FRAME | MAC_COMMAND_PASS_RX_FILTER;

	// init phy
	if(!phy_reset(&_phy))
		return 0;

	// configure hw address 
	LPC_EMAC->SA0 = (mac->Byte[1] << 8) | mac->Byte[0];
	LPC_EMAC->SA1 = (mac->Byte[3] << 8) | mac->Byte[2];
	LPC_EMAC->SA2 = (mac->Byte[5] << 8) | mac->Byte[4];

	// initialize Tx and Rx DMA Descriptors
	emac_mem_initialize();
	
	// setup rx filter
	LPC_EMAC->RxFilterCtrl = MAC_RXFC_BROADCAST_EN | MAC_RXFC_PERFECT_EN;
	
	// enable interrupts
	LPC_EMAC->IntEnable = MAC_INT_RX_DONE | MAC_INT_TX_DONE;
	LPC_EMAC->IntClear = 0xFFFF;
	VIC_EnableIRQ(ENET_IRQn);

	// configure PHY
	phy_restart_neg(&_phy);
	return 1;
}

ETH_LINK emac_init_link()
{
	ETH_LINK mode = phy_link_state(&_phy);
	if (mode != ETH_LINK_NONE)
	{
		if (mode & ETH_LINK_FULL_DUPLEX)
		{
			// link is full duplex
			LPC_EMAC->MAC2 |= MAC2_FULL_DUPLEX;
			LPC_EMAC->Command |= MAC_COMMAND_FULL_DUPLEX;
			LPC_EMAC->IPGT = MAC_IPGT_FULL_DUPLEX;
		}
		else
		{
			// link is half duplex
			LPC_EMAC->IPGT = MAC_IPGT_HALF_DUPLEX;
		}
	
		// configure PHY support (speed)
		LPC_EMAC->SUPP = (mode & ETH_LINK_100M) ? MAC_SUPP_SPEED : 0;

		// enable receive and transmit mode of MAC Ethernet core
		LPC_EMAC->Command |= (MAC_COMMAND_RX_ENABLE | MAC_COMMAND_TX_ENABLE);
		LPC_EMAC->MAC1 |= MAC1_RECEIVE_ENABLE;
	}
	return mode;
}

static void _write_phy(PHYREG reg, unsigned short value)
{
	LPC_EMAC->MADR = PHY_BASE_ADDR | reg;
	LPC_EMAC->MWTD = value;
	
	int timeout = 0;
	while (LPC_EMAC->MIND & MAC_MIND_BUSY)
	{
		if (++timeout > 300000) break;
	}
}

static unsigned short _read_phy(PHYREG reg)
{
	LPC_EMAC->MADR = PHY_BASE_ADDR | reg;
	LPC_EMAC->MCMD = MAC_MCMD_READ;
	
	int timeout = 0;
	while (LPC_EMAC->MIND & MAC_MIND_BUSY)
	{
		if (++timeout > 300000)
		{
			LPC_EMAC->MCMD = 0;
			return 0;
		}
	}
	LPC_EMAC->MCMD = 0;
	return LPC_EMAC->MRDD;
}

void ENET_IRQHandler()
{
	int status = LPC_EMAC->IntStatus;
	if (status & MAC_INT_RX_DONE)
	{
		LPC_EMAC->IntClear = MAC_INT_RX_DONE;
		if (_handler) _handler(_handler_state);
	}
	if (status & MAC_INT_TX_DONE)
	{
		emac_mem_tx_handler();
	}
	// clear int flags
	LPC_EMAC->IntClear = status;
}
