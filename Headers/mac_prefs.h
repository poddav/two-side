/* -*- C++ -*-
 * File:        mac_prefs.h
 * Created:     Sat Mar 12 2005
 * Description: Preferences handling.
 *
 * $Id$
 */

#ifndef MAC_PREFS_H
#define MAC_PREFS_H

struct MacPrefs {
//	virtual ~MacPrefs () {}

	static void PrefGetInt (CFStringRef key, int *value);
	static void PrefSetInt (CFStringRef key, int value);
	static void PrefGetFloat (CFStringRef key, float *value);
	static void PrefSetFloat (CFStringRef key, float value);
	static void PrefGetBool (CFStringRef key, bool *value);
	static void PrefSetBool (CFStringRef key, bool value);
	static void PrefGetStr (CFStringRef key, char *value, size_t size);
	static void PrefSetStr (CFStringRef key, const char *value);

	virtual void LoadPrefs () = 0;
	virtual void SavePrefs () = 0;
};

struct SidePrefs : MacPrefs
{
	float	fold;
	float	pocket;
	int		hole_marks;
	bool	top_bottom;

	void LoadPrefs ();
	void SavePrefs ();
};

extern SidePrefs side_prefs;

#endif
