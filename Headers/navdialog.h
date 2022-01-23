/* -*- C++ -*-
 * File:        navdialog.h
 * Created:     Thu Mar 10 2005
 * Description: File open dialog declarations.
 *
 * $Id$
 */

#ifndef NAVDIALOG_H
#define NAVDIALOG_H

class NavDialog {
	NavEventUPP			nav_event_upp;		// event proc for our Nav Dialogs 

	enum pref_key_t {
		OPEN_PREF_KEY = 1,
		SAVE_PREF_KEY,
	};

	static OSErr		open_file_handler (const AppleEvent *, AppleEvent*, long);
	static void			nav_event_handler (NavEventCallbackMessage, NavCBRecPtr, void*);

public:
	void				initialize ();
	void				open_file ();
};


#endif /* NAVDIALOG_H */
