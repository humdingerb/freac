 /* BonkEnc Audio Encoder
  * Copyright (C) 2001-2003 Robert Kausch <robert.kausch@gmx.net>
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the "GNU General Public License".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#define __THROW_BAD_ALLOC exit(1)
#define MAKEUNICODESTR(x) L##x

#include <smooth/main.h>
#include <main.h>
#include <resources.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <time.h>

#include <dllinterfaces.h>

#include <genconfig.h>
#include <bonkconfig.h>
#include <bladeconfig.h>
#include <lameconfig.h>
#include <vorbisconfig.h>
#include <faacconfig.h>
#include <tvqconfig.h>

#include <cddb.h>
#include <cddb_multimatch.h>
#include <cddb_submit.h>

#include <language.h>

Int smooth::Main()
{
	bonkEncGUI	*app = new bonkEncGUI();

	Loop();

	delete app;

	return 0;
}

bonkEncGUI::bonkEncGUI()
{
	playing = false;
	play_thread = NIL;

	currentConfig->enable_console = false;
	currentConfig->appMain = this;

	dontUpdateInfo = False;
	cddbRetry = True;
	cddbInfo = NIL;
	clicked_drive = -1;

	Point	 pos;
	Size	 size;

	mainWnd_menubar		= new Menubar();
	mainWnd_iconbar		= new Menubar();
	mainWnd			= new Window(String("BonkEnc ").Append(bonkEnc::version));
	mainWnd_titlebar	= new Titlebar();
	mainWnd_statusbar	= new Statusbar(String("BonkEnc ").Append(bonkEnc::version).Append(" - Copyright (C) 2001-2003 Robert Kausch"));
	menu_file		= new Menu();
	menu_options		= new Menu();
	menu_addsubmenu		= new Menu();
	menu_encode		= new Menu();
	menu_drives		= new Menu();
	menu_seldrive		= new Menu();
	menu_database		= new Menu();
	menu_trackmenu		= new Menu();
	menu_help		= new Menu();

	pos.x = 91;
	pos.y = -22;

	hyperlink		= new Hyperlink("www.bonkenc.org", NULL, "http://www.bonkenc.org", pos);
	hyperlink->SetOrientation(OR_UPPERRIGHT);

	if (winamp_out_modules.GetNOfEntries() > 0)
	{
		pos.x = 116;
		pos.y = -1;
		size.cx = 25;
		size.cy = 25;

		button_play	= new Button(NIL, SMOOTH::LoadImage("BonkEnc.pci", 11, NIL), pos, size);
		button_play->onClick.Connect(&bonkEnc::PlaySelectedItem, (bonkEnc *) this);
		button_play->SetOrientation(OR_UPPERRIGHT);
		button_play->SetFlags(BF_NOFRAME);

		pos.x -= 22;

		button_pause	= new Button(NIL, SMOOTH::LoadImage("BonkEnc.pci", 12, NIL), pos, size);
		button_pause->onClick.Connect(&bonkEnc::PausePlayback, (bonkEnc *) this);
		button_pause->SetOrientation(OR_UPPERRIGHT);
		button_pause->SetFlags(BF_NOFRAME);

		pos.x -= 22;

		button_stop	= new Button(NIL, SMOOTH::LoadImage("BonkEnc.pci", 13, NIL), pos, size);
		button_stop->onClick.Connect(&bonkEnc::StopPlayback, (bonkEnc *) this);
		button_stop->SetOrientation(OR_UPPERRIGHT);
		button_stop->SetFlags(BF_NOFRAME);

		pos.x -= 22;

		button_prev	= new Button(NIL, SMOOTH::LoadImage("BonkEnc.pci", 14, NIL), pos, size);
		button_prev->onClick.Connect(&bonkEnc::PlayPrevious, (bonkEnc *) this);
		button_prev->SetOrientation(OR_UPPERRIGHT);
		button_prev->SetFlags(BF_NOFRAME);

		pos.x -= 22;

		button_next	= new Button(NIL, SMOOTH::LoadImage("BonkEnc.pci", 15, NIL), pos, size);
		button_next->onClick.Connect(&bonkEnc::PlayNext, (bonkEnc *) this);
		button_next->SetOrientation(OR_UPPERRIGHT);
		button_next->SetFlags(BF_NOFRAME);
	}

	pos.x = 7;
	pos.y = 96;

	enc_filename		= new Text(i18n->TranslateString("Encoding file:"), pos);
	enc_filename->SetOrientation(OR_LOWERLEFT);

	pos.y -= 24;

	enc_time		= new Text(i18n->TranslateString("Time left:"), pos);
	enc_time->SetOrientation(OR_LOWERLEFT);

	enc_percent		= new Text(i18n->TranslateString("Percent done:"), pos);
	enc_percent->SetOrientation(OR_LOWERLEFT);

	enc_encoder		= new Text(i18n->TranslateString("Selected encoder:"), pos);
	enc_encoder->SetOrientation(OR_LOWERLEFT);

	pos.y -= 24;

	enc_progress		= new Text(i18n->TranslateString("File progress:"), pos);
	enc_progress->SetOrientation(OR_LOWERLEFT);

	pos.y -= 24;

	enc_outdir		= new Text(i18n->TranslateString("Output dir.:"), pos);
	enc_outdir->SetOrientation(OR_LOWERLEFT);

	Int	 maxTextLength = max(max(enc_progress->GetObjectProperties()->textSize.cx, enc_outdir->GetObjectProperties()->textSize.cx), max(enc_filename->GetObjectProperties()->textSize.cx, enc_time->GetObjectProperties()->textSize.cx));

	enc_progress->SetPosition(Point(maxTextLength + 7 - enc_progress->GetObjectProperties()->textSize.cx, enc_progress->GetObjectProperties()->pos.y));
	enc_outdir->SetPosition(Point(maxTextLength + 7 - enc_outdir->GetObjectProperties()->textSize.cx, enc_outdir->GetObjectProperties()->pos.y));
	enc_filename->SetPosition(Point(maxTextLength + 7 - enc_filename->GetObjectProperties()->textSize.cx, enc_filename->GetObjectProperties()->pos.y));
	enc_time->SetPosition(Point(maxTextLength + 7 - enc_time->GetObjectProperties()->textSize.cx, enc_time->GetObjectProperties()->pos.y));
	enc_percent->SetPosition(Point(maxTextLength + 55, enc_percent->GetObjectProperties()->pos.y));
	enc_encoder->SetPosition(Point(maxTextLength + 102 + enc_percent->GetObjectProperties()->textSize.cx, enc_encoder->GetObjectProperties()->pos.y));

	pos.x = 7;
	pos.y = 5;

	txt_joblist		= new Text(String("0").Append(i18n->TranslateString(" file(s) in joblist:")), pos);

	pos.x = 7;
	pos.y += 19;
	size.cx = currentConfig->wndSize.cx - 20;
	size.cy = currentConfig->wndSize.cy - 251 - (currentConfig->showTitleInfo ? 65 : 0);

	joblist			= new ListBox(pos, size);
	joblist->onClick.Connect(&bonkEncGUI::SelectJoblistEntry, this);
	joblist->SetFlags(LF_MULTICHECKBOX);
	joblist->AddTab(i18n->TranslateString("Title"));
	joblist->AddTab(i18n->TranslateString("Track"), currentConfig->tab_width_track);
	joblist->AddTab(i18n->TranslateString("Length"), currentConfig->tab_width_length);
	joblist->AddTab(i18n->TranslateString("Size"), currentConfig->tab_width_size);

	droparea		= new DropArea(pos, size);
	droparea->onDropFile.Connect(&bonkEncGUI::AddDragDropFile, this);

	Int	 n = 0;

	if (currentConfig->showTitleInfo) n = 65;

	info_divider		= new Divider(136 + n, OR_HORZ | OR_BOTTOM);
	info_bottom		= new Divider(136, OR_HORZ | OR_BOTTOM);

	pos.y = 144 + n;
	pos.x = 8;
	size.cx = 90;
	size.cy = 17;

	info_background		= new Layer();
	info_background->SetMetrics(pos, size);
	info_background->SetOrientation(OR_LOWERLEFT);
	info_background->SetColor(Setup::BackgroundColor);

	pos.y = 0;
	pos.x = 2;
	size.cx = 85;
	size.cy = 0;

	info_checkbox		= new CheckBox(i18n->TranslateString("Show title info"), pos, size, &currentConfig->showTitleInfo);
	info_checkbox->onClick.Connect(&bonkEncGUI::ShowHideTitleInfo, this);
	info_checkbox->SetMetrics(pos, Size(info_checkbox->GetObjectProperties()->textSize.cx + 19, info_checkbox->GetObjectProperties()->size.cy));

	info_background->SetMetrics(info_background->GetObjectProperties()->pos, Size(info_checkbox->GetObjectProperties()->textSize.cx + 24, info_background->GetObjectProperties()->size.cy));

	pos.x = 7;
	pos.y = 161;

	info_text_artist	= new Text(i18n->TranslateString("Artist").Append(":"), pos);
	info_text_artist->SetOrientation(OR_LOWERLEFT);

	pos.y -= 24;

	info_text_album		= new Text(i18n->TranslateString("Album").Append(":"), pos);
	info_text_album->SetOrientation(OR_LOWERLEFT);

	pos.x += (7 + (Int) Math::Max(info_text_album->GetObjectProperties()->textSize.cx, info_text_artist->GetObjectProperties()->textSize.cx));
	pos.y += 27;
	size.cx = 180;
	size.cy = 0;

	info_edit_artist	= new EditBox("", pos, size, 0);
	info_edit_artist->onClick.Connect(&bonkEncGUI::UpdateTitleInfo, this);
	info_edit_artist->SetOrientation(OR_LOWERLEFT);

	pos.y -= 24;

	info_edit_album	= new EditBox("", pos, size, 0);
	info_edit_album->onClick.Connect(&bonkEncGUI::UpdateTitleInfo, this);
	info_edit_album->SetOrientation(OR_LOWERLEFT);

	pos.x += (7 + info_edit_artist->GetObjectProperties()->size.cx);
	pos.y += 21;

	info_text_title		= new Text(i18n->TranslateString("Title").Append(":"), pos);
	info_text_title->SetOrientation(OR_LOWERLEFT);

	pos.y -= 24;

	info_text_track		= new Text(i18n->TranslateString("Track").Append(":"), pos);
	info_text_track->SetOrientation(OR_LOWERLEFT);

	pos.x += (7 + (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx));
	pos.y += 27;
	size.cx = 100;

	info_edit_title		= new EditBox("", pos, size, 0);
	info_edit_title->onClick.Connect(&bonkEncGUI::UpdateTitleInfo, this);
	info_edit_title->SetOrientation(OR_LOWERLEFT);

	pos.y -= 24;
	size.cx = 25;

	info_edit_track		= new EditBox("", pos, size, 3);
	info_edit_track->SetFlags(EDB_NUMERIC);
	info_edit_track->onClick.Connect(&bonkEncGUI::UpdateTitleInfo, this);
	info_edit_track->SetOrientation(OR_LOWERLEFT);

	pos.x += (7 + info_edit_track->GetObjectProperties()->size.cx);
	pos.y -= 3;

	info_text_year		= new Text(i18n->TranslateString("Year").Append(":"), pos);
	info_text_year->SetOrientation(OR_LOWERLEFT);

	pos.x += (7 + info_text_year->GetObjectProperties()->textSize.cx);
	pos.y += 3;
	size.cx = 31;

	info_edit_year		= new EditBox("", pos, size, 4);
	info_edit_year->SetFlags(EDB_NUMERIC);
	info_edit_year->onClick.Connect(&bonkEncGUI::UpdateTitleInfo, this);
	info_edit_year->SetOrientation(OR_LOWERLEFT);

	pos.x += (7 + info_edit_year->GetObjectProperties()->size.cx);
	pos.y -= 3;

	info_text_genre		= new Text(i18n->TranslateString("Genre").Append(":"), pos);
	info_text_genre->SetOrientation(OR_LOWERLEFT);

	info_list_genre		= new ListBox(pos, size);
	info_list_genre->AddEntry("");
	info_list_genre->AddEntry("A Cappella");
	info_list_genre->AddEntry("Acid");
	info_list_genre->AddEntry("Acid Jazz");
	info_list_genre->AddEntry("Acid Punk");
	info_list_genre->AddEntry("Acoustic");
	info_list_genre->AddEntry("Alt. Rock");
	info_list_genre->AddEntry("Alternative");
	info_list_genre->AddEntry("Ambient");
	info_list_genre->AddEntry("Anime");
	info_list_genre->AddEntry("Avantgarde");
	info_list_genre->AddEntry("Ballad");
	info_list_genre->AddEntry("Bass");
	info_list_genre->AddEntry("Beat");
	info_list_genre->AddEntry("Bebob");
	info_list_genre->AddEntry("Big Band");
	info_list_genre->AddEntry("Black Metal");
	info_list_genre->AddEntry("Bluegrass");
	info_list_genre->AddEntry("Blues");
	info_list_genre->AddEntry("Booty Bass");
	info_list_genre->AddEntry("BritPop");
	info_list_genre->AddEntry("Cabaret");
	info_list_genre->AddEntry("Celtic");
	info_list_genre->AddEntry("Chamber Music");
	info_list_genre->AddEntry("Chanson");
	info_list_genre->AddEntry("Chorus");
	info_list_genre->AddEntry("Christian Gangsta Rap");
	info_list_genre->AddEntry("Christian Rap");
	info_list_genre->AddEntry("Christian Rock");
	info_list_genre->AddEntry("Classic Rock");
	info_list_genre->AddEntry("Classical");
	info_list_genre->AddEntry("Club");
	info_list_genre->AddEntry("Club-House");
	info_list_genre->AddEntry("Comedy");
	info_list_genre->AddEntry("Contemporary Christian");
	info_list_genre->AddEntry("Country");
	info_list_genre->AddEntry("Cover");
	info_list_genre->AddEntry("Crossover");
	info_list_genre->AddEntry("Cult");
	info_list_genre->AddEntry("Dance");
	info_list_genre->AddEntry("Dance Hall");
	info_list_genre->AddEntry("Darkwave");
	info_list_genre->AddEntry("Death Metal");
	info_list_genre->AddEntry("Disco");
	info_list_genre->AddEntry("Dream");
	info_list_genre->AddEntry("Drum & Bass");
	info_list_genre->AddEntry("Drum Solo");
	info_list_genre->AddEntry("Duet");
	info_list_genre->AddEntry("Easy Listening");
	info_list_genre->AddEntry("Electronic");
	info_list_genre->AddEntry("Ethnic");
	info_list_genre->AddEntry("Eurodance");
	info_list_genre->AddEntry("Euro-House");
	info_list_genre->AddEntry("Euro-Techno");
	info_list_genre->AddEntry("Fast-Fusion");
	info_list_genre->AddEntry("Folk");
	info_list_genre->AddEntry("Folk/Rock");
	info_list_genre->AddEntry("Folklore");
	info_list_genre->AddEntry("Freestyle");
	info_list_genre->AddEntry("Funk");
	info_list_genre->AddEntry("Fusion");
	info_list_genre->AddEntry("Game");
	info_list_genre->AddEntry("Gangsta Rap");
	info_list_genre->AddEntry("Goa");
	info_list_genre->AddEntry("Gospel");
	info_list_genre->AddEntry("Gothic");
	info_list_genre->AddEntry("Gothic Rock");
	info_list_genre->AddEntry("Grunge");
	info_list_genre->AddEntry("Hard Rock");
	info_list_genre->AddEntry("Hardcore");
	info_list_genre->AddEntry("Heavy Metal");
	info_list_genre->AddEntry("Hip-Hop");
	info_list_genre->AddEntry("House");
	info_list_genre->AddEntry("Humour");
	info_list_genre->AddEntry("Indie");
	info_list_genre->AddEntry("Industrial");
	info_list_genre->AddEntry("Instrumental");
	info_list_genre->AddEntry("Instrumental Pop");
	info_list_genre->AddEntry("Instrumental Rock");
	info_list_genre->AddEntry("Jazz");
	info_list_genre->AddEntry("Jazz+Funk");
	info_list_genre->AddEntry("JPop");
	info_list_genre->AddEntry("Jungle");
	info_list_genre->AddEntry("Latin");
	info_list_genre->AddEntry("Lo-Fi");
	info_list_genre->AddEntry("Meditative");
	info_list_genre->AddEntry("Merengue");
	info_list_genre->AddEntry("Metal");
	info_list_genre->AddEntry("Musical");
	info_list_genre->AddEntry("National Folk");
	info_list_genre->AddEntry("Native American");
	info_list_genre->AddEntry("Negerpunk");
	info_list_genre->AddEntry("New Age");
	info_list_genre->AddEntry("New Wave");
	info_list_genre->AddEntry("Noise");
	info_list_genre->AddEntry("Oldies");
	info_list_genre->AddEntry("Opera");
	info_list_genre->AddEntry("Other");
	info_list_genre->AddEntry("Polka");
	info_list_genre->AddEntry("Polsk Punk");
	info_list_genre->AddEntry("Pop");
	info_list_genre->AddEntry("Pop/Funk");
	info_list_genre->AddEntry("Pop-Folk");
	info_list_genre->AddEntry("Porn Groove");
	info_list_genre->AddEntry("Power Ballad");
	info_list_genre->AddEntry("Pranks");
	info_list_genre->AddEntry("Primus");
	info_list_genre->AddEntry("Progressive Rock");
	info_list_genre->AddEntry("Psychedelic");
	info_list_genre->AddEntry("Psychedelic Rock");
	info_list_genre->AddEntry("Punk");
	info_list_genre->AddEntry("Punk Rock");
	info_list_genre->AddEntry("R&B");
	info_list_genre->AddEntry("Rap");
	info_list_genre->AddEntry("Rave");
	info_list_genre->AddEntry("Reggae");
	info_list_genre->AddEntry("Remix");
	info_list_genre->AddEntry("Retro");
	info_list_genre->AddEntry("Revival");
	info_list_genre->AddEntry("Rhythmic Soul");
	info_list_genre->AddEntry("Rock");
	info_list_genre->AddEntry("Rock & Roll");
	info_list_genre->AddEntry("Salsa");
	info_list_genre->AddEntry("Samba");
	info_list_genre->AddEntry("Satire");
	info_list_genre->AddEntry("Showtunes");
	info_list_genre->AddEntry("Ska");
	info_list_genre->AddEntry("Slow Jam");
	info_list_genre->AddEntry("Slow Rock");
	info_list_genre->AddEntry("Sonata");
	info_list_genre->AddEntry("Soul");
	info_list_genre->AddEntry("Sound Clip");
	info_list_genre->AddEntry("Soundtrack");
	info_list_genre->AddEntry("Southern Rock");
	info_list_genre->AddEntry("Space");
	info_list_genre->AddEntry("Speech");
	info_list_genre->AddEntry("Swing");
	info_list_genre->AddEntry("Symphonic Rock");
	info_list_genre->AddEntry("Symphony");
	info_list_genre->AddEntry("Synthpop");
	info_list_genre->AddEntry("Tango");
	info_list_genre->AddEntry("Techno");
	info_list_genre->AddEntry("Techno-Industrial");
	info_list_genre->AddEntry("Terror");
	info_list_genre->AddEntry("Thrash-Metal");
	info_list_genre->AddEntry("Top 40");
	info_list_genre->AddEntry("Trailer");
	info_list_genre->AddEntry("Trance");
	info_list_genre->AddEntry("Tribal");
	info_list_genre->AddEntry("Trip-Hop");
	info_list_genre->AddEntry("Vocal");

	pos.x += (7 + info_text_genre->GetObjectProperties()->textSize.cx);
	pos.y += 3;
	size.cx = 130;

	info_edit_genre	= new EditBox("", pos, size, 0);
	info_edit_genre->onClick.Connect(&bonkEncGUI::UpdateTitleInfo, this);
	info_edit_genre->SetOrientation(OR_LOWERLEFT);
	info_edit_genre->SetDropDownList(info_list_genre);

	info_edit_title->SetMetrics(Point(currentConfig->wndSize.cx - 227 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx, info_edit_title->GetObjectProperties()->pos.y), Size(214 + info_text_genre->GetObjectProperties()->textSize.cx + info_text_year->GetObjectProperties()->textSize.cx, info_edit_title->GetObjectProperties()->size.cy));
	info_edit_track->SetMetrics(Point(currentConfig->wndSize.cx - 227 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx, info_edit_track->GetObjectProperties()->pos.y), info_edit_track->GetObjectProperties()->size);
	info_text_year->SetMetrics(Point(info_edit_track->GetObjectProperties()->pos.x + 32, info_text_year->GetObjectProperties()->pos.y), info_text_year->GetObjectProperties()->size);
	info_edit_year->SetMetrics(Point(info_text_year->GetObjectProperties()->pos.x + info_text_year->GetObjectProperties()->textSize.cx + 7, info_edit_year->GetObjectProperties()->pos.y), info_edit_year->GetObjectProperties()->size);
	info_text_genre->SetMetrics(Point(info_edit_year->GetObjectProperties()->pos.x + 38, info_text_genre->GetObjectProperties()->pos.y), info_text_genre->GetObjectProperties()->size);
	info_text_title->SetMetrics(Point(info_edit_title->GetObjectProperties()->pos.x - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx) - 7, info_text_title->GetObjectProperties()->pos.y), info_text_title->GetObjectProperties()->size);
	info_text_track->SetMetrics(Point(info_edit_title->GetObjectProperties()->pos.x - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx) - 7, info_text_track->GetObjectProperties()->pos.y), info_text_track->GetObjectProperties()->size);
	info_edit_artist->SetMetrics(Point((Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) + 15, info_edit_artist->GetObjectProperties()->pos.y), Size(currentConfig->wndSize.cx - 256 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx - (Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx), info_edit_artist->GetObjectProperties()->size.cy));
	info_edit_album->SetMetrics(Point((Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) + 15, info_edit_album->GetObjectProperties()->pos.y), Size(currentConfig->wndSize.cx - 256 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx - (Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx), info_edit_album->GetObjectProperties()->size.cy));
	info_edit_genre->SetMetrics(Point(currentConfig->wndSize.cx - 143, info_edit_genre->GetObjectProperties()->pos.y), info_edit_genre->GetObjectProperties()->size);

	info_edit_artist->Deactivate();
	info_edit_title->Deactivate();
	info_edit_album->Deactivate();
	info_edit_track->Deactivate();
	info_edit_year->Deactivate();
	info_edit_genre->Deactivate();

	pos.y = 99;
	pos.x = maxTextLength + 14;
	size.cx = currentConfig->wndSize.cx - 27 - maxTextLength;
	size.cy = 0;

	edb_filename		= new EditBox(i18n->TranslateString("none"), pos, size, 1024);
	edb_filename->SetOrientation(OR_LOWERLEFT);
	edb_filename->Deactivate();

	pos.y -= 24;
	size.cx = 34;

	edb_time		= new EditBox("00:00", pos, size, 5);
	edb_time->SetOrientation(OR_LOWERLEFT);
	edb_time->Deactivate();

	pos.x += (48 + enc_percent->GetObjectProperties()->textSize.cx);
	size.cx = 33;

	edb_percent		= new EditBox("0%", pos, size, 4);
	edb_percent->SetOrientation(OR_LOWERLEFT);
	edb_percent->Deactivate();

	pos.x += (47 + enc_encoder->GetObjectProperties()->textSize.cx);
	size.cx = currentConfig->wndSize.cx - 122 - maxTextLength - enc_percent->GetObjectProperties()->textSize.cx - enc_encoder->GetObjectProperties()->textSize.cx;

	if (currentConfig->encoder == ENCODER_BONKENC)		edb_encoder = new EditBox("Bonk", pos, size, 4);
	else if (currentConfig->encoder == ENCODER_BLADEENC)	edb_encoder = new EditBox("BladeEnc", pos, size, 4);
	else if (currentConfig->encoder == ENCODER_LAMEENC)	edb_encoder = new EditBox("LAME", pos, size, 4);
	else if (currentConfig->encoder == ENCODER_VORBISENC)	edb_encoder = new EditBox("Ogg Vorbis", pos, size, 4);
	else if (currentConfig->encoder == ENCODER_FAAC)	edb_encoder = new EditBox("FAAC", pos, size, 4);
	else if (currentConfig->encoder == ENCODER_TVQ)		edb_encoder = new EditBox("TwinVQ", pos, size, 4);
	else if (currentConfig->encoder == ENCODER_WAVE)	edb_encoder = new EditBox("WAVE Out", pos, size, 4);

	edb_encoder->SetOrientation(OR_LOWERLEFT);
	edb_encoder->Deactivate();

	pos.x = maxTextLength + 14;
	pos.y -= 48;
	size.cx = currentConfig->wndSize.cx - 27 - maxTextLength;

	edb_outdir		= new EditBox(currentConfig->enc_outdir, pos, size, 1024);
	edb_outdir->SetOrientation(OR_LOWERLEFT);
	edb_outdir->Deactivate();

	pos.x = maxTextLength + 14;
	pos.y = 51;
	size.cx = currentConfig->wndSize.cx - 27 - maxTextLength;

	progress		= new Progressbar(pos, size, OR_HORZ, PB_NOTEXT, 0, 1000, 0);
	progress->SetOrientation(OR_LOWERLEFT);
	progress->Deactivate();

	menu_file->AddEntry(i18n->TranslateString("Add"), NIL, menu_addsubmenu);
	menu_file->AddEntry(i18n->TranslateString("Remove"))->onClick.Connect(&bonkEncGUI::RemoveFile, this);
	menu_file->AddEntry();
	menu_file->AddEntry(i18n->TranslateString("Clear joblist"))->onClick.Connect(&bonkEnc::ClearList, (bonkEnc *) this);
	menu_file->AddEntry();
	menu_file->AddEntry(i18n->TranslateString("Exit"))->onClick.Connect(&bonkEncGUI::Close, this);

	menu_options->AddEntry(i18n->TranslateString("General settings..."))->onClick.Connect(&bonkEncGUI::ConfigureGeneral, this);
	menu_options->AddEntry(i18n->TranslateString("Configure selected encoder..."))->onClick.Connect(&bonkEncGUI::ConfigureEncoder, this);

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1)
	{
		menu_options->AddEntry();

		for (Int j = 0; j < currentConfig->cdrip_numdrives; j++)
		{
			menu_seldrive->AddEntry(currentConfig->cdrip_drives.GetNthEntry(j), NIL, NIL, NIL, &currentConfig->cdrip_activedrive, j);
		}

		menu_options->AddEntry(i18n->TranslateString("Active CD-ROM drive"), NIL, menu_seldrive);
	}

	menu_addsubmenu->AddEntry(i18n->TranslateString("Audio file(s)..."))->onClick.Connect(&bonkEncGUI::AddFile, this);

	MenuEntry	*entry;

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1)
	{
		for (Int j = 0; j < currentConfig->cdrip_numdrives; j++)
		{
			entry = menu_drives->AddEntry(currentConfig->cdrip_drives.GetNthEntry(j), NIL, NIL, NIL, &clicked_drive, j);
			entry->onClick.Connect(&bonkEncGUI::ReadSpecificCD, this);
		}

		menu_addsubmenu->AddEntry(i18n->TranslateString("Audio CD contents"))->onClick.Connect(&bonkEnc::ReadCD, (bonkEnc *) this);

		if (currentConfig->cdrip_numdrives > 1)
		{
			menu_addsubmenu->AddEntry();
			menu_addsubmenu->AddEntry(i18n->TranslateString("Audio CD contents"), NIL, currentConfig->cdrip_numdrives > 1 ? menu_drives : NIL);
		}
	}

	menu_encode->AddEntry(i18n->TranslateString("Start encoding"))->onClick.Connect(&bonkEnc::Encode, (bonkEnc *) this);
	menu_encode->AddEntry(i18n->TranslateString("Stop encoding"))->onClick.Connect(&bonkEnc::StopEncoding, (bonkEnc *) this);

	menu_database->AddEntry(i18n->TranslateString("Query CDDB database"))->onClick.Connect(&bonkEncGUI::QueryCDDB, this);
	menu_database->AddEntry(i18n->TranslateString("Submit CDDB data..."))->onClick.Connect(&bonkEncGUI::SubmitCDDBData, this);
	menu_database->AddEntry();
	menu_database->AddEntry(i18n->TranslateString("Automatic CDDB queries"), NIL, NIL, &currentConfig->enable_auto_cddb);
	menu_database->AddEntry(i18n->TranslateString("Enable CDDB cache"), NIL, NIL, &currentConfig->enable_cddb_cache);

	if (winamp_out_modules.GetNOfEntries() > 0)
	{
		menu_trackmenu->AddEntry(i18n->TranslateString("Play"))->onClick.Connect(&bonkEnc::PlaySelectedItem, (bonkEnc *) this);
		menu_trackmenu->AddEntry(i18n->TranslateString("Stop"))->onClick.Connect(&bonkEnc::StopPlayback, (bonkEnc *) this);
		menu_trackmenu->AddEntry();
	}

	menu_trackmenu->AddEntry(i18n->TranslateString("Remove"))->onClick.Connect(&bonkEncGUI::RemoveFile, this);
	menu_trackmenu->AddEntry();
	menu_trackmenu->AddEntry(i18n->TranslateString("Clear joblist"))->onClick.Connect(&bonkEnc::ClearList, (bonkEnc *) this);

	menu_help->AddEntry(i18n->TranslateString("Help topics..."))->onClick.Connect(&bonkEncGUI::ShowHelp, this);
	menu_help->AddEntry();
	menu_help->AddEntry(i18n->TranslateString("About BonkEnc").Append("..."))->onClick.Connect(&bonkEncGUI::About, this);

	mainWnd_menubar->AddEntry(i18n->TranslateString("File"), NIL, menu_file);

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1) mainWnd_menubar->AddEntry(i18n->TranslateString("Database"), NIL, menu_database);

	mainWnd_menubar->AddEntry(i18n->TranslateString("Options"), NIL, menu_options);
	mainWnd_menubar->AddEntry(i18n->TranslateString("Encode"), NIL, menu_encode);
	mainWnd_menubar->AddEntry()->SetOrientation(OR_RIGHT);
	mainWnd_menubar->AddEntry(i18n->TranslateString("Help"), NIL, menu_help, NIL, NIL, 0, OR_RIGHT);

	mainWnd_iconbar->SetBitmapSize(24);

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 1, NIL));
	entry->onClick.Connect(&bonkEncGUI::AddFile, this);
	entry->SetStatusText(i18n->TranslateString("Add audio file(s) to the joblist"));

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1)
	{
		entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 2, NIL), currentConfig->cdrip_numdrives > 1 ? menu_drives : NIL);
		entry->onClick.Connect(&bonkEnc::ReadCD, (bonkEnc *) this);
		entry->SetStatusText(i18n->TranslateString("Add audio CD contents to the joblist"));
	}

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 3, NIL));
	entry->onClick.Connect(&bonkEncGUI::RemoveFile, this);
	entry->SetStatusText(i18n->TranslateString("Remove the selected entry from the joblist"));

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 4, NIL));
	entry->onClick.Connect(&bonkEnc::ClearList, (bonkEnc *) this);
	entry->SetStatusText(i18n->TranslateString("Clear the entire joblist"));

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1)
	{
		mainWnd_iconbar->AddEntry();

		entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 9, NIL));
		entry->onClick.Connect(&bonkEncGUI::QueryCDDB, this);
		entry->SetStatusText(i18n->TranslateString("Query CDDB database"));

		entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 10, NIL));
		entry->onClick.Connect(&bonkEncGUI::SubmitCDDBData, this);
		entry->SetStatusText(i18n->TranslateString("Submit CDDB data..."));
	}

	mainWnd_iconbar->AddEntry();

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 5, NIL));
	entry->onClick.Connect(&bonkEncGUI::ConfigureGeneral, this);
	entry->SetStatusText(i18n->TranslateString("Configure general settings"));

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 6, NIL));
	entry->onClick.Connect(&bonkEncGUI::ConfigureEncoder, this);
	entry->SetStatusText(i18n->TranslateString("Configure the selected audio encoder"));

	mainWnd_iconbar->AddEntry();

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 7, NIL));
	entry->onClick.Connect(&bonkEnc::Encode, (bonkEnc *) this);
	entry->SetStatusText(i18n->TranslateString("Start the encoding process"));

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 8, NIL));
	entry->onClick.Connect(&bonkEnc::StopEncoding, (bonkEnc *) this);
	entry->SetStatusText(i18n->TranslateString("Stop encoding"));

	RegisterObject(mainWnd);

	mainWnd->RegisterObject(joblist);
	mainWnd->RegisterObject(droparea);

	if (winamp_out_modules.GetNOfEntries() > 0)
	{
		mainWnd->RegisterObject(button_play);
		mainWnd->RegisterObject(button_pause);
		mainWnd->RegisterObject(button_stop);
		mainWnd->RegisterObject(button_prev);
		mainWnd->RegisterObject(button_next);
	}

	mainWnd->RegisterObject(txt_joblist);
	mainWnd->RegisterObject(info_divider);
	mainWnd->RegisterObject(info_bottom);
	mainWnd->RegisterObject(info_background);
	mainWnd->RegisterObject(info_text_artist);
	mainWnd->RegisterObject(info_edit_artist);
	mainWnd->RegisterObject(info_text_title);
	mainWnd->RegisterObject(info_edit_title);
	mainWnd->RegisterObject(info_text_album);
	mainWnd->RegisterObject(info_edit_album);
	mainWnd->RegisterObject(info_text_track);
	mainWnd->RegisterObject(info_edit_track);
	mainWnd->RegisterObject(info_text_year);
	mainWnd->RegisterObject(info_edit_year);
	mainWnd->RegisterObject(info_text_genre);
	mainWnd->RegisterObject(info_edit_genre);
	mainWnd->RegisterObject(enc_filename);
	mainWnd->RegisterObject(enc_time);
	mainWnd->RegisterObject(enc_percent);
	mainWnd->RegisterObject(enc_encoder);
	mainWnd->RegisterObject(enc_progress);
	mainWnd->RegisterObject(enc_outdir);
	mainWnd->RegisterObject(edb_filename);
	mainWnd->RegisterObject(edb_time);
	mainWnd->RegisterObject(edb_percent);
	mainWnd->RegisterObject(edb_encoder);
	mainWnd->RegisterObject(edb_outdir);
	mainWnd->RegisterObject(progress);
	mainWnd->RegisterObject(hyperlink);
	mainWnd->RegisterObject(mainWnd_titlebar);
	mainWnd->RegisterObject(mainWnd_statusbar);
	mainWnd->RegisterObject(mainWnd_menubar);
	mainWnd->RegisterObject(mainWnd_iconbar);

	info_background->RegisterObject(info_checkbox);

	if (!currentConfig->showTitleInfo)
	{
		info_bottom->Hide();
		info_text_artist->Hide();
		info_edit_artist->Hide();
		info_text_title->Hide();
		info_edit_title->Hide();
		info_text_album->Hide();
		info_edit_album->Hide();
		info_text_track->Hide();
		info_edit_track->Hide();
		info_text_year->Hide();
		info_edit_year->Hide();
		info_text_genre->Hide();
		info_edit_genre->Hide();
	}

	mainWnd->SetIcon(SMOOTH::LoadImage("BonkEnc.pci", 0, NIL));
	mainWnd->SetApplicationIcon(MAKEINTRESOURCE(IDI_ICON));
	mainWnd->SetMetrics(currentConfig->wndPos, currentConfig->wndSize);
	mainWnd->onResize.Connect(&bonkEncGUI::ResizeProc, this);
	mainWnd->doQuit.Connect(&bonkEncGUI::ExitProc, this);
	mainWnd->getTrackMenu.Connect(&bonkEncGUI::GetTrackMenu, this);
	mainWnd->SetMinimumSize(Size(530, 300 + n));

	if (currentConfig->maximized) mainWnd->Maximize();
}

bonkEncGUI::~bonkEncGUI()
{
	ClearList();

	delete mainWnd_menubar;
	delete mainWnd_iconbar;
	delete mainWnd_titlebar;
	delete mainWnd_statusbar;
	delete mainWnd;
	delete joblist;
	delete droparea;

	if (winamp_out_modules.GetNOfEntries() > 0)
	{
		delete button_play;
		delete button_pause;
		delete button_stop;
		delete button_prev;
		delete button_next;
	}

	delete txt_joblist;
	delete info_divider;
	delete info_bottom;
	delete info_background;
	delete info_checkbox;
	delete info_text_artist;
	delete info_edit_artist;
	delete info_text_title;
	delete info_edit_title;
	delete info_text_album;
	delete info_edit_album;
	delete info_text_track;
	delete info_edit_track;
	delete info_text_year;
	delete info_edit_year;
	delete info_text_genre;
	delete info_edit_genre;
	delete info_list_genre;
	delete enc_filename;
	delete enc_time;
	delete enc_percent;
	delete enc_encoder;
	delete enc_progress;
	delete enc_outdir;
	delete edb_filename;
	delete edb_time;
	delete edb_percent;
	delete edb_encoder;
	delete edb_outdir;
	delete progress;
	delete menu_file;
	delete menu_options;
	delete menu_addsubmenu;
	delete menu_encode;
	delete menu_drives;
	delete menu_seldrive;
	delete menu_database;
	delete menu_trackmenu;
	delete menu_help;
	delete hyperlink;

	delete i18n;
}

Bool bonkEncGUI::ExitProc()
{
	if (encoding)
	{
		if (IDNO == SMOOTH::MessageBox(i18n->TranslateString("The encoding thread is still running! Do you really want to quit?"), i18n->TranslateString("Currently encoding"), MB_YESNO, IDI_QUESTION)) return False;

		StopEncoding();
	}

	if (playing) StopPlayback();

	Rect	 wndRect = mainWnd->GetWindowRect();

	currentConfig->wndPos = Point(wndRect.left, wndRect.top);
	currentConfig->wndSize = Size(wndRect.right - wndRect.left, wndRect.bottom - wndRect.top);
	currentConfig->maximized = mainWnd->IsMaximized();

	currentConfig->tab_width_track = joblist->GetNthTabWidth(1);
	currentConfig->tab_width_length = joblist->GetNthTabWidth(2);
	currentConfig->tab_width_size = joblist->GetNthTabWidth(3);

	currentConfig->SaveSettings();

	return True;
}

Void bonkEncGUI::ResizeProc()
{
	mainWnd_statusbar->GetObjectProperties()->text = String("BonkEnc ").Append(bonkEnc::version).Append(" - Copyright (C) 2001-2003 Robert Kausch");

	currentConfig->wndPos = mainWnd->GetObjectProperties()->pos;
	currentConfig->wndSize = mainWnd->GetObjectProperties()->size;

	info_edit_title->GetObjectProperties()->pos = Point(currentConfig->wndSize.cx - 227 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx, info_edit_title->GetObjectProperties()->pos.y);
	info_edit_title->GetObjectProperties()->size = Size(214 + info_text_genre->GetObjectProperties()->textSize.cx + info_text_year->GetObjectProperties()->textSize.cx, info_edit_title->GetObjectProperties()->size.cy);
	info_edit_track->GetObjectProperties()->pos = Point(currentConfig->wndSize.cx - 227 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx, info_edit_track->GetObjectProperties()->pos.y);
	info_text_year->GetObjectProperties()->pos = Point(info_edit_track->GetObjectProperties()->pos.x + 32, info_text_year->GetObjectProperties()->pos.y);
	info_edit_year->GetObjectProperties()->pos = Point(info_text_year->GetObjectProperties()->pos.x + info_text_year->GetObjectProperties()->textSize.cx + 7, info_edit_year->GetObjectProperties()->pos.y);
	info_text_genre->GetObjectProperties()->pos = Point(info_edit_year->GetObjectProperties()->pos.x + 38, info_text_genre->GetObjectProperties()->pos.y);
	info_text_title->GetObjectProperties()->pos = Point(info_edit_title->GetObjectProperties()->pos.x - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx) - 7, info_text_title->GetObjectProperties()->pos.y);
	info_text_track->GetObjectProperties()->pos = Point(info_edit_title->GetObjectProperties()->pos.x - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx) - 7, info_text_track->GetObjectProperties()->pos.y);
	info_edit_artist->GetObjectProperties()->size = Size(currentConfig->wndSize.cx - 256 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx - (Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx), info_edit_artist->GetObjectProperties()->size.cy);
	info_edit_album->GetObjectProperties()->size = Size(currentConfig->wndSize.cx - 256 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx - (Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx), info_edit_album->GetObjectProperties()->size.cy);
	info_edit_genre->GetObjectProperties()->pos = Point(currentConfig->wndSize.cx - 143, info_edit_genre->GetObjectProperties()->pos.y);

	Int	 maxTextLength = (Int) Math::Max(Math::Max(enc_progress->GetObjectProperties()->textSize.cx, enc_outdir->GetObjectProperties()->textSize.cx), Math::Max(enc_filename->GetObjectProperties()->textSize.cx, enc_time->GetObjectProperties()->textSize.cx));

	edb_filename->GetObjectProperties()->size = Size(currentConfig->wndSize.cx - 27 - maxTextLength, edb_filename->GetObjectProperties()->size.cy);
	edb_encoder->GetObjectProperties()->size = Size(currentConfig->wndSize.cx - 122 - maxTextLength - enc_percent->GetObjectProperties()->textSize.cx - enc_encoder->GetObjectProperties()->textSize.cx, edb_encoder->GetObjectProperties()->size.cy);
	edb_outdir->GetObjectProperties()->size = Size(currentConfig->wndSize.cx - 27 - maxTextLength, edb_outdir->GetObjectProperties()->size.cy);

	progress->GetObjectProperties()->size = Size(currentConfig->wndSize.cx - 27 - maxTextLength, progress->GetObjectProperties()->size.cy);

	joblist->GetObjectProperties()->size = Size(currentConfig->wndSize.cx - 20, currentConfig->wndSize.cy - 251 - (currentConfig->showTitleInfo ? 65 : 0));
	droparea->GetObjectProperties()->size = Size(currentConfig->wndSize.cx - 20, currentConfig->wndSize.cy - 251 - (currentConfig->showTitleInfo ? 65 : 0));

	currentConfig->tab_width_track = joblist->GetNthTabWidth(1);
	currentConfig->tab_width_length = joblist->GetNthTabWidth(2);
	currentConfig->tab_width_size = joblist->GetNthTabWidth(3);
}

Void bonkEncGUI::Close()
{
	mainWnd->Close();
}

Void bonkEncGUI::About()
{
	SMOOTH::MessageBox(String("BonkEnc ").Append(bonkEnc::version).Append("\nCopyright (C) 2001-2003 Robert Kausch\n\n").Append(i18n->TranslateString("This program is being distributed under the terms")).Append("\n").Append(i18n->TranslateString("of the GNU General Public License (GPL).")), i18n->TranslateString("About BonkEnc"), MB_OK, MAKEINTRESOURCE(IDI_ICON));
}

Void bonkEncGUI::ConfigureEncoder()
{
	if (encoding)
	{
		SMOOTH::MessageBox(i18n->TranslateString("Cannot configure encoder while encoding!"), i18n->TranslateString("Error"), MB_OK, IDI_HAND);

		return;
	}

	if (currentConfig->encoder == ENCODER_WAVE)
	{
		SMOOTH::MessageBox(i18n->TranslateString("No options can be configured for the WAVE Out filter!"), i18n->TranslateString("WAVE Out filter"), MB_OK, IDI_INFORMATION);

		return;
	}

	Dialog	*dlg = NIL;

	if (currentConfig->encoder == ENCODER_BONKENC)		dlg = new configureBonkEnc();
	else if (currentConfig->encoder == ENCODER_BLADEENC)	dlg = new configureBladeEnc();
	else if (currentConfig->encoder == ENCODER_LAMEENC)	dlg = new configureLameEnc();
	else if (currentConfig->encoder == ENCODER_VORBISENC)	dlg = new configureVorbisEnc();
	else if (currentConfig->encoder == ENCODER_FAAC)	dlg = new configureFAAC();
	else if (currentConfig->encoder == ENCODER_TVQ)		dlg = new configureTVQ();

	dlg->SetParentWindow(mainWnd);
	dlg->ShowDialog();

	DeleteObject(dlg);
}

Void bonkEncGUI::ConfigureGeneral()
{
	if (encoding)
	{
		SMOOTH::MessageBox(i18n->TranslateString("Cannot change settings while encoding!"), i18n->TranslateString("Error"), MB_OK, IDI_HAND);

		return;
	}

	configureGeneralSettings	*dlg = new configureGeneralSettings();

	dlg->ShowDialog();

	delete dlg;

	if (currentConfig->languageChanged)
	{
		SetLanguage(currentConfig->language);

		currentConfig->languageChanged = false;
	}

	if (currentConfig->encoder == ENCODER_BONKENC)		edb_encoder->SetText("Bonk");
	else if (currentConfig->encoder == ENCODER_BLADEENC)	edb_encoder->SetText("BladeEnc");
	else if (currentConfig->encoder == ENCODER_LAMEENC)	edb_encoder->SetText("LAME");
	else if (currentConfig->encoder == ENCODER_VORBISENC)	edb_encoder->SetText("Ogg Vorbis");
	else if (currentConfig->encoder == ENCODER_FAAC)	edb_encoder->SetText("FAAC");
	else if (currentConfig->encoder == ENCODER_TVQ)		edb_encoder->SetText("TwinVQ");
	else if (currentConfig->encoder == ENCODER_WAVE)	edb_encoder->SetText("WAVE Out");

	edb_outdir->SetText(currentConfig->enc_outdir);
}

Void bonkEncGUI::ReadSpecificCD()
{
	currentConfig->cdrip_activedrive = clicked_drive;

	clicked_drive = -1;

	ReadCD();
}

Void bonkEncGUI::QueryCDDB()
{
	Array<Int>	 discIDs;

	for (Int i = 0; i < joblist->GetNOfEntries(); i++)
	{
		bonkFormatInfo	*format = sa_formatinfo.GetNthEntry(i);

		if (format->trackInfo->isCDTrack) discIDs.AddEntry(format->trackInfo->discid, format->trackInfo->drive);
	}

	for (Int j = 0; j < discIDs.GetNOfEntries(); j++)
	{
		bonkEncCDDB	 cddb(currentConfig);
		Int		 discID = discIDs.GetNthEntry(j);

		Array<bonkFormatInfo::bonkTrackInfo *>	*cdInfo = NIL;

		if (currentConfig->enable_cddb_cache)
		{
			cdInfo = bonkEncCDDB::titleCache.GetEntry(discID);
			currentConfig->appMain->cddbInfo = bonkEncCDDB::infoCache.GetEntry(discID);
		}

		if (cdInfo == NIL)
		{
			Int	 oDrive = currentConfig->cdrip_activedrive;

			currentConfig->cdrip_activedrive = discIDs.GetNthEntryIndex(j);

			cdInfo = currentConfig->appMain->GetCDDBData();

			bonkEncCDDB::titleCache.RemoveEntry(discID);
			bonkEncCDDB::titleCache.AddEntry(cdInfo, discID);

			bonkEncCDDB::infoCache.RemoveEntry(discID);
			bonkEncCDDB::infoCache.AddEntry(currentConfig->appMain->cddbInfo, discID);

			currentConfig->cdrip_activedrive = oDrive;
		}

		if (cdInfo != NIL)
		{
			for (Int k = 0; k < joblist->GetNOfEntries(); k++)
			{
				bonkFormatInfo	*format = sa_formatinfo.GetNthEntry(k);

				if (format->trackInfo->isCDTrack && format->trackInfo->discid == discID)
				{
					format->trackInfo->track	= format->trackInfo->cdTrack;
					format->trackInfo->outfile	= NIL;
					format->trackInfo->hasText	= True;
					format->trackInfo->artist	= cdInfo->GetEntry(0)->artist;
					format->trackInfo->title	= cdInfo->GetEntry(format->trackInfo->cdTrack)->title;
					format->trackInfo->album	= cdInfo->GetEntry(0)->album;
					format->trackInfo->genre	= cdInfo->GetEntry(0)->genre;
					format->trackInfo->year		= cdInfo->GetEntry(0)->year;

					String	 jlEntry;

					if (format->trackInfo->artist.Length() == 0 &&
					    format->trackInfo->title.Length() == 0)	jlEntry = String(format->trackInfo->origFilename).Append("\t");
					else						jlEntry = String(format->trackInfo->artist.Length() > 0 ? format->trackInfo->artist : i18n->TranslateString("unknown artist")).Append(" - ").Append(format->trackInfo->title.Length() > 0 ? format->trackInfo->title : i18n->TranslateString("unknown title")).Append("\t");

					jlEntry.Append(format->trackInfo->track > 0 ? (format->trackInfo->track < 10 ? String("0").Append(String::FromInt(format->trackInfo->track)) : String::FromInt(format->trackInfo->track)) : String("")).Append("\t").Append(format->trackInfo->length).Append("\t").Append(format->trackInfo->fileSize);

					joblist->GetNthEntry(k)->name = jlEntry;
				}
			}
		}
	}

	joblist->Paint(SP_PAINT);
}

Void bonkEncGUI::SubmitCDDBData()
{
	cddbSubmitDlg	*dlg = new cddbSubmitDlg();

	dlg->ShowDialog();

	delete dlg;
}

Void bonkEncGUI::ShowHideTitleInfo()
{
	Int	 n = 0;

	if (currentConfig->showTitleInfo)
	{
		n = 65;

		mainWnd->SetMinimumSize(Size(530, 300 + n));
	}
	else
	{
		n = -65;

		mainWnd->SetMinimumSize(Size(530, 300));

		info_bottom->Hide();
		info_text_artist->Hide();
		info_edit_artist->Hide();
		info_text_title->Hide();
		info_edit_title->Hide();
		info_text_album->Hide();
		info_edit_album->Hide();
		info_text_track->Hide();
		info_edit_track->Hide();
		info_text_year->Hide();
		info_edit_year->Hide();
		info_text_genre->Hide();
		info_edit_genre->Hide();
	}

	if (mainWnd->IsMaximized())
	{
		joblist->SetMetrics(joblist->GetObjectProperties()->pos, Size(currentConfig->wndSize.cx - 20, currentConfig->wndSize.cy - 251 - (currentConfig->showTitleInfo ? 65 : 0)));
		droparea->SetMetrics(droparea->GetObjectProperties()->pos, Size(currentConfig->wndSize.cx - 20, currentConfig->wndSize.cy - 251 - (currentConfig->showTitleInfo ? 65 : 0)));
	}

	info_divider->SetPos(info_divider->GetPos() + n);
	info_background->SetMetrics(Point(info_background->GetObjectProperties()->pos.x, info_background->GetObjectProperties()->pos.y + n), info_background->GetObjectProperties()->size);

	joblist->Paint(SP_PAINT);

	if (!mainWnd->IsMaximized())
	{
		mainWnd->SetMetrics(mainWnd->GetObjectProperties()->pos, Size(mainWnd->GetObjectProperties()->size.cx, mainWnd->GetObjectProperties()->size.cy + n));
	}

	info_checkbox->Paint(SP_PAINT);

	if (currentConfig->showTitleInfo)
	{
		info_bottom->Show();
		info_text_artist->Show();
		info_edit_artist->Show();
		info_text_title->Show();
		info_edit_title->Show();
		info_text_album->Show();
		info_edit_album->Show();
		info_text_track->Show();
		info_edit_track->Show();
		info_text_year->Show();
		info_edit_year->Show();
		info_text_genre->Show();
		info_edit_genre->Show();
	}
}

Bool bonkEncGUI::SetLanguage(String newLanguage)
{
	i18n->ActivateLanguage(currentConfig->language);

	txt_joblist->SetText(String::FromInt(joblist->GetNOfEntries()).Append(i18n->TranslateString(" file(s) in joblist:")));

	enc_filename->Hide();
	enc_time->Hide();
	enc_percent->Hide();
	enc_encoder->Hide();
	enc_progress->Hide();
	enc_outdir->Hide();

	edb_filename->Hide();
	edb_time->Hide();
	edb_percent->Hide();
	edb_encoder->Hide();
	edb_outdir->Hide();

	progress->Hide();

	info_checkbox->Hide();

	enc_filename->SetText(i18n->TranslateString("Encoding file:"));
	enc_time->SetText(i18n->TranslateString("Time left:"));
	enc_percent->SetText(i18n->TranslateString("Percent done:"));
	enc_encoder->SetText(i18n->TranslateString("Selected encoder:"));
	enc_progress->SetText(i18n->TranslateString("File progress:"));
	enc_outdir->SetText(i18n->TranslateString("Output dir.:"));

	Int	 maxTextLength = max(max(enc_progress->GetObjectProperties()->textSize.cx, enc_outdir->GetObjectProperties()->textSize.cx), max(enc_filename->GetObjectProperties()->textSize.cx, enc_time->GetObjectProperties()->textSize.cx));

	enc_progress->SetPosition(Point(maxTextLength + 7 - enc_progress->GetObjectProperties()->textSize.cx, enc_progress->GetObjectProperties()->pos.y));
	enc_outdir->SetPosition(Point(maxTextLength + 7 - enc_outdir->GetObjectProperties()->textSize.cx, enc_outdir->GetObjectProperties()->pos.y));
	enc_filename->SetPosition(Point(maxTextLength + 7 - enc_filename->GetObjectProperties()->textSize.cx, enc_filename->GetObjectProperties()->pos.y));
	enc_time->SetPosition(Point(maxTextLength + 7 - enc_time->GetObjectProperties()->textSize.cx, enc_time->GetObjectProperties()->pos.y));
	enc_percent->SetPosition(Point(maxTextLength + 55, enc_percent->GetObjectProperties()->pos.y));
	enc_encoder->SetPosition(Point(maxTextLength + 102 + enc_percent->GetObjectProperties()->textSize.cx, enc_encoder->GetObjectProperties()->pos.y));

	edb_filename->SetText(i18n->TranslateString("none"));

	edb_filename->SetMetrics(Point(maxTextLength + 14, edb_filename->GetObjectProperties()->pos.y), Size(currentConfig->wndSize.cx - 27 - maxTextLength, edb_filename->GetObjectProperties()->size.cy));
	edb_time->SetMetrics(Point(maxTextLength + 14, edb_time->GetObjectProperties()->pos.y), Size(34, edb_time->GetObjectProperties()->size.cy));
	edb_percent->SetMetrics(Point(maxTextLength + 62 + enc_percent->GetObjectProperties()->textSize.cx, edb_percent->GetObjectProperties()->pos.y), Size(33, edb_percent->GetObjectProperties()->size.cy));
	edb_encoder->SetMetrics(Point(maxTextLength + 109 + enc_percent->GetObjectProperties()->textSize.cx + enc_encoder->GetObjectProperties()->textSize.cx, edb_encoder->GetObjectProperties()->pos.y), Size(currentConfig->wndSize.cx - 122 - maxTextLength - enc_percent->GetObjectProperties()->textSize.cx - enc_encoder->GetObjectProperties()->textSize.cx, edb_encoder->GetObjectProperties()->size.cy));
	edb_outdir->SetMetrics(Point(maxTextLength + 14, edb_outdir->GetObjectProperties()->pos.y), Size(currentConfig->wndSize.cx - 27 - maxTextLength, edb_outdir->GetObjectProperties()->size.cy));

	progress->SetMetrics(Point(maxTextLength + 14, progress->GetObjectProperties()->pos.y), Size(currentConfig->wndSize.cx - 27 - maxTextLength, progress->GetObjectProperties()->size.cy));
 
	info_checkbox->SetText(i18n->TranslateString("Show title info"));
	info_checkbox->SetMetrics(info_checkbox->GetObjectProperties()->pos, Size(info_checkbox->GetObjectProperties()->textSize.cx + 19, info_checkbox->GetObjectProperties()->size.cy));

	info_background->SetMetrics(info_background->GetObjectProperties()->pos, Size(info_checkbox->GetObjectProperties()->textSize.cx + 24, info_background->GetObjectProperties()->size.cy));

	enc_filename->Show();
	enc_time->Show();
	enc_percent->Show();
	enc_encoder->Show();
	enc_progress->Show();
	enc_outdir->Show();

	edb_filename->Show();
	edb_time->Show();
	edb_percent->Show();
	edb_encoder->Show();
	edb_outdir->Show();

	progress->Show();

	info_checkbox->Show();

	joblist->Hide();

	for (Int i = 0; i < joblist->GetNOfEntries(); i++)
	{
		Int		 id = joblist->GetNthEntry(i)->id;
		bonkFormatInfo	*format = sa_formatinfo.GetEntry(id);

		if ((format->trackInfo->artist.Length() == 0 && format->trackInfo->title.Length() != 0) ||
		    (format->trackInfo->artist.Length() != 0 && format->trackInfo->title.Length() == 0))
		{
			joblist->ModifyEntry(id, String(format->trackInfo->artist.Length() > 0 ? format->trackInfo->artist : i18n->TranslateString("unknown artist")).Append(" - ").Append(format->trackInfo->title.Length() > 0 ? format->trackInfo->title : i18n->TranslateString("unknown title")).Append("\t").Append(format->trackInfo->track > 0 ? (format->trackInfo->track < 10 ? String("0").Append(String::FromInt(format->trackInfo->track)) : String::FromInt(format->trackInfo->track)) : String("")).Append("\t").Append(format->trackInfo->length).Append("\t").Append(format->trackInfo->fileSize));
		}
	}

	currentConfig->tab_width_track = joblist->GetNthTabWidth(1);
	currentConfig->tab_width_length = joblist->GetNthTabWidth(2);
	currentConfig->tab_width_size = joblist->GetNthTabWidth(3);

	joblist->ClearTabs();

	joblist->AddTab(i18n->TranslateString("Title"));
	joblist->AddTab(i18n->TranslateString("Track"), currentConfig->tab_width_track);
	joblist->AddTab(i18n->TranslateString("Length"), currentConfig->tab_width_length);
	joblist->AddTab(i18n->TranslateString("Size"), currentConfig->tab_width_size);

	joblist->Show();

	if (currentConfig->showTitleInfo)
	{
		info_text_artist->Hide();
		info_edit_artist->Hide();
		info_text_title->Hide();
		info_edit_title->Hide();
		info_text_album->Hide();
		info_edit_album->Hide();
		info_text_track->Hide();
		info_edit_track->Hide();
		info_text_year->Hide();
		info_edit_year->Hide();
		info_text_genre->Hide();
		info_edit_genre->Hide();
	}

	info_text_artist->SetText(i18n->TranslateString("Artist").Append(":"));
	info_text_album->SetText(i18n->TranslateString("Album").Append(":"));
	info_text_title->SetText(i18n->TranslateString("Title").Append(":"));
	info_text_track->SetText(i18n->TranslateString("Track").Append(":"));
	info_text_year->SetText(i18n->TranslateString("Year").Append(":"));
	info_text_genre->SetText(i18n->TranslateString("Genre").Append(":"));

	info_edit_title->SetMetrics(Point(currentConfig->wndSize.cx - 227 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx, info_edit_title->GetObjectProperties()->pos.y), Size(214 + info_text_genre->GetObjectProperties()->textSize.cx + info_text_year->GetObjectProperties()->textSize.cx, info_edit_title->GetObjectProperties()->size.cy));
	info_edit_track->SetMetrics(Point(currentConfig->wndSize.cx - 227 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx, info_edit_track->GetObjectProperties()->pos.y), info_edit_track->GetObjectProperties()->size);
	info_text_year->SetMetrics(Point(info_edit_track->GetObjectProperties()->pos.x + 32, info_text_year->GetObjectProperties()->pos.y), info_text_year->GetObjectProperties()->size);
	info_edit_year->SetMetrics(Point(info_text_year->GetObjectProperties()->pos.x + info_text_year->GetObjectProperties()->textSize.cx + 7, info_edit_year->GetObjectProperties()->pos.y), info_edit_year->GetObjectProperties()->size);
	info_text_genre->SetMetrics(Point(info_edit_year->GetObjectProperties()->pos.x + 38, info_text_genre->GetObjectProperties()->pos.y), info_text_genre->GetObjectProperties()->size);
	info_text_title->SetMetrics(Point(info_edit_title->GetObjectProperties()->pos.x - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx) - 7, info_text_title->GetObjectProperties()->pos.y), info_text_title->GetObjectProperties()->size);
	info_text_track->SetMetrics(Point(info_edit_title->GetObjectProperties()->pos.x - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx) - 7, info_text_track->GetObjectProperties()->pos.y), info_text_track->GetObjectProperties()->size);
	info_edit_artist->SetMetrics(Point((Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) + 15, info_edit_artist->GetObjectProperties()->pos.y), Size(currentConfig->wndSize.cx - 256 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx - (Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx), info_edit_artist->GetObjectProperties()->size.cy));
	info_edit_album->SetMetrics(Point((Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) + 15, info_edit_album->GetObjectProperties()->pos.y), Size(currentConfig->wndSize.cx - 256 - info_text_genre->GetObjectProperties()->textSize.cx - info_text_year->GetObjectProperties()->textSize.cx - (Int) Math::Max(info_text_artist->GetObjectProperties()->textSize.cx, info_text_album->GetObjectProperties()->textSize.cx) - (Int) Math::Max(info_text_title->GetObjectProperties()->textSize.cx, info_text_track->GetObjectProperties()->textSize.cx), info_edit_album->GetObjectProperties()->size.cy));
	info_edit_genre->SetMetrics(Point(currentConfig->wndSize.cx - 143, info_edit_genre->GetObjectProperties()->pos.y), info_edit_genre->GetObjectProperties()->size);

	if (currentConfig->showTitleInfo)
	{
		info_text_artist->Show();
		info_edit_artist->Show();
		info_text_title->Show();
		info_edit_title->Show();
		info_text_album->Show();
		info_edit_album->Show();
		info_text_track->Show();
		info_edit_track->Show();
		info_text_year->Show();
		info_edit_year->Show();
		info_text_genre->Show();
		info_edit_genre->Show();
	}

	mainWnd_menubar->Hide();
	mainWnd_iconbar->Hide();

	menu_file->Clear();
	menu_options->Clear();
	menu_addsubmenu->Clear();
	menu_encode->Clear();
	menu_database->Clear();
	menu_trackmenu->Clear();
	menu_help->Clear();
	mainWnd_menubar->Clear();
	mainWnd_iconbar->Clear();

	menu_file->AddEntry(i18n->TranslateString("Add"), NIL, menu_addsubmenu);
	menu_file->AddEntry(i18n->TranslateString("Remove"))->onClick.Connect(&bonkEncGUI::RemoveFile, this);
	menu_file->AddEntry();
	menu_file->AddEntry(i18n->TranslateString("Clear joblist"))->onClick.Connect(&bonkEnc::ClearList, (bonkEnc *) this);
	menu_file->AddEntry();
	menu_file->AddEntry(i18n->TranslateString("Exit"))->onClick.Connect(&bonkEncGUI::Close, this);

	menu_options->AddEntry(i18n->TranslateString("General settings..."))->onClick.Connect(&bonkEncGUI::ConfigureGeneral, this);
	menu_options->AddEntry(i18n->TranslateString("Configure selected encoder..."))->onClick.Connect(&bonkEncGUI::ConfigureEncoder, this);

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1)
	{
		menu_options->AddEntry();

		menu_options->AddEntry(i18n->TranslateString("Active CD-ROM drive"), NIL, menu_seldrive);
	}

	menu_addsubmenu->AddEntry(i18n->TranslateString("Audio file(s)..."))->onClick.Connect(&bonkEncGUI::AddFile, this);

	MenuEntry	*entry;

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1)
	{
		menu_addsubmenu->AddEntry(i18n->TranslateString("Audio CD contents"))->onClick.Connect(&bonkEnc::ReadCD, (bonkEnc *) this);

		if (currentConfig->cdrip_numdrives > 1)
		{
			menu_addsubmenu->AddEntry();
			menu_addsubmenu->AddEntry(i18n->TranslateString("Audio CD contents"), NIL, currentConfig->cdrip_numdrives > 1 ? menu_drives : NIL);
		}
	}

	menu_encode->AddEntry(i18n->TranslateString("Start encoding"))->onClick.Connect(&bonkEnc::Encode, (bonkEnc *) this);
	menu_encode->AddEntry(i18n->TranslateString("Stop encoding"))->onClick.Connect(&bonkEnc::StopEncoding, (bonkEnc *) this);

	menu_database->AddEntry(i18n->TranslateString("Query CDDB database"))->onClick.Connect(&bonkEncGUI::QueryCDDB, this);
	menu_database->AddEntry(i18n->TranslateString("Submit CDDB data..."))->onClick.Connect(&bonkEncGUI::SubmitCDDBData, this);
	menu_database->AddEntry();
	menu_database->AddEntry(i18n->TranslateString("Automatic CDDB queries"), NIL, NIL, &currentConfig->enable_auto_cddb);
	menu_database->AddEntry(i18n->TranslateString("Enable CDDB cache"), NIL, NIL, &currentConfig->enable_cddb_cache);

	if (winamp_out_modules.GetNOfEntries() > 0)
	{
		menu_trackmenu->AddEntry(i18n->TranslateString("Play"))->onClick.Connect(&bonkEnc::PlaySelectedItem, (bonkEnc *) this);
		menu_trackmenu->AddEntry(i18n->TranslateString("Stop"))->onClick.Connect(&bonkEnc::StopPlayback, (bonkEnc *) this);
		menu_trackmenu->AddEntry();
	}

	menu_trackmenu->AddEntry(i18n->TranslateString("Remove"))->onClick.Connect(&bonkEncGUI::RemoveFile, this);
	menu_trackmenu->AddEntry();
	menu_trackmenu->AddEntry(i18n->TranslateString("Clear joblist"))->onClick.Connect(&bonkEnc::ClearList, (bonkEnc *) this);

	menu_help->AddEntry(i18n->TranslateString("Help topics..."))->onClick.Connect(&bonkEncGUI::ShowHelp, this);
	menu_help->AddEntry();
	menu_help->AddEntry(i18n->TranslateString("About BonkEnc").Append("..."))->onClick.Connect(&bonkEncGUI::About, this);

	mainWnd_menubar->AddEntry(i18n->TranslateString("File"), NIL, menu_file);

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1) mainWnd_menubar->AddEntry(i18n->TranslateString("Database"), NIL, menu_database);

	mainWnd_menubar->AddEntry(i18n->TranslateString("Options"), NIL, menu_options);
	mainWnd_menubar->AddEntry(i18n->TranslateString("Encode"), NIL, menu_encode);
	mainWnd_menubar->AddEntry()->SetOrientation(OR_RIGHT);
	mainWnd_menubar->AddEntry(i18n->TranslateString("Help"), NIL, menu_help, NIL, NIL, 0, OR_RIGHT);

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 1, NIL));
	entry->onClick.Connect(&bonkEncGUI::AddFile, this);
	entry->SetStatusText(i18n->TranslateString("Add audio file(s) to the joblist"));

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1)
	{
		entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 2, NIL), currentConfig->cdrip_numdrives > 1 ? menu_drives : NIL);
		entry->onClick.Connect(&bonkEnc::ReadCD, (bonkEnc *) this);
		entry->SetStatusText(i18n->TranslateString("Add audio CD contents to the joblist"));
	}

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 3, NIL));
	entry->onClick.Connect(&bonkEncGUI::RemoveFile, this);
	entry->SetStatusText(i18n->TranslateString("Remove the selected entry from the joblist"));

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 4, NIL));
	entry->onClick.Connect(&bonkEnc::ClearList, (bonkEnc *) this);
	entry->SetStatusText(i18n->TranslateString("Clear the entire joblist"));

	if (currentConfig->enable_cdrip && currentConfig->cdrip_numdrives >= 1)
	{
		mainWnd_iconbar->AddEntry();

		entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 9, NIL));
		entry->onClick.Connect(&bonkEncGUI::QueryCDDB, this);
		entry->SetStatusText(i18n->TranslateString("Query CDDB database"));

		entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 10, NIL));
		entry->onClick.Connect(&bonkEncGUI::SubmitCDDBData, this);
		entry->SetStatusText(i18n->TranslateString("Submit CDDB data..."));
	}

	mainWnd_iconbar->AddEntry();

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 5, NIL));
	entry->onClick.Connect(&bonkEncGUI::ConfigureGeneral, this);
	entry->SetStatusText(i18n->TranslateString("Configure general settings"));

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 6, NIL));
	entry->onClick.Connect(&bonkEncGUI::ConfigureEncoder, this);
	entry->SetStatusText(i18n->TranslateString("Configure the selected audio encoder"));

	mainWnd_iconbar->AddEntry();

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 7, NIL));
	entry->onClick.Connect(&bonkEnc::Encode, (bonkEnc *) this);
	entry->SetStatusText(i18n->TranslateString("Start the encoding process"));

	entry = mainWnd_iconbar->AddEntry(NIL, SMOOTH::LoadImage("BonkEnc.pci", 8, NIL));
	entry->onClick.Connect(&bonkEnc::StopEncoding, (bonkEnc *) this);
	entry->SetStatusText(i18n->TranslateString("Stop encoding"));

	mainWnd_menubar->Show();
	mainWnd_iconbar->Show();

	hyperlink->Hide();
	hyperlink->Show();

	return true;
}

Menu *bonkEncGUI::GetTrackMenu(Int mouseX, Int mouseY)
{
	if (mouseX > mainWnd->GetMainLayer()->GetObjectProperties()->pos.x + joblist->GetObjectProperties()->pos.x + 1 &&
	    mouseX < mainWnd->GetMainLayer()->GetObjectProperties()->pos.x + joblist->GetObjectProperties()->pos.x + joblist->GetObjectProperties()->size.cx - 1 &&
	    mouseY > mainWnd->GetMainLayer()->GetObjectProperties()->pos.y + joblist->GetObjectProperties()->pos.y + 17 &&
	    mouseY < mainWnd->GetMainLayer()->GetObjectProperties()->pos.y + joblist->GetObjectProperties()->pos.y + joblist->GetObjectProperties()->size.cy - 1)
	{
		joblist->Process(SM_LBUTTONDOWN, 0, 0);

		if (joblist->GetSelectedEntry() != NIL) return menu_trackmenu;
	}

	return NIL;
}

Void bonkEncGUI::ShowHelp()
{
	ShellExecuteA(NIL, "open", GetApplicationDirectory().Append("manual/").Append(i18n->TranslateString("index.html")), NIL, NIL, 0);
}
