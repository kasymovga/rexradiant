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
// Small functions to help with GTK
//

#include "gtkmisc.h"

#include <gtk/gtk.h>

#include "math/vector.h"
#include "os/path.h"

#include "gtkutil/dialog.h"
#include "gtkutil/filechooser.h"
#include "gtkutil/menu.h"
#include "gtkutil/toolbar.h"
#include "commands.h"


void process_gui(){
	while ( gtk_events_pending() )
	{
		gtk_main_iteration();
	}
}

// =============================================================================
// Misc stuff

void command_connect_accelerator( const char* name ){
	const Command& command = GlobalCommands_find( name );
	GlobalShortcuts_register( name, 1 );
	global_accel_group_connect( command.m_accelerator, command.m_callback );
}

void command_disconnect_accelerator( const char* name ){
	const Command& command = GlobalCommands_find( name );
	global_accel_group_disconnect( command.m_accelerator, command.m_callback );
}

void toggle_add_accelerator( const char* name ){
	const Toggle& toggle = GlobalToggles_find( name );
	GlobalShortcuts_register( name, 2 );
	global_accel_group_connect( toggle.m_command.m_accelerator, toggle.m_command.m_callback );
}

void toggle_remove_accelerator( const char* name ){
	const Toggle& toggle = GlobalToggles_find( name );
	global_accel_group_disconnect( toggle.m_command.m_accelerator, toggle.m_command.m_callback );
}

GtkCheckMenuItem* create_check_menu_item_with_mnemonic( GtkMenu* menu, const char* mnemonic, const char* commandName ){
	GlobalShortcuts_register( commandName, 2 );
	const Toggle& toggle = GlobalToggles_find( commandName );
	global_accel_group_connect( toggle.m_command.m_accelerator, toggle.m_command.m_callback );
	return create_check_menu_item_with_mnemonic( menu, mnemonic, toggle );
}

GtkMenuItem* create_menu_item_with_mnemonic( GtkMenu* menu, const char *mnemonic, const char* commandName ){
	GlobalShortcuts_register( commandName, 1 );
	const Command& command = GlobalCommands_find( commandName );
	global_accel_group_connect( command.m_accelerator, command.m_callback );
	return create_menu_item_with_mnemonic( menu, mnemonic, command );
}

GtkToolButton* toolbar_append_button( GtkToolbar* toolbar, const char* description, const char* icon, const char* commandName ){
	return toolbar_append_button( toolbar, description, icon, GlobalCommands_find( commandName ) );
}

GtkToggleToolButton* toolbar_append_toggle_button( GtkToolbar* toolbar, const char* description, const char* icon, const char* commandName ){
	return toolbar_append_toggle_button( toolbar, description, icon, GlobalToggles_find( commandName ) );
}

// =============================================================================
// File dialog

bool color_dialog( GtkWidget *parent, Vector3& color, const char* title ){
	GtkWidget* dlg;
	GdkColor clr = { 0, guint16( color[0] * 65535 ),
						guint16( color[1] * 65535 ),
						guint16( color[2] * 65535 ) };
	ModalDialog dialog;

	dlg = gtk_color_selection_dialog_new( title );
	gtk_color_selection_set_current_color( GTK_COLOR_SELECTION( gtk_color_selection_dialog_get_color_selection( GTK_COLOR_SELECTION_DIALOG( dlg ) ) ), &clr );
	g_signal_connect( G_OBJECT( dlg ), "delete_event", G_CALLBACK( dialog_delete_callback ), &dialog );
	GtkWidget *ok_button, *cancel_button;
	g_object_get( G_OBJECT( dlg ), "ok-button", &ok_button, "cancel-button", &cancel_button, nullptr );
	g_signal_connect( G_OBJECT( ok_button ), "clicked", G_CALLBACK( dialog_button_ok ), &dialog );
	g_signal_connect( G_OBJECT( cancel_button ), "clicked", G_CALLBACK( dialog_button_cancel ), &dialog );

	if ( parent != 0 ) {
		gtk_window_set_transient_for( GTK_WINDOW( dlg ), GTK_WINDOW( parent ) );
	}

	bool ok = modal_dialog_show( GTK_WINDOW( dlg ), dialog ) == eIDOK;
	if ( ok ) {
		gtk_color_selection_get_current_color( GTK_COLOR_SELECTION( gtk_color_selection_dialog_get_color_selection( GTK_COLOR_SELECTION_DIALOG( dlg ) ) ), &clr );
		color[0] = clr.red / 65535.0;
		color[1] = clr.green / 65535.0;
		color[2] = clr.blue / 65535.0;
	}

	gtk_widget_destroy( dlg );

	return ok;
}

bool OpenGLFont_dialog( GtkWidget *parent, const char* font, CopiedString &newfont ){
	GtkWidget* dlg;
	ModalDialog dialog;


	dlg = gtk_font_selection_dialog_new( "OpenGLFont" );
	gtk_font_selection_dialog_set_font_name( GTK_FONT_SELECTION_DIALOG( dlg ), font );

	g_signal_connect( G_OBJECT( dlg ), "delete_event", G_CALLBACK( dialog_delete_callback ), &dialog );
	g_signal_connect( G_OBJECT( gtk_font_selection_dialog_get_ok_button( GTK_FONT_SELECTION_DIALOG( dlg ) ) ), "clicked", G_CALLBACK( dialog_button_ok ), &dialog );
	g_signal_connect( G_OBJECT( gtk_font_selection_dialog_get_cancel_button( GTK_FONT_SELECTION_DIALOG( dlg ) ) ), "clicked", G_CALLBACK( dialog_button_cancel ), &dialog );

	if ( parent != 0 ) {
		gtk_window_set_transient_for( GTK_WINDOW( dlg ), GTK_WINDOW( parent ) );
	}

	bool ok = modal_dialog_show( GTK_WINDOW( dlg ), dialog ) == eIDOK;
	if ( ok ) {
		gchar* selectedfont = gtk_font_selection_dialog_get_font_name( GTK_FONT_SELECTION_DIALOG( dlg ) );
		newfont = selectedfont;
		g_free( selectedfont );
	}

	gtk_widget_destroy( dlg );

	return ok;
}

void button_clicked_entry_browse_file( GtkWidget* widget, GtkEntry* entry ){
	const char *filename = file_dialog( gtk_widget_get_toplevel( widget ), true, "Choose File", gtk_entry_get_text( entry ) );

	if ( filename != 0 ) {
		gtk_entry_set_text( entry, filename );
	}
}

void button_clicked_entry_browse_directory( GtkWidget* widget, GtkEntry* entry ){
	const char* text = gtk_entry_get_text( entry );
	char *dir = dir_dialog( gtk_widget_get_toplevel( widget ), "Choose Directory", path_is_absolute( text ) ? text : "" );

	if ( dir != 0 ) {
		gchar* converted = g_filename_to_utf8( dir, -1, 0, 0, 0 );
		gtk_entry_set_text( entry, converted );
		g_free( dir );
		g_free( converted );
	}
}
