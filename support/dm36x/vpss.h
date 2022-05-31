#ifndef DM36X_VPSS_H
#define DM36X_VPSS_H

#define VPSS_VENCCLKEN_ENABLE (1<<3) 
#define VPSS_DACCLKEN_ENABLE  (1<<4)

// VPSS module includes video backend & video frontend (VPFE, VPBE)
// Arg. 1 forces init; 0 will skip it if the vpps was already ON

void vpss_init(int hard);
int vpss_enable_clock(unsigned long mask);

#endif // DM36X_VPSS_H

