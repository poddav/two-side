/* -*- C++ -*-
 * File        : progress.cc
 * Created     : Fri Mar 18 2005
 * Description : progress bar window implementation.
 *
 * $Id$
 */

#include <stdexcept>

#include "progress.h"
#include "2side.h"

ProgressBar::ProgressBar (const char* title_text)
{
	OSStatus status = CreateWindowFromNib (main_app->nib_ref, CFSTR("Progress"), &window);
	if (status != noErr)
		throw std::runtime_error ("Can't create progress window");

	RepositionWindow (window, NULL, kWindowAlertPositionOnMainScreen);
	ShowWindow (window);

	ControlID progress_id = { 'PROG', 1 };
	GetControlByID (window, &progress_id, &progress_bar);

	set_text (title_text);
	reset();
}

void ProgressBar::
set_text (const char* text)
{
	CFStringRef str = CFStringCreateWithCString (NULL, text, kCFStringEncodingMacRoman);
	title = CFCopyLocalizedString (str, NULL);
	CFRelease (str);
	ControlID text_id = { 'PROG', 0 };
	ControlRef text_ctl;
	GetControlByID (window, &text_id, &text_ctl);
	SetControlData (text_ctl, kControlLabelPart, kControlStaticTextTextTag,
					strlen (text), text);
}
