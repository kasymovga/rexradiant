/*
   Copyright (c) 2001, Loki software, inc.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice, this list
   of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of Loki software nor the names of its contributors may be used
   to endorse or promote products derived from this software without specific prior
   written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
   DIRECT,INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//
// Some small dialogs that don't need much
//
// Leonardo Zide (leo@lokigames.com)
//

#include "gtkdlgs.h"

#include "debugging/debugging.h"
#include "version.h"
#include "aboutmsg.h"

#include "igl.h"
#include "iscenegraph.h"
#include "iselection.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktable.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktextview.h>
#include <gtk/gtktextbuffer.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkspinbutton.h>

#include "os/path.h"
#include "math/aabb.h"
#include "container/array.h"
#include "generic/static.h"
#include "stream/stringstream.h"
#include "convert.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/image.h"

#include "gtkmisc.h"
#include "brushmanip.h"
#include "build.h"
#include "qe3.h"
#include "texwindow.h"
#include "xywindow.h"
#include "mainframe.h"
#include "preferences.h"
#include "url.h"
#include "cmdlib.h"

#include "qerplugin.h"
#include "os/file.h"



// =============================================================================
// Project settings dialog

class GameComboConfiguration
{
public:
const char* basegame_dir;
const char* basegame;
const char* known_dir;
const char* known;
const char* custom;

GameComboConfiguration() :
	basegame_dir( g_pGameDescription->getRequiredKeyValue( "basegame" ) ),
	basegame( g_pGameDescription->getRequiredKeyValue( "basegamename" ) ),
	known_dir( g_pGameDescription->getKeyValue( "knowngame" ) ),
	known( g_pGameDescription->getKeyValue( "knowngamename" ) ),
	custom( g_pGameDescription->getRequiredKeyValue( "unknowngamename" ) ){
}
};

typedef LazyStatic<GameComboConfiguration> LazyStaticGameComboConfiguration;

inline GameComboConfiguration& globalGameComboConfiguration(){
	return LazyStaticGameComboConfiguration::instance();
}


struct gamecombo_t
{
	gamecombo_t( int _game, const char* _fs_game, bool _sensitive )
		: game( _game ), fs_game( _fs_game ), sensitive( _sensitive )
	{}
	int game;
	const char* fs_game;
	bool sensitive;
};

gamecombo_t gamecombo_for_dir( const char* dir ){
	if ( string_equal( dir, globalGameComboConfiguration().basegame_dir ) ) {
		return gamecombo_t( 0, "", false );
	}
	else if ( string_equal( dir, globalGameComboConfiguration().known_dir ) ) {
		return gamecombo_t( 1, dir, false );
	}
	else
	{
		return gamecombo_t( string_empty( globalGameComboConfiguration().known_dir ) ? 1 : 2, dir, true );
	}
}

gamecombo_t gamecombo_for_gamename( const char* gamename ){
	if ( ( strlen( gamename ) == 0 ) || !strcmp( gamename, globalGameComboConfiguration().basegame ) ) {
		return gamecombo_t( 0, "", false );
	}
	else if ( !strcmp( gamename, globalGameComboConfiguration().known ) ) {
		return gamecombo_t( 1, globalGameComboConfiguration().known_dir, false );
	}
	else
	{
		return gamecombo_t( string_empty( globalGameComboConfiguration().known_dir ) ? 1 : 2, "", true );
	}
}

inline void path_copy_clean( char* destination, const char* source ){
	char* i = destination;

	while ( *source != '\0' )
	{
		*i++ = ( *source == '\\' ) ? '/' : *source;
		++source;
	}

	if ( i != destination && *( i - 1 ) != '/' ) {
		*( i++ ) = '/';
	}

	*i = '\0';
}


struct GameCombo
{
	GtkComboBox* game_select;
	GtkEntry* fsgame_entry;
};

gboolean OnSelchangeComboWhatgame( GtkWidget *widget, GameCombo* combo ){
	const char *gamename;
	{
		GtkTreeIter iter;
		gtk_combo_box_get_active_iter( combo->game_select, &iter );
		gtk_tree_model_get( gtk_combo_box_get_model( combo->game_select ), &iter, 0, (gpointer*)&gamename, -1 );
	}

	gamecombo_t gamecombo = gamecombo_for_gamename( gamename );

	gtk_entry_set_text( combo->fsgame_entry, gamecombo.fs_game );
	gtk_widget_set_sensitive( GTK_WIDGET( combo->fsgame_entry ), gamecombo.sensitive );

	return FALSE;
}

class MappingMode
{
public:
bool do_mapping_mode;
const char* sp_mapping_mode;
const char* mp_mapping_mode;

MappingMode() :
	do_mapping_mode( !string_empty( g_pGameDescription->getKeyValue( "show_gamemode" ) ) ),
	sp_mapping_mode( "Single Player mapping mode" ),
	mp_mapping_mode( "Multiplayer mapping mode" ){
}
};

typedef LazyStatic<MappingMode> LazyStaticMappingMode;

inline MappingMode& globalMappingMode(){
	return LazyStaticMappingMode::instance();
}

class ProjectSettingsDialog
{
public:
GameCombo game_combo;
GtkComboBox* gamemode_combo;
};

GtkWindow* ProjectSettingsDialog_construct( ProjectSettingsDialog& dialog, ModalDialog& modal ){
	GtkWindow* window = create_dialog_window( MainFrame_getWindow(), "Project Settings", G_CALLBACK( dialog_delete_callback ), &modal );

	{
		GtkTable* table1 = create_dialog_table( 1, 2, 4, 4, 4 );
		gtk_container_add( GTK_CONTAINER( window ), GTK_WIDGET( table1 ) );
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_table_attach( table1, GTK_WIDGET( vbox ), 1, 2, 0, 1,
							  (GtkAttachOptions) ( GTK_FILL ),
							  (GtkAttachOptions) ( GTK_FILL ), 0, 0 );
			{
				GtkButton* button = create_dialog_button( "OK", G_CALLBACK( dialog_button_ok ), &modal );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
			}
			{
				GtkButton* button = create_dialog_button( "Cancel", G_CALLBACK( dialog_button_cancel ), &modal );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
			}
		}
		{
			GtkFrame* frame = create_dialog_frame( "Project settings" );
			gtk_table_attach( table1, GTK_WIDGET( frame ), 0, 1, 0, 1,
							  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
							  (GtkAttachOptions) ( GTK_FILL ), 0, 0 );
			{
				GtkTable* table2 = create_dialog_table( ( globalMappingMode().do_mapping_mode ) ? 4 : 3, 2, 4, 4, 4 );
				gtk_container_add( GTK_CONTAINER( frame ), GTK_WIDGET( table2 ) );

				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( "Select mod" ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table2, GTK_WIDGET( label ), 0, 1, 0, 1,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
				}
				{
					dialog.game_combo.game_select = GTK_COMBO_BOX( gtk_combo_box_new_text() );

					gtk_combo_box_append_text( dialog.game_combo.game_select, globalGameComboConfiguration().basegame );
					if ( globalGameComboConfiguration().known[0] != '\0' ) {
						gtk_combo_box_append_text( dialog.game_combo.game_select, globalGameComboConfiguration().known );
					}
					gtk_combo_box_append_text( dialog.game_combo.game_select, globalGameComboConfiguration().custom );

					gtk_widget_show( GTK_WIDGET( dialog.game_combo.game_select ) );
					gtk_table_attach( table2, GTK_WIDGET( dialog.game_combo.game_select ), 1, 2, 0, 1,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					g_signal_connect( G_OBJECT( dialog.game_combo.game_select ), "changed", G_CALLBACK( OnSelchangeComboWhatgame ), &dialog.game_combo );
				}

				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( "fs_game" ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table2, GTK_WIDGET( label ), 0, 1, 1, 2,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
				}
				{
					GtkEntry* entry = GTK_ENTRY( gtk_entry_new() );
					gtk_widget_show( GTK_WIDGET( entry ) );
					gtk_table_attach( table2, GTK_WIDGET( entry ), 1, 2, 1, 2,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					dialog.game_combo.fsgame_entry = entry;
				}

				if ( globalMappingMode().do_mapping_mode ) {
					GtkLabel* label = GTK_LABEL( gtk_label_new( "Mapping mode" ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table2, GTK_WIDGET( label ), 0, 1, 3, 4,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );

					GtkComboBox* combo = GTK_COMBO_BOX( gtk_combo_box_new_text() );
					gtk_combo_box_append_text( combo, globalMappingMode().sp_mapping_mode );
					gtk_combo_box_append_text( combo, globalMappingMode().mp_mapping_mode );

					gtk_widget_show( GTK_WIDGET( combo ) );
					gtk_table_attach( table2, GTK_WIDGET( combo ), 1, 2, 3, 4,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					dialog.gamemode_combo = combo;
				}
			}
		}
	}

	// initialise the fs_game selection from the project settings into the dialog
	const char* dir = gamename_get();
	gamecombo_t gamecombo = gamecombo_for_dir( dir );

	gtk_combo_box_set_active( dialog.game_combo.game_select, gamecombo.game );
	gtk_entry_set_text( dialog.game_combo.fsgame_entry, gamecombo.fs_game );
	gtk_widget_set_sensitive( GTK_WIDGET( dialog.game_combo.fsgame_entry ), gamecombo.sensitive );

	if ( globalMappingMode().do_mapping_mode ) {
		const char *gamemode = gamemode_get();
		if ( string_empty( gamemode ) || string_equal( gamemode, "sp" ) ) {
			gtk_combo_box_set_active( dialog.gamemode_combo, 0 );
		}
		else
		{
			gtk_combo_box_set_active( dialog.gamemode_combo, 1 );
		}
	}

	return window;
}

void ProjectSettingsDialog_ok( ProjectSettingsDialog& dialog ){
	const char* dir = gtk_entry_get_text( dialog.game_combo.fsgame_entry );

	const char* new_gamename = path_equal( dir, globalGameComboConfiguration().basegame_dir )
							   ? ""
							   : dir;

	if ( !path_equal( new_gamename, gamename_get() ) ) {
		if ( ConfirmModified( "Edit Project Settings" ) ) {
			ScopeDisableScreenUpdates disableScreenUpdates( "Processing...", "Changing Game Name" );

			EnginePath_Unrealise();

			gamename_set( new_gamename );

			EnginePath_Realise();
		}
	}

	if ( globalMappingMode().do_mapping_mode ) {
		// read from gamemode_combo
		int active = gtk_combo_box_get_active( dialog.gamemode_combo );
		if ( active == -1 || active == 0 ) {
			gamemode_set( "sp" );
		}
		else
		{
			gamemode_set( "mp" );
		}
	}
}

void DoProjectSettings(){
	//if ( ConfirmModified( "Edit Project Settings" ) ) {
		ModalDialog modal;
		ProjectSettingsDialog dialog;

		GtkWindow* window = ProjectSettingsDialog_construct( dialog, modal );

		if ( modal_dialog_show( window, modal ) == eIDOK ) {
			ProjectSettingsDialog_ok( dialog );
		}

		gtk_widget_destroy( GTK_WIDGET( window ) );
	//}
}

// =============================================================================
// Arbitrary Sides dialog

void DoSides( int type, int axis ){
	ModalDialog dialog;
	//GtkEntry* sides_entry;
	GtkWidget* sides_spin;

	GtkWindow* window = create_dialog_window( MainFrame_getWindow(), "Arbitrary sides", G_CALLBACK( dialog_delete_callback ), &dialog );

	GtkAccelGroup* accel = gtk_accel_group_new();
	gtk_window_add_accel_group( window, accel );

	{
		GtkHBox* hbox = create_dialog_hbox( 4, 4 );
		gtk_container_add( GTK_CONTAINER( window ), GTK_WIDGET( hbox ) );
		{
			GtkLabel* label = GTK_LABEL( gtk_label_new( "Sides:" ) );
			gtk_widget_show( GTK_WIDGET( label ) );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
		}
//		{
//			GtkEntry* entry = GTK_ENTRY( gtk_entry_new() );
//			gtk_widget_show( GTK_WIDGET( entry ) );
//			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( entry ), FALSE, FALSE, 0 );
//			sides_entry = entry;
//			gtk_widget_grab_focus( GTK_WIDGET( entry ) );
//		}
		{
			GtkAdjustment* adj;
			EBrushPrefab BrushPrefabType = (EBrushPrefab)type;
			switch ( BrushPrefabType )
			{
			case eBrushPrism :
			case eBrushCone :
				adj = GTK_ADJUSTMENT( gtk_adjustment_new( 8, 3, 1022, 1, 10, 0 ) );
				break;
			case eBrushSphere :
				adj = GTK_ADJUSTMENT( gtk_adjustment_new( 8, 3, 31, 1, 10, 0 ) );
				break;
			case eBrushRock :
				adj = GTK_ADJUSTMENT( gtk_adjustment_new( 32, 10, 1000, 1, 10, 0 ) );
				break;
			case eBrushIcosahedron :
				adj = GTK_ADJUSTMENT( gtk_adjustment_new( 1, 0, 2, 1, 10, 0 ) ); //possible with 3, but buggy
				break;
			default:
				adj = GTK_ADJUSTMENT( gtk_adjustment_new( 8, 3, 31, 1, 10, 0 ) );
				break;
			}

			GtkWidget* spin = gtk_spin_button_new( adj, 1, 0 );
			gtk_widget_show( spin );
			gtk_box_pack_start( GTK_BOX( hbox ), spin, FALSE, FALSE, 0 );
			gtk_widget_set_size_request( spin, 64, -1 );
			gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( spin ), TRUE );

			sides_spin = spin;
		}
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );
			{
				GtkButton* button = create_dialog_button( "OK", G_CALLBACK( dialog_button_ok ), &dialog );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( GTK_WIDGET( button ) );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_Return, (GdkModifierType)0, (GtkAccelFlags)0 );
			}
			{
				GtkButton* button = create_dialog_button( "Cancel", G_CALLBACK( dialog_button_cancel ), &dialog );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_Escape, (GdkModifierType)0, (GtkAccelFlags)0 );
			}
		}
	}

	if ( modal_dialog_show( window, dialog ) == eIDOK ) {
//		const char *str = gtk_entry_get_text( sides_entry );

//		Scene_BrushConstructPrefab( GlobalSceneGraph(), (EBrushPrefab)type, atoi( str ), TextureBrowser_GetSelectedShader() );
		gtk_spin_button_update ( GTK_SPIN_BUTTON( sides_spin ) );
		int sides = static_cast<int>( gtk_spin_button_get_value( GTK_SPIN_BUTTON( sides_spin ) ) );
		Scene_BrushConstructPrefab( GlobalSceneGraph(), (EBrushPrefab)type, sides, TextureBrowser_GetSelectedShader() );
	}

	gtk_widget_destroy( GTK_WIDGET( window ) );
}

// =============================================================================
// About dialog (no program is complete without one)

void about_button_changelog( GtkWidget *widget, gpointer data ){
	StringOutputStream log( 256 );
	log << AppPath_get() << "changelog.txt";
	OpenURL( log.c_str() );
}

void about_button_credits( GtkWidget *widget, gpointer data ){
	StringOutputStream cred( 256 );
	cred << AppPath_get() << "credits.html";
	OpenURL( cred.c_str() );
}

void DoAbout(){
	ModalDialog dialog;
	ModalDialogButton ok_button( dialog, eIDOK );

	GtkWindow* window = create_modal_dialog_window( MainFrame_getWindow(), "About NetRadiant", dialog );

	{
		GtkVBox* vbox = create_dialog_vbox( 4, 4 );
		gtk_container_add( GTK_CONTAINER( window ), GTK_WIDGET( vbox ) );

		{
			GtkHBox* hbox = create_dialog_hbox( 4 );
			gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox ), FALSE, TRUE, 0 );

			{
				GtkVBox* vbox2 = create_dialog_vbox( 4 );
				gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox2 ), TRUE, FALSE, 0 );
				{
					//GtkFrame* frame = create_dialog_frame( 0, GTK_SHADOW_IN );
					//gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( frame ), FALSE, FALSE, 0 );
					{
						GtkImage* image = new_local_image( "logo.png" );
						gtk_widget_show( GTK_WIDGET( image ) );
						gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( image ), FALSE, FALSE, 0 );
						//gtk_container_add( GTK_CONTAINER( frame ), GTK_WIDGET( image ) );
					}
				}
			}

			{
				GtkLabel* label = GTK_LABEL( gtk_label_new( "NetRadiant " RADIANT_VERSION "\n"
															__DATE__ "\n\n"
															RADIANT_ABOUTMSG "\n\n"
																			 "By alientrap.org\n\n"
																			 "This program is free software\n"
																			 "licensed under the GNU GPL.\n"
															) );

				gtk_widget_show( GTK_WIDGET( label ) );
				gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
				gtk_misc_set_alignment( GTK_MISC( label ), 1, 0.5 );
				gtk_label_set_justify( label, GTK_JUSTIFY_LEFT );
			}

			{
				GtkVBox* vbox2 = create_dialog_vbox( 4 );
				gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox2 ), FALSE, TRUE, 0 );
				{
					GtkButton* button = create_modal_dialog_button( "OK", ok_button );
					gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				}
				{
					GtkButton* button = create_dialog_button( "Credits", G_CALLBACK( about_button_credits ), 0 );
					gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
					gtk_widget_set_sensitive( GTK_WIDGET( button ), FALSE);
				}
				{
					GtkButton* button = create_dialog_button( "Changelog", G_CALLBACK( about_button_changelog ), 0 );
					gtk_box_pack_start( GTK_BOX( vbox2 ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
					gtk_widget_set_sensitive( GTK_WIDGET( button ), FALSE);
				}
			}
		}
		{
			GtkFrame* frame = create_dialog_frame( "OpenGL Properties" );
			gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( frame ), FALSE, FALSE, 0 );
			{
				GtkTable* table = create_dialog_table( 3, 2, 4, 4, 4 );
				gtk_container_add( GTK_CONTAINER( frame ), GTK_WIDGET( table ) );
				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( "Vendor:" ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 0, 1,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( "Version:" ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 1, 2,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( "Renderer:" ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 2, 3,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( reinterpret_cast<const char*>( glGetString( GL_VENDOR ) ) ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table, GTK_WIDGET( label ), 1, 2, 0, 1,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( reinterpret_cast<const char*>( glGetString( GL_VERSION ) ) ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table, GTK_WIDGET( label ), 1, 2, 1, 2,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( reinterpret_cast<const char*>( glGetString( GL_RENDERER ) ) ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table, GTK_WIDGET( label ), 1, 2, 2, 3,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
			}
			{
				GtkFrame* frame = create_dialog_frame( "OpenGL Extensions" );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( frame ), TRUE, TRUE, 0 );
				{
					GtkScrolledWindow* sc_extensions = create_scrolled_window( GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS, 4 );
					gtk_container_add( GTK_CONTAINER( frame ), GTK_WIDGET( sc_extensions ) );
					{
						GtkWidget* text_extensions = gtk_text_view_new();
						gtk_text_view_set_editable( GTK_TEXT_VIEW( text_extensions ), FALSE );
						gtk_container_add( GTK_CONTAINER( sc_extensions ), text_extensions );
						GtkTextBuffer* buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( text_extensions ) );
						gtk_text_buffer_set_text( buffer, reinterpret_cast<const char*>( glGetString( GL_EXTENSIONS ) ), -1 );
						gtk_text_view_set_wrap_mode( GTK_TEXT_VIEW( text_extensions ), GTK_WRAP_WORD );
						gtk_widget_show( text_extensions );
					}
				}
			}
		}
	}

	modal_dialog_show( window, dialog );

	gtk_widget_destroy( GTK_WIDGET( window ) );
}

// =============================================================================
// TextureLayout dialog

// Last used texture scale values
static float last_used_texture_layout_scale_x = 4.0;
static float last_used_texture_layout_scale_y = 4.0;

EMessageBoxReturn DoTextureLayout( float *fx, float *fy ){
	ModalDialog dialog;
	ModalDialogButton ok_button( dialog, eIDOK );
	ModalDialogButton cancel_button( dialog, eIDCANCEL );
	GtkEntry* x;
	GtkEntry* y;

	GtkWindow* window = create_modal_dialog_window( MainFrame_getWindow(), "Patch texture layout", dialog );

	GtkAccelGroup* accel = gtk_accel_group_new();
	gtk_window_add_accel_group( window, accel );

	{
		GtkHBox* hbox = create_dialog_hbox( 4, 4 );
		gtk_container_add( GTK_CONTAINER( window ), GTK_WIDGET( hbox ) );
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );
			{
				GtkLabel* label = GTK_LABEL( gtk_label_new( "Texture will be fit across the patch based\n"
															"on the x and y values given. Values of 1x1\n"
															"will \"fit\" the texture. 2x2 will repeat\n"
															"it twice, etc." ) );
				gtk_widget_show( GTK_WIDGET( label ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), TRUE, TRUE, 0 );
				gtk_label_set_justify( label, GTK_JUSTIFY_LEFT );
			}
			{
				GtkTable* table = create_dialog_table( 2, 2, 4, 4 );
				gtk_widget_show( GTK_WIDGET( table ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( table ), TRUE, TRUE, 0 );
				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( "Texture x:" ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 0, 1,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					GtkLabel* label = GTK_LABEL( gtk_label_new( "Texture y:" ) );
					gtk_widget_show( GTK_WIDGET( label ) );
					gtk_table_attach( table, GTK_WIDGET( label ), 0, 1, 1, 2,
									  (GtkAttachOptions) ( GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );
					gtk_misc_set_alignment( GTK_MISC( label ), 0, 0.5 );
				}
				{
					GtkEntry* entry = GTK_ENTRY( gtk_entry_new() );
					gtk_widget_show( GTK_WIDGET( entry ) );
					gtk_table_attach( table, GTK_WIDGET( entry ), 1, 2, 0, 1,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					x = entry;
				}
				{
					GtkEntry* entry = GTK_ENTRY( gtk_entry_new() );
					gtk_widget_show( GTK_WIDGET( entry ) );
					gtk_table_attach( table, GTK_WIDGET( entry ), 1, 2, 1, 2,
									  (GtkAttachOptions) ( GTK_EXPAND | GTK_FILL ),
									  (GtkAttachOptions) ( 0 ), 0, 0 );

					y = entry;
				}
			}
		}
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), FALSE, FALSE, 0 );
			{
				GtkButton* button = create_modal_dialog_button( "OK", ok_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( GTK_WIDGET( button ) );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_Return, (GdkModifierType)0, (GtkAccelFlags)0 );
			}
			{
				GtkButton* button = create_modal_dialog_button( "Cancel", cancel_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_Escape, (GdkModifierType)0, (GtkAccelFlags)0 );
			}
		}
	}

	// Initialize with last used values
	char buf[16];

	sprintf( buf, "%f", last_used_texture_layout_scale_x );
	gtk_entry_set_text( x, buf );

	sprintf( buf, "%f", last_used_texture_layout_scale_y );
	gtk_entry_set_text( y, buf );

	// Set focus after intializing the values
	gtk_widget_grab_focus( GTK_WIDGET( x ) );

	EMessageBoxReturn ret = modal_dialog_show( window, dialog );
	if ( ret == eIDOK ) {
		*fx = static_cast<float>( atof( gtk_entry_get_text( x ) ) );
		*fy = static_cast<float>( atof( gtk_entry_get_text( y ) ) );

		// Remember last used values
		last_used_texture_layout_scale_x = *fx;
		last_used_texture_layout_scale_y = *fy;
	}

	gtk_widget_destroy( GTK_WIDGET( window ) );

	return ret;
}


class TextEditor
{
	GtkWidget *m_window = 0;
	GtkWidget *m_textView; // slave, text widget from the gtk editor
	GtkTextBuffer* m_textBuffer;
	GtkWidget *m_button; // save button
	CopiedString m_filename;

	void construct(){
		GtkWidget *vbox, *hbox, *scr;

		m_window = GTK_WIDGET( create_dialog_window( MainFrame_getWindow(), "", G_CALLBACK( gtk_widget_hide_on_delete ), 0, 400, 600 ) );

		vbox = gtk_vbox_new( FALSE, 5 );
		gtk_widget_show( vbox );
		gtk_container_add( GTK_CONTAINER( m_window ), GTK_WIDGET( vbox ) );
		gtk_container_set_border_width( GTK_CONTAINER( vbox ), 5 );

		scr = gtk_scrolled_window_new( 0, 0 );
		gtk_widget_show( scr );
		gtk_box_pack_start( GTK_BOX( vbox ), scr, TRUE, TRUE, 0 );
		gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scr ), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
		gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW( scr ), GTK_SHADOW_IN );

		m_textView = gtk_text_view_new();
		gtk_container_add( GTK_CONTAINER( scr ), m_textView );
		gtk_widget_show( m_textView );

		m_textBuffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( m_textView ) );
		g_signal_connect( G_OBJECT( m_textBuffer ), "modified-changed",
						G_CALLBACK( modified_changed ), this );

		hbox = gtk_hbox_new( FALSE, 5 );
		gtk_widget_show( hbox );
		gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, TRUE, 0 );

		m_button = gtk_button_new_with_label( "Close" );
		gtk_widget_show( m_button );
		gtk_box_pack_end( GTK_BOX( hbox ), m_button, FALSE, FALSE, 0 );
		g_signal_connect( G_OBJECT( m_button ), "clicked",
						G_CALLBACK( editor_close ), this );
		gtk_widget_set_usize( m_button, 60, -2 );

		m_button = gtk_button_new_with_label( "Save" );
		gtk_widget_show( m_button );
		gtk_box_pack_end( GTK_BOX( hbox ), m_button, FALSE, FALSE, 0 );
		g_signal_connect( G_OBJECT( m_button ), "clicked",
						G_CALLBACK( editor_save ), this );
		gtk_widget_set_usize( m_button, 60, -2 );
	}
	static void editor_close( GtkWidget *widget, TextEditor* self ){
		gtk_widget_hide( self->m_window );
	}
	static void editor_save( GtkWidget *widget, TextEditor* self ){
		FILE *f = fopen( self->m_filename.c_str(), "wb" ); //write in binary mode to preserve line feeds

		if ( f == 0 ) {
			gtk_MessageBox( self->m_window, "Error saving file !" );
			return;
		}

		/* Obtain iters for the start and end of points of the buffer */
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds( self->m_textBuffer, &start, &end );

		/* Get the entire buffer text. */
		char *str = gtk_text_buffer_get_text( self->m_textBuffer, &start, &end, FALSE );

		fwrite( str, 1, strlen( str ), f );
		fclose( f );
		g_free ( str );

		gtk_text_buffer_set_modified( self->m_textBuffer, FALSE );
	}
	static void modified_changed( GtkTextBuffer *textbuffer, TextEditor* self ){
		const gboolean modified = gtk_text_buffer_get_modified( textbuffer );
		gtk_widget_set_sensitive( self->m_button, modified );

		StringOutputStream str( 256 );
		str << ( modified? "*" : "" ) << self->m_filename.c_str();
		gtk_window_set_title( GTK_WINDOW( self->m_window ), str.c_str() );
	}

public:
	void DoGtkTextEditor( const char* text, std::size_t size, std::size_t offset, const char* filename, const bool editable ){
		if ( !m_window ) {
			construct(); // build it the first time we need it
		}

		m_filename = filename;

		gtk_text_buffer_set_text( m_textBuffer, text, size );
		gtk_text_buffer_set_modified( m_textBuffer, FALSE );

		gtk_text_view_set_editable( GTK_TEXT_VIEW( m_textView ), editable );

		// trying to show later
		gtk_widget_show( m_window );
		gtk_window_present( GTK_WINDOW( m_window ) );

	//#ifdef WIN32
		process_gui();
	//#endif

		{
			GtkTextIter text_iter;
			gtk_text_buffer_get_iter_at_offset( m_textBuffer, &text_iter, offset );
			gtk_text_buffer_place_cursor( m_textBuffer, &text_iter );
			gtk_text_view_scroll_to_iter( GTK_TEXT_VIEW( m_textView ), &text_iter, 0, TRUE, 0, 0);
		}

	//#ifdef WIN32
		gtk_widget_queue_draw( m_textView );
	//#endif
	}
};

static TextEditor g_textEditor;


// =============================================================================
// Light Intensity dialog
#include <gtk/gtkcheckbutton.h>

bool g_dontDoLightIntensityDlg = false;

void dontDoLightIntensityDlg_toggled( GtkToggleButton* togglebutton, gpointer user_data ){
	g_dontDoLightIntensityDlg = gtk_toggle_button_get_active( togglebutton );
}

EMessageBoxReturn DoLightIntensityDlg( int *intensity ){
	if( g_dontDoLightIntensityDlg ){
		return eIDOK;
	}
	ModalDialog dialog;
	GtkEntry* intensity_entry;
	ModalDialogButton ok_button( dialog, eIDOK );
	ModalDialogButton cancel_button( dialog, eIDCANCEL );

	GtkWindow* window = create_modal_dialog_window( MainFrame_getWindow(), "Light intensity", dialog, -1, -1 );

	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group( window, accel_group );

	{
		GtkHBox* hbox = create_dialog_hbox( 4, 4 );
		gtk_container_add( GTK_CONTAINER( window ), GTK_WIDGET( hbox ) );
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );
			{
				GtkLabel* label = GTK_LABEL( gtk_label_new( "ESC for default, ENTER to validate" ) );
				gtk_widget_show( GTK_WIDGET( label ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				GtkEntry* entry = GTK_ENTRY( gtk_entry_new() );
				gtk_widget_show( GTK_WIDGET( entry ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( entry ), TRUE, TRUE, 0 );

				gtk_widget_grab_focus( GTK_WIDGET( entry ) );

				intensity_entry = entry;
			}
			{
				GtkWidget* check = gtk_check_button_new_with_label( "Don't Show" );
				gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check ), FALSE );
				gtk_widget_show( check );
				gtk_box_pack_start( GTK_BOX( vbox ), check, FALSE, FALSE, 0 );
				g_signal_connect( G_OBJECT( check ), "toggled", G_CALLBACK( dontDoLightIntensityDlg_toggled ), 0 );
			}
		}
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), FALSE, FALSE, 0 );

			{
				GtkButton* button = create_modal_dialog_button( "OK", ok_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( GTK_WIDGET( button ) );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_Return, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
			{
				GtkButton* button = create_modal_dialog_button( "Cancel", cancel_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_Escape, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
		}
	}

	char buf[16];
	sprintf( buf, "%d", *intensity );
	gtk_entry_set_text( intensity_entry, buf );

	EMessageBoxReturn ret = modal_dialog_show( window, dialog );
	if ( ret == eIDOK ) {
		*intensity = atoi( gtk_entry_get_text( intensity_entry ) );
	}

	gtk_widget_destroy( GTK_WIDGET( window ) );

	return ret;
}

// =============================================================================
// Add new shader tag dialog

EMessageBoxReturn DoShaderTagDlg( CopiedString* tag, const char* title ){
	ModalDialog dialog;
	GtkEntry* textentry;
	ModalDialogButton ok_button( dialog, eIDOK );
	ModalDialogButton cancel_button( dialog, eIDCANCEL );

	GtkWindow* window = create_modal_dialog_window( MainFrame_getWindow(), title, dialog, -1, -1 );

	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group( window, accel_group );

	{
		GtkHBox* hbox = create_dialog_hbox( 4, 4 );
		gtk_container_add( GTK_CONTAINER( window ), GTK_WIDGET( hbox ) );
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );
			{
				//GtkLabel* label = GTK_LABEL(gtk_label_new("Enter one ore more tags separated by spaces"));
				GtkLabel* label = GTK_LABEL( gtk_label_new( "ESC to cancel, ENTER to validate" ) );
				gtk_widget_show( GTK_WIDGET( label ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				GtkEntry* entry = GTK_ENTRY( gtk_entry_new() );
				gtk_widget_show( GTK_WIDGET( entry ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( entry ), TRUE, TRUE, 0 );

				gtk_widget_grab_focus( GTK_WIDGET( entry ) );

				textentry = entry;
			}
		}
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), FALSE, FALSE, 0 );

			{
				GtkButton* button = create_modal_dialog_button( "OK", ok_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( GTK_WIDGET( button ) );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_Return, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
			{
				GtkButton* button = create_modal_dialog_button( "Cancel", cancel_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_Escape, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
		}
	}

	EMessageBoxReturn ret = modal_dialog_show( window, dialog );
	if ( ret == eIDOK ) {
		*tag = gtk_entry_get_text( textentry );
	}

	gtk_widget_destroy( GTK_WIDGET( window ) );

	return ret;
}

EMessageBoxReturn DoShaderInfoDlg( const char* name, const char* filename, const char* title ){
	ModalDialog dialog;
	ModalDialogButton ok_button( dialog, eIDOK );

	GtkWindow* window = create_modal_dialog_window( MainFrame_getWindow(), title, dialog, -1, -1 );

	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group( window, accel_group );

	{
		GtkHBox* hbox = create_dialog_hbox( 4, 4 );
		gtk_container_add( GTK_CONTAINER( window ), GTK_WIDGET( hbox ) );
		{
			GtkVBox* vbox = create_dialog_vbox( 4 );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), FALSE, FALSE, 0 );
			{
				GtkLabel* label = GTK_LABEL( gtk_label_new( "The selected shader" ) );
				gtk_widget_show( GTK_WIDGET( label ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				GtkLabel* label = GTK_LABEL( gtk_label_new( name ) );
				gtk_widget_show( GTK_WIDGET( label ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				GtkLabel* label = GTK_LABEL( gtk_label_new( "is located in file" ) );
				gtk_widget_show( GTK_WIDGET( label ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				GtkLabel* label = GTK_LABEL( gtk_label_new( filename ) );
				gtk_widget_show( GTK_WIDGET( label ) );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( label ), FALSE, FALSE, 0 );
			}
			{
				GtkButton* button = create_modal_dialog_button( "OK", ok_button );
				gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
				widget_make_default( GTK_WIDGET( button ) );
				gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel_group, GDK_Return, (GdkModifierType)0, GTK_ACCEL_VISIBLE );
			}
		}
	}

	EMessageBoxReturn ret = modal_dialog_show( window, dialog );

	gtk_widget_destroy( GTK_WIDGET( window ) );

	return ret;
}



CopiedString g_TextEditor_editorCommand( "" );

#include "ifilesystem.h"
#include "iarchive.h"
#include "idatastream.h"
#ifdef WIN32
#include <gdk/gdkwin32.h>
#endif

void DoShaderView( const char *shaderFileName, const char *shaderName, bool external_editor ){
	const char* pathRoot = GlobalFileSystem().findFile( shaderFileName );
	const bool pathEmpty = string_empty( pathRoot );
	const bool pathIsDir = !pathEmpty && file_is_directory( pathRoot );

	StringOutputStream pathFull( 256 );
	pathFull << pathRoot << ( pathIsDir? "" : "::" ) << shaderFileName;

	if( pathEmpty ){
		globalErrorStream() << "Failed to load shader file " << shaderFileName << "\n";
	}
	else if( external_editor && pathIsDir ){
		if( g_TextEditor_editorCommand.empty() ){
#ifdef WIN32
			ShellExecute( (HWND)GDK_WINDOW_HWND( GTK_WIDGET( MainFrame_getWindow() )->window ), 0, pathFull.c_str(), 0, 0, SW_SHOWNORMAL );
#else
			globalWarningStream() << "Failed to open '" << pathFull.c_str() << "'\nSet Shader Editor Command in preferences\n";
#endif
		}
		else{
			StringOutputStream command( 256 );
			command << g_TextEditor_editorCommand.c_str() << " \"" << pathFull.c_str() << "\"";
			globalOutputStream() << "Launching: " << command.c_str() << "\n";
			// note: linux does not return false if the command failed so it will assume success
			if ( !Q_Exec( 0, const_cast<char*>( command.c_str() ), 0, true, false ) )
				globalErrorStream() << "Failed to execute " << command.c_str() << "\n";
		}
	}
	else if( ArchiveFile* file = GlobalFileSystem().openFile( shaderFileName ) ){
		const std::size_t size = file->size();
		char* text = ( char* )malloc( size + 1 );
		file->getInputStream().read( ( InputStream::byte_type* )text, size );
		text[size] = 0;
		file->release();

		// look for the shader declaration
		std::size_t offset = 0;
		bool startOK = false;
		bool endOK = false;
		while ( !startOK || !endOK ){
			const char* found = string_in_string_nocase( text + offset, shaderName );
			if ( found == 0 ) {
				break;
			}
			else{
				offset = found - text;
				//validate found one...
				startOK = endOK = false;
				if ( offset == 0 ){
					startOK = true;
				}
				else{
					for ( const char* i = found - 1; i >= text; --i ){
						if( *i == '\t' || *i == ' ' ){
							startOK = true;
							continue;
						}
						else if( *i == '\n' || *i == '\r' || *i == '}' ){
							startOK = true;
							break;
						}
						else{
							startOK = false;
							break;
						}
					}
				}
				for ( const char* i = found + strlen( shaderName ); i < text + size; ++i ){
					if( *i == '\t' || *i == ' ' ){
						endOK = true;
						continue;
					}
					else if( *i == '\n' || *i == '\r' || *i == '{' || string_equal_nocase_n( i, ":q3map", 6 ) ){
						endOK = true;
						break;
					}
					else{
						endOK = false;
						break;
					}
				}
				if( !startOK || !endOK ){
					++offset;
				}
			}
		}
		g_textEditor.DoGtkTextEditor( text, size, offset, pathFull.c_str(), pathIsDir );
		free( text );
	}
}
