#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "debugging/debugging.h"
#include "callbacks.h"
#include "support.h"
#include "plugin.h"

#define GLADE_HOOKUP_OBJECT( component,widget,name ) \
	g_object_set_data_full( G_OBJECT( component ), name, \
							gtk_widget_ref( widget ), (GDestroyNotify) gtk_widget_unref )

#define GLADE_HOOKUP_OBJECT_NO_REF( component,widget,name )	\
	g_object_set_data( G_OBJECT( component ), name, widget )

// created by glade
GtkWidget*
create_w_plugplug2( void ){
	GtkWidget *w_plugplug2;
	GtkWidget *vbox1;
	GtkWidget *hbox2;
	GtkWidget *vbox4;
	GtkWidget *r_collapse;
	GSList *r_collapse_group = NULL;
	GtkWidget *r_collapsebymaterial;
	GtkWidget *r_nocollapse;
	GtkWidget *vbox3;
	GtkWidget *b_export;
	GtkWidget *b_exportAs;
	GtkWidget *b_close;
	GtkWidget *vbox2;
	GtkWidget *label1;
	GtkWidget *scrolledwindow1;
	GtkWidget *t_materialist;
	GtkWidget *ed_materialname;
	GtkWidget *hbox1;
	GtkWidget *b_addmaterial;
	GtkWidget *b_removematerial;
	GtkWidget *t_exportmaterials;
	GtkWidget *t_limitmatnames;
	GtkWidget *t_objects;
	GtkWidget *t_weld;

	w_plugplug2 = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title( GTK_WINDOW( w_plugplug2 ), "BrushExport-Plugin 3.0 by namespace" );
	gtk_window_set_position( GTK_WINDOW( w_plugplug2 ), GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_transient_for( GTK_WINDOW( w_plugplug2 ), GTK_WINDOW( g_pRadiantWnd ) );
	gtk_window_set_destroy_with_parent( GTK_WINDOW( w_plugplug2 ), TRUE );

	vbox1 = gtk_vbox_new( FALSE, 0 );
	gtk_container_add( GTK_CONTAINER( w_plugplug2 ), vbox1 );
	gtk_container_set_border_width( GTK_CONTAINER( vbox1 ), 5 );

	hbox2 = gtk_hbox_new( TRUE, 5 );
	gtk_box_pack_start( GTK_BOX( vbox1 ), hbox2, FALSE, FALSE, 0 );
	gtk_container_set_border_width( GTK_CONTAINER( hbox2 ), 5 );

	vbox4 = gtk_vbox_new( TRUE, 0 );
	gtk_box_pack_start( GTK_BOX( hbox2 ), vbox4, TRUE, FALSE, 0 );

	r_collapse = gtk_radio_button_new_with_mnemonic( NULL, "Collapse mesh" );
	gtk_widget_set_tooltip_text( r_collapse, "Collapse all brushes into a single group" );
	gtk_box_pack_start( GTK_BOX( vbox4 ), r_collapse, FALSE, FALSE, 0 );
	gtk_radio_button_set_group( GTK_RADIO_BUTTON( r_collapse ), r_collapse_group );
	r_collapse_group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( r_collapse ) );

	r_collapsebymaterial = gtk_radio_button_new_with_mnemonic( NULL, "Collapse by material" );
	gtk_widget_set_tooltip_text( r_collapsebymaterial, "Collapse into groups by material" );
	gtk_box_pack_start( GTK_BOX( vbox4 ), r_collapsebymaterial, FALSE, FALSE, 0 );
	gtk_radio_button_set_group( GTK_RADIO_BUTTON( r_collapsebymaterial ), r_collapse_group );
	r_collapse_group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( r_collapsebymaterial ) );

	r_nocollapse = gtk_radio_button_new_with_mnemonic( NULL, "Don't collapse" );
	gtk_widget_set_tooltip_text( r_nocollapse, "Every brush is stored in its own group" );
	gtk_box_pack_start( GTK_BOX( vbox4 ), r_nocollapse, FALSE, FALSE, 0 );
	gtk_radio_button_set_group( GTK_RADIO_BUTTON( r_nocollapse ), r_collapse_group );
	r_collapse_group = gtk_radio_button_get_group( GTK_RADIO_BUTTON( r_nocollapse ) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( r_nocollapse ), TRUE );

	vbox3 = gtk_vbox_new( FALSE, 0 );
	gtk_box_pack_start( GTK_BOX( hbox2 ), vbox3, FALSE, FALSE, 0 );

	b_export = gtk_button_new_from_stock( "gtk-save" );
	gtk_box_pack_start( GTK_BOX( vbox3 ), b_export, TRUE, FALSE, 0 );
	gtk_container_set_border_width( GTK_CONTAINER( b_export ), 5 );
	gtk_widget_set_sensitive( b_export, FALSE );

	b_exportAs = gtk_button_new_from_stock( "gtk-save-as" );
	gtk_box_pack_start( GTK_BOX( vbox3 ), b_exportAs, TRUE, FALSE, 0 );
	gtk_container_set_border_width( GTK_CONTAINER( b_exportAs ), 5 );

	b_close = gtk_button_new_from_stock( "gtk-cancel" );
	gtk_box_pack_start( GTK_BOX( vbox3 ), b_close, TRUE, FALSE, 0 );
	gtk_container_set_border_width( GTK_CONTAINER( b_close ), 5 );

	vbox2 = gtk_vbox_new( FALSE, 5 );
	gtk_box_pack_start( GTK_BOX( vbox1 ), vbox2, TRUE, TRUE, 0 );
	gtk_container_set_border_width( GTK_CONTAINER( vbox2 ), 2 );

	label1 = gtk_label_new( "Ignored materials:" );
	gtk_box_pack_start( GTK_BOX( vbox2 ), label1, FALSE, FALSE, 0 );

	scrolledwindow1 = gtk_scrolled_window_new( NULL, NULL );
	gtk_box_pack_start( GTK_BOX( vbox2 ), scrolledwindow1, TRUE, TRUE, 0 );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolledwindow1 ), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW( scrolledwindow1 ), GTK_SHADOW_IN );

	{
		t_materialist = gtk_tree_view_new();
		gtk_container_add( GTK_CONTAINER( scrolledwindow1 ), t_materialist );
		gtk_tree_view_set_headers_visible( GTK_TREE_VIEW( t_materialist ), FALSE );
		gtk_tree_view_set_enable_search( GTK_TREE_VIEW( t_materialist ), FALSE );

		// column & renderer
		GtkTreeViewColumn* col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title( col, "materials" );
		gtk_tree_view_append_column( GTK_TREE_VIEW( t_materialist ), col );
		GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( t_materialist ), -1, "", renderer, "text", 0, NULL );

		// list store
		GtkListStore* ignorelist = gtk_list_store_new( 1, G_TYPE_STRING );
		gtk_tree_view_set_model( GTK_TREE_VIEW( t_materialist ), GTK_TREE_MODEL( ignorelist ) );
		g_object_unref( ignorelist );
	}

	ed_materialname = gtk_entry_new();
	gtk_box_pack_start( GTK_BOX( vbox2 ), ed_materialname, FALSE, FALSE, 0 );

	hbox1 = gtk_hbox_new( TRUE, 0 );
	gtk_box_pack_start( GTK_BOX( vbox2 ), hbox1, FALSE, FALSE, 0 );

	b_addmaterial = gtk_button_new_from_stock( "gtk-add" );
	gtk_box_pack_start( GTK_BOX( hbox1 ), b_addmaterial, FALSE, FALSE, 0 );

	b_removematerial = gtk_button_new_from_stock( "gtk-remove" );
	gtk_box_pack_start( GTK_BOX( hbox1 ), b_removematerial, FALSE, FALSE, 0 );

	t_limitmatnames = gtk_check_button_new_with_mnemonic( "Use short material names (max. 20 chars)" );
	gtk_box_pack_end( GTK_BOX( vbox2 ), t_limitmatnames, FALSE, FALSE, 0 );

	t_objects = gtk_check_button_new_with_mnemonic( "Create (o)bjects instead of (g)roups" );
	gtk_box_pack_end( GTK_BOX( vbox2 ), t_objects, FALSE, FALSE, 0 );

	t_weld = gtk_check_button_new_with_mnemonic( "Weld vertices" );
	gtk_widget_set_tooltip_text( t_weld, "inside groups/objects" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( t_weld ), TRUE );
	gtk_box_pack_end( GTK_BOX( vbox2 ), t_weld, FALSE, FALSE, 0 );

	t_exportmaterials = gtk_check_button_new_with_mnemonic( "Create material information (.mtl file)" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( t_exportmaterials ), TRUE );
	gtk_box_pack_end( GTK_BOX( vbox2 ), t_exportmaterials, FALSE, FALSE, 0 );

	using namespace callbacks;
	g_signal_connect( G_OBJECT( w_plugplug2 ), "delete_event", G_CALLBACK( gtk_widget_hide_on_delete ), NULL );
	g_signal_connect_swapped( G_OBJECT( b_close ), "clicked", G_CALLBACK( gtk_widget_hide ), w_plugplug2 );

	g_signal_connect( G_OBJECT( b_export ), "clicked", G_CALLBACK( OnExportClicked ), NULL );
	g_signal_connect( G_OBJECT( b_exportAs ), "clicked", G_CALLBACK( OnExportClicked ), gpointer( 1 ) );
	g_signal_connect( G_OBJECT( b_addmaterial ), "clicked", G_CALLBACK( OnAddMaterial ), NULL );
	g_signal_connect( G_OBJECT( ed_materialname ), "activate", G_CALLBACK( OnAddMaterial ), NULL ); // NB: wrong callback, but pointer casting works
	g_signal_connect( G_OBJECT( b_removematerial ), "clicked", G_CALLBACK( OnRemoveMaterial ), NULL );
	g_signal_connect( G_OBJECT( t_materialist ), "key-press-event", G_CALLBACK( OnRemoveMaterialKb ), NULL );


	/* Store pointers to all widgets, for use by lookup_widget(). */
	GLADE_HOOKUP_OBJECT_NO_REF( w_plugplug2, w_plugplug2, "w_plugplug2" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, vbox1, "vbox1" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, hbox2, "hbox2" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, vbox4, "vbox4" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, r_collapse, "r_collapse" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, r_collapsebymaterial, "r_collapsebymaterial" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, r_nocollapse, "r_nocollapse" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, vbox3, "vbox3" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, b_export, "b_export" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, b_close, "b_close" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, vbox2, "vbox2" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, label1, "label1" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, scrolledwindow1, "scrolledwindow1" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, t_materialist, "t_materialist" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, ed_materialname, "ed_materialname" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, hbox1, "hbox1" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, b_addmaterial, "b_addmaterial" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, b_removematerial, "b_removematerial" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, t_exportmaterials, "t_exportmaterials" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, t_limitmatnames, "t_limitmatnames" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, t_objects, "t_objects" );
	GLADE_HOOKUP_OBJECT( w_plugplug2, t_weld, "t_weld" );

	gtk_widget_show_all( w_plugplug2 );

	return w_plugplug2;
}

// global main window, is 0 when not created
GtkWidget* g_brushexp_window = 0;

// spawn or unhide plugin window
void CreateWindow( void ){
	if( !g_brushexp_window )
		g_brushexp_window = create_w_plugplug2();
	gtk_widget_show( g_brushexp_window );
}
