/* -*- C++ -*-
 * File        : navdialog.cc
 * Created     : Thu Mar 10 2005
 * Description : File open dialog.
 *
 * $Id$
 */

#include <stdexcept>
#include <iostream>

#include "2side.h"

struct dialog_data_t {
    NavDialogRef	dialog_ref;
	NavDialog		*parent_app;
    Boolean			is_open;
    const void 		*document_data;
};

void NavDialog::
initialize ()
{
	nav_event_upp = NewNavEventUPP (nav_event_handler);

	OSErr err = AEInstallEventHandler (kCoreEventClass, kAEOpenDocuments,
	  								   NewAEEventHandlerUPP (open_file_handler), 0, false);
	if (err)
		throw std::runtime_error ("NavDialog initialization failed");
}

static OSStatus
handle_dropped_spec (FSSpec *dropped_spec)
{
    CInfoPBRec		cat_info_rec;
    cat_info_rec.dirInfo.ioNamePtr = dropped_spec->name;
    cat_info_rec.dirInfo.ioFDirIndex = 0;
    cat_info_rec.dirInfo.ioVRefNum = dropped_spec->vRefNum;
    cat_info_rec.dirInfo.ioDrDirID = dropped_spec->parID;
    OSStatus err = PBGetCatInfoSync(&cat_info_rec);

    if (err) return err;

	Boolean is_folder = (cat_info_rec.hFileInfo.ioFlAttrib & (1 << 4)) != 0;

	if (is_folder) {
		err = noErr;	// we don't do anything for folder
	} else {
		err = main_app->open_file (dropped_spec);
	}

    return err;
}

static OSErr
got_required_params (const AppleEvent *apple_event)
{
    DescType returned_type;
    Size actual_size;
    
    OSErr err = AEGetAttributePtr (apple_event, keyMissedKeywordAttr, typeWildCard,
	  							   &returned_type, nil, 0, &actual_size);
    if (err == errAEDescNotFound)	// you got all the required parameters
		return noErr;
    else if (!err)			// you missed a required parameter
		return errAEEventNotHandled;
    else					// the call to AEGetAttributePtr failed
		return err;
}

OSErr NavDialog::
open_file_handler (const AppleEvent *apple_event, AppleEvent*, long)
try
{
	AEDescList doc_list;

	// get the direct parameter--a descriptor list--and put it into a doc_list
	OSErr err = AEGetParamDesc (apple_event, keyDirectObject, typeAEList, &doc_list);
	if (err) throw std::runtime_error ("AEGetParamDesc failed");

	struct OnReturn {
		AEDescList *list;
		OnReturn (AEDescList *l) : list (l) {}
		~OnReturn () { AEDisposeDesc (list); }
	} doc (&doc_list);
	
	// check for missing parameters
	err = got_required_params (apple_event);
	if (err) throw std::runtime_error ("Can't get required parameters");

	// count the number of descriptor records in the list
	long itemsInList;
	err = AECountItems (&doc_list, &itemsInList);
	if (err) throw std::runtime_error ("AECountItems failed");
	
	// now get each descriptor record from the list, coerce the returned
	// data to an FSSpec record, and open the associated file
	for (long index = 1; index <= itemsInList && !err; index++)
	{
		AEKeyword	keywd;
		DescType	returned_type;
		FSSpec		file_spec;
		Size		actualSize;

		err = AEGetNthPtr (&doc_list, index, typeFSS, &keywd, &returned_type,
						   (Ptr)&file_spec, sizeof(FSSpec), &actualSize);
		if (!err) err = handle_dropped_spec (&file_spec);
	}
	
	return 128;
}
catch (std::runtime_error& err)
{
	std::cerr << err.what() << std::endl;
	return 128;
}

void NavDialog::
open_file ()
{
	// while our open dialog is up we'll disable our Open command, else
	// we might end up with more than one open dialog. Yuk
	DisableMenuCommand (NULL, kHICommandOpen);

	OSStatus err = noErr;

	dialog_data_t *dialog_data = new dialog_data_t;
	bzero (dialog_data, sizeof (dialog_data_t));
	dialog_data->parent_app = this;
	dialog_data->is_open = true;

	NavDialogCreationOptions	dialog_options;
	err = NavGetDefaultDialogCreationOptions (&dialog_options);
	if (err != noErr)
		return;

	const int num_types = 1;

	NavTypeListHandle open_list = (NavTypeListHandle)
		NewHandle (sizeof(NavTypeList) + num_types * sizeof(OSType));
	(*open_list)->componentSignature = kNavGenericSignature;
	(*open_list)->reserved = 0;
	(*open_list)->osTypeCount = num_types;
	(*open_list)->osType[0] = FILETYPE_TIFF;

	dialog_options.preferenceKey = OPEN_PREF_KEY;
	dialog_options.modality = kWindowModalityAppModal;

	if ((err = NavCreateChooseFileDialog (&dialog_options,
						open_list,
						nav_event_upp,
						NULL,		// no custom previews
						NULL,		// filter proc is NULL
						dialog_data,
						&dialog_data->dialog_ref )) == noErr)
	{
		if ((err = NavDialogRun (dialog_data->dialog_ref)) != noErr)
		{
			if (dialog_data->dialog_ref != NULL)
			{
				NavDialogDispose (dialog_data->dialog_ref);
				delete dialog_data;
			}
		}
	}

	if (open_list != NULL)
		DisposeHandle ((Handle)open_list);

	if (err == userCanceledErr)
		err = noErr;

	return;
}

// *****************************************************************************
// *
// *	GetFSSpecInfo( )
// *
// *	Given a generic AEDesc, this routine returns the FSSpec of that object.
// *	Otherwise it returns an error.
// *	
// *****************************************************************************
static OSStatus GetFSSpecInfo (AEDesc* fileObject, FSSpec* returnSpec)
{
    OSStatus 	theErr = noErr;
    AEDesc		theDesc;
    
    if ((theErr = AECoerceDesc (fileObject, typeFSS, &theDesc)) == noErr)
    {
		theErr = AEGetDescData (&theDesc, returnSpec, sizeof (FSSpec));
		AEDisposeDesc( &theDesc );
    }
    else
    {
		if ((theErr = AECoerceDesc (fileObject, typeFSRef, &theDesc)) == noErr)
		{
			FSRef ref;
			if ((theErr = AEGetDescData (&theDesc, &ref, sizeof (FSRef))) == noErr)
			theErr = FSGetCatalogInfo (&ref, kFSCatInfoGettableInfo, NULL, NULL, returnSpec, NULL);
			AEDisposeDesc (&theDesc);
		}
    }

    return theErr;
}

void NavDialog::
nav_event_handler (NavEventCallbackMessage callback_selector, NavCBRecPtr callback_parms,
				   void *user_data)
{
	dialog_data_t *dialog_data = (dialog_data_t*) user_data;

	switch (callback_selector)
	{
	case kNavCBUserAction:
	{
		NavReplyRecord 	reply;
		OSStatus err = NavDialogGetReply (callback_parms->context, &reply);
		if (err != noErr)
			break;

		NavUserAction user_action = NavDialogGetUserAction (callback_parms->context);

		if ((user_action == kNavUserActionOpen || user_action == kNavUserActionChoose)
			&& dialog_data != NULL)
		{
			long count = 0;
			err = AECountItems (&reply.selection, &count);
			for (short index = 1; index <= count; index++)
			{
				AEKeyword 	keyword;
				AEDesc		the_desc;
				if (noErr != AEGetNthDesc (&reply.selection, index, typeWildCard,
											&keyword, &the_desc ))
					continue;

				FSSpec 	file_spec;
				if ((err = GetFSSpecInfo (&the_desc, &file_spec)) == noErr)
				{
					FInfo	file_info;
					// decide if the doc we are opening is a 'TIFF':
					if ((err = FSpGetFInfo (&file_spec, &file_info)) == noErr)
					{
						if (file_info.fdType == FILETYPE_TIFF)
							main_app->open_file (&file_spec);
						else
						{
						// error:
						// if we got this far, the document is a type we can't open and
						// (most likely) built-in translation was turned off.
						// You can alert the user that this returned selection or file spec
						// needs translation.
							throw std::runtime_error("Invalid file type");
						}
					}
				}
				AEDisposeDesc (&the_desc);
			}
		}
			  
		NavDisposeReply (&reply);
		break;
	}
	case kNavCBTerminate:
		if (dialog_data)
		{
			if (dialog_data->dialog_ref)
				NavDialogDispose (dialog_data->dialog_ref);
			
			dialog_data->dialog_ref = NULL;
			// re-enable open if needed
			if (dialog_data->is_open)
				EnableMenuCommand (NULL, kHICommandOpen);

			delete dialog_data;
		}
		break;
	}
}
