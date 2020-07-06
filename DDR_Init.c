/******************************************************************
 * COPYRIGHT (c) 2015 Nuvoton, Inc.  All rights reserved.         *
 * -------------------------------------------------------------- *
 * This code is proprietary and confidential information of       *
 * Nuvoton. It may not be reproduced, used or transmitted         *
 * in any form whatsoever without the express and written         *
 * permission of Nuvoton.                                         *
 *****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Common.h"
#include "CoreRegisters.h"
#include "TestMsgCore.h"
#include "Utility.h"
#include "Poleg.h"
#include "DDR4_FHY_Registers.h"
#include "MC_Micron_regconfig_C.h"
#include "MC_Cadence_Define_Register_CTL_C.h"

static int l_MC_Init (void);
static int l_DDR4_Init (void);
static int l_MemTest (void);

static int l_vref_training (void);
static int l_WaitForSclDone (void);
static int l_phy_init (void);
static int l_phy_write_levelling (void);
static int l_phy_read_leveling (void);
static int l_vref_training (void);
static int l_phy_levelling (void);


//----------------------------------------------------------------------------------------
#define FREQ_500MHz 500
#define FREQ_667MHz 667
#define FREQ_800MHz 800

#define DDR4_FREQ FREQ_800MHz
#define CPU_FREQ  FREQ_800MHz
#define LANE_MASK 3 // 3 if lane_num == 2;  7 if lane_num == 3
#define NUM_OF_LANES 2

#define VREF_STARTPOINT 0x7 // @ range 1 it value is 75% of DRAM Voltage.

#define IOW32(addr,data) WRITE_REG(HW_DWORD(addr),data)
#define IOR32(addr)      READ_REG(HW_DWORD(addr))
#define REG_SET_FIELD(address,width,position,value) IOW32(address, (IOR32(address) & (~((((UINT32)1<<(width))-1)<<(position))))  | ((value)<<(position)))
#define REG_READ_FIELD(address,width,position) 	   ((IOR32(address)>>(position)) & ((((UINT32)1)<<(width))-1))

//------------------------------------------------------------------------------------------------------
// Configure PLL0 (CPU) to 800MHz and PLL 1 (DDR4) to 800MHz/667MHz/500MHz
//------------------------------------------------------------------------------------------------------
static void l_Clock_Init (void)
{
	UINT32 CLKSEL_reg;

	LogMessage ("> Clock Init \n");
	UartWaitForEmpty();
	
	// Change all clock MUX to CLKREF
	CLKSEL_reg = CLKSEL;
	CLKSEL = 0x14AAAAA;

	if (PDID == 0x00A92750)
	{ // for Z1
		#if DDR4_FREQ == FREQ_800MHz
				PLLCON1 = 0x00402201; //800Mhz (25MHz*64)/2/1
				//PLLCON1 = 0x00202101; //800Mhz (25MHz*32)/1/1 		
		#elif DDR4_FREQ == FREQ_667MHz
				PLLCON1 = 0x00A02203; // 667Mhz (25MHz*160)/2/3
		#elif DDR4_FREQ == FREQ_500MHz
				PLLCON1 = 0x00282201; // 500Mhz (25MHz*40)/2/1
		#else
			#error "Invalid DDR4_FREQ value";
		#endif
	}
	else
	{ // for Z2 and above
		#if DDR4_FREQ == FREQ_800MHz
				PLLCON1 = 0x00402101; // 800Mhz [(25MHz*64)/1/1]/2
		#elif DDR4_FREQ == FREQ_667MHz
				PLLCON1 = 0x00A02103; // 667Mhz [(25MHz*160)/1/3]/2
		#elif DDR4_FREQ == FREQ_500MHz
				PLLCON1 = 0x00282101; // 500Mhz [(25MHz*40)/1/1]/2
		#else
			#error "Invalid DDR4_FREQ value";
		#endif
	}

	#if CPU_FREQ == FREQ_800MHz
		PLLCON0 = 0x00402201; //800Mhz (25MHz*64)/2/1
		//PLLCON1 = 0x00202101; //800Mhz (25MHz*32)/1/1 	
		
		// Configure all system clocks (dividers value are suitable for CPU 800MHz); the main purpose is to enable UART log on test mode.
		CLKDIV1 = 0x54008251; 
		CLKDIV2 = 0xEA4F8F97; 
		CLKDIV3 = 0x0000013E;
		CLKSEL_reg = 0x01F38E09;  

	#elif CPU_FREQ == FREQ_667MHz
		PLLCON0 = 0x00A02203; // 667Mhz (25MHz*160)/2/3

		// Configure all system clocks (dividers value are suitable for CPU 667MHz); the main purpose is to enable UART log on test mode.
		CLKDIV1 = 0x54005191; 
		CLKDIV2 = 0x954F4F94; 
		CLKDIV3 = 0x0000013E;
		CLKSEL_reg = 0x01F38E08;   

	#else
		#error "Invalid CPU_FREQ value";
	#endif


	// wait for PLLs to lock
	while ((PLLCON1 & 0x80000000) == 0);
	while ((PLLCON0 & 0x80000000) == 0);


	// at this time , no need to drive clkout on pad 
	// Divide PLL1 by 30 before driving it out to clkout pin
	//WRITE_FIELD_REG (CLKDIV2,5,16,30-1); // CLKOUTDIV = 29 (800MHz/30 = 26.6MHz)
	//SET_BIT_REG (MFSEL1, 21); // Set CLKOUT/GPIO160 MUX to CLKOUT
	//CLEAR_BIT_REG (MFSEL4, 4); // Set [CLKOUT/GPIO160]/RNGOSCOUT MUX to CLKOUT/GPIO160

	Sleep (10);
	CLKSEL = ((CLKSEL_reg & 0xFFE7CFFC) | 0x00040000 ); // assuming CLKREF is at reset value (0x004A_AAAA), CLKREF => 0x0046_8AA8  (CPUCKSEL=PLL0, MCCKSEL=PLL1, CLKOUTSEL=PLL1) 

}
//-------------------------------------------------------------
extern int DDR_Init (void)
{

	LogHeader ("DDR Init");

	if (READ_BIT_REG(INTCR2, 19) == 1)
	{
		LogWarning ("> DDR is already init, skipping DDR Init.\n");
		return 0;
	}

	l_Clock_Init ();
	
	while (1)
	{
		while (l_DDR4_Init()!=0); // repeat DDR4 init endless if fail
		if (l_MemTest()==0)
		{
			LogPass ("Done.\n");
			return (0);
		}
	}

	return (0);
} 
//------------------------------------------------------------------------------------------------------
const UINT32 Denali_Ctl_Initialization[] = {
	DENALI_CTL_00_DATA,
	DENALI_CTL_01_DATA,
	DENALI_CTL_02_DATA,
	DENALI_CTL_03_DATA,
	DENALI_CTL_04_DATA,
	DENALI_CTL_05_DATA,
	DENALI_CTL_06_DATA,
	DENALI_CTL_07_DATA,
	DENALI_CTL_08_DATA,
	DENALI_CTL_09_DATA,
	DENALI_CTL_10_DATA,
	DENALI_CTL_11_DATA,
	DENALI_CTL_12_DATA,
	DENALI_CTL_13_DATA,
	DENALI_CTL_14_DATA,
	DENALI_CTL_15_DATA,
	DENALI_CTL_16_DATA,
	DENALI_CTL_17_DATA,
	DENALI_CTL_18_DATA,
	DENALI_CTL_19_DATA,
	DENALI_CTL_20_DATA,
	DENALI_CTL_21_DATA,
	DENALI_CTL_22_DATA,
	DENALI_CTL_23_DATA,
	DENALI_CTL_24_DATA,
	DENALI_CTL_25_DATA,
	DENALI_CTL_26_DATA,
	DENALI_CTL_27_DATA,
	DENALI_CTL_28_DATA,
	DENALI_CTL_29_DATA,
	DENALI_CTL_30_DATA,
	DENALI_CTL_31_DATA,
	DENALI_CTL_32_DATA,
	DENALI_CTL_33_DATA,
	DENALI_CTL_34_DATA,
	DENALI_CTL_35_DATA,
	DENALI_CTL_36_DATA,
	DENALI_CTL_37_DATA,
	DENALI_CTL_38_DATA,
	DENALI_CTL_39_DATA,
	DENALI_CTL_40_DATA,
	DENALI_CTL_41_DATA,
	DENALI_CTL_42_DATA,
	DENALI_CTL_43_DATA,
	DENALI_CTL_44_DATA,
	DENALI_CTL_45_DATA,
	DENALI_CTL_46_DATA,
	DENALI_CTL_47_DATA,
	DENALI_CTL_48_DATA,
	DENALI_CTL_49_DATA,
	DENALI_CTL_50_DATA,
	DENALI_CTL_51_DATA,
	DENALI_CTL_52_DATA,
	DENALI_CTL_53_DATA,
	DENALI_CTL_54_DATA,
	DENALI_CTL_55_DATA,
	DENALI_CTL_56_DATA,
	DENALI_CTL_57_DATA,
	DENALI_CTL_58_DATA,
	DENALI_CTL_59_DATA,
	DENALI_CTL_60_DATA,
	DENALI_CTL_61_DATA,
	DENALI_CTL_62_DATA,
	DENALI_CTL_63_DATA,
	DENALI_CTL_64_DATA,
	DENALI_CTL_65_DATA,
	DENALI_CTL_66_DATA,
	DENALI_CTL_67_DATA,
	DENALI_CTL_68_DATA,
	DENALI_CTL_69_DATA,
	DENALI_CTL_70_DATA,
	DENALI_CTL_71_DATA,
	DENALI_CTL_72_DATA,
	DENALI_CTL_73_DATA,
	DENALI_CTL_74_DATA,
	DENALI_CTL_75_DATA,
	DENALI_CTL_76_DATA,
	DENALI_CTL_77_DATA,
	DENALI_CTL_78_DATA,
	DENALI_CTL_79_DATA,
	DENALI_CTL_80_DATA,
	DENALI_CTL_81_DATA,
	DENALI_CTL_82_DATA,
	DENALI_CTL_83_DATA,
	DENALI_CTL_84_DATA,
	DENALI_CTL_85_DATA,
	DENALI_CTL_86_DATA,
	DENALI_CTL_87_DATA,
	DENALI_CTL_88_DATA,
	DENALI_CTL_89_DATA,
	DENALI_CTL_90_DATA,
	DENALI_CTL_91_DATA,
	DENALI_CTL_92_DATA,
	DENALI_CTL_93_DATA,
	DENALI_CTL_94_DATA,
	DENALI_CTL_95_DATA,
	DENALI_CTL_96_DATA,
	DENALI_CTL_97_DATA,
	DENALI_CTL_98_DATA,
	DENALI_CTL_99_DATA,
	DENALI_CTL_100_DATA,
	DENALI_CTL_101_DATA,
	DENALI_CTL_102_DATA,
	DENALI_CTL_103_DATA,
	DENALI_CTL_104_DATA,
	DENALI_CTL_105_DATA,
	DENALI_CTL_106_DATA,
	DENALI_CTL_107_DATA,
	DENALI_CTL_108_DATA,
	DENALI_CTL_109_DATA,
	DENALI_CTL_110_DATA,
	DENALI_CTL_111_DATA,
	DENALI_CTL_112_DATA,
	DENALI_CTL_113_DATA,
	DENALI_CTL_114_DATA,
	DENALI_CTL_115_DATA,
	DENALI_CTL_116_DATA,
	DENALI_CTL_117_DATA,
	DENALI_CTL_118_DATA,
	DENALI_CTL_119_DATA,
	DENALI_CTL_120_DATA,
	DENALI_CTL_121_DATA,
	DENALI_CTL_122_DATA,
	DENALI_CTL_123_DATA,
	DENALI_CTL_124_DATA,
	DENALI_CTL_125_DATA,
	DENALI_CTL_126_DATA,
	DENALI_CTL_127_DATA,
	DENALI_CTL_128_DATA,
	DENALI_CTL_129_DATA,
	DENALI_CTL_130_DATA,
	DENALI_CTL_131_DATA,
	DENALI_CTL_132_DATA,
	DENALI_CTL_133_DATA,
	DENALI_CTL_134_DATA,
	DENALI_CTL_135_DATA,
	DENALI_CTL_136_DATA,
	DENALI_CTL_137_DATA,
	DENALI_CTL_138_DATA,
	DENALI_CTL_139_DATA,
	DENALI_CTL_140_DATA,
	DENALI_CTL_141_DATA,
	DENALI_CTL_142_DATA,
	DENALI_CTL_143_DATA,
	DENALI_CTL_144_DATA,
	DENALI_CTL_145_DATA,
	DENALI_CTL_146_DATA,
	DENALI_CTL_147_DATA,
	DENALI_CTL_148_DATA,
	DENALI_CTL_149_DATA,
	DENALI_CTL_150_DATA,
	DENALI_CTL_151_DATA,
	DENALI_CTL_152_DATA,
	DENALI_CTL_153_DATA,
	DENALI_CTL_154_DATA,
	DENALI_CTL_155_DATA,
	DENALI_CTL_156_DATA,
	DENALI_CTL_157_DATA,
	DENALI_CTL_158_DATA,
	DENALI_CTL_159_DATA,
	DENALI_CTL_160_DATA,
	DENALI_CTL_161_DATA,
	DENALI_CTL_162_DATA,
	DENALI_CTL_163_DATA,
	DENALI_CTL_164_DATA,
	DENALI_CTL_165_DATA,
	DENALI_CTL_166_DATA,
	DENALI_CTL_167_DATA,
	DENALI_CTL_168_DATA,
	DENALI_CTL_169_DATA,
	DENALI_CTL_170_DATA,
	DENALI_CTL_171_DATA,
	DENALI_CTL_172_DATA,
	DENALI_CTL_173_DATA,
	DENALI_CTL_174_DATA,
	DENALI_CTL_175_DATA,
	DENALI_CTL_176_DATA,
	DENALI_CTL_177_DATA,
	DENALI_CTL_178_DATA,
	DENALI_CTL_179_DATA,
	DENALI_CTL_180_DATA,
	DENALI_CTL_181_DATA,
	DENALI_CTL_182_DATA,
	DENALI_CTL_183_DATA,
	DENALI_CTL_184_DATA,
	DENALI_CTL_185_DATA,
	DENALI_CTL_186_DATA,
	DENALI_CTL_187_DATA,
	DENALI_CTL_188_DATA,
	DENALI_CTL_189_DATA,
	DENALI_CTL_190_DATA,
	DENALI_CTL_191_DATA,
	DENALI_CTL_192_DATA,
	DENALI_CTL_193_DATA,
	DENALI_CTL_194_DATA,
	DENALI_CTL_195_DATA,
	DENALI_CTL_196_DATA,
	DENALI_CTL_197_DATA,
	DENALI_CTL_198_DATA,
	DENALI_CTL_199_DATA,
	DENALI_CTL_200_DATA,
	DENALI_CTL_201_DATA,
	DENALI_CTL_202_DATA,
	DENALI_CTL_203_DATA,
	DENALI_CTL_204_DATA,
	DENALI_CTL_205_DATA,
	DENALI_CTL_206_DATA,
	DENALI_CTL_207_DATA,
	DENALI_CTL_208_DATA,
	DENALI_CTL_209_DATA,
	DENALI_CTL_210_DATA,
	DENALI_CTL_211_DATA,
	DENALI_CTL_212_DATA,
	DENALI_CTL_213_DATA,
	DENALI_CTL_214_DATA,
	DENALI_CTL_215_DATA,
	DENALI_CTL_216_DATA,
	DENALI_CTL_217_DATA,
	DENALI_CTL_218_DATA,
	DENALI_CTL_219_DATA,
	DENALI_CTL_220_DATA,
	DENALI_CTL_221_DATA,
	DENALI_CTL_222_DATA,
	DENALI_CTL_223_DATA,
	DENALI_CTL_224_DATA,
	DENALI_CTL_225_DATA,
	DENALI_CTL_226_DATA,
	DENALI_CTL_227_DATA,
	DENALI_CTL_228_DATA,
	DENALI_CTL_229_DATA,
	DENALI_CTL_230_DATA,
	DENALI_CTL_231_DATA,
	DENALI_CTL_232_DATA,
	DENALI_CTL_233_DATA,
	DENALI_CTL_234_DATA,
	DENALI_CTL_235_DATA,
	DENALI_CTL_236_DATA,
	DENALI_CTL_237_DATA
};

//------------------------------------------------------------------
static int l_WaitForSclDone (void)
{
	UINT32 SCL_START_reg;
	UINT32 TimeOut = 400; // 20/10/2015: increase timeout from 200 to 400
	while (1)  
	{
		SCL_START_reg = READ_REG(SCL_START);   
		if ((SCL_START_reg & ((UINT32)1<<28)) == 0)
			return (0);

		if (TimeOut==0)
			return (-1);

		TimeOut--;	
	}
}
//------------------------------------------------------------------
static int l_phy_init (void)
{
	// Note: In Z1, software reset does not reset PHY registers 

	UINT32 TmpReg32;
	UINT32 TimeOut;

	//LogMessage (">LOG_PHY_INIT_START\n");

	WRITE_REG (SCL_START, 0x00000000);
	WRITE_REG (DSCL_CNT, 0); 
    WRITE_REG (PHY_DLL_INCR_TRIM_3, 0); 
	WRITE_REG (PHY_DLL_INCR_TRIM_1, 0); 
	
	WRITE_REG (SCL_WINDOW_TRIM, 0x00000001); 
	WRITE_REG (PHY_DLL_RECALIB, 0xA0001000); 

	WRITE_REG (PHY_PAD_CTRL, PHY_DRV_DQ_48R | PHY_DRV_CTL_48R | PHY_DRV_CLK_48R | PHY_ODT_DQ_120R | 0x50EEC0E8);   

	// WRLVL_ON_OFF: (MR1 field values to be used during Write Levelling Entry and Exit): DDR4 Device
	// Should be the same value as in DENALI_CTL_80_DATA
	WRITE_REG (WRLVL_ON_OFF, ((MC_DDR_WL_ENABLE|MC_MR1) << 16) | MC_MR1); 

	// WRLVL_DYN_ODT: (MRS2 field values to be used during Write Levelling Entry and Exit): DDR4 Device
	// Should be the same value as in DENALI_CTL_81_DATA
	WRITE_REG (WRLVL_DYN_ODT, 0x00D000D0);  //  Dynamic ODT Off, CAS Write Latency = 11, Auto Self Refresh, Write CRC disable 

	// Update SCL_CONFIG_1, SCL_CONFIG_2 and SCL_CONFIG_3 according to DENALI MC init !
	WRITE_REG (SCL_CONFIG_1, 0x01004069);
	WRITE_REG (SCL_CONFIG_2, 0x83000601 | (1<<28));		// lior: enable analog_dll_for_scl bit 28 (origin 0x83000601)
	WRITE_REG (SCL_CONFIG_3, 7 - LANE_MASK);	//  4 for 2 lanes (skip lane-2); 0 is for 3 lanes with ECC (no skip)
	
	WRITE_REG (DDR4_CONFIG_1, 0x81); //  scl_bit_lvl_in_ddr4  | ddr4
    WRITE_REG (DYNAMIC_WRITE_BIT_LVL, 0x000231C1);   // LSB is bit levelling enable in DSCL = run in DSCL

	WRITE_REG (SCL_CONFIG_4,0x00000000);  
    WRITE_REG (PHY_DLL_ADRCTRL, 0x0000021A); // Prior to write-levelling this register must be programmed

	WRITE_REG (PHY_LANE_SEL,0);
	WRITE_REG (PHY_DLL_TRIM_CLK, 0x0000005A); // Used to control the output delay of clock with respect to data signals (dq/dqs/dm). // Prior to write-levelling this register must be programmed
    WRITE_REG (PHY_DLL_RECALIB, 0xA800101A); // PHY_DLL_RECALIB  
	WRITE_REG (SCL_LATENCY, 0x00005076); 
 
	//-----------------------------------------------------------------------
	//LogMessage ("  >> Reset DLL and wait for lock ... ");  // This state will update: mas_dly, dll_mas_dly, dll_slv_dly_wire, dlls_trim_clk, rise_cycle_cnt,
	WRITE_REG (UNQ_ANALOG_DLL_1, 0x00000007); // reset DLL for all lanes
	Sleep(1); // ~ 10usec
	WRITE_REG (UNQ_ANALOG_DLL_1, 0x00000000); // release DLL reset 
	TimeOut = 20;
	while (1)
	{  
		TmpReg32 = READ_REG (UNQ_ANALOG_DLL_2);	
		if (( TmpReg32 & LANE_MASK ) == LANE_MASK )  // 1-pass, 0-fail
			break;

		if (TimeOut==0)
		{
			LogMessage ("LOG_ERROR_PHY_INIT_DLL_TIMEOUT\n");
			return (-1);
		}

		TimeOut--;
	}
	Sleep(1); // ~ 10usec
	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------
	//LogMessage ("  >> ZQ long calibration  ... ");
	UINT32 nfet_cal = 0;
	UINT32 pfet_cal = 0;
	WRITE_REG (UNIQUIFY_IO_1, 0); 
	WRITE_REG (UNIQUIFY_IO_1, (UINT32)1<<6/*cal_long*/); // issue ZQL calibration long start, clear cal_end_seen and upd_phy_reg.
	TimeOut = 10000;
	while (1)
	{  
		TmpReg32 = READ_REG (UNIQUIFY_IO_1); 
		if (( TmpReg32 & 0x2 ) != 0 )
			break;

		if (TimeOut==0)
		{
			//LogError ("\n  >>  Timeout. UNIQUIFY_IO_1(0x5C) = 0x%08lX. \n",TmpReg32); 
			return (-1);
		}

		TimeOut--;

		Sleep(1); 
	}
	//LogPass ("  Completed successfully.\n");
	TmpReg32 = READ_REG (UNIQUIFY_IO_2);
	nfet_cal = ( TmpReg32 >> 8 ) & 0xf ;
	pfet_cal = ( TmpReg32 >> 0 ) & 0xf ;
	//LogWarning ("    * ZQ Values:  nfet_cal:0x%lX, pfet_cal:0x%lX", nfet_cal, pfet_cal);
	//------------------------------------------------------
	
	TmpReg32 = (UINT32)1<<2/*upd_phy_reg, 0->1*/; // force update I/O 
	WRITE_REG (UNIQUIFY_IO_1, TmpReg32);  // affects in 4 cycles
	Sleep(10);

	// 22/08/2016: disabled Auto ZQ calibration (see Errata)


	//LogWarning (".\n");
	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------
	//LogMessage ("  >> Init lane related registers ... \n");
	UINT32 ilane,ibit;
	for (ilane=0; ilane<3; ilane++)
	{
		// reset all DQ trim delay
		for (ibit = 0 ; ibit < 8 ; ibit++) 
		{
			WRITE_REG (PHY_LANE_SEL,(ilane*7)+((UINT32)ibit<<8));
			WRITE_REG (OP_DQ_DM_DQS_BITWISE_TRIM, 0); // output DQn
			WRITE_REG (IP_DQ_DQS_BITWISE_TRIM,  0); // input DQn
		}
		ibit = 8;
		WRITE_REG (PHY_LANE_SEL,(ilane*7)+((UINT32)ibit<<8));

		#if DDR4_FREQ == FREQ_800MHz
			WRITE_REG (IP_DQ_DQS_BITWISE_TRIM,0x1F); // input DQS  
		#elif DDR4_FREQ == FREQ_667MHz
			WRITE_REG (IP_DQ_DQS_BITWISE_TRIM,0x1F); // input DQS  
		#elif DDR4_FREQ == FREQ_500MHz
			WRITE_REG (IP_DQ_DQS_BITWISE_TRIM,0x28); // input DQS  
		#else
			#error "Invalid DDR4_FREQ value";
		#endif


		WRITE_REG (OP_DQ_DM_DQS_BITWISE_TRIM,0); // output DM 
		ibit = 9;
		WRITE_REG (PHY_LANE_SEL,(ilane*7)+((UINT32)ibit<<8));
		WRITE_REG (OP_DQ_DM_DQS_BITWISE_TRIM,0x0000001F); // output DQS 

		// Init BMC and DDR VREF (Note: DDR VREF can be be update after DLL is valid)
		WRITE_REG (PHY_LANE_SEL,(ilane*7));
		WRITE_REG (VREF_TRAINING, (VREF_STARTPOINT<<12) | (VREF_STARTPOINT<<4) | 0x04 /*| 0x02 | 0x01*/);  // vref_output_enable /*| update_ddr_vref | vref_training */
	
		// Init PHY_DLL_TRIM_1 and PHY_DLL_TRIM_3
		WRITE_REG (PHY_LANE_SEL,(ilane*6));
		WRITE_REG (PHY_DLL_TRIM_1,0x00000001); 
		WRITE_REG (PHY_DLL_TRIM_3,0x00000010); 

		// Init phase1 and phase2 
		WRITE_REG (PHY_LANE_SEL,(ilane*5));
		WRITE_REG (UNQ_ANALOG_DLL_3,0x00001010); // set phase1 and phase2 to delay by 1/4 cycle  

		// Init data_capture_clk to default value after reset 
		WRITE_REG (PHY_LANE_SEL,(ilane*8));
		WRITE_REG (SCL_DCAPCLK_DLY, 0x10); 

		// Init main_clk_delta_dly 
		WRITE_REG (PHY_LANE_SEL,(ilane*3)); 
		WRITE_REG (SCL_MAIN_CLK_DELTA, 0); 
	}

	// Init dlls_trim_2 
	WRITE_REG (PHY_DLL_TRIM_2, (UINT32)1<<16);	// Clear all 'dlls_trim_2' registers 
	//-----------------------------------------------------------------------

	return (0);
}

//-----------------------------------------------------------------------
static int l_phy_write_levelling (void)
{
	UINT32 TmpReg32; 

	//LogMessage (">LOG_WRITE_LEVELLING_START\n");

	//------------------------------------------------------------------
	// Write-Leveling 
	// -----------------
	// SCL tune output dq/dqs/dm (dlls_trim_2) delay so DQS output signal arrive at same phase as MCLK signal at each DRAM devices side.
	// SCL send command to DRAM device to enter DRAM into Write-Levelling mode. In this mode the DRAM sample MCLK using DQSn strobe; sampled value (high or low) is output on DQnx corespanding lane.
	// SCL scan dlls_trim_2 from 0 to 63 and trigger DQS pulses until low to high data is detected (0x00 to 0xFF). 
	// For this stage to work, PHY VREF level should be at a working area. Input DQnx delay are not relevant duo to static DQnx signal in this test.
	// The SCL update dlls_trim_2 that adjust output dq/dqs/dm timing with respect to DRAM clock; 
	//------------------------------------------------------------------

	WRITE_REG (WRLVL_AUTOINC_TRIM, LANE_MASK);	// Set 'start_wr_lvl' for all lanes               
	WRITE_REG (PHY_DLL_TRIM_2, (UINT32)1<<16);	// Clear all 'dlls_trim_2' registers 
	WRITE_REG(SCL_START, ((UINT32)1<<30)/*wrlvl*/ | ((UINT32)1<<28)/*set_ddr_scl_go_done*/ );	
	if (l_WaitForSclDone ()!=0)
	{
		LogMessage ("LOG_ERROR_WRITE_LEVELLING_TIMEOUT\n");
		return (-1);
	}

	// Write Levelling completed, check if it passed
	TmpReg32 = READ_REG (WRLVL_CTRL);
	if ( (TmpReg32 & LANE_MASK) != LANE_MASK ) // 1-pass, 0-fail
	{
		LogMessage ("LOG_ERROR_WRITE_LEVELLING_ERROR\n");
		return (-1);
	}
	//------------------------------------------------------------------
	return (0);
}
//-----------------------------------------------------------------------
static int l_phy_bit_leveling (void) // Step 3 to 5 of Uniquify doc
{
	UINT32 TmpReg32; 
	UINT32 ilane, ibit;

	//LogMessage (">LOG_BIT_LEVELLING_START\n");

		//------------------------------------------------------------------
	// Read/Write Bit-Levelling (normal mode using DDR array)
	// ----------------------------------------------------------------
	// SCL Read Bit-Levelling expect for 0xFFFF0000 data pattern to previously written to DDR address 0x20-0x3F (0x20 offset from SCL_START when SCL starts) 
	// SCL Write Bit-Levelling write 0xFFFF0000 data pattern to DDR  
	// When SCL Bit-Levelling completed, Input and Output DQn delay registers are update. 
	// There is a bug in the PHY that Output DM delay registers are not update and as a bypass, we average Output DQn and use it as Output DM assuming DM traces are same as all other DQn traces in the lane.
	//------------------------------------------------------------------
	
	//LogMessage ("  >> Run Read/Write Bit-Levelling ... "); // Parameters Update: Input and output DQn - not DM

	// Step 3. Write in bit levelling data pattern into DDR at address 0x0020 - 0x003F
	WRITE_REG (SCL_DATA_0, 0xff00ff00); 
	WRITE_REG (SCL_DATA_1, 0xff00ff00); 
	WRITE_REG (PHY_SCL_START_ADDR,((UINT32)0x8 << 16)); // scl_start_col_addr = 0x0008;  scl_start_row_addr = 0x0000; DDR address 0x0020 for 16 x16bit.
	WRITE_REG(SCL_START, ((UINT32)1<<24)/*wr_only*/ | ((UINT32)1<<28)/*set_ddr_scl_go_done*/ );  //  write SCL data to DRAM   
	if (l_WaitForSclDone ()!=0)
	{
		LogMessage ("LOG_ERROR_BIT_LEVELLING_TIMEOUT_STEP3\n");
		return (-1);
	}

	// Step 4
	WRITE_REG (PHY_SCL_START_ADDR,((UINT32)0x00 << 16)); // scl_start_col_addr = 0x0000;  scl_start_row_addr = 0x0000; DDR address 0x0000 for 16 x16bit.

	// Step 5 - Run Bit-Levelling
	WRITE_REG (SCL_START, ((UINT32)1<<29)/*continuous_rds*/  | ((UINT32)1<<20)/*write_side_bit_lvl*/ | ((UINT32)1<<22)/*bit_lvl_norm*/ |  ((UINT32)1<<28)/*set_ddr_scl_go_done*/); 
	if (l_WaitForSclDone ()!=0)
	{
		LogMessage ("LOG_ERROR_BIT_LEVELLING_TIMEOUT_STEP5\n");
		return (-1);
	}
	
	// Bit-Levelling completed, check if it read side passed
	TmpReg32 = (READ_REG (DYNAMIC_BIT_LVL) >> 14) & LANE_MASK;
	if ( TmpReg32 != 0)  // 1- fail, 0-pass
	{
		LogMessage ("LOG_ERROR_BIT_LEVELLING_READ_ERROR\n");
		return (-1);;
	}

	// Bit-Levelling completed, check if it write side passed
	TmpReg32 = (READ_REG (DYNAMIC_WRITE_BIT_LVL) >> 20) & LANE_MASK;
	if ( TmpReg32 != 0)  // 1- fail, 0-pass 
	{
		LogMessage ("LOG_ERROR_BIT_LEVELLING_WRITE_ERROR\n");
		return (-1);
	}

	//---------------------------------------------------------	

	//--------------------------------------------------------
	// duo to a bug, DM signal has no training, calculate the average DQn output delay and use the average value as DM delay
	//LogMessage ("  >> Calculate average DQn output delay to use as DM delay for each lanes ... ");
	UINT32 sum;
	for (ilane = 0; ilane < NUM_OF_LANES; ilane++)	
	{		
		sum = 0;
		for (ibit = 0 ; ibit < 8 ; ibit++)
		{
			WRITE_REG (PHY_LANE_SEL, ilane*7 + ibit*0x100);
			TmpReg32 = READ_REG (OP_DQ_DM_DQS_BITWISE_TRIM) & 0x000007f;
			if ((TmpReg32&0x40) == 0x00)
				TmpReg32 = 0x40 - TmpReg32; // value is negative, convert it to negative relative to base 
			sum += TmpReg32;
		}
		sum = ((sum+4) / 8); // 22/07/2015: fixed around func; add +4
		if ((sum&0x40) == 0x00)
			sum = 0x40 - sum;
		WRITE_REG (PHY_LANE_SEL, ilane*7 + 0x800);
		WRITE_REG (OP_DQ_DM_DQS_BITWISE_TRIM, sum);
		    		
	}
	//--------------------------------------------------------
	return (0);
}
//--------------------------------------------------------
static int l_phy_read_leveling (void) // Step 7 to 8 in Uniquify doc
{
	UINT32 TmpReg32; 

	//LogMessage (">LOG_READ_LEVELLING_START\n");

	
	//------------------------------------------------------------------
	// Round-Trip-Delay and Read-Leveling  
	// ------------------------------------
	// Round-Trip-Delay:
	// It tune input gate timing (cycle_cnt) so PHY will know when to expect data arrive from each lane to BMC. This timing relate to the distance between BMC and DDR (Round-Trip-Delay).
	// Note: For unknown reason (bug??) this input gate timing parameter is not exposed to user and can be set by SCL only.  
	// Input DQS to MCLK Sync:
	// It tune dcap_byte_dly (data_capture_clk, clock phase) that sync between input DQS clock and PHY internal DCLK clock.
	//
	// In addition, it tune main_clk_delta_dly (and main_clk_dly) that add extra clock delay to a line between all return data from DDR such a way that a common dfi_rddata_valid can be generated for all byte lanes aligned with the controller’s clock (DCLK)
	// Since we use a single DDR device when lane0 and lane1 traces are most likely with the same length (in 800MHz boundary), this value is always 0.
	//--------------------------------------------------------------------------------------------------------

	//LogMessage ("  >> Run Round-Trip-Delay and Read-Levelling ... ");   

	// Step 7 - Write in SCL data pattern into DDR at address 0
	WRITE_REG (PHY_SCL_START_ADDR,((UINT32)0x00 << 16)); // scl_start_col_addr = 0x0000;  scl_start_row_addr = 0x0000; DDR address 0x0000 for 16 x16bit.
	WRITE_REG (SCL_DATA_0, 0x789b3de0);
	WRITE_REG (SCL_DATA_1, 0xf10e4a56);
	WRITE_REG(SCL_START,((UINT32)1<<24)/*wr_only*/ |(UINT32)1<<28/*set_ddr_scl_go_done*/); //  write SCL data to DRAM 
	if (l_WaitForSclDone ()!=0)
	{
		LogMessage ("LOG_ERROR_READ_LEVELLING_TIMEOUT_STEP7\n");
		return (-1);
	}
	
	// Step 8 - Run Round-Trip-Delay 
	WRITE_REG(SCL_START, (0x34 << 24)); // incr_scl |  set_ddr_scl_go_done | continuous_rds
	if (l_WaitForSclDone ()!=0)
	{
		LogMessage ("LOG_ERROR_READ_LEVELLING_TIMEOUT_STEP8\n");
		return (-1);
	}

	TmpReg32 = READ_REG(SCL_START);
	if ((TmpReg32&LANE_MASK)!=LANE_MASK)  // 0-fail, 1-pass
	{
		LogMessage ("LOG_ERROR_READ_LEVELLING_ERROR\n");
		return (-1);
	}

	return (0);
}
//-----------------------------------------------------------------------------------
#define VREF_NUM_OF_AVRG 5
#define NUM_OF_RUNS 10
// in Z11 in PHY VREF, bit 0 is fixed to VREF select therefore must be fixed to 1 
// Those same values will be use for both PHY and DDR although DDR has no issue.
#define VREF_MAX 0x31 
#define VREF_MIN 0x01 
#define VREF_STEP 0x02 

static int l_vref_training (void)  
{
	int phy_window_diff[3]; // it's sum of diff from several cycles
	UINT32 phy_vref [3] = {VREF_STARTPOINT, VREF_STARTPOINT, VREF_STARTPOINT}; // 6 bits while bit 0 fixed to 1 //  0x17 is common good value, use as starting point
	
	int ddr_window_diff; // it's sum of diff from several cycles
	UINT32 ddr_vref = VREF_STARTPOINT;  // 6 bits // 0x17 is common good value, use as starting point
	UINT32 ddr_vref_range = 0; // 1 bit (in this code it fixed to 0 for range 1 on DDR device side only)

	UINT32 cycle_iteration;
	UINT8 lane;
	UINT8 iave;
	UINT32 val;
	UINT8 bit_lvl_failure_status, last_bit_lvl_failure_status;
	BOOL WasError;

	//LogMessage (">LOG_VREF_TRAINING_START\n");
	// Running VREF training routine
	cycle_iteration = NUM_OF_RUNS; 
	while (cycle_iteration!=0)
	{
		cycle_iteration--;

		//LogMessage ("\n>> cycle_iteration: %lu\n", cycle_iteration);

		//------------------------------------------
		// Set the VREF level at BMC and DDR device
		//-----------------------------------------
		for (lane=0 ; lane<3 ; lane++) 
		{
			WRITE_REG (PHY_LANE_SEL, lane*7);
			val = (ddr_vref<<12) | (ddr_vref_range<<11) | (phy_vref [lane]<<4); 
			WRITE_REG (VREF_TRAINING, val | 0x04/*vref_output_enable*/ | 0x02/*update_ddr_vref*/ | 0x01/*vref_training*/);    
		}
		// Generate DDR VREF update transaction to DDR device
		WRITE_REG (SCL_START,((UINT32)1<<28) /*set_ddr_scl_go_done*/);	
		if (l_WaitForSclDone ()!=0)
		{
			LogMessage ("LOG_ERROR_VREF_TRAINING_TIMEOUT_STEP2\n");
			return (-1);
		}
		// lior: 20-OCT-2015, clear update_ddr_vref bit to avoid from SCL write DDR4 VREF value on the next SCL run.
		WRITE_REG (VREF_TRAINING, val | 0x04/*vref_output_enable*/ | 0x01/*vref_training*/);    

		//------------------------------------------
		// Check windows size - do averaging of several runs
		//-----------------------------------------

		// initialize values before averaging; those will be use to sum windows size of several runs
		for (lane=0; lane<3; lane++)
			phy_window_diff[lane] = 0;
		ddr_window_diff = 0;
		WasError = FALSE;

		// do averaging of several runs
		for (iave=0; iave < VREF_NUM_OF_AVRG; iave++) 
		{    
			//LogMessage (">> iave:%u; ",iave);

			bit_lvl_failure_status = l_phy_bit_leveling ();
			if ( bit_lvl_failure_status != 0) 
			{
				//LogError ("Bit-Levelling at VREF training state failed (bit_lvl_failure_status=0x%X).\n", bit_lvl_failure_status);
				WasError = TRUE;
				last_bit_lvl_failure_status = bit_lvl_failure_status;
			}
			else
			{
				//------------------------------
				// calculate windows size and add them to average 
				for (lane=0; lane<NUM_OF_LANES; lane++) 
				{		
					WRITE_REG (PHY_LANE_SEL, lane*6);
					val = READ_REG (VREF_TRAINING_WINDOWS);

					UINT32 phy_window_0_l = (val >> 0) & 0x3F;	// reg_read_val[5:0];
					UINT32 phy_window_1_l = (val >> 8) & 0x3F;	// reg_read_val[13:8];
					UINT32 ddr_window_0_l = (val >> 16) & 0x3F;	// reg_read_val[21:16];
					UINT32 ddr_window_1_l = (val >> 24) & 0x3F;	// reg_read_val[29:24];

					int phy_window_diff_l = ((int)phy_window_0_l - (int)phy_window_1_l); 
					int ddr_window_diff_l = ((int)ddr_window_0_l - (int)ddr_window_1_l); 

					//LogMessage ("  lane:%lu; val=0x%08lX (DDR:%ld, BMC:%ld) ",lane,val,ddr_window_diff_l,phy_window_diff_l);

					// Note: PHY window is "averaging" average_num times
					//       "averaging" is sum only, no need to divided.
					phy_window_diff[lane] += phy_window_diff_l; 

					// Note: DDR window is "averaging" average_num*NUM_OF_LANES times
					//       "averaging" is sum only, no need to divided.
					ddr_window_diff += ddr_window_diff_l; 
				}
			}
			//LogMessage ("\n");
		}
		//-------------------------------------------------

		//-------------------------------------------------
		// check average results 
		//-------------------------------------------------
		for (lane=0; lane<NUM_OF_LANES; lane++) 
		{		
			//LogMessage ("* lane:%lu; sum phy_window_diff:%d; use phy_vref:0x%lx; ",lane,phy_window_diff[lane], phy_vref[lane]);

			if ( (phy_window_diff[lane]<(-(VREF_NUM_OF_AVRG))) &&  (phy_vref[lane]<VREF_MAX) )
			{
				phy_vref[lane] += VREF_STEP;
				//LogPass ("Step Up.\n");
			}
			else if ( (phy_window_diff[lane]>(VREF_NUM_OF_AVRG)) &&  (phy_vref[lane]>VREF_MIN) )
			{
				phy_vref[lane] -= VREF_STEP;
				//LogWarning ("Step Down.\n");
			}
			else
			{
				//LogMessage ("No change.\n");
			}
		}
		//--------------------------------
		//LogMessage ("* sum ddr_window_diff:%d; use ddr_vref:0x%lx; ",ddr_window_diff, ddr_vref);

		if ( (ddr_window_diff<(-(VREF_NUM_OF_AVRG*2))) && (ddr_vref<VREF_MAX) )
		{
			ddr_vref += VREF_STEP;
			//LogPass ("Step Up.\n");
		}
		else if ( (ddr_window_diff>(+(VREF_NUM_OF_AVRG*2))) &&  (ddr_vref>VREF_MIN) )
		{
			ddr_vref -= VREF_STEP;
			//LogWarning ("Step Down.\n");
		}
		else
		{
			//LogMessage ("No change.\n");
		}
		//---------------------------------------------

	}
	//----------------------------------------------------------------------
	// clear vref_training and update_ddr_vref bits.
	WRITE_REG (PHY_LANE_SEL, 0*7);
	val = (ddr_vref<<12) | (ddr_vref_range<<11) | (phy_vref[0]<<4); 
	WRITE_REG (VREF_TRAINING, val | 0x04); // vref_output_enable 

	if (WasError==TRUE)
	{
		// fail, there was at least one error on the last iteration
		LogMessage ("LOG_ERROR_VREF_TRAINING_ERROR\n");
		return (-1);
	}

	return (0);
} 
//---------------------------------------------------------------------------------------------
static int l_phy_levelling (void)
{

	if (l_phy_write_levelling()!=0)
		return (-1);

	// Step 1. Enable gating for bit-levelling
	WRITE_REG (DISABLE_GATING_FOR_SCL, 0x0);

	// Step 2. Enable DDR4 logic for scl/bit-lvl
	//WRITE_REG (DDR4_CONFIG_1, ((UINT32)1<<7) | ((UINT32)1<<0)); // scl_bit_lvl_in_ddr4  | ddr4

	if (l_vref_training()!=0) // including phy_bit_leveling()
		return (-1);

	if (l_phy_bit_leveling()!=0) // Step 3 to 5 
		return (-1);

	// Step 6 - Disable gating for bit-levelling to allow proper SCL gating
	WRITE_REG (DISABLE_GATING_FOR_SCL, 0x1);

	if (l_phy_read_leveling()!=0) // Step 7 to 8 
		return (-1);

	// Lior - keep DDR4_CONFIG_1 fix to 0x81 and not set to 0x01
	// Step 9
	//WRITE_REG (DDR4_CONFIG_1, ((UINT32)1<<7) | ((UINT32)1<<0)); // scl_bit_lvl_in_ddr4  | ddr4

	// Step 10 - Turn off x-prop fix in simulation
	//WRITE_REG (DISABLE_GATING_FOR_SCL, 0x3);  // register h6E  - only in simulation
	
	WRITE_REG (PHY_LANE_SEL,0); // 10/03/2016: add to reset lane select

	return (0);
}
//------------------------------------------------------------------------------
static int l_MC_Init (void)  
{
	int Registerindex;
	UINT32 TimeOut;

	//LogMessage (">LOG_MC_INIT_START\n");

	//LogMessage ("> Initializing memory controller (MC) ... ");
	for (Registerindex=0; Registerindex<(sizeof(Denali_Ctl_Initialization)/sizeof(Denali_Ctl_Initialization[0])); Registerindex++)
	{
		IOW32(MC_BASE_ADDR + (Registerindex*4), Denali_Ctl_Initialization[Registerindex]);
	}

	// override settings
	REG_SET_FIELD(MC_BASE_ADDR + ((UINT32)(MR6_DATA_F0_0_ADDR) * 4), 6, 0, (UINT32)VREF_STARTPOINT); // DENALI_CTL_92_DATA // Set VrefDQ good start point VREF level with ODT 120R and DS 48R
	REG_SET_FIELD(MC_BASE_ADDR + ((UINT32)(MR6_DATA_F0_0_ADDR) * 4), 1, 6, 0);  // DENALI_CTL_92_DATA // VrefDQ range 1
	REG_SET_FIELD(MC_BASE_ADDR + ((UINT32)(MR6_DATA_F0_0_ADDR) * 4), 1, 7, 1);  // DENALI_CTL_92_DATA // VrefDQ Training Enable

	
	// Start Initialization Seq
	REG_SET_FIELD(MC_BASE_ADDR + ((UINT32)(START_ADDR) * 4), START_WIDTH, START_OFFSET, 1);
	
	// wait for MC initialization complete.
	TimeOut = 500000;
	while (((IOR32(MC_BASE_ADDR + ((UINT32)(INT_STATUS_ADDR) * 4))) & ((UINT32)0x01<<8)) == 0)
	{
		if (TimeOut==0)
		{
			//LogError ("\n  >>  Memory Controller initialization stuck.\n");
			return (-1);
		}
		TimeOut--;
	}
	// disable VREF write to DDR.
	REG_SET_FIELD(MC_BASE_ADDR + ((UINT32)(MR6_DATA_F0_0_ADDR) * 4), 1, 7, 0);  // DENALI_CTL_92_DATA // VrefDQ Training disable

	//LogPass (" Done. \n");
	
	return (0);
}
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
static int l_DDR4_Init (void)
{
	LogMessage ("> DDR4 Init ... \n");

	// before initialization we clear INTCR2.MC_INIT to mark that initialization was not finished
	CLEAR_BIT_REG(INTCR2, 19);

	// performs MC software reset
	SET_BIT_REG (IPSRST1,12);
	Sleep (1); // ~ 10usec
	CLEAR_BIT_REG (IPSRST1,12);
	Sleep (1); // ~ 10usec

	if (l_phy_init()!=0)
		return (-1);
	
	if (l_MC_Init()!=0)
		return (-1);
	
	if (l_phy_levelling()!=0) // bit, write/read and vref levelling
		return (-1);


    // after initialization we set INTCR2.MC_INIT to mark that initialization was finished
	SET_BIT_REG(INTCR2, 19);

	return (0);
}
//----------------------------------------------------------------------------------------------


#define MEM_TEST_STRAT_ADDR 0x1000
#define MEM_TEST_SIZE_IN_DWORD 0x100 // ~ 1KB memory test x 2

// Simple memory test 
// ~ 80usec (for MEM_TEST_SIZE_IN_DWORD==0x100)
static int l_MemTest (void)
{
	UINT32 Addr;
	UINT32 pattern,read;
	UINT32 index;

	LogMessage ("> Memory test ... \n");
	//--------------------------------------
	// Fill memory with a sequential pattern.
	Addr = MEM_TEST_STRAT_ADDR;
	pattern = 0;
	for (index=0; index<MEM_TEST_SIZE_IN_DWORD; index++)
	{
		*((UINT32*)Addr) = pattern; 

		Addr +=sizeof(UINT32);
		pattern++;
	}
	//--------------------------------------
	// Verify sequential pattern and write anti-pattern
	Addr = MEM_TEST_STRAT_ADDR;
	pattern = 0;
	for (index=0; index<MEM_TEST_SIZE_IN_DWORD; index++)
	{
		read = *((UINT32*)Addr);
		*((UINT32*)Addr) = ~pattern; 

		if (read != pattern) 
		{
			LogMessage ("LOG_ERROR_MEMTEST_STEP1\n");
			return (-1);
		}

		Addr +=sizeof(UINT32);
		pattern ++;
	}
	//--------------------------------------

	//LogMessage (">LOG_MEMTEST_STEP2_START\n");
	//--------------------------------------
	// Verify anti-pattern.
	Addr = MEM_TEST_STRAT_ADDR;
	pattern = 0;
	for (index=0; index<MEM_TEST_SIZE_IN_DWORD; index++)
	{
		read = *((UINT32*)Addr);
		if (read != ~pattern) 
		{
			LogMessage ("LOG_ERROR_MEMTEST_STEP2\n");
			return (-1);
		}
		Addr +=sizeof(UINT32);
		pattern ++;
	}
	//--------------------------------------

	return (0);
}