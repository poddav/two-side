/* -*- C++ -*-
 * File        : tiffdata.cc
 * Created     : Thu Mar 10 2005
 * Description : TIFF file handling.
 *
 * $Id$
 */

#include <stdexcept>
#include <iostream>
#include <cstdio>

#include "tiffdata.h"

void TiffData::
init (const char* file_name)
{
	std::clog << "reading TIFF: " << file_name << std::endl;

	tiff = TIFFOpen (file_name, "r");
	if (!tiff)
		throw std::runtime_error ("TIFF open failed");

	TIFFGetField (tiff, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField (tiff, TIFFTAG_IMAGELENGTH, &height);
	TIFFGetField (tiff, TIFFTAG_RESOLUTIONUNIT, &res_unit);
	TIFFGetField (tiff, TIFFTAG_BITSPERSAMPLE, &bps);
	TIFFGetField (tiff, TIFFTAG_SAMPLESPERPIXEL, &smp_pixel);
	TIFFGetField (tiff, TIFFTAG_XRESOLUTION, &xres);
	TIFFGetField (tiff, TIFFTAG_YRESOLUTION, &yres);
/*
	std::clog << "width: " << width << std::endl;
	std::clog << "height: " << height << std::endl;
	std::clog << "res_unit: " << res_unit << std::endl;
	std::clog << "smp_pixel: " << smp_pixel << std::endl;
	std::clog << "xres: " << xres << std::endl;
	std::clog << "yres: " << yres << std::endl;
	std::clog << "bps: " << bps << std::endl;
	std::clog << "smp_pixel: " << smp_pixel << std::endl;
*/
}

TiffData::~TiffData ()
{
	if (tiff) TIFFClose (tiff);
	tiff = NULL;
}

void TiffData::
check_sanity ()
{
	short photometric;
	TIFFGetField (tiff, TIFFTAG_PHOTOMETRIC, &photometric);
	if (photometric != PHOTOMETRIC_SEPARATED)
							throw std::runtime_error ("Not a CMYK file");
	if (smp_pixel != 4)		throw std::runtime_error ("Not a CMYK file");
	if (xres != yres)		throw std::runtime_error ("Horizontal resolution doesn't match vertical");
	if (res_unit != RESUNIT_INCH && res_unit != RESUNIT_CENTIMETER)
							throw std::runtime_error ("Unknown resolution units");
}
