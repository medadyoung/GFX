#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Common.h"
#include "CoreRegisters.h"
#include "TestMsgCore.h"
#include "Utility.h"
#include "Poleg.h"
#include "GraphicsInit.h"
#include "GFXI_IF.h"

void RunMemoryTest (UINT32 Start_Addr, UINT32 End_Addr, UINT32 Pattern);
int main_menu (void);

// --------------------------------------------------------------
// Interrupts
// --------------------------------------------------------------

typedef void (*INT_ROUTINE) (UINT32 Address);

// table pointer of the interrupt routine
extern INT_ROUTINE pUndefinedInstructionRoutine; 
extern INT_ROUTINE pSvcRoutine;
extern INT_ROUTINE pPrefetchAbortRoutine;
extern INT_ROUTINE pDataAbortRoutine;
extern INT_ROUTINE pIrqRoutine;
extern INT_ROUTINE pFiqRoutine;


void FiqRoutine (UINT32 Address)
{
	LOG_ERROR (("'FIQ' event has been triggered at address 0x%08lX. \n",Address));
	//exit (-1);
}
//-------------------------------------------------------------
void DataAbortRoutine (UINT32 Address)
{
	LOG_ERROR (("'Data Abort' event has been triggered at address 0x%08lx. \n",Address));
	//exit (-1);
}
//-------------------------------------------------------------
void UndefinedInstructionRoutine (UINT32 Address)
{
	LOG_ERROR (("'Undefined Instruction' event has been triggered at address 0x%08lx in ARM state or 0x%08lx in Thumb state. \n",Address-4,Address-2));
	//exit (-1);
}
//-------------------------------------------------------------
void PrefetchAbortRoutine (UINT32 Address)
{
	LOG_ERROR (("'Prefetch Abort' event has been triggered at address 0x%08lx. \n",Address));
	//exit (-1);
}
//-------------------------------------------------------------
void SvcRoutine (UINT32 Address)
{
	LOG_ERROR (("'SVC' event has been triggered from address 0x%08lx. \n",Address));
	//exit (-1);
}
//-------------------------------------------------------------



//int g_test_variable = 100;

volatile UINT32 codestartup;
static void Check_Clocks (void);


UINT32 CPU_ID_CODE;
UINT32 CPU_ID;
UINT32 PLL0_Freq_In_KHz, PLL1_Freq_In_KHz, PLL2_Freq_In_KHz;

//-------------------------------------------------------------
int main (void)
{
	UINT32 SCTLR_reg;

	Uart_AutoDetect ();

	// At startup, all interrupts vectors are assign to error() function (this assuming code is loaded at 0x0000_0000 and core vectore table is at 0x0000_0000).
	pDataAbortRoutine = (INT_ROUTINE) DataAbortRoutine;
	pUndefinedInstructionRoutine = (INT_ROUTINE) UndefinedInstructionRoutine;
	pPrefetchAbortRoutine = (INT_ROUTINE) PrefetchAbortRoutine;
	pSvcRoutine =  (INT_ROUTINE) SvcRoutine;
	pFiqRoutine =  (INT_ROUTINE) FiqRoutine;
	
	LogMessage ("\n\n");
	LogHeader (" Build Date: %s,%s with GCC %u.%u.%u ",__DATE__,__TIME__,__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__);
	//LogMessage ("Core test random seed: 0x%08lX \n",GetRandSeed());
	//-------------------------------------------------------------
	// Invalidate Instruction cache.
	CP15_WRITE (0,0,c7,c5,0); // ICIALLU: Invalidate all instruction caches to PoU. Also flushes branch target cache.
	CP15_WRITE (0,0,c7,c5,6); // BPIALL: Invalidate entire branch predictor array.
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	// verify data initialize correctly (used when value is stored at ROM and copy to RAM). 
	/*
	g_test_variable++; // g_test_variable initialize to 100
	if (g_test_variable!=101)
	{
		LogWarning ("start-up code failed to initialize data variables ! \n");
		exit (-1);
	}
	*/
	//-------------------------------------------------------------

	//-------------------------------------------------------------
	CP15_READ (SCTLR_reg, 0, c1, c0, 0);
	//LogWarning ("> Code start-up at address 0x%08lX. \n",(UINT32)&_startup);
	if ((UINT32)&_startup==0)
	{
		if (((SCTLR_reg>>13)&0x1) != 0) 	
		{
			LogWarning ("> Set CPU vector table to address 0x0000_0000. \n");
			SCTLR_reg &= ~((UINT32)1<<13); // set vector table into address 0x0000_0000
			CP15_WRITE (SCTLR_reg, 0, c1, c0, 0);
		}
	}
	else if ((UINT32)&_startup==0xFFFD0000)
	{
		if (((SCTLR_reg>>13)&0x1) == 0) 	
		{
			LogWarning ("> Override vector table.\n");
			LogWarning ("  >> Mapped first 256 bytes of address 0xFFFF_0000 to address 0xFFFD_0000. \n");
			SET_BIT_REG (FLOCKR1,18); // RAM2 first 256 bytes are mapped to address range FFFF_0000h to FFFF_00FFh.
			LogWarning ("  >> Set CPU vector table to address 0xFFFF_0000. \n");
			SCTLR_reg |= ((UINT32)1<<13); // set vector table into address 0xFFFF_0000
			CP15_WRITE (SCTLR_reg, 0, c1, c0, 0);
		}
	}
	else
	{
		LogWarning (" Code was not compiled to address 0x00000000 or 0xFFFD0000; interrupts will not work unless code vector table are copied ! \n");
	}
	//-------------------------------------------------------------


	//-------------------------------------------------------------
	LogHeader (" Display Some Chip Info ");
	//-------------------------------------------------------------
	UINT32 PDID_reg = READ_REG(PDID);
	switch (PDID_reg) 
	{
		case Poleg_ID_Z1:	LogWarning ("> Found NPCM750x BMC chip version Z1.\n");	break;
		case Poleg_ID_Z2:	LogPass ("> Found NPCM750x BMC chip version A1.\n");	break;
		default:
			LogWarning ("Unknown NPCM750x BMC chip version. Found PDID=0x%08lX\n",PDID_reg); // 23/12/2015: change "Invalid" to "Unknown" with only Warning message
	}
	//-------------------------------------------------------------

#if 0

	CPU_ID_CODE = Get_CPU_ID_CODE();
	if (CPU_ID_CODE == 0x41069265)
		LogMessage ("> Found CPU ID Code: 0x%08lX (ARM 927). \n",CPU_ID_CODE);
	else if (CPU_ID_CODE == 0x414FC091)
	{
		CPU_ID = Get_MPCore_ID();
		LogMessage ("> Found CPU ID Code: 0x%08lX (ARM Cortex A9 r4p1); CPU ID 0x%08lX. \n",CPU_ID_CODE, CPU_ID);
		CPU_ID &= 0x03;
		if (CPU_ID==0)
		{
			LogMessage ("   >> Hellow from Core-0. \n");
			#ifndef Core_0 
				LOG_ERROR (("This is Core-0 while the code was compiled for other core. \n"));
				exit (-1);
			#endif
		}
		else
		{
			LogMessage ("   >> Hellow from Core-1. \n");
			#ifndef Core_1 
				LOG_ERROR (("This is Core-1 while the code was compiled for other core. \n"));
				exit (-1);
			#endif
		}
	}
	else
	{
		LOG_ERROR (("> CPU ID Code: 0x%08lX (Unknown); CPU ID 0x%08lX. \n",CPU_ID_CODE,CPU_ID));
		exit (-1);
	}
	//--------------------------------------------------------------
	// read SCTLR register
	CP15_READ (SCTLR_reg, 0, c1, c0, 0);
	LogMessage ("> SCTLR register value 0x%08lx \n",SCTLR_reg);
	if (((SCTLR_reg>>0)&0x1)== 1)
		LogMessage ("   >> MMU is enabled .\n");
	else
		LogMessage ("   >> MMU is disabled.\n");
	if (((SCTLR_reg>>2)&0x1)== 1)
		LogMessage ("   >> Data caching is enabled.\n");
	else
		LogMessage ("   >> Data caching is disabled.\n");
	if (((SCTLR_reg>>11)&0x1)== 1)
		LogMessage ("   >> Program flow prediction is enabled.\n");
	else
		LogMessage ("   >> Program flow prediction is disabled.\n");
	if (((SCTLR_reg>>12)&0x1)== 1)
		LogMessage ("   >> Instruction caching is enabled.\n");
	else
		LogMessage ("   >> Instruction caching is disabled.\n");
	if (((SCTLR_reg>>13)&0x1)== 1)
		LogMessage ("   >> High exception vectors, Hivecs, base address 0xFFFF0000.\n");
	else
		LogMessage ("   >> Normal exception vectors, base address 0x00000000.\n");
#endif	
	
	//-------------------------------------------------------------
	Check_Clocks ();
	//--------------------------------------------------------------

	//extern int DDR_Init (void);
	//DDR_Init();

	return main_menu ();

	//return (0);
	
} // *End of main*
//------------------------------------------------------------------------------------------------------




//--------------------------------------------------------------------
// 16/02/2015: Fixed PLL function; the function always returned "PLL in reset"
// 08/04/2015: Fixed PLL_FOUT calculation so it will not overflow DWORD.
UINT32 Check_BMC_PLL (DWORD PllCon, DWORD PllIndex)
{
	DWORD PLL_FBDV,PLL_OTDV1, PLL_OTDV2, PLL_INDV,PLL_FOUT;

	LogMessage ("  * PLLCON%u = 0x%08lX. ",PllIndex,PllCon);

	if ( ((PllCon>>12)&0x1) ==1 )
	{
		LogWarning (" (PLL is in Power-Down).\n");
		return (0);
	}

	PLL_FBDV = (PllCon >> 16) & 0xFFF; //  (bits 16..27, 12bit)
	PLL_OTDV1 = (PllCon >> 8) & 0x7; //  (bits 8..10, 3bit)
	PLL_OTDV2 = (PllCon >> 13) & 0x7; //  (bits 13..15, 3bit)
	PLL_INDV = (PllCon >> 0) & 0x3F; //  (bits 0..5, 6 bits)

	PLL_FOUT = 25000/*000*/ * (PLL_FBDV);
	PLL_FOUT = PLL_FOUT / ( (PLL_OTDV1) * (PLL_OTDV2) * (PLL_INDV) );
	if ((PllIndex==1) && (READ_REG(PDID)!=Poleg_ID_Z1))
		PLL_FOUT = PLL_FOUT /2;
	LogMessage (" (%lu MHz; ",PLL_FOUT/1000/*000*/);

	if ( ((PllCon>>31)&0x1) == 0 )
		LogWarning ("PLL is not locked");
	else
		LogPass ("locked");

	LogMessage (")\n");

	return (PLL_FOUT); // clock in KHz
}
//--------------------------------------------------------------------------
void Check_Clocks (void)
{
	UINT32 ClkSel,ClkDiv1,ClkDiv2,ClkDiv3;

	LogMessage ("> PLLs and Clocks: \n");

	PLL0_Freq_In_KHz = Check_BMC_PLL (READ_REG (PLLCON0),0);
	PLL1_Freq_In_KHz = Check_BMC_PLL (READ_REG (PLLCON1),1);
	PLL2_Freq_In_KHz = Check_BMC_PLL (READ_REG (PLLCON2),2);
	ClkSel = READ_REG (CLKSEL);
	LogMessage ("  * CLKSEL = 0x%08lX \n", ClkSel);
	//----------------------------------------
	LogMessage ("    - CPU Clock Source (CPUCKSEL) => ");
	switch ((ClkSel>>0)&0x03)
	{
	case 0:
		LogPass ("PLL 0. \n");
		break;
	case 1:
		LogPass ("PLL 1. \n");
		break;
	case 2:
		LogWarning ("CLKREF clock (debug). \n");
		break;
	case 3:
		LogWarning ("Bypass clock from pin SYSBPCK (debug). \n");
		break;
	default :
		LOG_ERROR  (("Invalid source."));
		exit (-1);
	}
	//----------------------------------------
	LogMessage ("    - Memory Controller Clock Source (MCCKSEL) => ");
	switch ((ClkSel>>12)&0x03)
	{
	case 0:
		LogPass ("PLL 1. \n");
		break;
	case 2:
		LogWarning ("CLKREF Clock (debug). \n");
		break;
	case 3:
		LogWarning ("MCBPCK Clock (debug). \n");
		break;
	default :
		LOG_ERROR  (("Invalid source."));
		exit (-1);
	}
	//----------------------------------------

	//----------------------------------------
	ClkDiv1 = READ_REG (CLKDIV1);
	LogMessage ("  * CLKDIV1 = 0x%08lX \n",ClkDiv1);
	if ((ClkDiv1&0x1)==0)
		LogWarning ("    > CLK2(AXI16) = CPU clock. \n");
	else
		LogPass ("    > CLK2(AXI16) = CPU clock /2.  \n");
	switch ((ClkDiv1>>26)&0x03)
	{
	case 0:
		LogWarning ("    > CLK4 = CLK2 \n");
		break;
	case 1:
		LogPass ("    > CLK4 = CLK2/2 \n");
		break;
	case 2:
		LogWarning ("    > CLK4 = CLK2/3 \n");
		break;
	case 3:
		LogWarning ("    > CLK4 = CLK2/4 \n");
		break;
	}
	ClkDiv2 = READ_REG (CLKDIV2);
	LogMessage ("  * CLKDIV2 = 0x%08lX \n",ClkDiv2);
	ClkDiv3 = READ_REG (CLKDIV3);
	LogMessage ("  * CLKDIV3 = 0x%08lX \n",ClkDiv3);
	//----------------------------------------
} 


//------------------------------------------------------------------------------
int Check_PWRGD_PS (void)
{
	if (READ_BIT_REG(BPCFMSTAT,3)==0)
	{
		LogError (" PWRGD_PS signal is low. Set PWRGD_PS high and try again.");
		return (-1);
	}
	LogPass ("> PWRGD_PS signal is high.\n");
	return (0);
}
//------------------------------------------------------------------------------




//-------------------------------------------------------------
void dumpGFX_InfoRegs (DWORD *hRes, DWORD *vRes, DWORD *colorDepth, DWORD *pll_freq)
{
	int i;
	const DWORD COLOR_DEPTH_MAPPING[8] = {8, 15, 16, 24, 0, 0, 0, 32};

	*hRes = 0; *vRes = 0; *colorDepth = 0; *pll_freq = 0;

	LogMessage ("\n\n");
	LogHeader("Dump Graphics Core (GFX) Information Registers (Read-Only)");
	for (i = 0; i < (NUM_OF_GFXI_REGS); ++i)
	{
		LogMessage("> %s = 0x%02X\n", GFXI_Regs[i].Name, (HW_BYTE(GFXI_Regs[i].address)));
	}

	LogMessage ("\n");
	LogMessage("Parsing:\n");

	// calculate pixel clock PLL:
	DWORD FeedbackDivider = (WORD) (READ_BIT_REG(GPLLST,6) << 9) | (READ_BIT_REG(GPLLINDI,7) << 8) | (READ_REG(GPLLFBDI));
	BYTE InputDivider =(BYTE) READ_REG(GPLLINDI) & FIELD_MASK(0, 5);
	BYTE OutputDivider1 = GET_FIELD_REG(GPLLST, 0 , 2);
	BYTE OutputDivider2 = GET_FIELD_REG(GPLLST, 3 , 5);

	if (FeedbackDivider && InputDivider && OutputDivider1 && OutputDivider2)
	{
		*pll_freq = (25 * (FeedbackDivider)) / ((DWORD) InputDivider * (DWORD) OutputDivider1 * (DWORD) OutputDivider2) ;
	}

	LogMessage("> Calculated PLLG output: %u[MHz] (rounded)\n", *pll_freq);

	*hRes = (((DWORD) READ_REG(HVCNTH) << 8) | ((DWORD) READ_REG(HVCNTL))) + 1;
	*vRes = (((DWORD) READ_REG(VVCNTH) << 8) | ((DWORD) READ_REG(VVCNTL)));
	*colorDepth = COLOR_DEPTH_MAPPING[READ_REG(COLDEP)];
	LogMessage("> Resolution: %ux%ux%ubpp\n", *hRes, *vRes, *colorDepth);

	
}
//-------------------------------------------------------------------------------------------------------

int main_menu (void)
{
	DWORD hRes, vRes, colorDepth, pll_freq;
	int err = 0;
	//---------------------------------------
	// init global
	GFX_IsPllDebug_g = FALSE;
	//---------------------------------------

	LogMessage ("> INTCR = 0x%08lx \n", READ_REG(INTCR));

	SET_BIT_REG(INTCR2, 0);
	Sleep (1000);
	SET_BIT_REG(IPSRST2, 10);
	Sleep (1000);


	LogMessage (" Release PCI reset (forced off for Hostless mode or when Host is powered-down).\n");
	if (!(Check_PWRGD_PS() != 0)) {
		SET_BIT_REG(INTCR3, 30);
		LogMessage ("> Released internal nPCIRST. Reset is forced off. The input level of nPCIRST and the PCI Express reference clock doesn't affect the operation of PCI devices.\n");
	}

	Sleep (1000);

	LogMessage ("Select DAC2. \n");
	if (READ_BIT_REG (MFSEL3,22)!=0)
	{
		LogWarning("> Clear GPOCSEL bit. Select VSYNC2, HSYNC2, DDC2SCL, DDC2SDA. \n");
		CLEAR_BIT_REG (MFSEL3, 22);
	}
	else
		LogPass("> VSYNC2, HSYNC2, DDC2SCL, DDC2SDA are selected. \n");

	if (READ_FIELD_REG (INTCR,2,10)!=0x1)
	{
		LogWarning("> Set DACOSOVR to 01b. DAC output is seclected by DACSEL bit. \n");
		WRITE_FIELD_REG (INTCR,2,10,0x1);
	}
	else
		LogPass("> DAC output is seclected by DACSEL bit. \n");

	if (READ_BIT_REG (INTCR,14)!=1)
	{
		LogWarning("> Set DACSEL bit. Select DAC2 output. \n");
		SET_BIT_REG (INTCR, 14);
	}
	else
		LogPass("> DAC2 output is selected. \n");

	LogMessage ("> INTCR = 0x%08lx \n", READ_REG(INTCR));

	Sleep (1000);

	LogMessage ("Init GFX (using AHB2PCI bridge) to 1920x1200@16bit \n");
	if (!GFX_ConfigureDisplayTo1920x1200(COLOR_DEPTH_16BIT)) {
		LogError ("Failed to Init GFX to 1920x1200@16bit \n");
		err = 2;
		goto exit;
	}
	
	dumpGFX_InfoRegs(&hRes, &vRes, &colorDepth, &pll_freq);
	if ((hRes!=1920) || (vRes!=1200) || (colorDepth!=16) || (pll_freq!=154 )) {
		LogError ("Failed to configure GFX. Check register info.\n");
		err = 2;
		goto exit;
	} else {
		LogPass ("GFX configuration succeed.\n");
	}

	Sleep (1000 * 1000);

	LogMessage ("Init GFX (using AHB2PCI bridge) to 1024x768@16bit \n");
	if (!GFX_ConfigureDisplayTo1024x768(COLOR_DEPTH_16BIT)){
		LogError ("Failed to Init GFX to 1024x768@16bit\n");
		err = 1;
		goto exit;
	}

	dumpGFX_InfoRegs(&hRes, &vRes, &colorDepth, &pll_freq);
	if ((hRes!=1024) || (vRes!=768) || (colorDepth!=16) || (pll_freq!=65)) {
		LogError ("Failed to configure GFX. Check register info.\n");
		err = 1;
		goto exit;
	} else {
		LogPass ("GFX configuration succeed.\n");
	}

	Sleep (1000 * 1000);

exit:
	LogMessage ("Set GFX memory to 0 \n");
	GFX_ClearMemorySpace(0);

	CLEAR_BIT_REG(IPSRST2, 10);
	Sleep (1000);
	CLEAR_BIT_REG(INTCR2, 0);
	Sleep (1000);
	CLEAR_BIT_REG(INTCR3, 30);
	Sleep (1000);
	return err;
}




	


	
	



