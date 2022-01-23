/* -*- C++ -*-
 * File:        progress.h
 * Created:     Fri Mar 18 2005
 * Description: progress bar window class.
 *
 * $Id$
 */

#ifndef PROGRESS_H
#define PROGRESS_H

class ProgressBar
{
	WindowRef		window;
	ControlRef		progress_bar;
	CFStringRef		title;

public:
	ProgressBar (const char* title);
	ProgressBar::~ProgressBar () { close(); }

	void set_text (const char* text);
	void reset (int value = 0) { SetControl32BitValue (progress_bar, value); }
	void close () { DisposeWindow (window); }
};

#endif /* PROGRESS_H */
