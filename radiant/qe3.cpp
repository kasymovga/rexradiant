/*
   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
   The following source code is licensed by Id Software and subject to the terms of
   its LIMITED USE SOFTWARE LICENSE AGREEMENT, a copy of which is included with
   GtkRadiant. If you did not receive a LIMITED USE SOFTWARE LICENSE AGREEMENT,
   please contact Id Software immediately at info@idsoftware.com.
 */

//
// Linux stuff
//
// Leonardo Zide (leo@lokigames.com)
//

#include "qe3.h"

#include "debugging/debugging.h"

#include "ifilesystem.h"
//#include "imap.h"

#include <map>

#include <gtk/gtk.h>

#include "stream/textfilestream.h"
#include "commandlib.h"
#include "stream/stringstream.h"
#include "os/path.h"
#include "scenelib.h"

#include "gtkutil/messagebox.h"
#include "error.h"
#include "map.h"
#include "build.h"
#include "points.h"
#include "camwindow.h"
#include "mainframe.h"
#include "preferences.h"
#include "watchbsp.h"
#include "autosave.h"
#include "convert.h"

QEGlobals_t g_qeglobals;


#if defined( WIN32 )
#define PATH_MAX 260
#endif

#if defined( POSIX )
#include <sys/stat.h> // chmod
#endif

#define RADIANT_MONITOR_ADDRESS "127.0.0.1:39000"


void QE_InitVFS(){
	// VFS initialization -----------------------
	// we will call GlobalFileSystem().initDirectory, giving the directories to look in (for files in pk3's and for standalone files)
	// we need to call in order, the mod ones first, then the base ones .. they will be searched in this order
	// *nix systems have a dual filesystem in ~/.q3a, which is searched first .. so we need to add that too

	const char* gamename = gamename_get();
	const char* basegame = basegame_get();
	const char* userRoot = g_qeglobals.m_userEnginePath.c_str();
	const char* globalRoot = EnginePath_get();

	const char* extrapath = ExtraResourcePath_get();
	if( !string_empty( extrapath ) )
		GlobalFileSystem().initDirectory( extrapath );

	StringOutputStream str( 256 );
	// if we have a mod dir
	if ( !string_equal( gamename, basegame ) ) {
		// ~/.<gameprefix>/<fs_game>
		if ( !string_equal( globalRoot, userRoot ) ) {
			GlobalFileSystem().initDirectory( str( userRoot, gamename, '/' ) ); // userGamePath
		}

		// <fs_basepath>/<fs_game>
		{
			GlobalFileSystem().initDirectory( str( globalRoot, gamename, '/' ) ); // globalGamePath
		}
	}

	// ~/.<gameprefix>/<fs_main>
	if ( !string_equal( globalRoot, userRoot ) ) {
		GlobalFileSystem().initDirectory( str( userRoot, basegame, '/' ) ); // userBasePath
	}

	// <fs_basepath>/<fs_main>
	{
		GlobalFileSystem().initDirectory( str( globalRoot, basegame, '/' ) ); // globalBasePath
	}
}


SimpleCounter g_brushCount;
SimpleCounter g_patchCount;
SimpleCounter g_entityCount;

void QE_brushCountChange(){
	std::size_t counts[3] = { g_brushCount.get(), g_patchCount.get(), g_entityCount.get() };
	if( GlobalSelectionSystem().countSelected() != 0 )
		GlobalSelectionSystem().countSelectedStuff( counts[0], counts[1], counts[2] );
	for ( int i = 0; i < 3; ++i ){
		char buffer[32];
		buffer[0] = '\0';
		if( counts[i] != 0 )
			sprintf( buffer, "%zu", counts[i] );
		g_pParentWnd->SetStatusText( c_status_brushcount + i, buffer );
	}
}

IdleDraw g_idle_scene_counts_update = IdleDraw( FreeCaller<QE_brushCountChange>() );
void QE_brushCountChanged(){
	g_idle_scene_counts_update.queueDraw();
}


bool ConfirmModified( const char* title ){
	if ( !Map_Modified( g_map ) ) {
		return true;
	}

	EMessageBoxReturn result = gtk_MessageBox( GTK_WIDGET( MainFrame_getWindow() ),
	                                           "The current map has changed since it was last saved.\nDo you want to save the current map before continuing?",
	                                           title, eMB_YESNOCANCEL, eMB_ICONQUESTION );
	if ( result == eIDCANCEL ) {
		return false;
	}
	if ( result == eIDYES ) {
		if ( Map_Unnamed( g_map ) ) {
			return Map_SaveAs();
		}
		else
		{
			return Map_Save();
		}
	}
	return true;
}

void bsp_init(){
	build_set_variable( "RadiantPath", AppPath_get() );
	build_set_variable( "ExecutableType", RADIANT_EXECUTABLE );
	build_set_variable( "EnginePath", EnginePath_get() );
	build_set_variable( "UserEnginePath", g_qeglobals.m_userEnginePath.c_str() );
	build_set_variable( "ExtraResoucePath", string_empty( ExtraResourcePath_get() )? ""
	                                       : StringOutputStream()( " -fs_pakpath ", makeQuoted( ExtraResourcePath_get() ) ) );
	build_set_variable( "MonitorAddress", ( g_WatchBSP_Enabled ) ? RADIANT_MONITOR_ADDRESS : "" );
	build_set_variable( "GameName", gamename_get() );

	const char* mapname = Map_Name( g_map );
	StringOutputStream stream( 256 );
	{
		build_set_variable( "BspFile", stream( PathExtensionless( mapname ), ".bsp" ) );
	}

	if( g_region_active ){
		build_set_variable( "MapFile", stream( PathExtensionless( mapname ), ".reg" ) );
	}
	else{
		build_set_variable( "MapFile", mapname );
	}

	{
		build_set_variable( "MapName", stream( PathFilename( mapname ) ) );
	}
}

void bsp_shutdown(){
	build_clear_variables();
}

class ArrayCommandListener : public CommandListener
{
	GPtrArray* m_array;
public:
	ArrayCommandListener(){
		m_array = g_ptr_array_new();
	}
	~ArrayCommandListener(){
		g_ptr_array_free( m_array, TRUE );
	}

	void execute( const char* command ){
		g_ptr_array_add( m_array, g_strdup( command ) );
	}

	GPtrArray* array() const {
		return m_array;
	}
};

class BatchCommandListener : public CommandListener
{
	TextOutputStream& m_file;
	std::size_t m_commandCount;
	const char* m_outputRedirect;
public:
	BatchCommandListener( TextOutputStream& file, const char* outputRedirect ) : m_file( file ), m_commandCount( 0 ), m_outputRedirect( outputRedirect ){
	}

	void execute( const char* command ){
		m_file << command;
		if( m_outputRedirect ){
			m_file << ( m_commandCount == 0? " > " : " >> " );
			m_file << "\"" << m_outputRedirect << "\"";
		}
		m_file << "\n";
		++m_commandCount;
	}
};


void RunBSP( const char* name ){
	if( !g_region_active )
		SaveMap();

	if ( Map_Unnamed( g_map ) ) {
		globalErrorStream() << "build cancelled: the map is unnamed\n";
		return;
	}

	if ( !g_region_active && g_SnapShots_Enabled && Map_Modified( g_map ) )
		Map_Snapshot();

	if ( g_region_active ) {
		const char* mapname = Map_Name( g_map );
		Map_SaveRegion( StringOutputStream( 256 )( PathExtensionless( mapname ), ".reg" ).c_str() );
	}

	Pointfile_Delete();

	bsp_init();

	ArrayCommandListener listener;
	build_run( name, listener );
	bool monitor = false;
	for ( guint i = 0; i < listener.array()->len; ++i )
		if( strstr( (char*)g_ptr_array_index( listener.array(), i ), RADIANT_MONITOR_ADDRESS ) )
			monitor = true;

	if ( g_WatchBSP_Enabled && monitor ) {
		// grab the file name for engine running
		const char* fullname = Map_Name( g_map );
		const auto bspname = StringOutputStream( 64 )( PathFilename( fullname ) );
		BuildMonitor_Run( listener.array(), bspname.c_str() );
	}
	else
	{
		char junkpath[PATH_MAX];
		strcpy( junkpath, SettingsPath_get() );
		strcat( junkpath, "junk.txt" );

		char batpath[PATH_MAX];
#if defined( POSIX )
		strcpy( batpath, SettingsPath_get() );
		strcat( batpath, "qe3bsp.sh" );
#elif defined( WIN32 )
		strcpy( batpath, SettingsPath_get() );
		strcat( batpath, "qe3bsp.bat" );
#else
#error "unsupported platform"
#endif
		bool written = false;
		{
			TextFileOutputStream batchFile( batpath );
			if ( !batchFile.failed() ) {
#if defined ( POSIX )
				batchFile << "#!/bin/sh \n\n";
#endif
				BatchCommandListener listener( batchFile, g_WatchBSP0_DumpLog? junkpath : 0 );
				build_run( name, listener );
				written = true;
			}
		}
		if ( written ) {
#if defined ( POSIX )
			chmod( batpath, 0744 );
#endif
			globalOutputStream() << "Writing the compile script to '" << batpath << "'\n";
			if( g_WatchBSP0_DumpLog )
				globalOutputStream() << "The build output will be saved in '" << junkpath << "'\n";
			Q_Exec( batpath, NULL, NULL, true, false );
		}
	}

	bsp_shutdown();
}

// =============================================================================
// Sys_ functions

void Sys_SetTitle( const char *text, bool modified ){
	StringOutputStream title;
	title << text;

	if ( modified ) {
		title << " *";
	}

	gtk_window_set_title( MainFrame_getWindow(), title.c_str() );
}

bool g_bWaitCursor = false;

void Sys_BeginWait(){
	ScreenUpdates_Disable( "Processing...", "Please Wait" );
	GdkCursor *cursor = gdk_cursor_new( GDK_WATCH );
	gdk_window_set_cursor( gtk_widget_get_window( GTK_WIDGET( MainFrame_getWindow() ) ), cursor );
	gdk_cursor_unref( cursor );
	g_bWaitCursor = true;
}

void Sys_EndWait(){
	ScreenUpdates_Enable();
	gdk_window_set_cursor( gtk_widget_get_window( GTK_WIDGET( MainFrame_getWindow() ) ), 0 );
	g_bWaitCursor = false;
}

void Sys_Beep(){
	gdk_beep();
}
