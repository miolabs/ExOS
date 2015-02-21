#ifndef LPC17_CPU_H
#define LPC17_CPU_H

#define LPC2387 2387
#define LPC2468 2468
#define LPC2478 2478

#ifdef __cplusplus
  #define   __I     volatile             /*!< Defines 'read only' permissions                 */
#else
  #define   __I     volatile const       /*!< Defines 'read only' permissions                 */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions                */
#define     __IO    volatile             /*!< Defines 'read / write' permissions              */

extern unsigned long SystemCoreClock;     /*!< System Clock Frequency (Core Clock)  */

/*------------- System Control (SC) ------------------------------------------*/
typedef struct
{
  __IO unsigned long FLASHCFG;               /* Flash Accelerator Module           */
       unsigned long RESERVED0[31];
  __IO unsigned long PLL0CON;                /* Clocking and Power Control         */
  __IO unsigned long PLL0CFG;
  __I  unsigned long PLL0STAT;
  __O  unsigned long PLL0FEED;
       unsigned long RESERVED1[4];
  __IO unsigned long PLL1CON;
  __IO unsigned long PLL1CFG;
  __I  unsigned long PLL1STAT;
  __O  unsigned long PLL1FEED;
       unsigned long RESERVED2[4];
  __IO unsigned long PCON;
  __IO unsigned long PCONP;
       unsigned long RESERVED3[15];
  __IO unsigned long CCLKCFG;
  __IO unsigned long USBCLKCFG;
  __IO unsigned long CLKSRCSEL;
       unsigned long RESERVED4[12];
  __IO unsigned long EXTINT;                 /* External Interrupts                */
       unsigned long RESERVED5;
  __IO unsigned long EXTMODE;
  __IO unsigned long EXTPOLAR;
       unsigned long RESERVED6[12];
  __IO unsigned long RSID;                   /* Reset                              */
       unsigned long RESERVED7[7];
  __IO unsigned long SCS;                    /* Syscon Miscellaneous Registers     */
  __IO unsigned long IRCTRIM;                /* Clock Dividers                     */
  __IO unsigned long PCLKSEL0;
  __IO unsigned long PCLKSEL1;
       unsigned long RESERVED8[4];
  __IO unsigned long USBIntSt;               /* USB Device/OTG Interrupt Register  */
  __IO unsigned long DMAREQSEL;
  __IO unsigned long CLKOUTCFG;              /* Clock Output Configuration         */
 } LPC_SC_TypeDef;

extern LPC_SC_TypeDef *LPC_SC;

#define PCONP_PCTIM0	0x00000002
#define PCONP_PCTIM1	0x00000004
#define PCONP_PCUART0	0x00000008
#define PCONP_PCUART1	0x00000010
#define PCONP_PCPWM0	0x00000020
#define PCONP_PCPWM1	0x00000040
#define PCONP_PCI2C0	0x00000080
#define PCONP_PCSPI		0x00000100
#define PCONP_PCRTC		0x00000200
#define PCONP_PCSSP1	0x00000400
#define PCONP_PCEMC		0x00000800
#define PCONP_PCADC		0x00001000
#define PCONP_PCAN1		0x00002000
#define PCONP_PCAN2		0x00004000
#define PCONP_PCAN3		0x00008000
#define PCONP_PCAN4		0x00010000
#define PCONP_PCI2C1	0x00080000
#define PCONP_PCSSP0	0x00200000
#define PCONP_PCTIM2	0x00400000
#define PCONP_PCTIM3	0x00800000
#define PCONP_PCUART2	0x01000000
#define PCONP_PCUART3	0x02000000
#define PCONP_PCI2C2	0x04000000
#define PCONP_PCI2CS	0x08000000
#define PCONP_PCSDC		0x10000000
#define PCONP_PCGPDMA	0x20000000
#define PCONP_PCENET	0x40000000
#define PCONP_PUSB		0x80000000

#define CCLKCFG_CCLKSEL_MASK 0x000000FF

#define SCS_GPIOM	0x00000001
#define SCS_MCIPWR	0x00000008

typedef struct
{
	unsigned PCLK_WDT:2;
	unsigned PCLK_TIMER0:2;
	unsigned PCLK_TIMER1:2; 
	unsigned PCLK_UART0:2;

	unsigned PCLK_UART1:2;
	unsigned :2;
	unsigned PCLK_PWM1:2;
	unsigned PCLK_I2C0:2;

	unsigned PCLK_SPI:2;
	unsigned :2;
	unsigned PCLK_SSP1:2;
	unsigned PCLK_DAC:2;

	unsigned PCLK_ADC:2;
	unsigned PCLK_CAN1:2;
	unsigned PCLK_CAN2:2;
	unsigned PCLK_ACF:2;
} _PCLKSEL0;

#define PCLKSEL0bits (*(_PCLKSEL0 *) &LPC_SC->PCLKSEL0)

typedef struct
{
	unsigned PCLK_BAT_RAM:2;
	unsigned PCLK_GPIO:2;
	unsigned PCLK_PCB:2; 
	unsigned PCLK_I2C1:2;

	unsigned :2;
	unsigned PCLK_SSP0:2;
	unsigned PCLK_TIMER2:2;
	unsigned PCLK_TIMER3:2;

	unsigned PCLK_UART2:2;
	unsigned PCLK_UART3:2;
	unsigned PCLK_I2C2:2;
	unsigned PCLK_I2S:2;
	
	unsigned PCLK_MCI:2;
	unsigned :2;
	unsigned PCLK_SYSCON:2;
	unsigned :2;
} _PCLKSEL1;

#define PCLKSEL1bits (*(_PCLKSEL1 *) &LPC_SC->PCLKSEL1)


typedef struct
{
	__I  unsigned long IRQStatus;
	__I  unsigned long FIQStatus;
	__I  unsigned long RawIntr;
	__IO unsigned long IntSelect;
	__IO unsigned long IntEnable;
	__O  unsigned long IntEnClr;
	__IO unsigned long SoftInt;
	__O  unsigned long SoftIntClear;
	__IO unsigned long Protection;
	__IO unsigned long PriorityMask;
	unsigned long Reserved1[55];
	__IO unsigned long VectAddr[32];
	unsigned long Reserved2[33];
	__IO unsigned long VectPriority[32];
	unsigned long Reserved3[801];
	__IO unsigned long Address;
} LPC_VIC_TypeDef;

extern LPC_VIC_TypeDef *LPC_VIC;

typedef enum
{
	TIMER0_IRQn = 4,
    TIMER1_IRQn = 5,
	UART0_IRQn = 6,
	UART1_IRQn = 7,
    ENET_IRQn = 21,
	USB_IRQn = 22,
	MCI_IRQn = 24,
	DMA_IRQn = 25,
    TIMER2_IRQn = 26,
    TIMER3_IRQn = 27,
	UART2_IRQn = 28,
	UART3_IRQn = 29,
} IRQn_Type;


// prototypes

int cpu_cclk() __attribute__((deprecated)); 
int cpu_pclk(int cclk, int setting);

void VIC_EnableIRQ(IRQn_Type irq);
void VIC_DisableIRQ(IRQn_Type irq);

#endif // LPC17_CPU_H
