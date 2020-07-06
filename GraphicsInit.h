#ifndef GraphicsInit_h__
#define GraphicsInit_h__


/*------------------------------------------------------------------------*/
/*----------------------         Data Types		  ------------------------*/
/*------------------------------------------------------------------------*/

typedef enum GFX_ColorDepth
{
	COLOR_DEPTH_8BIT,
	COLOR_DEPTH_15BIT,
	COLOR_DEPTH_16BIT,
	COLOR_DEPTH_24BIT,
	COLOR_DEPTH_32BIT
} GFX_ColorDepth;


extern BOOL GFX_IsPllDebug_g;

BOOL GFX_ConfigureDisplayTo1024x768(GFX_ColorDepth colorDepth);
BOOL GFX_ConfigureDisplayTo1920x1200(GFX_ColorDepth colorDepth);
void GFX_ClearMemorySpace(BYTE presetValue);
#endif // GraphicsInit_h__
