/*------------------------------------------------------------------------
 * Copyright (c) 2009 by Nuvoton Technology Israel
 * All rights reserved.
 *------------------------------------------------------------------------*/
#include "CoreRegisters.h"
#include "TestMsgCore.h"
#include "AHB2PCI_defs.h"



int		i;

/*------------------------------------------------------------------------*/
/*-------------------   AHB to PCI Bridge transactions   -----------------*/
/*------------------------------------------------------------------------*/

BOOL AHB2PCI_IsLogEnable_g;

/*--------------------------------------------------------------------------------*/
/* Function:     AHB2PCI_Config_Read											  */
/*                                                                           	  */
/* Parameters:                                                               	  */
/*               address  - The configuration register address, to read from.	  */
/*               size	  -	The size of the read data.                       	  */
/*																			 	  */
/* Returns:      The read data.												 	  */
/*				 If the size is less than 4 bytes, data will be in the lower byte.*/
/* Side effects: None                                                        	  */
/* Description:                                                              	  */
/*               Configuration Read transaction through the AHB2PCI Bridge.  	  */
/*--------------------------------------------------------------------------------*/
DWORD AHB2PCI_Config_Read(DWORD address, BYTE size)
{
	DWORD readData, aligned_addr, aligned_data, offset = 0;
	DWORD cnt = 0;

	if ((size > sizeof(DWORD)) || (size == 3))
	{
		LogError ("AHB2PCI_Memory_Read error : Illegal data size (%d).\n", size);
		return(0xDEADBEEF);
	}

	// Set the Initial Byte offset
	// A non-zero offset cannot be used in Read transactions.
	WRITE_REG(RD_AHB_IBYTE, 0); 

	// Clear Status Register
	WRITE_REG(AHB_ISTATUS, READ_REG(AHB_ISTATUS));

	// Write the configuration space address to read
	aligned_addr = (address & ~0x3); // Bits 0,1 must be 0
	
	WRITE_REG(RD_PCI_ADDR, aligned_addr);

	// Initiate the transaction
	WRITE_REG(RD_CONTROL, (1 << 0) | (PCI_CFG_READ << 4) | (4 << 8)); // read size must be 4
	
	// Check that the transaction has ended
	while(!READ_BIT_REG(AHB_ISTATUS, RD_END))
	{
		cnt++;
		if(cnt > 1000)
		{
			LogError("AHB2PCI_Config_Read: Read transaction time out! AHB_ISTATUS = 0x%04X\n", READ_REG(AHB_ISTATUS));
			return(0xDEADBEEF);
		}		
	}

	// Read the data (the whole DWORD is read)
	readData = READ_REG(DATA_REG);

	offset = (address % 4);			 // the original address includes the offset in the lower two bits

	aligned_data = (readData >> (offset * 8)) & FIELD_MASK(0, size*8);

	if (AHB2PCI_IsLogEnable_g == TRUE)
		LogMessage("AHB2PCI_Config_Read: read from address=0x%X, size=%d, data=0x%X (DW data=0x%X)\n", address, size, aligned_data, readData);

	return(aligned_data);
} // *End of AHB2PCI_Config_Read*


/*--------------------------------------------------------------------------------*/
/* Function:     AHB2PCI_Config_Write											  */
/*                                                                           	  */
/* Parameters:                                                               	  */
/*               address  - The configuration register address, to write to.	  */
/*               size	  -	The size of the written data.                      	  */
/*               data	  -	The data to write, starting at the lower byte.     	  */
/*               iByte	  -	The offset (Initial Byte)							  */
/*																			 	  */
/* Returns:      The read data.												 	  */
/*				 If the size is less than 4 bytes, data will be in the lower byte.*/
/* Side effects: None                                                        	  */
/* Description:                                                              	  */
/*               Configuration Write transaction through the AHB2PCI Bridge.  	  */
/* Important:																	  */
/* The two lower bits of the address are ignored (alignment to DWORD), and the    */
/* offset is taken from	iByte register.											  */	
/* This function can get either a non-aligned address OR aligned address, and iByte.*/
/* The function takes care of "Data" - and puts it in the right place in the data */ 
/* reg(with iByte offset).														  */
/*--------------------------------------------------------------------------------*/

BOOL AHB2PCI_Config_Write(DWORD address, BYTE size, DWORD data, BYTE iByte)
{
	DWORD offset;
	DWORD cnt = 0;
	
	if ((size > sizeof(DWORD)) || (size == 3))
	{
		LogError("AHB2PCI_Config_Write error: Illegal data size\n");
		return(FALSE);
	}

	offset = (address % 4) + iByte;
	if (offset > 3)
	{
		LogError("Config_Write: Illegal offset(iByte): %d. Must be in range 0-3\n", offset);
		return(FALSE);
	}

	// Verify single data phase
	if((offset + size) > 4)
	{
		LogError("Config_Write: Offset + size exceeding 4 byte size.\n", offset);
		return(FALSE);
	}

	// Set the Initial Byte offset
	// A non-zero offset can be used only here, in Configuration Write transactions
	WRITE_REG(WR_AHB_IBYTE, offset);

	// Clear Status Register
	WRITE_REG(AHB_ISTATUS, READ_REG(AHB_ISTATUS));

	// Write the configuration space address to read (aligned to DWORD)
	WRITE_REG(WR_PCI_ADDR, (address & 0xFFFFFFFC));

	// Write the data
	// The data must start at the right byte in the Data Register, according to the offset
	WRITE_REG(DATA_REG, ((data & FIELD_MASK(0,size*8)) << (offset * 8)));

	// Initiate the transaction
	WRITE_REG(WR_CONTROL, (1 << 0) | (PCI_CFG_WRITE << 4) | (size << 8));

	// Check that the transaction has ended
	while(!READ_BIT_REG(AHB_ISTATUS, WR_END))
	{
		cnt++;
		if(cnt > 1000)
		{
			LogError("AHB2PCI_Config_Write: Write transaction time out! AHB_ISTATUS = 0x%04X\n", READ_REG(AHB_ISTATUS));
			return(FALSE);
		}
	}

	if (AHB2PCI_IsLogEnable_g == TRUE)
		LogMessage("AHB2PCI_Config_Write: wrote to address=0x%X, size=%d, data=0x%X, ibyte=%d\n", address, size, data, offset);

	return(TRUE);
} // *End of AHB2PCI_Config_Write*


/*--------------------------------------------------------------------------------*/
/* Function:     AHB2PCI_Memory_Read											  */
/*                                                                           	  */
/* Parameters:                                                               	  */
/*               address  - The configuration register address, to read from.	  */
/*               size	  -	The size of the read data.                       	  */
/*																			 	  */
/* Returns:      The read data.												 	  */
/*				 If the size is less than 4 bytes, data will be in the lower byte.*/
/* Side effects: None                                                        	  */
/* Description:                                                              	  */
/*               Memory Read transaction through the AHB2PCI Bridge.        	  */
/*--------------------------------------------------------------------------------*/

DWORD AHB2PCI_Memory_Read(DWORD address, BYTE size)
{
	DWORD readData, aligned_addr;
	BYTE cnt = 0;

	if (size != 4)
	{
		LogError("AHB2PCI_Memory_Read error : Illegal data size (%d). Read transfer must be 4 bytes.\n", size);
		return(0xDEADBEEF);
	}

	if (address % 4 != 0)
	{
		LogError("AHB2PCI_Memory_Read error : Illegal address (0x%X) must be aligned to DWORD.\n", size);
		return(0xDEADBEEF);
	}

	// Set the Initial Byte offset
	// A non-zero offset cannot be used in Memory Read transactions
	WRITE_REG(RD_AHB_IBYTE, 0); 
		
	// Clear Status Register
	WRITE_REG(AHB_ISTATUS, READ_REG(AHB_ISTATUS));
	
	// Write the configuration space address to read
	aligned_addr = (address & ~0x3); // Bits 0,1 must be 0
	
	WRITE_REG(RD_PCI_ADDR, aligned_addr);
	
	// Initiate the transaction
	WRITE_REG(RD_CONTROL, (1 << 0) | (PCI_MEM_READ << 4) | (size << 8)); // Read size must be 4
	
	// Check that the transaction has ended
	while(!READ_BIT_REG(AHB_ISTATUS, RD_END))
	{
		cnt++;
		if(cnt > 1000)
		{
			LogError("AHB2PCI_Memory_Read: Read transaction time out! AHB_ISTATUS = 0x%04X\n", READ_REG(AHB_ISTATUS));
			return(0xDEADBEEF);
		}		
	}

	readData = READ_REG(DATA_REG);

	if (AHB2PCI_IsLogEnable_g == TRUE)
		LogMessage("AHB2PCI_Memory_Read: Addr=0x%lX, Size=%d, Data=0x%lX\n", address, size, readData);

	return(readData);
	
} // *End of AHB2PCI_Memory_Read*


/*--------------------------------------------------------------------------------*/
/* Function:     if (!AHB2PCI_Memory_Write											  */
/*                                                                           	  */
/* Parameters:                                                               	  */
/*               address  - The configuration register address, to write to.	  */
/*               size	  -	The size of the written data.                      	  */
/*               data	  -	The data to write, starting at the lower byte.     	  */
/*																			 	  */
/* Returns:      TRUE if the transaction succeeded.							 	  */
/*				 If the size is less than 4 bytes, data will be in the lower byte.*/
/* Side effects: None                                                        	  */
/* Description:                                                              	  */
/*               Memory Write transaction through the AHB2PCI Bridge.  	          */
/* Notes:																		  */
/* In memory transactions, as opposed to configuration transactions, there is the */
/* regular "shifting" of the data, so the data can start at the lowest byte of	  */
/* the data register, and the address defines where to write the data (no need    */
/* for address alignment).														  */
/*--------------------------------------------------------------------------------*/

BOOL AHB2PCI_Memory_Write(DWORD address, BYTE size, DWORD data)
{
	DWORD cnt = 0;

	if ((size > sizeof(DWORD)) || (size == 3))
	{
		LogError("AHB2PCI_Memory_Write error : Illegal data size\n");
		return(FALSE);
	}

	// Set the Initial Byte offset
	// A non-zero offset cannot be used in Memory Write transactions
	WRITE_REG(WR_AHB_IBYTE, 0);
		
	// Clear Status Register
	WRITE_REG(AHB_ISTATUS, READ_REG(AHB_ISTATUS));
	
	// Write the configuration space address to read
	WRITE_REG(WR_PCI_ADDR, address);

	// Write the data
	WRITE_REG(DATA_REG, data);
	
	// Initiate the transaction
	WRITE_REG(WR_CONTROL, (1 << 0) | (PCI_MEM_WRITE << 4) | (size << 8));
	
	// Check that transaction has ended
	while(!READ_BIT_REG(AHB_ISTATUS, WR_END))
	{
		cnt++;
		if(cnt > 1000)
		{
			LogError("AHB2PCI_Memory_Write: Write transaction time out! AHB_ISTATUS = 0x%04X\n", READ_REG(AHB_ISTATUS));
			return(FALSE);
		}
	}

	if (AHB2PCI_IsLogEnable_g == TRUE)
		LogMessage ("AHB2PCI_Memory_Write: Addr=0x%lX, Size=%d, Data=0x%lX\n", address, size, data);
	
	return(TRUE);
} // *End of AHB2PCI_Memory_Write*
