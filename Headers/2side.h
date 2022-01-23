/* -*- C++ -*-
 * File:        2side.h
 * Created:     Thu Mar 10 2005
 * Description: 2side application declaration.
 *
 * $Id$
 */

#ifndef TWO_SIDE_H
#define TWO_SIDE_H

#include <string>
#include "navdialog.h"
#include "tiffdata.h"

#define POCKET_STITCH_SIZE	3.5		// cm
#define INK_THRESHOLD		260		// 102%
#define MARK_SIZE			1.1		// cm

struct SideApp
{
	SideApp ();
	~SideApp ();

	void				run ();

private:
    IBNibRef			nib_ref;
	Rect				main_screen_rect;
	WindowRef			main_dialog;
	WindowRef			progress_window;
	NavDialog			nav_dialog;
	bool				start_button_enabled;

	bool				processing_state;

	std::string			name1, name2;
	bool				identical_sides;

	double				fold, pocket;
	int					hole_marks;
	bool				both_side;

	double				dpi;
	int					base_width, base_height;
	int					new_width, new_height;
	int					fold_pixels;
	int					pocket_pixels;
	int					cmyk_fold_size;
	int					cmyk_pocket_size;

	static const ControlID	controls[];

	enum control_t {
		INPUT_FRONT_TEXT,
		INPUT_FRONT_BUTTON,
		INPUT_IDENTICAL,
		INPUT_BACK_TEXT,
		INPUT_BACK_BUTTON,
		INPUT_BACK_LABEL,
		SETTING_FOLD,
		SETTING_POCKET,
		SETTING_MARKS,
		SETTING_MARKS_CHKBOX,
		ATTR_WIDTH,
		ATTR_HEIGHT,
		START_BUTTON,
		PROGRESS_TEXT,
		PROGRESS_BAR,
		PROGRESS_CANCEL,
	};

	enum mark_t { mark_any, no_white };

	static OSStatus		event_handler (EventHandlerCallRef, EventRef, void*);
	static OSStatus		window_event_handler (EventHandlerCallRef, EventRef, void*);
	static OSErr		quit_apple_event_handler (const AppleEvent*, AppleEvent*, long);
	static ControlKeyFilterResult key_filter (ControlRef, SInt16*, SInt16*, EventModifiers*);
	static void			run_events_manual ();
	static void			error_message (const char*);

	void				initialize ();
	void				cleanup_menu ();
	void				restore_menu ();
	void				reset_dialog ();
	void				enable_start_button ();
	void				disable_start_button ();
	double				get_control_num_value (const ControlID* ctl_id) const;
	bool				get_control_bool_value (const ControlID* ctl_id) const;
	void				install_app_events ();
	void				check_start_button ();
	void				process_tiff ();

	void				get_control_values();
	std::string			append_name (const char* suffix);
	char*				read_side (const char* name, bool is_front = true);
	void				write_side (const char* image_buffer, const char* suffix);
	void				make_folds (char* image_buffer);
	void				make_pockets (char* image_buffer);
	void				make_holemarks (char* image_buffer);
	void				stroke_base (char* image_buffer);
	void				stroke_edge (char *image_buffer);
	void				rotate180 (char* image_buffer);
	void				clear_perimeter (char *image_buffer);
	void				pocket_copy (char* src, char* dst);
	void				fold_copy (char* src, char* dst);
	int					cm_to_px (double cm);
	void				mark_pixel (char* pixel, mark_t white_dot = mark_any);
	void				mark_cross (char* pixel, int size, int weight);

public:
	OSStatus			open_file (const FSSpec *);

	friend class ProgressBar;
};

enum filetype_t {
	FILETYPE_TIFF = 'TIFF',
};

extern SideApp* main_app;

#endif /* TWO_SIDE_H */
