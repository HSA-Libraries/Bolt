#pragma once
#ifndef bitmap_header_h__
#define bitmap_header_h__

#pragma pack(1)

typedef struct _RGBQUAD { // rgbq 
    unsigned char rgbBlue; 
    unsigned char rgbGreen; 
    unsigned char rgbRed; 
    unsigned char rgbReserved; 
} RGBQuad; 

typedef struct _BITMAPFILEHEADER { // bmfh 
	unsigned short	bfType;
    unsigned long   bfSize; 
    unsigned short  bfReserved1; 
    unsigned short  bfReserved2; 
    unsigned long   bfOffBits; 
} BitmapFileHeader; 

typedef struct _BITMAPINFOHEADER{ // bmih 
    unsigned long  biSize; 
    long		   biWidth; 
    long		   biHeight; 
    unsigned short biPlanes; 
    unsigned short biBitCount; 
    unsigned long  biCompression; 
    unsigned long  biSizeImage; 
    long		   biXPelsPerMeter; 
    long		   biYPelsPerMeter; 
    unsigned long  biClrUsed; 
    unsigned long  biClrImportant; 
} BitmapInfoHeader; 

#pragma pack()


#endif