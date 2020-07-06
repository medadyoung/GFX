#ifndef GFXI_IF_h__
#define GFXI_IF_h__

#ifndef GFXI_BA
#define	GFXI_BA				0xF000E000
#endif

#define	GFXI_DISPHDST		(GFXI_BA + 0x0)			//	Display Head Status (DISPHDST)	
#define	GFXI_FBADL 			(GFXI_BA + 0x4)			//	Frame Buffer Address Low (FBADL)	
#define	GFXI_FBADM 			(GFXI_BA + 0x8)			//	Frame Buffer Address Middle (FBADM)	
#define	GFXI_FBADH 			(GFXI_BA + 0xC)			//	Frame Buffer Address High (FBADH)	
#define	GFXI_HVCNTL			(GFXI_BA + 0x10)			//	Horizontal Visible Counter Low (HVCNTL)	
#define	GFXI_HVCNTH			(GFXI_BA + 0x14)			//	Horizontal Visible Counter High (HVCNTH)	
#define	GFXI_HBPCNTL		(GFXI_BA + 0x18)			//	 Horizontal Back-Porch Counter Low (HBPCNTL)	
#define	GFXI_HBPCNTH		(GFXI_BA + 0x1C)			//	 Horizontal Back-Porch Counter High (HBPCNTH)	
#define	GFXI_VVCNTL			(GFXI_BA + 0x20)			//	Vertical Visible Counter Low (VVCNTL)	
#define	GFXI_VVCNTH			(GFXI_BA + 0x24)			//	Vertical Visible Counter High (VVCNTH)	
#define	GFXI_VBPCNTL		(GFXI_BA + 0x28)			//	 Vertical Back-Porch Counter Low (VBPCNTL)	
#define	GFXI_VBPCNTH		(GFXI_BA + 0x2C)			//	 Vertical Back-Porch Counter High (VBPCNTH)	
#define	GFXI_CURPOSXL		(GFXI_BA + 0x30)			//	 Cursor Position X Low (CURPOSXL)	
#define	GFXI_CURPOSXH		(GFXI_BA + 0x34)			//	 Cursor Position X High (CURPOSXH)	
#define	GFXI_CURPOSYL		(GFXI_BA + 0x38)			//	 Cursor Position Y Low (CURPOSYL)	
#define	GFXI_CURPOSYH		(GFXI_BA + 0x3C)			//	 Cursor Position Y High (CURPOSYH)	
#define	GFXI_GPLLINDI		(GFXI_BA + 0x40)			//	V  Graphics PLL Input Divider (GPLLINDIV)	
#define	GFXI_GPLLFBDI		(GFXI_BA + 0x44)			//	V  Graphics PLL Feedback Divider (GPLLFBDIV)	
#define	GFXI_GPLLST			(GFXI_BA + 0x48)			//	Graphics PLL Status (GPLLST)	
#define	GFXI_KVMHDST		(GFXI_BA + 0x4C)			//	 KVM Head status	
#define	GFXI_COLDEP			(GFXI_BA + 0x50)			//	Color Depth (COLDEP)	
#define	GFXI_VDISPEND		(GFXI_BA + 0x54)			//	General Purpose register 0,Vertical Display End Low (VDISPEND)	
#define	GFXI_VBLANKSTR		(GFXI_BA + 0x58)			//	General Purpose register 1, Vertical Blank Start Low (VBLANKSTR)	
#define	GFXI_VBLANKEND		(GFXI_BA + 0x5C)			//	General Purpose register 2, Vertical Blank End (VBLANKEND)	
#define	GFXI_VTOTAL			(GFXI_BA + 0x60)			//	General Purpose register 3, Vertical Total Low (VTOTAL)	
#define	GFXI_VHIGH			(GFXI_BA + 0x64)			//	General Purpose register 4, Vertical High (VHIGH)	
#define	GFXI_HDISPEND		(GFXI_BA + 0x68)			//	GPR5 RO General Purpose register 5,	Horizontal Display End (HDISPEND) 
#define	GFXI_HBLANKSTR		(GFXI_BA + 0x6C)			//	GPR6 RO General Purpose register 6,	Horizontal Blank Start Low (HBLANKSTR) 
#define	GFXI_HBLANKEND		(GFXI_BA + 0x70)			//	GPR7 RO General Purpose register 7,	Horizontal Blank End (HBLANKEND) 
#define	GFXI_HTOTAL			(GFXI_BA + 0x74)			//	GPR8 RO General Purpose register 8,	Horizontal Total (HTOTAL) 
#define	GFXI_CURWIDTH		(GFXI_BA + 0x78)			//	GPR9 RO General Purpose register 9	Cursor Width (CURWIDTH) 
#define	GFXI_CURHEIGHT		(GFXI_BA + 0x7C)			//	GPR10 RO General Purpose register 1	0 Cursor Height (CURHEIGHT) 
#define	GFXI_CURHSXL		(GFXI_BA + 0x80)			//	GPR11 RO General Purpose register 1	1 Cursor Hot Spot X Position Low (CURHSXL)
#define	GFXI_GPR12			(GFXI_BA + 0x84)			//	GPR12 RO General Purpose register 1	2 PLL Reset Counter (PLLRSTCNT) 
#define	GFXI_GPR13			(GFXI_BA + 0x88)			//	GPR13 RO General Purpose register 1	3 Cursor Hot Spot High (CURHSH) 
#define	GFXI_GPR14			(GFXI_BA + 0x8C)			//	GPR14 RO General Purpose register 1	4 PLL Reset Counter (PLLRSTCNT) 
#define	GFXI_GPR15			(GFXI_BA + 0x90)			//	GPR15 RO General Purpose register 1	5 Cursor Hot Spot High (CURHSH)

typedef struct GFXI_RegInfo
{
	DWORD	address;
	char	Name[30];

}GFXI_RegInfo;

const GFXI_RegInfo GFXI_Regs[] = 
{
	{GFXI_DISPHDST	, "DISPHDST     "},
	{GFXI_FBADL 	, "FBADL        "},
	{GFXI_FBADM 	, "FBADM        "},
	{GFXI_FBADH 	, "FBADH        "},
	{GFXI_HVCNTL	, "HVCNTL       "},
	{GFXI_HVCNTH	, "HVCNTH       "},
	{GFXI_HBPCNTL	, "HBPCNTL      "},
	{GFXI_HBPCNTH	, "HBPCNTH      "},
	{GFXI_VVCNTL	, "VVCNTL       "},
	{GFXI_VVCNTH	, "VVCNTH       "},
	{GFXI_VBPCNTL	, "VBPCNTL      "},
	{GFXI_VBPCNTH	, "VBPCNTH      "},
	{GFXI_CURPOSXL	, "CURPOSXL     "},
	{GFXI_CURPOSXH	, "CURPOSXH     "},
	{GFXI_CURPOSYL	, "CURPOSYL     "},
	{GFXI_CURPOSYH	, "CURPOSYH     "},
	{GFXI_GPLLINDI	, "GPLLINDIV    "},
	{GFXI_GPLLFBDI	, "GPLLFBDIV    "},
	{GFXI_GPLLST	, "GPLLST       "},
	{GFXI_KVMHDST	, "KVMHDST      "},
	{GFXI_COLDEP	, "COLDEP       "},
	{GFXI_VDISPEND	, "VDISPEND     "},
	{GFXI_VBLANKSTR	, "VBLANKSTR    "},
	{GFXI_VBLANKEND	, "VBLANKEND    "},
	{GFXI_VTOTAL	, "VTOTAL       "},
	{GFXI_VHIGH		, "VHIGH        "},
	{GFXI_HDISPEND	, "HDISPEND     "},
	{GFXI_HBLANKSTR	, "HBLANKSTR    "},
	{GFXI_HBLANKEND	, "HBLANKEND    "},
	{GFXI_HTOTAL	, "HTOTAL       "},
	{GFXI_CURWIDTH	, "CURWIDTH     "},
	{GFXI_CURHEIGHT	, "CURHEIGHT    "},
	{GFXI_CURHSXL	, "CURHSXL      "},
	{GFXI_GPR12		, "GPR12        "},
	{GFXI_GPR13		, "GPR13        "},
	{GFXI_GPR14		, "GPR14        "},
	{GFXI_GPR15		, "GPR15        "}		
};


#define NUM_OF_GFXI_REGS (sizeof(GFXI_Regs)/sizeof(GFXI_RegInfo))
#endif // GFXI_IF_h__
