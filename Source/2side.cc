//
//  main.cc
//  2side
//
//  Created by poddav on 02.03.2005.
//  Copyright (c) 2005 poddav. All rights reserved.
//

#include <stdexcept>
#include <iostream>
#include <cmath>

#include "2side.h"
#include "mac_prefs.h"
#include "progress.h"

#define kInsetBits 		40

SideApp *main_app;

const ControlID SideApp::controls[] = {
	{ 'INP1', 0 }, // front side
	{ 'INP1', 1 }, // front side, button
	{ 'INP1', 2 }, // ... "identical" checkbox
	{ 'INP2', 0 }, // back side
	{ 'INP2', 1 }, // back side, button
	{ 'INP2', 2 }, // back side, label
	{ 'SETN', 1 }, // fold
	{ 'SETN', 2 }, // pocket
	{ 'SETN', 3 }, // hole marks
	{ 'SETN', 4 }, // hole marks checkbox
	{ 'ATTR', 1 }, // width
	{ 'ATTR', 2 }, // height
	{ 'STRT', 0 }, // start button
//	{ 'STRT', 1 }, // progress bar
	{ 'PROG', 0 }, // progress text
	{ 'PROG', 1 }, // progress bar
	{ 'PROG', 2 }, // cancel button
};

SideApp::SideApp()
{
    main_app = this;

    // Create a Nib reference passing the name of the nib file (without the .nib extension)
    // CreateNibReference only searches into the application bundle.
    OSStatus err = CreateNibReference (CFSTR("main"), &nib_ref);
	if (0 != err)
		throw std::runtime_error ("Can't get Nib reference");

    // Once the nib reference is created, set the menu bar. "MainMenu" is the name of the menu bar
    // object. This name is set in InterfaceBuilder when the nib is created.
    err = SetMenuBarFromNib (nib_ref, CFSTR("MenuBar"));
	if (0 != err)
		throw std::runtime_error ("Can't set MenuBar");

	// we can't save as yet
	DisableMenuCommand (NULL, kHICommandSave);

	initialize();
}

SideApp::~SideApp ()
{
	DisposeNibReference (nib_ref);
}

void SideApp::
initialize ()
{
	InitCursor();

	BitMap bitMap;
	GetQDGlobalsScreenBits (&bitMap);
	SetRect (&main_screen_rect, 0, 0,
			 bitMap.bounds.right - kInsetBits,
			 bitMap.bounds.bottom - kInsetBits);

	OSErr err = AEInstallEventHandler (kCoreEventClass, kAEQuitApplication,
	  								   NewAEEventHandlerUPP (quit_apple_event_handler), 0, false);
	if (err)
		throw std::runtime_error ("Application initialization failed");

	nav_dialog.initialize();
	install_app_events();

	side_prefs.LoadPrefs();

	processing_state = false;
}

void SideApp::
install_app_events ()
{
	EventHandlerUPP handler_upp = NewEventHandlerUPP (event_handler);
	const EventTypeSpec event_type = { kEventClassCommand, kEventCommandProcess };
	InstallApplicationEventHandler (handler_upp, 1, &event_type, this, NULL);
}

OSErr SideApp::
quit_apple_event_handler (const AppleEvent*, AppleEvent*, long)
{
	QuitApplicationEventLoop();
	return 128;
}

void SideApp::
check_start_button ()
{
	ControlRef text_ctl;
	Size ctl_size;

	GetControlByID (main_dialog, &controls[INPUT_FRONT_TEXT], &text_ctl);
	GetControlDataSize (text_ctl, kControlEditTextPart, kControlEditTextTextTag, &ctl_size);
	if (!ctl_size)
	{
		disable_start_button();
		return;
	}

	ControlRef size_ctl;

	GetControlByID (main_dialog, &controls[SETTING_FOLD], &size_ctl);
	GetControlDataSize (size_ctl, kControlEditTextPart, kControlEditTextTextTag, &ctl_size);
	if (!ctl_size)
	{
		disable_start_button();
		return;
	}

	GetControlByID (main_dialog, &controls[SETTING_POCKET], &size_ctl);
	GetControlDataSize (size_ctl, kControlEditTextPart, kControlEditTextTextTag, &ctl_size);
	if (!ctl_size)
	{
		disable_start_button();
		return;
	}

	enable_start_button();
}

void SideApp::
enable_start_button ()
{
	if (start_button_enabled)
		return;

	start_button_enabled = true;
	ControlRef btn_ctl;
	GetControlByID (main_dialog, &controls[START_BUTTON], &btn_ctl);
	EnableControl (btn_ctl);
}

void SideApp::
disable_start_button ()
{
	if (!start_button_enabled)
		return;

	start_button_enabled = false;
	ControlRef btn_ctl;
	GetControlByID (main_dialog, &controls[START_BUTTON], &btn_ctl);
	DisableControl (btn_ctl);
}

OSStatus SideApp::
open_file (const FSSpec *file_spec)
try
{
	FSRef file_ref;
	OSErr err = FSpMakeFSRef (file_spec, &file_ref);
	if (err != noErr) return err;

	char file_path[PATH_MAX];
	FSRefMakePath (&file_ref, (UInt8*) file_path, sizeof(file_path) - 1);

	err = noErr;

	TiffData try_tiff (file_path);
	try_tiff.check_sanity();

	name1 = file_path;

	HFSUniStr255 file_name;
	FSGetCatalogInfo (&file_ref, kFSCatInfoNone, NULL, &file_name, NULL, NULL);

	ControlRef text_ctl;
	GetControlByID (main_dialog, &controls[INPUT_FRONT_TEXT], &text_ctl);
	SetControlData (text_ctl, kControlEditTextPart, kControlEditTextTextTag,
					file_name.length*2, file_name.unicode);

	double width_cm = try_tiff.width / try_tiff.xres;
	double height_cm = try_tiff.height / try_tiff.yres;

	if (try_tiff.res_unit == RESUNIT_INCH)
	{
		width_cm *= 2.54;
		height_cm *= 2.54;
	}

	char num[16];
	ControlRef size_ctl;

	GetControlByID (main_dialog, &controls[ATTR_WIDTH], &size_ctl);
	snprintf (num, sizeof(num), "%.2f", width_cm);
	SetControlData (size_ctl, kControlLabelPart, kControlStaticTextTextTag, strlen(num), num);

	GetControlByID (main_dialog, &controls[ATTR_HEIGHT], &size_ctl);
	snprintf (num, sizeof(num), "%.2f", height_cm);
	SetControlData (size_ctl, kControlLabelPart, kControlStaticTextTextTag, strlen(num), num);

	check_start_button();

	return err;
}
catch (std::runtime_error& err)
{
	std::cerr << err.what() << std::endl;
	error_message (err.what());
	return 128;
}

void SideApp::
cleanup_menu ()
{
	DisableMenuCommand (NULL, kHICommandNew);
	DisableMenuCommand (NULL, kHICommandClose);
	DisableMenuCommand (NULL, kHICommandSave);
	DisableMenuCommand (NULL, kHICommandSaveAs);
	DisableMenuCommand (NULL, kHICommandPrint);
	DisableMenuCommand (NULL, kHICommandPageSetup);
}

void SideApp::
restore_menu ()
{
	EnableMenuCommand (NULL, kHICommandNew);
	EnableMenuCommand (NULL, kHICommandClose);
	EnableMenuCommand (NULL, kHICommandSave);
	EnableMenuCommand (NULL, kHICommandSaveAs);
	EnableMenuCommand (NULL, kHICommandPrint);
	EnableMenuCommand (NULL, kHICommandPageSetup);
}

ControlKeyFilterResult SideApp::
key_filter (ControlRef, SInt16 *key_code, SInt16 *char_code, EventModifiers *)
{
	if ((*char_code >= '0' && *char_code <= '9')
		|| *char_code == '.' || *char_code == 8 || *char_code == 127
		|| (*char_code >=28 && *char_code <= 31))
	{
		main_app->check_start_button();
		return kControlKeyFilterPassKey;
	}

	std::clog << "filtered char: " << *char_code << std::endl;
	
	SysBeep(1);
	return kControlKeyFilterBlockKey;
}

void
size_validation ()
{
	std::clog << "validation" << std::endl;
}

void SideApp::
reset_dialog ()
{
	ControlRef size_ctl;
	GetControlByID (main_dialog, &controls[ATTR_WIDTH], &size_ctl);
	SetControlData (size_ctl, kControlLabelPart, kControlStaticTextTextTag, 1, "0");
	GetControlByID (main_dialog, &controls[ATTR_HEIGHT], &size_ctl);
	SetControlData (size_ctl, kControlLabelPart, kControlStaticTextTextTag, 1, "0");

	char num[16];
	snprintf (num, sizeof(num), "%.1f", side_prefs.fold);
	ControlRef fold_ctl;
	GetControlByID (main_dialog, &controls[SETTING_FOLD], &fold_ctl);
	SetControlData (fold_ctl, kControlEditTextPart, kControlEditTextTextTag,
					strlen(num), num);

	snprintf (num, sizeof(num), "%.1f", side_prefs.pocket);
	ControlRef pocket_ctl;
	GetControlByID (main_dialog, &controls[SETTING_POCKET], &pocket_ctl);
	SetControlData (pocket_ctl, kControlEditTextPart, kControlEditTextTextTag,
					strlen(num), num);

	snprintf (num, sizeof(num), "%d", side_prefs.hole_marks);
	GetControlByID (main_dialog, &controls[SETTING_MARKS], &size_ctl);
	SetControlData (size_ctl, kControlEditTextPart, kControlEditTextTextTag,
					strlen(num), num);

	GetControlByID (main_dialog, &controls[SETTING_MARKS_CHKBOX], &size_ctl);
	SetControlValue (size_ctl, side_prefs.top_bottom);

	ControlKeyFilterUPP filter_upp = NewControlKeyFilterUPP (key_filter);
	SetControlData (fold_ctl, kControlEditTextPart, kControlEditTextKeyFilterTag,
					sizeof(filter_upp), &filter_upp);
	SetControlData (pocket_ctl, kControlEditTextPart, kControlEditTextKeyFilterTag,
					sizeof(filter_upp), &filter_upp);
	DisposeControlKeyFilterUPP (filter_upp);
}

double SideApp::
get_control_num_value (const ControlID* ctl_id) const
{
	ControlRef	ctl_ref;
	Size		actual_size;
	char		num[16];

	GetControlByID (main_dialog, ctl_id, &ctl_ref);
	GetControlData (ctl_ref, kControlEditTextPart, kControlEditTextTextTag,
					sizeof(num), num, &actual_size);
	if (actual_size > 15)
		num[15] = 0;
	else
		num[actual_size] = 0;
	return (atof (num));
}

bool SideApp::
get_control_bool_value (const ControlID* ctl_id) const
{
	ControlRef ctl_ref;
	GetControlByID (main_dialog, ctl_id, &ctl_ref);
	return (GetControlValue (ctl_ref));
}

void SideApp::
run ()
try
{
	cleanup_menu();

	OSStatus status = CreateWindowFromNib (nib_ref, CFSTR("MainDialog"), &main_dialog);
	if (status != noErr)
		throw std::runtime_error ("Can't create main dialog");

	EventHandlerUPP win_handler_upp = NewEventHandlerUPP (window_event_handler);
	const EventTypeSpec event_type = { kEventClassWindow, kEventWindowClose };
	InstallWindowEventHandler (main_dialog, win_handler_upp, 1, &event_type, this, NULL);

	start_button_enabled = false;

	reset_dialog();

	RepositionWindow (main_dialog, NULL, kWindowAlertPositionOnMainScreen);
	ShowWindow (main_dialog);

	// prompt for a document
//	nav_dialog.open_file();

//	error_message ("This is a test");
//	std::clog << "this is a test" << std::endl;
	CFStringRef str = CFCopyLocalizedString (CFSTR("this is a test"), NULL);
//	std::clog << strlen(CFStringGetCStringPtr (str, kCFStringEncodingUnicode)) << std::endl;
	std::clog << "test length: " << CFStringGetLength (str) << std::endl;
	CFRelease (str);

    RunApplicationEventLoop();

	side_prefs.fold = get_control_num_value (&controls[SETTING_FOLD]);
	side_prefs.pocket = get_control_num_value (&controls[SETTING_POCKET]);
	side_prefs.hole_marks = int (get_control_num_value (&controls[SETTING_MARKS]));
	side_prefs.top_bottom = get_control_bool_value (&controls[SETTING_MARKS_CHKBOX]);

	side_prefs.SavePrefs();
	DisposeWindow (main_dialog);

	std::clog << "event loop ended" << std::endl;
}
catch (std::runtime_error& err)
{
	std::cerr << err.what() << std::endl;
    QuitApplicationEventLoop();
}

OSStatus SideApp::
window_event_handler (EventHandlerCallRef next_handler, EventRef event, void* data)
{
	OSStatus result = CallNextEventHandler (next_handler, event);
	if (result != noErr)
		return result;

	int ev_class = GetEventClass (event);
	int ev_kind = GetEventKind (event);

	if (ev_class == kEventClassWindow && ev_kind == kEventWindowClose)
	{
		QuitApplicationEventLoop();
		return noErr;
	}
	return eventNotHandledErr;
}

void SideApp::
error_message (const char* message)
{
	CFStringRef str = CFStringCreateWithCString (NULL, message, kCFStringEncodingMacRoman);
	CFStringRef msg = CFCopyLocalizedString (str, NULL);
	CFRelease (str);
	char error[256];
	strlcpy (error+1, message, 255);
	error[0] = strlen (error+1);
	short button_pressed;
	StandardAlert (kAlertStopAlert, (unsigned char*)error, NULL, NULL, &button_pressed);
}

OSStatus SideApp::
event_handler (EventHandlerCallRef handler, EventRef event, void* data)
try
{
	SideApp *side_app = reinterpret_cast<SideApp*> (data);

	HICommand cmd;
	GetEventParameter (event, kEventParamDirectObject, typeHICommand,
					   NULL, sizeof (cmd), NULL, &cmd);
	OSStatus result;

	switch (cmd.commandID)
	{
	case kHICommandOpen:
		side_app->nav_dialog.open_file();
		result = noErr;
		break;

	case kHICommandQuit:
	    QuitApplicationEventLoop();
	    result = noErr;
	    break;

	case kHICommandOK:
		side_app->process_tiff();
		result = noErr;
		break;

	default:
		std::clog << "got event " << cmd.commandID << std::endl;
		result = eventNotHandledErr;
		break;
	}

	HiliteMenu (0);
	return result;
}
catch (std::runtime_error& err)
{
	std::cerr << err.what() << std::endl;
	return eventNotHandledErr;
}

void SideApp::
get_control_values()
{
	identical_sides = get_control_bool_value (&controls[INPUT_IDENTICAL]);

	fold = get_control_num_value (&controls[SETTING_FOLD]);
	pocket = get_control_num_value (&controls[SETTING_POCKET]);
	hole_marks = int (get_control_num_value (&controls[SETTING_MARKS]));

	if (pocket) pocket += POCKET_STITCH_SIZE;

	both_side = get_control_bool_value (&controls[SETTING_MARKS_CHKBOX]);
}

std::string SideApp::
append_name (const char* suffix)
{
	std::string new_name (name1);
	size_t dot_pos = name1.find_last_of ('.');
	if (dot_pos == std::string::npos || dot_pos == 0)
	{
		new_name.append (suffix);
		return new_name;
	}
	new_name.insert (dot_pos, suffix);
	return new_name;
}

int
ink_sum (char* pixel)
{
	return (pixel[0] + pixel[1] + pixel[2] + pixel[3]);
}

int SideApp::
cm_to_px (double cm)
{
	return (int) ceil (cm * dpi / 2.54);
}

void SideApp::
fold_copy (char* src, char* dst)
{
	dst += new_width*4*(base_height - 1) + cmyk_fold_size - 4;
	for (int y = base_height; y; --y)
	{
		for (int x = 0; x < fold_pixels; ++x)
		{
			memcpy (dst, src, 4);
			dst -= 4;
			src += 4;
		}
		dst -= new_width * 4 - cmyk_fold_size;
		src += new_width * 4 - cmyk_fold_size;
	}
}

void SideApp::
make_folds (char* image_buffer)
{
	char* src_region;
	char* dst_region;

	std::clog << "making top fold ..." << std::endl;
   
	src_region = image_buffer + new_width * cmyk_pocket_size + base_width*4;
	dst_region = src_region + cmyk_fold_size;
	fold_copy (src_region, dst_region);

	std::clog << "making bottom fold ..." << std::endl;

	src_region = image_buffer + new_width * cmyk_pocket_size + cmyk_fold_size;
	dst_region = src_region - cmyk_fold_size;
	fold_copy (src_region, dst_region);
}

void SideApp::
pocket_copy (char* src, char* dst)
{
	for (int y = 0; y < pocket_pixels; ++y)
	{
		for (int x = 0; x < base_width; ++x)
		{
			memcpy (dst, src, 4 * base_width);
		}
		dst += new_width * 4;
		src += new_width * 4;
	}
}

void SideApp::
make_pockets (char* image_buffer)
{
	char* src_region;
	char* dst_region;

	std::clog << "making right pocket ..." << std::endl;
   
	src_region = image_buffer + new_width * cmyk_pocket_size + cmyk_fold_size;
	dst_region = src_region + new_width * 4 * base_height;
	pocket_copy (src_region, dst_region);

	std::clog << "making left pocket ..." << std::endl;

	src_region = image_buffer + new_width * 4 * base_height + cmyk_fold_size;
	dst_region = image_buffer + cmyk_fold_size;
	pocket_copy (src_region, dst_region);
}

void SideApp::
mark_cross (char* pixel, int size, int weight)
{
}

void SideApp::
make_holemarks (char* image_buffer)
{
	int mark_x_top = new_width - fold_pixels / 2;
	int mark_x_bot = fold_pixels / 2;
	int mark_y_first = pocket_pixels * 2 - int(ceil (POCKET_STITCH_SIZE / 2.0 * dpi / 2.54));
	int mark_y_last  = new_height - mark_y_first;
	int step = (mark_y_last - mark_y_first) / hole_marks;
	int weight = int (cm_to_px (0.1));
	if (!weight) weight = 1;
	int size = int (MARK_SIZE * dpi / 2.54);

	int y = mark_y_first;
	char* top_edge = image_buffer + mark_y_first * new_width * 4 + mark_x_top * 4;
	char* bot_edge = image_buffer + mark_y_first * new_width * 4 + mark_x_bot * 4;

	for (int i = 0; i < hole_marks; ++i)
	{
		mark_cross (top_edge, size, weight);
		if (both_side)
			mark_cross (bot_edge, size, weight);
		y += step;
	}
}

void SideApp::
mark_pixel (char* pixel, SideApp::mark_t white_dot)
{
	if (white_dot == no_white
		|| (ink_sum (pixel) < INK_THRESHOLD && pixel[2] < (10 * 255 / 100)))
		pixel[2] = 70 * 255 / 100;	// yellow 70%
	else
		*(int32_t*) pixel = 0;
}

void SideApp::
stroke_base (char* image_buffer)
{
	int weight = cm_to_px (0.1);
	if (!weight) weight = 1;
	int half_weight = weight/2;

	std::clog << "stroke base image " << weight << "px ..." << std::endl;

	int stroke_width = base_width + 2 * half_weight;
	if (stroke_width > new_width)
		stroke_width = new_width;
	int stroke_height = base_height + 2 * half_weight;
	if (stroke_height > new_height)
		stroke_height = new_height;

	char *top_left, *bot_left, *top_right;

	top_left = image_buffer + new_width * 4 * (pocket_pixels - half_weight);
	top_left += cmyk_fold_size - 4 * half_weight;
	bot_left = top_left + new_width * 4 * base_height;

	for (int x = 0; x < stroke_width; ++x)
	{
		char* pixel_top = top_left;
		char* pixel_bot = bot_left;
		for (int s = 0; s < weight; ++s)
		{
			mark_pixel (pixel_top);
			mark_pixel (pixel_bot);
			pixel_top += new_width * 4;
			pixel_bot += new_width * 4;
		}
		top_left += 4;
		bot_left += 4;
	}
	top_left = image_buffer + new_width * 4 * (pocket_pixels - half_weight + weight) + cmyk_fold_size - 4 * half_weight;
	top_right = top_left + base_width * 4;
	for (int y = 0; y < (stroke_height - 2 * weight); ++y)
	{
		char* pixel_left = top_left;
		char* pixel_right = top_right;
		for (int s = 0; s < weight; ++s)
		{
			mark_pixel (pixel_left);
			mark_pixel (pixel_right);
			pixel_left += 4;
			pixel_right += 4;
		}
		top_left += new_width * 4;
		top_right += new_width * 4;
	}
}

void SideApp::
stroke_edge (char *image_buffer)
{
	int weight = cm_to_px (0.1);
	if (!weight) weight = 1;

	std::clog << "stroke edge " << weight << "px ..." << std::endl;

	char *top, *left, *right, *bottom;

	top = image_buffer;
	left = image_buffer + new_width * 4 * weight;
	right = left + new_width * 4 - weight * 4;
	bottom = top + new_width * 4 * (new_height - weight);

	for (int x = 0; x < new_width; ++x)
	{
		char* pixel_top = top;
		char* pixel_bot = bottom;
		for (int s = 0; s < weight; ++s)
		{
			mark_pixel (pixel_top, no_white);
			mark_pixel (pixel_bot, no_white);
			pixel_top += new_width * 4;
			pixel_bot += new_width * 4;
		}
		top += 4;
		bottom += 4;
	}
	for (int y = 0; y < (new_height - 2 * weight); ++y)
	{
		char* pixel_left = left;
		char* pixel_right = right;
		for (int s = 0; s < weight; ++s)
		{
			mark_pixel (pixel_left, no_white);
			mark_pixel (pixel_right, no_white);
			pixel_left += 4;
			pixel_right += 4;
		}
		left += new_width * 4;
		right += new_width * 4;
	}
}

void SideApp::
run_events_manual ()
{
	EventRef event;
	EventTargetRef target = GetEventDispatcherTarget();

	if (ReceiveNextEvent (0, NULL, 0, true, &event) == noErr)
	{
		uint32_t event_class = GetEventClass (event);
		if (event_class == kEventClassControl)
			std::clog << "got control event" << std::endl;
		else if (event_class == kEventClassCommand)
			std::clog << "got command event" << std::endl;
		else if (event_class == kEventClassWindow)
			std::clog << "got window event" << std::endl;

		SendEventToEventTarget (event, target);
		ReleaseEvent (event);
	}
}

void SideApp::
rotate180 (char* image_buffer)
{
	size_t buf_size = new_width * new_height * 4;
	char* buffer_end = image_buffer + buf_size - 4;

	ProgressBar progress_bar ("Rotate 180 degrees");
//	ProgressBar progress_bar (CFCopyLocalizedString (CFSTR("Rotate 180 degrees"), NULL));

	size_t total_count = buf_size / 4 / 2;

	for (size_t i = 0; i < total_count; ++i)
	{
		std::swap (*(uint32_t*)image_buffer, *(uint32_t*)buffer_end);
		image_buffer += 4;
		buffer_end -= 4;

		if (!(i & 31))
		{
			progress_bar.reset (i * 100 / (total_count - 1));
			run_events_manual();
		}
	}
}

void SideApp::
clear_perimeter (char *image_buffer)
{
	std::clog << "clean up side B ..." << std::endl;

	char *top = image_buffer;
	int clean_pocket_size = pocket_pixels * 2 - cm_to_px (0.3);
	clean_pocket_size *= 4;
	char *bottom = image_buffer + new_width * (4 * new_height - clean_pocket_size);
	memset (top, 0, new_width * clean_pocket_size);
	memset (bottom, 0, new_width * clean_pocket_size);

	int clean_fold_size = fold_pixels * 2 - cm_to_px (0.3);
	clean_fold_size *= 4;

	char *left = image_buffer + new_width * clean_pocket_size;
	char *right = left + new_width * 4 - clean_fold_size;

	for (int i = 0; i < (base_height - 2 * cm_to_px (0.3)); ++i)
	{
		memset (left, 0, clean_fold_size);
		memset (right, 0, clean_fold_size);
		left += new_width * 4;
		right += new_width * 4;
	}
}

void SideApp::
write_side (const char* image_buffer, const char* suffix)
{
	std::string side_a_name = append_name (suffix);
	TIFF* side_A = TIFFOpen (side_a_name.c_str(), "w");
	if (!side_A)
		throw std::runtime_error ("unable to open result file");

	TIFFSetField (side_A, TIFFTAG_IMAGEWIDTH, new_width);
	TIFFSetField (side_A, TIFFTAG_IMAGELENGTH, new_height);
	TIFFSetField (side_A, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField (side_A, TIFFTAG_SAMPLESPERPIXEL, 4);
	TIFFSetField (side_A, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	TIFFSetField (side_A, TIFFTAG_XRESOLUTION, dpi);
	TIFFSetField (side_A, TIFFTAG_YRESOLUTION, dpi);
	TIFFSetField (side_A, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
	TIFFSetField (side_A, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField (side_A, TIFFTAG_INKSET, INKSET_CMYK);
	TIFFSetField (side_A, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_SEPARATED);

	std::string message ("Writing side ");
	size_t suf_len = strlen (suffix);
	if (suf_len > 1)
		message.append (1, suffix[suf_len - 1]);
	else
		message.append (suffix);

	ProgressBar progress_bar (message.c_str());

	const char *row_ptr = image_buffer;
	for (size_t row = 0; row < new_height; ++row)
	{
		progress_bar.reset (row * 100 / (new_height - 2));
		run_events_manual();
		TIFFWriteScanline (side_A, const_cast<char*> (row_ptr), row, 0);
		row_ptr += 4*new_width;
	}

	TIFFClose (side_A);
}

char* SideApp::
read_side (const char* name, bool is_front)
{
	TiffData side_data (name);

	bool rotate90 = side_data.height < side_data.width;

	if (!rotate90) {
		base_width = side_data.width;
		base_height = side_data.height;
	} else {
		base_width = side_data.height;
		base_height = side_data.width;
	}

	get_control_values();

	if (base_width < 2*fold)
		throw std::runtime_error ("Invalid base_width/fold");

	dpi = side_data.xres;

	if (side_data.res_unit == RESUNIT_CENTIMETER)
		dpi *= 2.54;

	fold_pixels = int (fold * dpi / 2.54);
	pocket_pixels = int (pocket * dpi / 2.54);

	cmyk_fold_size = fold_pixels * 4;
	cmyk_pocket_size = pocket_pixels * 4;

	size_t cmyk_row_size = side_data.width * 4;
	size_t row_size = side_data.row_size();

	if (cmyk_row_size != row_size)
	{
		std::cerr << "CMYK row: " << cmyk_row_size << ", actual row: " << row_size << std::endl;
		throw std::runtime_error ("CMYK row size unmatched");
	}

	new_width = base_width + 2 * fold_pixels;
	new_height = base_height + 2 * pocket_pixels;

	char* side_buf = new char[new_width * new_height * 4];
	char* row_ptr = side_buf + new_width * cmyk_pocket_size + cmyk_fold_size;
	if (rotate90)
		row_ptr += base_width*4 - 4;

	ProgressBar progress_bar ("Reading TIFF file");
//	ControlRef progress_bar = display_progress (CFCopyLocalizedString (CFSTR("Reading TIFF file"), NULL));

	char* row_buf = new char[cmyk_row_size];

	for (int row = 0; row < side_data.height; ++row)
	{
		progress_bar.reset (row * 100 / (side_data.height - 2));
		run_events_manual();
		side_data.get_row (row, row_buf);
		if (rotate90)
		{
			char* ptr = row_ptr;
			char* row_buf_ptr = (char*)row_buf;
			for (int count = base_height; count; --count)
			{
				*(int32_t*)ptr = *(int32_t*)row_buf_ptr;
				row_buf_ptr += 4;
				ptr += base_width*4 + 2*cmyk_fold_size;
			}
			row_ptr -= 4;
		}
		else
		{
			std::copy (row_buf, row_buf + cmyk_row_size, row_ptr);
			row_ptr += new_width*4;
		}
	}
	delete row_buf;

	std::clog << "got " << (row_size * side_data.height) << " bytes" << std::endl;

	return side_buf;
}

void SideApp::
process_tiff ()
try
{
	if (processing_state)
	{
		std::clog << "already processing" << std::endl;
		return;
	}
	processing_state = true;
	disable_start_button();

	std::auto_ptr<char> side_buf (read_side (name1.c_str()));

	if (fold) make_folds (side_buf.get());
	if (pocket) make_pockets (side_buf.get());
	if (hole_marks) make_holemarks (side_buf.get());

	stroke_base (side_buf.get());
	stroke_edge (side_buf.get());

	write_side (side_buf.get(), "-A");

	if (identical_sides)
		rotate180 (side_buf.get());
	else
		side_buf.reset (read_side (name2.c_str(), false));

	clear_perimeter (side_buf.get());
	stroke_edge (side_buf.get());

	write_side (side_buf.get(), "-B");

	std::clog << "done." << std::endl;

	processing_state = false;
	enable_start_button();
}
catch (std::runtime_error& err)
{
	error_message (err.what());

	processing_state = false;
	enable_start_button();
}

int main (int argc, char* argv[])
try
{
	SideApp side_app;
	side_app.run();
}
catch (std::runtime_error& err)
{
	std::cerr << err.what() << std::endl;
	return 1;
}
