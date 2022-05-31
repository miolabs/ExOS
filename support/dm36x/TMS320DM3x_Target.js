/******************************************************************************
  Target Script for DM365 (Tested with Leopardboard)

  Copyright (c) 2011 Rowley Associates Limited.

  This file may be distributed under the terms of the License Agreement
  provided with this software.

  THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE
  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ******************************************************************************/

load("targets/TMS320DM3x/ICEPick.js")

function ResetTAP()
{  
 // TAP reset
  TargetInterface.setTMS(1);
  TargetInterface.cycleTCK(6);
  TargetInterface.runTestIdle();

  ICEPickStart();      
  ICEPickAddTAP(1); // ETB
  ICEPickAddTAP(0); // ARM9
  ICEPickFinish();
}

function Connect()
{
  TargetInterface.selectDevice(0, 0, 0, 0);
  ResetTAP();
  TargetInterface.selectDevice(4, 6, 1, 1);
}

function ResetDebugInterface()
{

}

function Reset()
{ 
  // Set the vector catch register to trap on reset
  TargetInterface.setICEBreakerRegister(2, 1);

  // Select ICEPick
  TargetInterface.selectDevice(4+4, 0, 1+1, 0);
  
  ICEPickSYSTEMRESET();

  // Select CPU
  TargetInterface.selectDevice(4, 6, 1, 1);
 
  TargetInterface.waitForDebugState(1000);
	
  TargetInterface.message("Disable IRQ/FIQ...");
  TargetInterface.setRegister("cpsr", 0x400000D3);  // Set to supervisor mode, disable IRQ/FIQ
  cpsr = TargetInterface.getRegister("cpsr");
  TargetInterface.message("CPSR: 0x" + cpsr.toString(16));

  idcode = TargetInterface.executeMRC(0xEE000F00);
  TargetInterface.message("ID Code: 0x" + idcode.toString(16));

  TargetInterface.message("Flush Cache...");
  //MCR p15, 0, Rd, c7, c10, 4
  TargetInterface.executeMCR(0xEE070F8A, 0);

  TargetInterface.message("Disable TCM...");
  // MCR p15, 0, R0, c9, c1, 0 DTCM register
  //TargetInterface.executeMCR(0xEE090F01, 0x00010000 | (6 << 2) | 0);
  // MCR p15, 0, R0, c9, c1, 1 ITCM register
  TargetInterface.executeMCR(0xEE090F21, 0x00000000 | (6 << 2) | 0);

  TargetInterface.delay(200);

  TargetInterface.message("Disable Caches and MMU");
  // turn off the caches/MMU (I - ICache, C - DCache, M - MMU)
  i = TargetInterface.executeMRC(0xEE010F00);
  TargetInterface.executeMCR(0xEE010F00, i & ~(1<<0|1<<2|1<<12));

  TargetInterface.delay(200);
}

function PLLSetup(plli, prediv, mult, postdiv, ratio, count)
{
  switch(plli)
  {
    case 0: // PLLC1
      pll = 0x01c40800;
      break;
    case 1: // PLLC2
      pll = 0x01c40c00;
      break;
    default:
      TargetInterface.message("ERROR: spec PLLC doesn't exits");
      return;
  }

  TargetInterface.message("Resetting PLLC at 0x" + pll.toString(16) + "...");
  i = TargetInterface.peekWord(pll+0x100) & (1<<3); // PLLCTL
  TargetInterface.pokeWord(pll+0x100, i)  // clear all but PLLRST
  TargetInterface.delay(10);
  TargetInterface.pokeWord(pll+0x100, (1<<3));  // assert PLLRST
  TargetInterface.delay(10);
  TargetInterface.pokeWord(pll+0x100, 0);  // assert PLLRST

	TargetInterface.pokeWord(pll+0x114, prediv);
	TargetInterface.pokeWord(pll+0x128, (1<<15) | postdiv);
	TargetInterface.pokeWord(pll+0x110, mult);

	// The following sequence is required in PLLSECCTL for the multiplier and pre-divider values to take effect
	i = TargetInterface.peekWord(pll+0x108);
	i |= ((1<<17) | (1<<18) | (1<<16)); // TENABLE | TENABLEDIV | TINITZ
	TargetInterface.pokeWord(pll+0x108, i);
  i &= ~(1<<16); // deassert TINITZ
	TargetInterface.pokeWord(pll+0x108, i);
  i &= ~((1<<17) | (1<<18)); // deassert TENABLE | TENABLEDIV
	TargetInterface.pokeWord(pll+0x108, i);
  i |= (1<<16);
	TargetInterface.pokeWord(pll+0x108, i);

  // configure SYSCLKn
	for(i = 0; i < count; i++)
  {
    value = (1<<15) | ((ratio[i] - 1) & 0x1f);
    TargetInterface.message("Configuring SYSCLK" + (i+1) + "...");
    switch(i)
    {
      case 0: TargetInterface.pokeWord(pll+0x118, value); break; // PLLDIV1
      case 1: TargetInterface.pokeWord(pll+0x11c, value); break; // PLLDIV2
      case 2: TargetInterface.pokeWord(pll+0x120, value); break; // PLLDIV3
      case 3: TargetInterface.pokeWord(pll+0x160, value); break; // PLLDIV4
      case 4: TargetInterface.pokeWord(pll+0x164, value); break; // PLLDIV5
      case 5: TargetInterface.pokeWord(pll+0x168, value); break; // PLLDIV6
      case 6: TargetInterface.pokeWord(pll+0x16c, value); break; // PLLDIV7
      case 7: TargetInterface.pokeWord(pll+0x170, value); break; // PLLDIV8
      case 8: TargetInterface.pokeWord(pll+0x174, value); break; // PLLDIV9
    }
	}
	
 	// NOTE: DM365 requires all clocks to remain synchronized
  TargetInterface.pokeWord(pll+0x140, 0x1FF); // ALNCTL
	TargetInterface.pokeWord(pll+0x138, 1);  // PLLCMD
  TargetInterface.message("Waiting PLLSTAT...");
  while(TargetInterface.peekWord(pll+0x13c) & 1);	// PLLSTAT

  lock123 = (0x7 << 25);
  // wait PLL to lock
  switch(plli)
  {
    case 0: // PLLC1
      TargetInterface.message("Waiting PLLC1 lock...");
      while((TargetInterface.peekWord(0x01c40084) & lock123) != lock123);	// PLLC1_CONFIG
      break;  
    case 1: // PLLC2
      TargetInterface.message("Waiting PLLC2 lock...");
      while((TargetInterface.peekWord(0x01c40088) & lock123) != lock123);	// PLLC2_CONFIG
      break;  
  }
  
  // enable PLLC
  TargetInterface.pokeWord(pll+0x100, 1); // assert PLLCEN
  TargetInterface.message("PLLC Enabled");
} 

function VTPIOCalibration()
{
  VTPIOCR = 0x01c40074;
  vtpio = TargetInterface.peekWord(VTPIOCR);
  vtpio &= ~((1<<13) | (1<<7) | (1<<14) | (1<<6)); // CLRZ | LOCK | IOPWRDN | PWRDN
  TargetInterface.pokeWord(VTPIOCR, vtpio);
  TargetInterface.delay(10);
  vtpio |= (1<<13); // CLRZ
  TargetInterface.pokeWord(VTPIOCR, vtpio);
  while((TargetInterface.peekWord(VTPIOCR) & (1<<15)) == 0); // while READY == 0
  vtpio |= (1<<7);  // LOCK
  TargetInterface.pokeWord(VTPIOCR, vtpio);
  vtpio |= ((1<<14) | (1<<6));  // IOPWRDN | PWRDN
  TargetInterface.pokeWord(VTPIOCR, vtpio);    
}


function SRAMReset()
{
	Reset();
}

function DDR2Reset()
{
  Reset();

	// PLL freq = 500 MHz
	ratios_pllc1 = [
		10,	// SYSCLK1 = 50 MHz (-USB Ref)
		3,	// SYSCLK2 = 166.66 MHz (HDVICP+ARM9Core)
		3,	// SYSCLK3 = 166.66 MHz (HDVICP IF+MJCP IF)
		6,	// SYSCLK4 = 83.33 MHz (CFGBuf, VCLK, Peripheral)
		4,	// SYSCLK5 = 125 MHz (VPSS)
		10, // SYSCLK6 = (-VENC)
		3,	// SYSCLK7 = 166.66 MHz (DDR2)
		20, // SYSCLK8 = 25MHz (MMC/SD0)
		10, // SYSCLK9 = (DIV * CLKOUT2)
		]; 
  PLLSetup(0, 5, 125, 1, ratios_pllc1, 9);
  // NOTES: 
  // - ARM core will run OK at 166MHz (will take PLLC1SYSCLK2 by default)
  // - PLLC1SYSCLK2 is used by HDVICP and is close to its max speed (168MHz in DM365-210)
  // - ARM core can run at a higher speed (210 or faster) using PLLC2
  // - freq is not valid for USB (USB should use PLLC2)
  // - VPSS, VENC and MMC/SD0 remains UNTESTED
  

  // DDR2 mem spec
  tref = 7800; // 7.8 us
  trfc = 128; // 127.5 ns
  trp = 20;
  trcd = 20;
  twr = 15;
  tras = 45;
  trc = 65;
  trrd = 10;
  twtr = 10;
  trasmax = 70000; // 70us
  txp = 2; // cycles
  txsnr = 138;	// 137.5 ns
  txsrd = 200; // cycles
  trtp = 8;	// 7.5 ns
  tcke = 3; // cycles

	//system_select_ddr2_clock(PLLC1);
  periclk = TargetInterface.peekWord(0x01c40048)
  TargetInterface.pokeWord(0x01c40048, periclk & ~(1<<27)); // select PLLC1SYSCLK7 for DDR2
  tclk = 6; // 6ns = 166.666MHz of PLLC1SYSCLK7 (DDR_X2)

	SetPSCState(13, 3);  // enable DDR2 EMIF
	// VTP initialization
	VTPIOCalibration();

  emif = 0x20000000;  // address of the DDR2 config block

	// configure DDR PHY
	TargetInterface.pokeWord(emif+0x0e4, // DDR2PHYCR1
    (1<<7) | (1<<6) | 4); // EXT_STRBEN | PWRDNEN | CL + 1 - 1

	// configure PBBPR (see section 4.7)
	TargetInterface.pokeWord(emif+0x020, 0x7f); // PBBPR = 0x7f

	// register initialization (see section 2.14.1)
	TargetInterface.pokeWord(emif+0x008, // SDCR
    (1<<23)); // BOOTUNLOCK
	TargetInterface.pokeWord(emif+0x008, // SDCR
    (1<<15) | // TIMUNLOCK
		(4<<9) | // CL 
		(3<<4) | // IBANK = 8 
		(2<<0) | // PAGESIZE = 1024
    (1<<14) | (1<<22) | (1<<20) | (1<<17) | (1<<16)); // NM | DDR_DDQS | DDR2EN | DDREN | SDRAMEN
  
  TargetInterface.pokeWord(emif+0x010, // SDTIMR
    ((Math.ceil(trfc / tclk) - 1) << 25) | // T_RFC
		((Math.ceil(trp / tclk) - 1) << 22) | // T_RP
		((Math.ceil(trcd / tclk) - 1) << 19) | // T_RCD
		((Math.ceil(twr / tclk) - 1) << 16) | // T_WR
		((Math.ceil(tras / tclk) - 1) << 11) | // T_RAS
		((Math.ceil(trc / tclk) - 1) << 6) | // T_RC_BIT
		(((((trrd << 2) + (tclk << 1)) / (tclk << 2)) - 1) << 3) | // RRD_BIT
		((Math.ceil(twtr / tclk) - 1) << 0)); // T_WTR
  TargetInterface.pokeWord(emif+0x014, // SDTIMR2
    ((Math.round((trasmax * trfc) / 1000000) - 1) << 27) |  // T_RASMAX
		(((txp > tcke ? txp : tcke) - 1) << 25) | // T_XP
		((Math.ceil(txsnr / tclk) - 1) << 16) | // T_XSNR
		((txsrd - 1) << 8) | // T_XSRD
		((Math.ceil(trtp / tclk) - 1) << 5) | // T_RTP
		((tcke - 1) << 0)); // T_CKE
	sdcr = TargetInterface.peekWord(emif+0x008);
	TargetInterface.pokeWord(emif+0x008, sdcr & ~(1<<15)); // _TIMUNLOCK
	TargetInterface.pokeWord(emif+0x00c, Math.ceil(tref / tclk)); // SDRCR (RR)

	// reset and re-enable clocks
	SetPSCState(13, 1); // SYNC_RESET
	SetPSCState(13, 3); // ENABLE
}

function SetPSCState(module, state)
{
  PTCMD = 0x01c41120;
  PTSTAT = 0x01c41128;
  MDCTLn = 0x01c41a00 + (module << 2);

  while(TargetInterface.peekWord(PTSTAT) & 1);
	TargetInterface.pokeWord(MDCTLn, (1 << 8) | (state & 0x3));
	TargetInterface.pokeWord(PTCMD, 1);
  while(TargetInterface.peekWord(PTSTAT) & 1);
}



