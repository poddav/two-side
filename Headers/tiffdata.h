/* -*- C++ -*-
 * File:        tiffdata.h
 * Created:     Thu Mar 10 2005
 * Description: TIFF declarations
 *
 * $Id$
 */

#ifndef TIFFDATA_H
#define TIFFDATA_H

#include <tiffio.h>

class TiffData {
	TIFF			*tiff;

public:
	int				width, height;
	int				depth;
	short			res_unit;
	short			bps;
	short			smp_pixel;
	float			xres, yres;

	TiffData () : tiff(0) {}
	TiffData (const char* file_name) { init (file_name); }
	~TiffData ();

	void		init (const char* file_name);
	void		check_sanity ();
	size_t		row_size () const { return TIFFScanlineSize (tiff); }
	void		get_row (size_t row, char* buf)
	{
		if (-1 == TIFFReadScanline (tiff, buf, row))
			throw std::runtime_error ("TIFFReadScanline");
	}
};

#endif /* TIFFDATA_H */
