/*--------------------------------------------------------------------------
 * Copyright (c) 2009-2010 by Nuvoton Technology Israel
 * All rights reserved.
 *--------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/*-------------------------   Include files   ----------------------------*/
/*------------------------------------------------------------------------*/
#include <stdlib.h> // using rand()
#include <string.h>
#include "common.h"
#include "CoreRegisters.h"
#include "TestMsgCore.h"
#include "AHB2PCI_defs.h"
#include "GraphicsInit.h"
#include "Utility.h"

BOOL GFX_IsPllDebug_g;

/*------------------------------------------------------------------------*/
/*----------------------   GFX PCI Interface Regs ------------------------*/
/*------------------------------------------------------------------------*/
// GFX PCI registers (GFX Configuration space registers):
#define GFX_VENDOR_ID			0x0
#define GFX_DEVICE_ID			0x2
#define GFX_COMMAND_REG			0x4
#define GFX_STATUS_REG			0x6
#define GFX_DEV_REV_ID			0x8
#define GFX_PCI_CLASS_CODE		0x9
#define GFX_PCI_CACHE_LINE		0xC
#define GFX_PCI_LATENCY_TMR		0xD
#define GFX_PCI_HEADER_TYPE		0xE
#define GFX_BAR2				0x10
#define GFX_BAR1				0x14
#define GFX_SUB_VENDOR_ID		0x2C
#define GFX_ROMBASE				0x30
#define GFX_CAP_PTR				0x34
#define GFX_INT_LINE			0x3C
#define GFX_INT_PIN				0x3D
#define GFX_MIN_GRANT			0x3E
#define GFX_OPT1				0x40
#define GFX_MGA_INDEX			0x44
#define GFX_MGA_DATA			0x48
#define GFX_SIDW				0x4C
#define GFX_OPT2				0x50
#define GFX_PM_ID				0xDC
#define GFX_PM_CSR				0xE0

// Graphics Configuration register reset values
#define	GFX_VEN_DEV_ID_RV		0x0536102B		// 102Bh - Matrox; 0536h - Ge200W3

#define GFX_VENDOR_ID_RV		0x102B
#define GFX_DEVICE_ID_RV		0x0536
#define GFX_COMMAND_REG_RV		0
#define GFX_STATUS_REG_RV		0x2B0
#define GFX_DEV_REV_ID_RV		0
//#define GFX_PCI_CLASS_CODE_RV	//not final
#define GFX_PCI_CACHE_LINE_RV	0
#define GFX_PCI_LATENCY_TMR_RV	0
#define GFX_PCI_HEADER_TYPE_RV	0
#define GFX_BAR2_RV				0x8
#define GFX_BAR1_RV				0
//#define GFX_SUB_VENDOR_ID_RV	//not final
#define GFX_ROMBASE_RV			0
#define GFX_CAP_PTR_RV			0xDC
#define GFX_INT_LINE_RV			0xFF
#define GFX_INT_PIN_RV			0x1
#define GFX_MIN_GRANT_RV		0x10
// #define GFX_OPT_RV			//strap-dependent		
#define GFX_MGA_INDEX_RV		0
//#define GFX_MGA_DATA_RV		//undefined
#define GFX_SIDW_RV				0xD0031050
#define GFX_OPT2_RV				0xB000
#define GFX_PM_ID_RV			0x230001
#define GFX_PMCSR_RV			0

// GFX Registers
#define PALWTADD				(MGABASE1 + 0x3C00)
#define PALDATA					(MGABASE1 + 0x3C01)
#define X_DATAREG				(MGABASE1 + 0x3C0A)
#define MISC					(MGABASE1 + 0x1FC2)
#define SEQ						(MGABASE1 + 0x1FC4)
#define GCTL					(MGABASE1 + 0x1FCE)
#define CRTC					(MGABASE1 + 0x1FD4)
#define CRTCEXT					(MGABASE1 + 0x1FDE)
#define INSTS1					(MGABASE1 + 0x1FDA)


/*------------------------------------------------------------------------*/
/*----------------------    General Poleg Regs    ------------------------*/
/*------------------------------------------------------------------------*/
/*----- System Global Control -----*/
#define GCR_BA				0xF0800000
#define INTCR3				HW_DWORD((GCR_BA+0x09C))		// Integration Control 3

#define	GFXI_BA				0xF000E000
#define	HVCNTL				HW_BYTE(GFXI_BA + 0x10)			//	Horizontal Visible Counter Low (HVCNTL)	
#define	HVCNTH				HW_BYTE(GFXI_BA + 0x14)			//	Horizontal Visible Counter High (HVCNTH)
#define	COLDEP				HW_BYTE(GFXI_BA + 0x50)			//	Color Depth (COLDEP)					





/*------------------------------------------------------------------------*/
/*----------------------   Variable definitions   ------------------------*/
/*------------------------------------------------------------------------*/

	

DWORD FB_BA;		// Frame buffer base address
DWORD MGABASE1 = 0;	// GFX Memory space address

typedef enum GFX_MEMORY_MAP_E
{
	GMMAP_0700_000 = 0,
	GMMAP_0F00_000 = 1,
	GMMAP_1F00_000 = 2,
	GMMAP_3F00_000 = 3,
	GMMAP_7F00_000 = 4,
	NUM_OF_GMMAP_OPTIONS
}GFX_MEMORY_MAP_E;

DWORD FB_BaseAddresses[NUM_OF_GMMAP_OPTIONS] = {0x07000000, 0x0F000000, 0x1F000000, 0x3F000000, 0x7F000000};

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////  GENERAL FUNCTIONS  /////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*--------------------------------------------------------------------------------*/
/* Function:     GFX_FB_Initialize												  */
/*                                                                           	  */
/* Parameters:                                                               	  */
/*               None															  */
/*																			 	  */
/* Returns:		 None															  */
/* Side effects: Set the global FB_BA with the Frame Buffer Base Address       	  */
/* Description:                                                              	  */
/*               Find the Frame Buffer base physical address		         	  */
/*--------------------------------------------------------------------------------*/
void FB_Initialize()
{
	
	BYTE gmmapVal = GET_FIELD_REG(INTCR3, 8, 10);

	if (gmmapVal >= NUM_OF_GMMAP_OPTIONS)
	{
		LogError("Error: Invalid value for Graphics Memory Map (GMMAP = %u) \n", gmmapVal);
		return;
	}
	FB_BA = FB_BaseAddresses[gmmapVal];

	LogMessage("FB_Initialize(): Frame-Buffer address = %08Xh\n", FB_BA);

} // *End of FB_Initialize*

//--------------------------------------------------------------
DWORD GFX_GetFrameBufferBase()
{
	BYTE gmmapVal = GET_FIELD_REG(INTCR3, 8, 10);

	if (gmmapVal >= NUM_OF_GMMAP_OPTIONS)
	{
		LogError("Error: Invalid value for Graphics Memory Map (GMMAP = %u) \n", gmmapVal);
		return 0 ;
	}
	 return FB_BaseAddresses[gmmapVal];
}
//----------------------------------------------------------------
BOOL PLL_Configure (BYTE GPLLINDIV, BYTE GPLLFBDIV, BYTE GPLLST)
{
	LogMessage("> Pixel Clock Configuration start\n");

	// Configure PLL (write to Pixel clock control - power down pixel clock)									
	if (!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x1A)) {return (FALSE);}
	if (!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x8D)) {return (FALSE);}			

	// Configure PLL C only 
	if (!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0xB6)) {return (FALSE);} // GPLLFBDIV (FBDV)												
	if (!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), GPLLFBDIV)) {return (FALSE);}												
	LogMessage("  * GPLLFBDIV (FBDV): %08Xh\n", AHB2PCI_Memory_Read((MGABASE1 + 0x3C08), sizeof(DWORD)));

	if (!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0xB7)) {return (FALSE);} // GPLLINDIV (INDV + FBDV8)										
	if (!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), GPLLINDIV)) {return (FALSE);}													
	LogMessage("  * GPLLINDIV (INDV + FBDV8): %08Xh\n", AHB2PCI_Memory_Read((MGABASE1 + 0x3C08), sizeof(DWORD)));

	if (!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0xB8)) {return (FALSE);}// GPLLST (OTDV1/OTDV2)
	if (!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), GPLLST)) {return (FALSE);}
	LogMessage("  * GPLLST (OTDV1/OTDV2): %08Xh\n", AHB2PCI_Memory_Read((MGABASE1 + 0x3C08), sizeof(DWORD)));


	if (!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x1A)) {return (FALSE);}// XPIXCLKCTRL
	if (!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x09)) {return (FALSE);}
	LogMessage("  * XPIXCLKCTRL: %08Xh\n", AHB2PCI_Memory_Read((MGABASE1 + 0x3C08), sizeof(DWORD)));

	if (!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x18)) {return (FALSE);}// XVREFCTRL
	if (!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x0F)) {return (FALSE);}
	LogMessage("  * XVREFCTRL: %08Xh\n", AHB2PCI_Memory_Read((MGABASE1 + 0x3C08), sizeof(DWORD)));

	LogMessage("> Pixel Clock Configuration done.\n");

	return (TRUE);
}
//----------------------------------------------------------------

BOOL GFX_ConfigureDisplayTo1024x768(GFX_ColorDepth colorDepth)
{
	LogHeader("GFX Display Configuration to 1024x768");
	FB_Initialize();
	DWORD	data;
	int		i;
	int		cnt = 0;

	/*----------------------------------------------------------------------------------------------*/
	/* GFX PCI Configuration Space - Read and Configure												*/
	/*----------------------------------------------------------------------------------------------*/
	// Wait until GFX Device/Vendor ID is read correctly
	while (AHB2PCI_Config_Read(GFX_IDSEL + GFX_VENDOR_ID, sizeof(DWORD)) != GFX_VEN_DEV_ID_RV)
	{
		LogMessage("Waiting until PCI is out of reset; Press any key to abort .... \n");
		if (cnt == 10) 
		{
			LogWarning ("Test Aborted .\n");
			return (FALSE);
		}
		Sleep (1000);
		cnt++;
	}
	LogPass ("> Found correct GFX Device/Vendor ID 0x%08lX. \n", GFX_VEN_DEV_ID_RV);

	// Get the MGA Registers base address (MGABASE1)
	AHB2PCI_Config_Write(GFX_IDSEL + GFX_BAR1, sizeof(DWORD), MGABASE1 = 0x10000000, 0);
	MGABASE1 = AHB2PCI_Config_Read(GFX_IDSEL + GFX_BAR1, sizeof(DWORD));
	if (MGABASE1 == 0)
	{
		LogError("TEST_ConfigureDisplay failed : MGABASE1 is invalid = %08Xh\n", MGABASE1);
	}
	else
	{
		LogPass("TEST_ConfigureDisplay : MGABASE1 = %08Xh\n", MGABASE1);
	}
	AHB2PCI_Config_Write(GFX_IDSEL + GFX_BAR2, sizeof(DWORD), 0xf0000000, 0);

	LogMessage("MGABASE1 = %08Xh, MGABASE2 = %08Xh\n",
		AHB2PCI_Config_Read(GFX_IDSEL + GFX_BAR1, sizeof(DWORD)),
		AHB2PCI_Config_Read(GFX_IDSEL + GFX_BAR2, sizeof(DWORD)));

	// Enable Bus Master and Memory Space
	data = AHB2PCI_Config_Read(GFX_IDSEL + GFX_COMMAND_REG, sizeof(DWORD));
	AHB2PCI_Config_Write(GFX_IDSEL + GFX_COMMAND_REG, sizeof(DWORD), data | 0x06, 0);
	

	/*----------------------------------------------------------------------------------------------*/
	/* Graphics PLL Configuration																	*/
	/*----------------------------------------------------------------------------------------------*/
									
	BYTE GPLLINDIV,GPLLFBDIV,GPLLST;

	if (GFX_IsPllDebug_g == FALSE)
	{ // the correct values
		GPLLINDIV = 0x03; // GPLLINDIV (INDV + FBDV8)
		GPLLFBDIV = 0x4E; // GPLLFBDIV (FBDV) 
		GPLLST    = 0x15; // GPLLST (OTDV1/OTDV2)
	}
	else
	{ // debug Dell values. - no video issue 
		GPLLINDIV = 0x1A;  // GPLLINDIV (INDV + FBDV8)
		GPLLFBDIV = 0x40;  // GPLLFBDIV (FBDV) 
		GPLLST    = 0x00;  // GPLLST (OTDV1/OTDV2)
	}

	if(!PLL_Configure(GPLLINDIV, GPLLFBDIV, GPLLST)) {return (FALSE);}

	/*----------------------------------------------------------------------------------------------*/
	/* Sequencers Configuration																		*/
	/*----------------------------------------------------------------------------------------------*/

	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0100)) {return (FALSE);}	// Reset sequencer
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0101)) {return (FALSE);}	// 8-dot character clock
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0302)) {return (FALSE);}	// Map 3 write enable
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0003)) {return (FALSE);}	// another mapping of VGA (??)
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0204)) {return (FALSE);}	// 256K memory is installed
	if (!AHB2PCI_Memory_Write(MISC, sizeof(BYTE), 0xC9)) {return (FALSE);}		// Configure CRTC addresses, Select MGA pixel clock, VSYNC and HSYNC active LOW
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0300)) {return (FALSE);}	// Reset sequencer

	LogMessage("> Sequencers Configuration done.\n");
	/*----------------------------------------------------------------------------------------------*/
	/* Graphics Controller Configuration															*/
	/*----------------------------------------------------------------------------------------------*/

	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0000)) {return (FALSE);}	// Reset bytes in the VGA memory map
	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0001)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0002)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0003)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0004)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x1005)) {return (FALSE);}	// Selects Odd/Even addressing mode
	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0E06)) {return (FALSE);}	// VGA mode select, Memory map is '0b11'
	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0007)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0xFF08)) {return (FALSE);}	// Write mask to 0xFF

	LogMessage("> Graphics Controller Configuration done.\n");

	/*----------------------------------------------------------------------------------------------*/
	/* CRTC Controller Configuration																*/
	/*----------------------------------------------------------------------------------------------*/

	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xA300)) {return (FALSE);} // HTOTAL[7:0] = 0xA3 (HTOTAL[8] appears in CRTCEXT1[0])
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x7F01)) {return (FALSE);} // HDISP_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x7F02)) {return (FALSE);} // HBLANK_START HBLKSTR[8] appears in CRTCEXT1[1]
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x8703)) {return (FALSE);} // HBLANK_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x8304)) {return (FALSE);} // HRETRACE_START 
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x9305)) {return (FALSE);} // HRETRACE_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x2606)) {return (FALSE);} // VTOTAL		// was 24
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xF507)) {return (FALSE);} // OVERFLOW (overflow of other fields)
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x0008)) {return (FALSE);} // PRE_ROW_SCAN
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x6009)) {return (FALSE);} // MAX_SCAN_LINE
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x200A)) {return (FALSE);} // CUR_START
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x000B)) {return (FALSE);} // CUR_END

	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x000C)) {return (FALSE);} // ST_ADDR_HI
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x000D)) {return (FALSE);} // ST_ADDR_LOW
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xA300)) {return (FALSE);} // HTOTAL again?

	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x000E)) {return (FALSE);} // CUR_POS_HI
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x000F)) {return (FALSE);} // CUR_POS_LOW
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x0210)) {return (FALSE);} // VRETRACE_START
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x2811)) {return (FALSE);} // VRETRACE_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xFF12)) {return (FALSE);} // VDISP_END
	if (colorDepth == COLOR_DEPTH_32BIT) {
		if (!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x0013)) // OFFSET
			return (FALSE);
	} else {
		if (colorDepth == COLOR_DEPTH_16BIT)
		{
			if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x8013)) // OFFSET
				return (FALSE);
		}
	}
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x0014)) {return (FALSE);} // UND_LOC
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xFF15)) {return (FALSE);} // VBLANK_START
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x2516)) {return (FALSE);} // VBLANK_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xC317)) {return (FALSE);} // MODE_CTRL
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xFF18)) {return (FALSE);} // LINE_COMP
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x0022)) {return (FALSE);} // CPU_LATCH_RD
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x8024)) {return (FALSE);} // DATA_SEL
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x3426)) {return (FALSE);} // ATTRIB_ADDR (34 in diag 20 in Yarkon?)
	

	LogMessage("> CRTC Configuration done.\n");

	/*----------------------------------------------------------------------------------------------*/
	/* CRTC Controller Extension Configuration														*/
	/*----------------------------------------------------------------------------------------------*/

	if (colorDepth == COLOR_DEPTH_32BIT) {
		if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x1000))  // ADDR_GEN
			return (FALSE);
	} else {
		if ((colorDepth == COLOR_DEPTH_16BIT) || (colorDepth == COLOR_DEPTH_24BIT)) {
			if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0000))  // ADDR_GEN
				return (FALSE);
		}
	}

	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0801)) {return (FALSE);} // H_COUNT
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x8002)) {return (FALSE);} // V_COUNT 
	
	if (colorDepth == COLOR_DEPTH_32BIT) {
		if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x8303)) //MISC
			return (FALSE);
	} else if (colorDepth == COLOR_DEPTH_16BIT) {
			if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x8103))  // MISC
				return (FALSE);
    } else if (colorDepth == COLOR_DEPTH_24BIT) {
			if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x8203)) // MISC
				return (FALSE);
	}

	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0004)) {return (FALSE);} // MEM_PAGE
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0005)) {return (FALSE);} // HVIDMID
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x2006)) {return (FALSE);} // P_REQ_CTL
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0007)) {return (FALSE);} // REQ_CTL
	
	// Poleg - New registers
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x1030)) {return (FALSE);} //MGABURSTSIZE
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0534)) {return (FALSE);} //MGAREQCTL
	LogMessage("> CRTC Extension configuration done.\n");

	/*----------------------------------------------------------------------------------------------*/
	/* DAC Eclipse Configuration																	*/
	/*----------------------------------------------------------------------------------------------*/

	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x04)) {return (FALSE);} // XCURADDL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x4F)) {return (FALSE);} 
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x05)) {return (FALSE);} // XCURADDH
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x3A)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x06)) {return (FALSE);} // XCURCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x08)) {return (FALSE);} // XCURCOL0RED
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x82)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x09)) {return (FALSE);} // XCURCOL0GREEN
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xFB)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x0A)) {return (FALSE);} // XCURCOL0BLUE
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x44)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x0C)) {return (FALSE);} // XCURCOL1RED
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x2C)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x0D)) {return (FALSE);} // XCURCOL1GREE
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x75)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x0E)) {return (FALSE);} // XCURCOL1BLUE
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x8B)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x10)) {return (FALSE);} // XCURCOL2RED
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x14)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x11)) {return (FALSE);} // XCURCOL2GREEN
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x2E)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x12)) {return (FALSE);} // XCURCOL2BLUE
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x4A)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x19)) {return (FALSE);}		// XMULCTRL
	if (colorDepth == COLOR_DEPTH_32BIT) {
		if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x07))
			return (FALSE);
	} else if (colorDepth == COLOR_DEPTH_16BIT) {
			if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x02))
				return (FALSE);
	} else if (colorDepth == COLOR_DEPTH_24BIT) {
			if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x03))
				return (FALSE);
	}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x1A)) {return (FALSE);}		// XPIXCLKCTRL (should be 9?)
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x09)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x1D)) {return (FALSE);}		// XGENCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x20)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x1E)) {return (FALSE);}		// XMISCCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x1F)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2A)) {return (FALSE);}		// XGENIOCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x01)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2B)) {return (FALSE);}		// XGENIODATA		
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x02)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2C)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x14)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2D)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x71)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2E)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x01)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2F)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x40)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x38)) {return (FALSE);}		// XZOOMCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x3A)) {return (FALSE);}		// XSENSETEST (RO)
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x3C)) {return (FALSE);}		// XCRCREML
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x3D)) {return (FALSE);}		// XCRCREMH (RO)
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xB5)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x3E)) {return (FALSE);}		// XCRCBITSEL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x0B)) {return (FALSE);}		
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x40)) {return (FALSE);}		// ?? rsvd. 
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xFD)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x41)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x42)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xB6)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x43)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x44)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x11)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x45)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x10)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x46)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x07)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x48)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x0C)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x49)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x75)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4A)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x07)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4C)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x13)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4D)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x5D)) {return (FALSE);}		
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4E)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x05)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4F)) {return (FALSE);}		// XPIXPLLSTAT
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x51)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x01)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x18)) {return (FALSE);}	// XVREFCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x04)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x52)) {return (FALSE);}	
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x61)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x53)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x7D)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x54)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xD1)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x55)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xFE)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x56)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x7D)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x57)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x3A)) {return (FALSE);}

	LogMessage("> DAC Eclipse Configuration done.\n");

	/*----------------------------------------------------------------------------------------------*/
	/* palette Configuration																		*/
	/*----------------------------------------------------------------------------------------------*/

	if(!AHB2PCI_Memory_Write(PALWTADD, sizeof(BYTE), 0)) {return (FALSE);}

	if ((colorDepth == COLOR_DEPTH_32BIT) || (colorDepth == COLOR_DEPTH_24BIT))
	{
		// palette for 8:8:8 color format
		for (i = 0; i <= 255; i++)
		{
			if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i)) {return (FALSE);}
			if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i)) {return (FALSE);}
			if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i)) {return (FALSE);}
		}
	}
	else
		if (colorDepth == COLOR_DEPTH_16BIT)
		{
			// palette for 5:6:5 color format
			for (i = 0; i <= 63; i++)
			{
				if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i * 8)) {return (FALSE);}
				if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i * 4)) {return (FALSE);}
				if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i * 8)) {return (FALSE);}
			}
		}

		LogMessage("> Palette Configuration done.\n");

		/*----------------------------------------------------------------------------------------------*/
		/* Configuration Done																			*/
		/*----------------------------------------------------------------------------------------------*/
	
		// fill frame buffer with random lines
		LogMessage("> Filling Frame Buffer...\n");

		// fill frame buffer with random lines
		DWORD lineNum = 0;
		const DWORD numOfBars=16;
		DWORD Address = FB_BA;
		DWORD Index;
		DWORD Color;

		if (colorDepth == COLOR_DEPTH_32BIT)
			for (lineNum = 0; lineNum < numOfBars; ++lineNum)
			{
				Color = (DWORD)rand() ^ ((DWORD)rand()<<16); 
				for (Index =0; Index<((1024*768)/numOfBars); Index++, Address=Address+4)
					WRITE_REG (HW_DWORD(Address), Color);
			}
		else if (colorDepth == COLOR_DEPTH_16BIT)
			for (lineNum = 0; lineNum < numOfBars; ++lineNum)
			{
				Color = (DWORD)rand() ^ ((DWORD)rand()<<16);  
				for (Index =0; Index<((1024*768)/numOfBars); Index++, Address=Address+2)
					WRITE_REG (HW_WORD(Address), (WORD)Color);
			}
		else if (colorDepth == COLOR_DEPTH_24BIT)
		{
			LogWarning ("\n Using 32 bit frame buffer method for 24bit mode ! \n");
			for (lineNum = 0; lineNum < numOfBars; ++lineNum)
			{
				Color = (DWORD)rand() ^ ((DWORD)rand()<<16); 
				for (Index =0; Index<((1024*768)/numOfBars); Index++, Address=Address+4)
					WRITE_REG (HW_DWORD(Address), Color);
			}
		}

		LogMessage("> Filling Frame Buffer done.\n");
}; 



//---------------------------------------------------------------------------------------
BOOL GFX_ConfigureDisplayTo1920x1200(GFX_ColorDepth colorDepth)
{
	LogHeader("GFX Display Configuration to 1920x1200");
	FB_Initialize();
	DWORD	data;
	int		i;
	int		cnt = 0;

	/*----------------------------------------------------------------------------------------------*/
	/* GFX PCI Configuration Space - Read and Configure												*/
	/*----------------------------------------------------------------------------------------------*/
	// Wait until GFX Device/Vendor ID is read correctly
	while (AHB2PCI_Config_Read(GFX_IDSEL + GFX_VENDOR_ID, sizeof(DWORD)) != GFX_VEN_DEV_ID_RV)
	{
		LogMessage("Waiting until PCI is out of reset; Press any key to abort .... \n");
		if (cnt == 10) 
		{
			LogWarning ("Test Aborted .\n");
			return (FALSE);
		}
		Sleep (1000);
		cnt++;
	}
	LogPass ("> Found correct GFX Device/Vendor ID 0x%08lX. \n", GFX_VEN_DEV_ID_RV);

	// Get the MGA Registers base address (MGABASE1)
	AHB2PCI_Config_Write(GFX_IDSEL + GFX_BAR1, sizeof(DWORD), MGABASE1 = 0x10000000, 0);
	MGABASE1 = AHB2PCI_Config_Read(GFX_IDSEL + GFX_BAR1, sizeof(DWORD));
	if (MGABASE1 == 0)
	{
		LogError("TEST_ConfigureDisplay failed : MGABASE1 is invalid = %08Xh\n", MGABASE1);
	}
	else
	{
		LogPass("TEST_ConfigureDisplay : MGABASE1 = %08Xh\n", MGABASE1);
	}
	AHB2PCI_Config_Write(GFX_IDSEL + GFX_BAR2, sizeof(DWORD), 0xf0000000, 0);

	LogMessage("MGABASE1 = %08Xh, MGABASE2 = %08Xh\n",
		AHB2PCI_Config_Read(GFX_IDSEL + GFX_BAR1, sizeof(DWORD)),
		AHB2PCI_Config_Read(GFX_IDSEL + GFX_BAR2, sizeof(DWORD)));

	// Enable Bus Master and Memory Space
	data = AHB2PCI_Config_Read(GFX_IDSEL + GFX_COMMAND_REG, sizeof(DWORD));
	AHB2PCI_Config_Write(GFX_IDSEL + GFX_COMMAND_REG, sizeof(DWORD), data | 0x06, 0);



	/*----------------------------------------------------------------------------------------------*/
	/* Graphics PLL Configuration																	*/
	/*----------------------------------------------------------------------------------------------*/

	BYTE GPLLINDIV,GPLLFBDIV,GPLLST;
	GPLLINDIV = 0x09; // GPLLINDIV (INDV + FBDV8)
	GPLLFBDIV = 0xDE; // GPLLFBDIV (FBDV) 
	GPLLST    = 0x0C; // GPLLST (OTDV1/OTDV2)
	if (!PLL_Configure  (GPLLINDIV, GPLLFBDIV, GPLLST)) {return (FALSE);}

	/*----------------------------------------------------------------------------------------------*/
	/* Sequencers Configuration																		*/
	/*----------------------------------------------------------------------------------------------*/

	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0100)) {return (FALSE);}	// Reset sequencer
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0101)) {return (FALSE);}	// 8-dot character clock
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0302)) {return (FALSE);}	// Map 3 write enable
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0003)) {return (FALSE);}	// another mapping of VGA (??)
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0204)) {return (FALSE);}	// 256K memory is installed
	if (colorDepth == COLOR_DEPTH_24BIT)
	{
		if (!AHB2PCI_Memory_Write(MISC, sizeof(BYTE), 0x09))
			return (FALSE);		// Configure CRTC addresses, Select MGA pixel clock, VSYNC and HSYNC active LOW
	} 
	else
	{
		if (!AHB2PCI_Memory_Write(MISC, sizeof(BYTE), 0xC9))
			return (FALSE);		// Configure CRTC addresses, Select MGA pixel clock, VSYNC and HSYNC active LOW
	}
	if (!AHB2PCI_Memory_Write(SEQ, sizeof(WORD), 0x0300)) {return (FALSE);}	// Reset sequencer

	LogMessage("> Sequencers Configuration done.\n");
	/*----------------------------------------------------------------------------------------------*/
	/* Graphics Controller Configuration															*/
	/*----------------------------------------------------------------------------------------------*/

	if (!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0000)) {return (FALSE);}	// Reset bytes in the VGA memory map
	if(!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0001)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if(!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0002)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if(!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0003)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if(!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0004)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if(!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x1005)) {return (FALSE);}	// Selects Odd/Even addressing mode
	if(!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0E06)) {return (FALSE);}	// VGA mode select, Memory map is '0b11'
	if(!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0x0007)) {return (FALSE);}	// ?? (probably VGA - keep reset value)
	if(!AHB2PCI_Memory_Write(GCTL, sizeof(WORD), 0xFF08)) {return (FALSE);}	// Write mask to 0xFF

	LogMessage("> Graphics Controller Configuration done.\n");

	/*----------------------------------------------------------------------------------------------*/
	/* CRTC Controller Configuration																*/
	/*----------------------------------------------------------------------------------------------*/

	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xFF00)) {return (FALSE);} // HTOTAL[7:0] = 0xA3 (HTOTAL[8] appears in CRTCEXT1[0])
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xEF01)) {return (FALSE);} // HDISP_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xEF02)) {return (FALSE);} // HBLANK_START HBLKSTR[8] appears in CRTCEXT1[1]
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x8303)) {return (FALSE);} // HBLANK_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xF504)) {return (FALSE);} // HRETRACE_START 
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x1905)) {return (FALSE);} // HRETRACE_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xD106)) {return (FALSE);} // VTOTAL		// was 24
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x1007)) {return (FALSE);} // OVERFLOW (overflow of other fields)
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x0008)) {return (FALSE);} // PRE_ROW_SCAN
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x4009)) {return (FALSE);} // MAX_SCAN_LINE
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x200A)) {return (FALSE);} // CUR_START
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x000B)) {return (FALSE);} // CUR_END

	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x000C)) {return (FALSE);} // ST_ADDR_HI
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x800D)) {return (FALSE);} // ST_ADDR_LOW - check where should FB_BA be
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xFF00)) {return (FALSE);} // HTOTAL again?

	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x000E)) {return (FALSE);} // CUR_POS_HI
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x000F)) {return (FALSE);} // CUR_POS_LOW
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xB210)) {return (FALSE);} // VRETRACE_START
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x2811)) {return (FALSE);} // VRETRACE_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xAF12)) {return (FALSE);} // VDISP_END
	if ((colorDepth == COLOR_DEPTH_32BIT)|| (colorDepth == COLOR_DEPTH_24BIT)) {
		if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x0013))
			return (FALSE); // OFFSET
	} else {
		if (colorDepth == COLOR_DEPTH_16BIT) 
		{
			if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xF013))
				return (FALSE); // OFFSET - note FB should be also moved
			FB_BA += 0x100*4;
		}
	}
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x0014)) {return (FALSE);} // UND_LOC
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xAF15)) {return (FALSE);} // VBLANK_START
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xD216)) {return (FALSE);} // VBLANK_END
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xC317)) {return (FALSE);} // MODE_CTRL
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0xFF18)) {return (FALSE);} // LINE_COMP
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x0022)) {return (FALSE);} // CPU_LATCH_RD
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x8024)) {return (FALSE);} // DATA_SEL
	if(!AHB2PCI_Memory_Write(CRTC, sizeof(WORD), 0x3426)) {return (FALSE);} // ATTRIB_ADDR 


	LogMessage("> CRTC Configuration done.\n");

	/*----------------------------------------------------------------------------------------------*/
	/* CRTC Controller Extension Configuration														*/
	/*----------------------------------------------------------------------------------------------*/

	if (colorDepth == COLOR_DEPTH_32BIT) {
		if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x1000))
			return (FALSE); // ADDR_GEN
	} else {
		if ((colorDepth == COLOR_DEPTH_16BIT) || (colorDepth == COLOR_DEPTH_24BIT)) {
			if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0000))
				return (FALSE); // ADDR_GEN
		}
	}

	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0801)) {return (FALSE);} // H_COUNT
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0xAD02)) {return (FALSE);} // V_COUNT 

	if (colorDepth == COLOR_DEPTH_32BIT) {
		if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x8303))
			return (FALSE); //MISC
	} else if (colorDepth == COLOR_DEPTH_16BIT) {
		if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x8103))
			return (FALSE); // MISC
	} else if (colorDepth == COLOR_DEPTH_24BIT) {
		if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x8203))
			return (FALSE); // MISC
	}

	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0004)) {return (FALSE);} // MEM_PAGE
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0005)) {return (FALSE);} // HVIDMID
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x2006)) {return (FALSE);} // P_REQ_CTL
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0007)) {return (FALSE);} // REQ_CTL

	// Poleg - New registers
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x1030)) {return (FALSE);} //MGABURSTSIZE
	if(!AHB2PCI_Memory_Write(CRTCEXT, sizeof(WORD), 0x0534)) {return (FALSE);} //MGAREQCTL
	LogMessage("> CRTC Extension configuration done.\n");

	/*----------------------------------------------------------------------------------------------*/
	/* DAC Eclipse Configuration																	*/
	/*----------------------------------------------------------------------------------------------*/

	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x04)) {return (FALSE);} // XCURADDL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x4F)) {return (FALSE);} 
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x05)) {return (FALSE);} // XCURADDH
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x3A)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x06)) {return (FALSE);} // XCURCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x08)) {return (FALSE);} // XCURCOL0RED
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x82)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x09)) {return (FALSE);} // XCURCOL0GREEN
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xFB)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x0A)) {return (FALSE);} // XCURCOL0BLUE
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x44)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x0C)) {return (FALSE);} // XCURCOL1RED
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x2C)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x0D)) {return (FALSE);} // XCURCOL1GREE
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x75)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x0E)) {return (FALSE);} // XCURCOL1BLUE
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x8B)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x10)) {return (FALSE);} // XCURCOL2RED
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x14)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x11)) {return (FALSE);} // XCURCOL2GREEN
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x2E)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x12)) {return (FALSE);} // XCURCOL2BLUE
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x4A)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x19)) {return (FALSE);}		// XMULCTRL
	if (colorDepth == COLOR_DEPTH_32BIT) {
		if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x07))
			return (FALSE);
	} else if (colorDepth == COLOR_DEPTH_16BIT) {
		if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x02))
			return (FALSE);
	} else if (colorDepth == COLOR_DEPTH_24BIT) {
		if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x03))
			return (FALSE);
	}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x1A)) {return (FALSE);}		// XPIXCLKCTRL (should be 9?)
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x09)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x1D)) {return (FALSE);}		// XGENCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x20)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x1E)) {return (FALSE);}		// XMISCCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x1F)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2A)) {return (FALSE);}		// XGENIOCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x01)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2B)) {return (FALSE);}		// XGENIODATA		
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x02)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2C)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x14)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2D)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x71)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2E)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x01)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x2F)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x40)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x38)) {return (FALSE);}		// XZOOMCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x3A)) {return (FALSE);}		// XSENSETEST (RO)
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x3C)) {return (FALSE);}		// XCRCREML
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x3D)) {return (FALSE);}		// XCRCREMH (RO)
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xB5)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x3E)) {return (FALSE);}		// XCRCBITSEL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x0B)) {return (FALSE);}		
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x40)) {return (FALSE);}		// ?? rsvd. 
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xFD)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x41)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x42)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xB6)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x43)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x44)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x11)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x45)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x10)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x46)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x07)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x48)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x0C)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x49)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x75)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4A)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x07)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4C)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x13)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4D)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x5D)) {return (FALSE);}		
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4E)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x05)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x4F)) {return (FALSE);}		// XPIXPLLSTAT
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x00)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x51)) {return (FALSE);}		// ?? rsvd.
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x01)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x18)) {return (FALSE);}	// XVREFCTRL
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x04)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x52)) {return (FALSE);}	
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x61)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x53)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x7D)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x54)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xD1)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x55)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0xFE)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x56)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x7D)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(PALWTADD,	sizeof(BYTE), 0x57)) {return (FALSE);}
	if(!AHB2PCI_Memory_Write(X_DATAREG, sizeof(BYTE), 0x3A)) {return (FALSE);}

	LogMessage("> DAC Eclipse Configuration done.\n");

	/*----------------------------------------------------------------------------------------------*/
	/* palette Configuration																		*/
	/*----------------------------------------------------------------------------------------------*/

	if(!AHB2PCI_Memory_Write(PALWTADD, sizeof(BYTE), 0)) {return (FALSE);}

	if ((colorDepth == COLOR_DEPTH_32BIT) || (colorDepth == COLOR_DEPTH_24BIT))
	{
		// palette for 8:8:8 color format
		for (i = 0; i <= 255; i++)
		{
			if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i)) {return (FALSE);}
			if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i)) {return (FALSE);}
			if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i)) {return (FALSE);}
		}
	}
	else
		if (colorDepth == COLOR_DEPTH_16BIT)
		{
			// palette for 5:6:5 color format
			for (i = 0; i <= 63; i++)
			{
				if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i * 8)) {return (FALSE);}
				if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i * 4)) {return (FALSE);}
				if(!AHB2PCI_Memory_Write(PALDATA, sizeof(BYTE), i * 8)) {return (FALSE);}
			}
		}

		LogMessage("> Palette Configuration done.\n");

		/*----------------------------------------------------------------------------------------------*/
		/* Configuration Done																			*/
		/*----------------------------------------------------------------------------------------------*/

		// fill frame buffer with random lines
		DWORD lineNum = 0;
		const DWORD numOfBars=16;
		DWORD Address = FB_BA;
		DWORD Index;
		DWORD Color;

		if (colorDepth == COLOR_DEPTH_32BIT)
			for (lineNum = 0; lineNum < numOfBars; ++lineNum)
			{
				Color = (DWORD)rand() ^ ((DWORD)rand()<<16); 
				for (Index =0; Index<((1920*1200)/numOfBars); Index++, Address=Address+4)
					WRITE_REG (HW_DWORD(Address), Color);
			}
		else if (colorDepth == COLOR_DEPTH_16BIT)
			for (lineNum = 0; lineNum < numOfBars; ++lineNum)
			{
				Color = (DWORD)rand() ^ ((DWORD)rand()<<16);  
				for (Index =0; Index<((1920*1200)/numOfBars); Index++, Address=Address+2)
					WRITE_REG (HW_WORD(Address), (WORD)Color);
			}
		else if (colorDepth == COLOR_DEPTH_24BIT)
		{
			LogWarning ("\n Using 32 bit frame buffer method for 24bit mode ! \n");
			for (lineNum = 0; lineNum < numOfBars; ++lineNum)
			{
				Color = (DWORD)rand() ^ ((DWORD)rand()<<16); 
				for (Index =0; Index<((1920*1200)/numOfBars); Index++, Address=Address+4)
					WRITE_REG (HW_DWORD(Address), Color);
			}
		}

		LogMessage("> Filling Frame Buffer done.\n");
		return (TRUE);
}; 

//----------------------------------------------------------------------------------------
void GFX_ClearMemorySpace(BYTE presetValue)
{
	FB_Initialize();
	memset((void *)FB_BA, presetValue, (16 * 1024 * 1024));
}
