/*
   Copyright (C) 2001-2006, William Joseph.
   All Rights Reserved.

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

#include "commands.h"

#include "debugging/debugging.h"
#include "warnings.h"

#include <map>
#include "string/string.h"
#include "versionlib.h"
#include "gtkutil/accelerator.h"
#include "gtkutil/messagebox.h"
#include <gtk/gtktreeselection.h>
#include <gtk/gtkbutton.h>
#include "gtkmisc.h"

typedef std::pair<Accelerator, int> ShortcutValue; // accelerator, isRegistered
typedef std::map<CopiedString, ShortcutValue> Shortcuts;

void Shortcuts_foreach( Shortcuts& shortcuts, CommandVisitor& visitor ){
	for ( Shortcuts::iterator i = shortcuts.begin(); i != shortcuts.end(); ++i )
	{
		visitor.visit( ( *i ).first.c_str(), ( *i ).second.first );
	}
}

Shortcuts g_shortcuts;

const Accelerator& GlobalShortcuts_insert( const char* name, const Accelerator& accelerator ){
	return ( *g_shortcuts.insert( Shortcuts::value_type( name, ShortcutValue( accelerator, false ) ) ).first ).second.first;
}

void GlobalShortcuts_foreach( CommandVisitor& visitor ){
	Shortcuts_foreach( g_shortcuts, visitor );
}

void GlobalShortcuts_register( const char* name, int type ){
	Shortcuts::iterator i = g_shortcuts.find( name );
	if ( i != g_shortcuts.end() ) {
		( *i ).second.second = type;
	}
}

void GlobalShortcuts_reportUnregistered(){
	for ( Shortcuts::iterator i = g_shortcuts.begin(); i != g_shortcuts.end(); ++i )
	{
		if ( ( *i ).second.first.key != 0 && !( *i ).second.second ) {
			globalWarningStream() << "shortcut not registered: " << ( *i ).first.c_str() << "\n";
		}
	}
}

typedef std::map<CopiedString, Command> Commands;

Commands g_commands;

void GlobalCommands_insert( const char* name, const Callback& callback, const Accelerator& accelerator ){
	bool added = g_commands.insert( Commands::value_type( name, Command( callback, GlobalShortcuts_insert( name, accelerator ) ) ) ).second;
	ASSERT_MESSAGE( added, "command already registered: " << makeQuoted( name ) );
}

const Command& GlobalCommands_find( const char* command ){
	Commands::iterator i = g_commands.find( command );
	ASSERT_MESSAGE( i != g_commands.end(), "failed to lookup command " << makeQuoted( command ) );
	return ( *i ).second;
}

typedef std::map<CopiedString, Toggle> Toggles;


Toggles g_toggles;

void GlobalToggles_insert( const char* name, const Callback& callback, const BoolExportCallback& exportCallback, const Accelerator& accelerator ){
	bool added = g_toggles.insert( Toggles::value_type( name, Toggle( callback, GlobalShortcuts_insert( name, accelerator ), exportCallback ) ) ).second;
	ASSERT_MESSAGE( added, "toggle already registered: " << makeQuoted( name ) );
}
const Toggle& GlobalToggles_find( const char* name ){
	Toggles::iterator i = g_toggles.find( name );
	ASSERT_MESSAGE( i != g_toggles.end(), "failed to lookup toggle " << makeQuoted( name ) );
	return ( *i ).second;
}

typedef std::map<CopiedString, KeyEvent> KeyEvents;


KeyEvents g_keyEvents;

void GlobalKeyEvents_insert( const char* name, const Accelerator& accelerator, const Callback& keyDown, const Callback& keyUp ){
	bool added = g_keyEvents.insert( KeyEvents::value_type( name, KeyEvent( GlobalShortcuts_insert( name, accelerator ), keyDown, keyUp ) ) ).second;
	ASSERT_MESSAGE( added, "command already registered: " << makeQuoted( name ) );
}
const KeyEvent& GlobalKeyEvents_find( const char* name ){
	KeyEvents::iterator i = g_keyEvents.find( name );
	ASSERT_MESSAGE( i != g_keyEvents.end(), "failed to lookup keyEvent " << makeQuoted( name ) );
	return ( *i ).second;
}


void disconnect_accelerator( const char *name ){
	Shortcuts::iterator i = g_shortcuts.find( name );
	if ( i != g_shortcuts.end() ) {
		switch ( ( *i ).second.second )
		{
		case 1:
			// command
			command_disconnect_accelerator( name );
			break;
		case 2:
			// toggle
			toggle_remove_accelerator( name );
			break;
		}
	}
}

void connect_accelerator( const char *name ){
	Shortcuts::iterator i = g_shortcuts.find( name );
	if ( i != g_shortcuts.end() ) {
		switch ( ( *i ).second.second )
		{
		case 1:
			// command
			command_connect_accelerator( name );
			break;
		case 2:
			// toggle
			toggle_add_accelerator( name );
			break;
		}
	}
}


#include <cctype>

#include <gtk/gtkbox.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertext.h>

#include "gtkutil/dialog.h"
#include "mainframe.h"

#include "stream/textfilestream.h"
#include "stream/stringstream.h"


struct command_list_dialog_t : public ModalDialog
{
	command_list_dialog_t()
		: m_close_button( *this, eIDCANCEL ), m_list( NULL ), m_command_iter(), m_model( NULL ), m_waiting_for_key( false ){
	}
	ModalDialogButton m_close_button;

	GtkTreeView *m_list;
	GtkTreeIter m_command_iter;
	GtkTreeModel *m_model;
	bool m_waiting_for_key;
};

void accelerator_clear_button_clicked( GtkButton *btn, gpointer dialogptr ){
	command_list_dialog_t &dialog = *(command_list_dialog_t *) dialogptr;

	if ( dialog.m_waiting_for_key ) {
		// just unhighlight, user wanted to cancel
		dialog.m_waiting_for_key = false;
		gtk_list_store_set( GTK_LIST_STORE( dialog.m_model ), &dialog.m_command_iter, 2, false, -1 );
		gtk_widget_set_sensitive( GTK_WIDGET( dialog.m_list ), true );
		dialog.m_model = NULL;
		return;
	}

	GtkTreeSelection *sel = gtk_tree_view_get_selection( dialog.m_list );
	GtkTreeModel *model;
	GtkTreeIter iter;
	if ( !gtk_tree_selection_get_selected( sel, &model, &iter ) ) {
		return;
	}

	GValue val;
	memset( &val, 0, sizeof( val ) );
	gtk_tree_model_get_value( GTK_TREE_MODEL( model ), &iter, 0, &val );
	const char *commandName = g_value_get_string( &val );;

	// clear the ACTUAL accelerator too!
	disconnect_accelerator( commandName );

	Shortcuts::iterator thisShortcutIterator = g_shortcuts.find( commandName );
	if ( thisShortcutIterator == g_shortcuts.end() ) {
		return;
	}
	thisShortcutIterator->second.first = accelerator_null();

	gtk_list_store_set( GTK_LIST_STORE( model ), &iter, 1, "", -1 );

	g_value_unset( &val );
}

void accelerator_edit_button_clicked( GtkButton *btn, gpointer dialogptr ){
	command_list_dialog_t &dialog = *(command_list_dialog_t *) dialogptr;

	// 1. find selected row
	GtkTreeSelection *sel = gtk_tree_view_get_selection( dialog.m_list );
	GtkTreeModel *model;
	GtkTreeIter iter;
	if ( !gtk_tree_selection_get_selected( sel, &model, &iter ) ) {
		return;
	}
	if ( dialog.m_waiting_for_key ) {
		// unhighlight highlit
		dialog.m_waiting_for_key = false;
		gtk_list_store_set( GTK_LIST_STORE( dialog.m_model ), &dialog.m_command_iter, 2, false, -1 );
	}
	dialog.m_command_iter = iter;
	dialog.m_model = model;

	// 2. disallow changing the row
	//gtk_widget_set_sensitive(GTK_WIDGET(dialog.m_list), false);

	// 3. highlight the row
	gtk_list_store_set( GTK_LIST_STORE( model ), &iter, 2, true, -1 );

	// 4. grab keyboard focus
	dialog.m_waiting_for_key = true;
}

gboolean accelerator_tree_butt_press( GtkWidget* widget, GdkEventButton* event, gpointer dialogptr ){
	if ( event->type == GDK_2BUTTON_PRESS && event->button == 1 ) {
		accelerator_edit_button_clicked( 0, dialogptr );
		return TRUE;
	}
	return FALSE;
}

gboolean accelerator_window_key_press( GtkWidget *widget, GdkEventKey *event, gpointer dialogptr ){
	command_list_dialog_t &dialog = *(command_list_dialog_t *) dialogptr;

	if ( !dialog.m_waiting_for_key ) {
		return false;
	}

#if 0
	if ( event->is_modifier ) {
		return false;
	}
#else
	switch ( event->keyval )
	{
	case GDK_Shift_L:
	case GDK_Shift_R:
	case GDK_Control_L:
	case GDK_Control_R:
	case GDK_Caps_Lock:
	case GDK_Shift_Lock:
	case GDK_Meta_L:
	case GDK_Meta_R:
	case GDK_Alt_L:
	case GDK_Alt_R:
	case GDK_Super_L:
	case GDK_Super_R:
	case GDK_Hyper_L:
	case GDK_Hyper_R:
		return false;
	}
#endif

	dialog.m_waiting_for_key = false;

	// 7. find the name of the accelerator
	GValue val;
	memset( &val, 0, sizeof( val ) );
	gtk_tree_model_get_value( GTK_TREE_MODEL( dialog.m_model ), &dialog.m_command_iter, 0, &val );
	const char *commandName = g_value_get_string( &val );;
	Shortcuts::iterator thisShortcutIterator = g_shortcuts.find( commandName );
	if ( thisShortcutIterator == g_shortcuts.end() ) {
		globalErrorStream() << "commandName " << makeQuoted( commandName ) << " not found in g_shortcuts.\n";
		gtk_list_store_set( GTK_LIST_STORE( dialog.m_model ), &dialog.m_command_iter, 2, false, -1 );
		gtk_widget_set_sensitive( GTK_WIDGET( dialog.m_list ), true );
		return true;
	}

	// 8. build an Accelerator
	Accelerator newAccel( event->keyval, (GdkModifierType) event->state );

	// 8. verify the key is still free, show a dialog to ask what to do if not
	class VerifyAcceleratorNotTaken : public CommandVisitor
	{
	const char *commandName;
	const Accelerator &newAccel;
	GtkWidget *widget;
	GtkTreeModel *model;
public:
	bool allow;
	VerifyAcceleratorNotTaken( const char *name, const Accelerator &accelerator, GtkWidget *w, GtkTreeModel *m ) :
								commandName( name ), newAccel( accelerator ), widget( w ), model( m ), allow( true ){
	}
	void visit( const char* name, Accelerator& accelerator ){
		if ( !strcmp( name, commandName ) ) {
			return;
		}
		if ( !allow ) {
			return;
		}
		if ( accelerator.key == 0 ) {
			return;
		}
		if ( accelerator == newAccel ) {
			StringOutputStream msg;
			msg << "The command " << name << " is already assigned to the key " << accelerator << ".\n\n"
				<< "Do you want to unassign " << name << " first?";
			EMessageBoxReturn r = gtk_MessageBox( widget, msg.c_str(), "Key already used", eMB_YESNOCANCEL );
			if ( r == eIDYES ) {
				// clear the ACTUAL accelerator too!
				disconnect_accelerator( name );
				// delete the modifier
				accelerator = accelerator_null();
				// empty the cell of the key binds dialog
				GtkTreeIter i;
				if ( gtk_tree_model_get_iter_first( GTK_TREE_MODEL( model ), &i ) ) {
					for (;; )
					{
						GValue val;
						memset( &val, 0, sizeof( val ) );
						gtk_tree_model_get_value( GTK_TREE_MODEL( model ), &i, 0, &val );
						const char *thisName = g_value_get_string( &val );;
						if ( !strcmp( thisName, name ) ) {
							gtk_list_store_set( GTK_LIST_STORE( model ), &i, 1, "", -1 );
						}
						g_value_unset( &val );
						if ( !gtk_tree_model_iter_next( GTK_TREE_MODEL( model ), &i ) ) {
							break;
						}
					}
				}
			}
			else if ( r == eIDCANCEL ) {
				// aborted
				allow = false;
			}
		}
	}
	} verify_visitor( commandName, newAccel, widget, dialog.m_model );
	GlobalShortcuts_foreach( verify_visitor );

	gtk_list_store_set( GTK_LIST_STORE( dialog.m_model ), &dialog.m_command_iter, 2, false, -1 );
	gtk_widget_set_sensitive( GTK_WIDGET( dialog.m_list ), true );

	if ( verify_visitor.allow ) {
		// clear the ACTUAL accelerator first
		disconnect_accelerator( commandName );

		thisShortcutIterator->second.first = newAccel;

		// write into the cell
		StringOutputStream modifiers;
		modifiers << newAccel;
		gtk_list_store_set( GTK_LIST_STORE( dialog.m_model ), &dialog.m_command_iter, 1, modifiers.c_str(), -1 );

		// set the ACTUAL accelerator too!
		connect_accelerator( commandName );
	}

	g_value_unset( &val );

	dialog.m_model = NULL;

	return true;
}

/*
    GtkTreeIter row;
    GValue val;
    if(!model) {g_error("Unable to get model from cell renderer");}
    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(model), &row, path_string);

    gtk_tree_model_get_value(GTK_TREE_MODEL(model), &row, 0, &val);
    const char *name = g_value_get_string(&val);
    Shortcuts::iterator i = g_shortcuts.find(name);
    if(i != g_shortcuts.end())
    {
        accelerator_parse(i->second.first, new_text);
        StringOutputStream modifiers;
        modifiers << i->second.first;
        gtk_list_store_set(GTK_LIST_STORE(model), &row, 1, modifiers.c_str(), -1);
    }
   };
 */

static gboolean mid_search_func( GtkTreeModel* model, gint column, const gchar* key, GtkTreeIter* iter, gpointer search_data ) {
	gchar* iter_string = 0;
	gtk_tree_model_get( model, iter, column, &iter_string, -1 );
	const gboolean ret = iter_string? !string_in_string_nocase( iter_string, key ) : TRUE;
	g_free( iter_string );
	return ret;
}

void DoCommandListDlg(){
	command_list_dialog_t dialog;

	GtkWindow* window = create_modal_dialog_window( MainFrame_getWindow(), "Mapped Commands", dialog, -1, 400 );
	g_signal_connect( G_OBJECT( window ), "key-press-event", (GCallback) accelerator_window_key_press, &dialog );

	GtkAccelGroup* accel = gtk_accel_group_new();
	gtk_window_add_accel_group( window, accel );

	GtkHBox* hbox = create_dialog_hbox( 4, 4 );
	gtk_container_add( GTK_CONTAINER( window ), GTK_WIDGET( hbox ) );

	{
		GtkScrolledWindow* scr = create_scrolled_window( GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
		gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( scr ), TRUE, TRUE, 0 );

		{
			GtkListStore* store = gtk_list_store_new( 4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_INT );

			GtkWidget* view = gtk_tree_view_new_with_model( GTK_TREE_MODEL( store ) );
			dialog.m_list = GTK_TREE_VIEW( view );

			//gtk_tree_view_set_enable_search( GTK_TREE_VIEW( view ), false ); // annoying
			gtk_tree_view_set_search_column( dialog.m_list, 0 );
			gtk_tree_view_set_search_equal_func( dialog.m_list, (GtkTreeViewSearchEqualFunc)mid_search_func, 0, 0 );

			g_signal_connect( G_OBJECT( view ), "button_press_event", G_CALLBACK( accelerator_tree_butt_press ), &dialog );

			{
				GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
				GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes( "Command", renderer, "text", 0, "weight-set", 2, "weight", 3, NULL );
				gtk_tree_view_append_column( GTK_TREE_VIEW( view ), column );
			}

			{
				GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
				GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes( "Key", renderer, "text", 1, "weight-set", 2, "weight", 3, NULL );
				gtk_tree_view_append_column( GTK_TREE_VIEW( view ), column );
			}

			gtk_widget_show( view );
			gtk_container_add( GTK_CONTAINER( scr ), view );

			{
				// Initialize dialog
				StringOutputStream path( 256 );
				path << SettingsPath_get() << "commandlist.txt";
				globalOutputStream() << "Writing the command list to " << path.c_str() << "\n";
				class BuildCommandList : public CommandVisitor
				{
				TextFileOutputStream m_commandList;
				GtkListStore* m_store;
public:
				BuildCommandList( const char* filename, GtkListStore* store ) : m_commandList( filename ), m_store( store ){
				}
				void visit( const char* name, Accelerator& accelerator ){
					StringOutputStream modifiers;
					modifiers << accelerator;

					{
						GtkTreeIter iter;
						gtk_list_store_append( m_store, &iter );
						gtk_list_store_set( m_store, &iter, 0, name, 1, modifiers.c_str(), 2, false, 3, 800, -1 );
					}

					if ( !m_commandList.failed() ) {
						int l = strlen( name );
						m_commandList << name;
						while ( l++ < 32 )
							m_commandList << ' ';
						m_commandList << modifiers.c_str() << '\n';
					}
				}
				} visitor( path.c_str(), store );

				GlobalShortcuts_foreach( visitor );
			}

			g_object_unref( G_OBJECT( store ) );
		}
	}

	GtkVBox* vbox = create_dialog_vbox( 4 );
	gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( vbox ), TRUE, TRUE, 0 );
	{
		GtkButton* editbutton = create_dialog_button( "Edit", (GCallback) accelerator_edit_button_clicked, &dialog );
		gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( editbutton ), FALSE, FALSE, 0 );

		GtkButton* clearbutton = create_dialog_button( "Clear", (GCallback) accelerator_clear_button_clicked, &dialog );
		gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( clearbutton ), FALSE, FALSE, 0 );

		GtkWidget *spacer = gtk_image_new();
		gtk_widget_show( spacer );
		gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( spacer ), TRUE, TRUE, 0 );

		GtkButton* button = create_modal_dialog_button( "Close", dialog.m_close_button );
		gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, FALSE, 0 );
		widget_make_default( GTK_WIDGET( button ) );
		gtk_widget_grab_default( GTK_WIDGET( button ) );
		gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_Return, (GdkModifierType)0, (GtkAccelFlags)0 );
		gtk_widget_add_accelerator( GTK_WIDGET( button ), "clicked", accel, GDK_Escape, (GdkModifierType)0, (GtkAccelFlags)0 );
	}

	modal_dialog_show( window, dialog );
	gtk_widget_destroy( GTK_WIDGET( window ) );
}

#include "profile/profile.h"

const char* const COMMANDS_VERSION = "1.0-gtk-accelnames";

void SaveCommandMap( const char* path ){
	StringOutputStream strINI( 256 );
	strINI << path << "shortcuts.ini";

	TextFileOutputStream file( strINI.c_str() );
	if ( !file.failed() ) {
		file << "[Version]\n";
		file << "number=" << COMMANDS_VERSION << "\n";
		file << "\n";
		file << "[Commands]\n";
		class WriteCommandMap : public CommandVisitor
		{
		TextFileOutputStream& m_file;
public:
		WriteCommandMap( TextFileOutputStream& file ) : m_file( file ){
		}
		void visit( const char* name, Accelerator& accelerator ){
			m_file << name << "=";

			const char* key = gtk_accelerator_name( accelerator.key, accelerator.modifiers );
			m_file << key;
			m_file << "\n";
		}
		} visitor( file );
		GlobalShortcuts_foreach( visitor );
	}
}

const char* stringrange_find( const char* first, const char* last, char c ){
	const char* p = strchr( first, '+' );
	if ( p == 0 ) {
		return last;
	}
	return p;
}

class ReadCommandMap : public CommandVisitor
{
const char* m_filename;
std::size_t m_count;
public:
ReadCommandMap( const char* filename ) : m_filename( filename ), m_count( 0 ){
}
void visit( const char* name, Accelerator& accelerator ){
	char value[1024];
	if ( read_var( m_filename, "Commands", name, value ) ) {
		if ( string_empty( value ) ) {
			accelerator = accelerator_null();
			return;
		}

		guint key;
		GdkModifierType modifiers;
		gtk_accelerator_parse( value, &key, &modifiers );
		accelerator = Accelerator( key, modifiers );

		if ( accelerator.key != 0 ) {
			++m_count;
		}
		else
		{
			globalWarningStream() << "WARNING: failed to parse user command " << makeQuoted( name ) << ": unknown key " << makeQuoted( value ) << "\n";
		}
	}
}
std::size_t count() const {
	return m_count;
}
};

void LoadCommandMap( const char* path ){
	StringOutputStream strINI( 256 );
	strINI << path << "shortcuts.ini";

	FILE* f = fopen( strINI.c_str(), "r" );
	if ( f != 0 ) {
		fclose( f );
		globalOutputStream() << "loading custom shortcuts list from " << makeQuoted( strINI.c_str() ) << "\n";

		Version version = version_parse( COMMANDS_VERSION );
		Version dataVersion = { 0, 0 };

		{
			char value[1024];
			if ( read_var( strINI.c_str(), "Version", "number", value ) ) {
				dataVersion = version_parse( value );
			}
		}

		if ( version_compatible( version, dataVersion ) ) {
			globalOutputStream() << "commands import: data version " << dataVersion << " is compatible with code version " << version << "\n";
			ReadCommandMap visitor( strINI.c_str() );
			GlobalShortcuts_foreach( visitor );
			globalOutputStream() << "parsed " << Unsigned( visitor.count() ) << " custom shortcuts\n";
		}
		else
		{
			globalWarningStream() << "commands import: data version " << dataVersion << " is not compatible with code version " << version << "\n";
		}
	}
	else
	{
		globalWarningStream() << "failed to load custom shortcuts from " << makeQuoted( strINI.c_str() ) << "\n";
	}
}
