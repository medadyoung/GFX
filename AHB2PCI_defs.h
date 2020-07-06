#ifndef AHB2PCI_defs_h__
#define AHB2PCI_defs_h__

/*------------------------------------------------------------------------
 * Copyright (c) 2009 by Nuvoton Technology Israel
 * All rights reserved.
 *------------------------------------------------------------------------*/
#include "common.h"
#include "CoreRegisters.h"



#if 0
/*------------------------------------------------------------------------*/
/*-----------  Log Functions (To be implemented by user)  ----------------*/
/*------------------------------------------------------------------------*/
#define LogMessage(...)
#define LogError(...)
#define LogPass(...)
#define LogTitle(...)
#endif

/*------------------------------------------------------------------------*/
/*----------------			AHB2PCI Registers		   -------------------*/
/*------------------------------------------------------------------------*/
#define		AHB2PCI_BA			0xf0400000
#define		WR_PCI_ADDR			HW_DWORD(AHB2PCI_BA+0x0)
#define		WR_AHB_IBYTE		HW_DWORD(AHB2PCI_BA+0x4)
#define		WR_CONTROL			HW_DWORD(AHB2PCI_BA+0x8)
#define		RD_PCI_ADDR			HW_DWORD(AHB2PCI_BA+0x20)
#define		RD_AHB_IBYTE		HW_DWORD(AHB2PCI_BA+0x24)
#define		RD_CONTROL			HW_DWORD(AHB2PCI_BA+0x28)
#define		AHB_IMASK			HW_DWORD(AHB2PCI_BA+0x40)
#define		AHB_ISTATUS			HW_DWORD(AHB2PCI_BA+0x44)
#define		DATA_REG			HW_DWORD(AHB2PCI_BA+0x400)



/*------------------------------------------------------------------------*/
/*----------------   Constants and macros definitions  -------------------*/
/*------------------------------------------------------------------------*/

#define	GFX_IDSEL						0x00010000
#define	MB_IDSEL						0x00020000


typedef enum CommandType
{
	PCI_IO_READ = 2,
	PCI_IO_WRITE = 3,
	PCI_MEM_READ = 6,
	PCI_MEM_WRITE = 7,
	PCI_CFG_READ = 10,
	PCI_CFG_WRITE = 11
} CommandType;

// AHB_ISTATUS Register bits
#define	RAHB_ERR						11				// Read AHB Error
#define	RPAR_ERR						10				// Read Parity Error Detected
#define	RD_ABORT						9				// Read Transaction Aborted
#define	RD_END							8				// Read Transaction Ended
#define	WAHB_ERR						3				// Write AHB Error
#define	WPAR_ERR						2				// Write Parity Error Detected
#define	WR_ABORT						1				// Write Transaction Aborted
#define	WR_END							0				// Write Transaction Ended


/*------------------------------------------------------------------------*/
/*-----------------------   Function headers   ---------------------------*/
/*------------------------------------------------------------------------*/

extern BOOL AHB2PCI_IsLogEnable_g;
DWORD AHB2PCI_Config_Read(DWORD address, BYTE size);

BOOL AHB2PCI_Config_Write(DWORD address, BYTE size, DWORD data, BYTE iByte);

DWORD AHB2PCI_Memory_Read(DWORD address, BYTE size);

BOOL AHB2PCI_Memory_Write(DWORD address, BYTE size, DWORD data);

/*------------------------------------------------------------------------*/
/*---------------------------   Externs   --------------------------------*/
/*------------------------------------------------------------------------*/

#endif // AHB2PCI_defs_h__
