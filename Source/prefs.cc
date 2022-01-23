/* -*- C++ -*-
 * File        : prefs.cc
 * Created     : Sat Mar 12 2005
 * Description : Preferences handling.
 *
 * $Id$
 */

#include "mac_prefs.h"

SidePrefs side_prefs;

void MacPrefs::
PrefGetInt (CFStringRef key, int *value)
{
	CFNumberRef value_ref;
	value_ref = (CFNumberRef) CFPreferencesCopyAppValue (key, kCFPreferencesCurrentApplication);

	*value = 0;
	if (value_ref)
	{
		CFNumberGetValue (value_ref, kCFNumberIntType, value);
		CFRelease (value_ref);
	}
}

void MacPrefs::
PrefSetInt (CFStringRef key, int value)
{
	CFNumberRef value_ref;
	value_ref = CFNumberCreate (NULL, kCFNumberIntType, &value);
	CFPreferencesSetAppValue (key, value_ref, kCFPreferencesCurrentApplication);
}

void MacPrefs::
PrefGetFloat (CFStringRef key, float *value)
{
	CFNumberRef value_ref;
	value_ref = (CFNumberRef) CFPreferencesCopyAppValue (key, kCFPreferencesCurrentApplication);

	*value = 0;
	if (value_ref)
	{
		CFNumberGetValue (value_ref, kCFNumberFloatType, value);
		CFRelease (value_ref);
	}
}

void MacPrefs::
PrefSetFloat (CFStringRef key, float value)
{
	CFNumberRef value_ref;
	value_ref = CFNumberCreate (NULL, kCFNumberFloatType, &value);
	CFPreferencesSetAppValue (key, value_ref, kCFPreferencesCurrentApplication);
}

void MacPrefs::
PrefGetBool (CFStringRef key, bool *value)
{
	CFNumberRef value_ref;
	value_ref = (CFNumberRef) CFPreferencesCopyAppValue (key, kCFPreferencesCurrentApplication);

	*value = false;
	if (value_ref)
	{
		short num;
		CFNumberGetValue (value_ref, kCFNumberShortType, &num);
		CFRelease (value_ref);
		*value = num != 0;
	}
}

void MacPrefs::
PrefSetBool (CFStringRef key, bool value)
{
	CFNumberRef value_ref;
	short num = value;
	value_ref = CFNumberCreate (NULL, kCFNumberShortType, &num);
	CFPreferencesSetAppValue (key, value_ref, kCFPreferencesCurrentApplication);
}

void MacPrefs::
PrefGetStr (CFStringRef key, char *value, size_t size)
{
	Boolean res;
	CFStringRef value_ref;
	const char *str;

	*value = 0;

	value_ref = (CFStringRef) CFPreferencesCopyAppValue (key, kCFPreferencesCurrentApplication);
	if (!value_ref)
		return;

	str = CFStringGetCStringPtr (value_ref, kCFStringEncodingMacRoman);
	if (!str)
	{
		res = CFStringGetCString (value_ref, value, size, kCFStringEncodingMacRoman);
		if (!res)
			*value = 0;
	}
	else
		strlcpy (value, str, size);
}

void MacPrefs::
PrefSetStr (CFStringRef key, const char *value)
{
	CFStringRef value_ref = CFStringCreateWithCString (NULL, value, kCFStringEncodingMacRoman);
	CFPreferencesSetAppValue (key, value_ref, kCFPreferencesCurrentApplication);
}

void SidePrefs::
LoadPrefs ()
{
	memset (&side_prefs, 0, sizeof(side_prefs));

	PrefGetFloat (CFSTR("SideApp::fold"), &fold);
	PrefGetFloat (CFSTR("SideApp::pocket"), &pocket);
	PrefGetInt (CFSTR("SideApp::hole_marks"), &hole_marks);
	PrefGetBool (CFSTR("SideApp::top_bottom"), &top_bottom);
}

void SidePrefs::
SavePrefs ()
{
	PrefSetFloat (CFSTR("SideApp::fold"), fold);
	PrefSetFloat (CFSTR("SideApp::pocket"), pocket);
	PrefSetInt (CFSTR("SideApp::hole_marks"), hole_marks);
	PrefSetBool (CFSTR("SideApp::top_bottom"), top_bottom);

	CFPreferencesAppSynchronize (kCFPreferencesCurrentApplication);
}
