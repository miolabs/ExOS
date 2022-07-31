#ifndef DM36X_EMAC_MDIO_H
#define DM36X_EMAC_MDIO_H

typedef volatile struct
{
	unsigned long VERSION;
	unsigned long CONTROL;
	unsigned long ALIVE;
	unsigned long LINK;
	unsigned long LINKINTRAW;
	unsigned long LINKINTMASKED;
	unsigned long Reserved18;
	unsigned long Reserved1C;
	unsigned long USERINTRAW;
	unsigned long USERINTMASKED;
	unsigned long USERINTSET;
	unsigned long USERINTCLEAR;
	unsigned long Reserved30[20];
	unsigned long USERACCESS0;
	unsigned long USERPHYSEL0;
	unsigned long USERACCESS1;
	unsigned long USERPHYSEL1;
} EMAC_MDIO;

#define MDIO_CONTROL_IDLE (1<<31)
#define MDIO_CONTROL_ENABLE (1<<30)
#define MDIO_CONTROL_CLKDIV_MASK (0xFFFF)

#define MDIO_USERACCESS_GO (1<<31)
#define MDIO_USERACCESS_WRITE (1<<30)
#define MDIO_USERACCESS_ACK (1<<29)
#define MDIO_USERACCESS_REGADR_BIT (21)
#define MDIO_USERACCESS_PHYADR_BIT (16)
#define MDIO_USERACCESS_DATA_MASK (0xFFFF)


// prototypes
void emac_mdio_reset(int emac_clk);
int emac_mdio_read(int phy_id, int phy_reg);
void emac_mdio_write(int phy_id, int phy_reg, unsigned short phy_data);
int emac_mdio_init_phy(int phy_id);


#endif // DM36X_EMAC_MDIO_H
