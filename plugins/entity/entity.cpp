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

#include "entity.h"

#include "ifilter.h"
#include "selectable.h"
#include "namespace.h"

#include "scenelib.h"
#include "entitylib.h"
#include "eclasslib.h"
#include "pivot.h"

#include "targetable.h"
#include "uniquenames.h"
#include "namekeys.h"
#include "stream/stringstream.h"
#include "filters.h"


#include "miscmodel.h"
#include "light.h"
#include "group.h"
#include "eclassmodel.h"
#include "generic.h"
#include "doom3group.h"

#include "namedentity.h"



EGameType g_gameType;

inline scene::Node& entity_for_eclass( EntityClass* eclass ){
	if ( eclass->miscmodel_is ) {
		return New_MiscModel( eclass );
	}
	else if ( classname_equal( eclass->name(), "light" )
	       || classname_equal( eclass->name(), "lightJunior" ) ) {
		return New_Light( eclass );
	}
	if ( !eclass->fixedsize ) {
		if ( g_gameType == eGameTypeDoom3 ) {
			return New_Doom3Group( eclass );
		}
		else
		{
			return New_Group( eclass );
		}
	}
	else if ( !string_empty( eclass->modelpath() ) ) {
		return New_EclassModel( eclass );
	}
	else
	{
		return New_GenericEntity( eclass );
	}
}

void Entity_setName( Entity& entity, const char* name ){
	entity.setKeyValue( "name", name );
}
typedef ReferenceCaller1<Entity, const char*, Entity_setName> EntitySetNameCaller;

inline Namespaced* Node_getNamespaced( scene::Node& node ){
	return NodeTypeCast<Namespaced>::cast( node );
}

inline scene::Node& node_for_eclass( EntityClass* eclass ){
	scene::Node& node = entity_for_eclass( eclass );
	Node_getEntity( node )->setKeyValue( "classname", eclass->name() );

	if ( g_gameType == eGameTypeDoom3
	     && string_not_empty( eclass->name() )
	     && !string_equal( eclass->name(), "worldspawn" )
	     && !string_equal( eclass->name(), "UNKNOWN_CLASS" ) ) {
		char buffer[1024];
		strcpy( buffer, eclass->name() );
		strcat( buffer, "_1" );
		GlobalNamespace().makeUnique( buffer, EntitySetNameCaller( *Node_getEntity( node ) ) );
	}

	Namespaced* namespaced = Node_getNamespaced( node );
	if ( namespaced != 0 ) {
		namespaced->setNamespace( GlobalNamespace() );
	}

	return node;
}

EntityCreator::KeyValueChangedFunc EntityKeyValues::m_entityKeyValueChanged = 0;
EntityCreator::KeyValueChangedFunc KeyValue::m_entityKeyValueChanged = 0;
Counter* EntityKeyValues::m_counter = 0;

bool g_showNames = true;
bool g_showBboxes = false;
bool g_showConnections = true;
int g_showNamesDist = 512;
int g_showNamesRatio = 64;
bool g_showTargetNames = false;
bool g_showAngles = true;
bool g_lightRadii = true;

bool g_stupidQuakeBug = false;


class ConnectEntities
{
public:
	Entity* m_e1;
	Entity* m_e2;
	int m_index;
	ConnectEntities( Entity* e1, Entity* e2, int index ) : m_e1( e1 ), m_e2( e2 ), m_index( index ){
	}
	const char *keyname(){
		StringOutputStream key( 16 );
		if ( m_index <= 0 ) {
			return "target";
		}
		if ( m_index == 1 ) {
			return "killtarget";
		}
		key << "target" << m_index;
		return key.c_str();
	}
	void connect( const char* name ){
		m_e1->setKeyValue( keyname(), name );
		m_e2->setKeyValue( "targetname", name );
	}
	typedef MemberCaller1<ConnectEntities, const char*, &ConnectEntities::connect> ConnectCaller;
};

inline Entity* ScenePath_getEntity( const scene::Path& path ){
	Entity* entity = Node_getEntity( path.top() );
	if ( entity == 0 && path.size() > 1 ) {
		entity = Node_getEntity( path.parent() );
	}
	return entity;
}

class Quake3EntityCreator : public EntityCreator
{
public:
	scene::Node& createEntity( EntityClass* eclass ){
		return node_for_eclass( eclass );
	}
	void setKeyValueChangedFunc( KeyValueChangedFunc func ){
		EntityKeyValues::setKeyValueChangedFunc( func );
	}
	void setCounter( Counter* counter ){
		EntityKeyValues::setCounter( counter );
	}
	void connectEntities( const scene::Path& path, const scene::Path& targetPath, int index ){
		Entity* e1 = ScenePath_getEntity( path );
		Entity* e2 = ScenePath_getEntity( targetPath );

		if ( e1 == 0 || e2 == 0 ) {
			globalErrorStream() << "entityConnectSelected: both of the selected instances must be an entity\n";
			return;
		}

		if ( e1 == e2 ) {
			globalErrorStream() << "entityConnectSelected: the selected instances must not both be from the same entity\n";
			return;
		}


		UndoableCommand undo( "entityConnectSelected" );

		if ( g_gameType == eGameTypeDoom3 ) {
			StringOutputStream key( 16 );
			if ( index >= 0 ) {
				key << "target";
				if ( index != 0 ) {
					key << index;
				}
				e1->setKeyValue( key.c_str(), e2->getKeyValue( "name" ) );
				key.clear();
			}
			else
			{
				for ( unsigned int i = 0; ; ++i )
				{
					key << "target";
					if ( i != 0 ) {
						key << i;
					}
					if ( !e1->hasKeyValue( key.c_str() ) ) {
						e1->setKeyValue( key.c_str(), e2->getKeyValue( "name" ) );
						break;
					}
					key.clear();
				}
			}
		}
		else
		{
			ConnectEntities connector( e1, e2, index );
			//killconnect
			if( index == 1 ){
				const char* value = e2->getKeyValue( "targetname" );
				if ( !string_empty( value ) ) {
					connector.connect( value );
				}
				else
				{
					const char* type = e2->getClassName();
					if ( string_empty( type ) ) {
						type = "t";
					}
					StringOutputStream key( 64 );
					key << type << "1";
					GlobalNamespace().makeUnique( key.c_str(), ConnectEntities::ConnectCaller( connector ) );
				}
			}
			//normal connect
			else{
				//prioritize existing target key
				//checking, if ent got other connected ones already, could be better solution
				const char* value = e1->getKeyValue( "target" );
				if ( !string_empty( value ) ) {
					connector.connect( value );
				}
				else{
					value = e2->getKeyValue( "targetname" );
					if ( !string_empty( value ) ) {
						connector.connect( value );
					}
					else{
						const char* type = e2->getClassName();
						if ( string_empty( type ) ) {
							type = "t";
						}
						StringOutputStream key( 64 );
						key << type << "1";
						GlobalNamespace().makeUnique( key.c_str(), ConnectEntities::ConnectCaller( connector ) );
					}
				}
			}
		}
		SceneChangeNotify();
	}
	void setLightRadii( bool lightRadii ){
		g_lightRadii = lightRadii;
	}
	bool getLightRadii(){
		return g_lightRadii;
	}
	void setShowNames( bool showNames ){
		g_showNames = showNames;
	}
	bool getShowNames(){
		return g_showNames;
	}
	void setShowBboxes( bool showBboxes ){
		g_showBboxes = showBboxes;
	}
	bool getShowBboxes(){
		return g_showBboxes;
	}
	void setShowConnections( bool showConnections ){
		g_showConnections = showConnections;
	}
	bool getShowConnections(){
		return g_showConnections;
	}
	void setShowNamesDist( int dist ){
		g_showNamesDist = dist;
	}
	int getShowNamesDist(){
		return g_showNamesDist;
	}
	void setShowNamesRatio( int ratio ){
		g_showNamesRatio = ratio;
	}
	int getShowNamesRatio(){
		return g_showNamesRatio;
	}
	void setShowTargetNames( bool showNames ){
		g_showTargetNames = showNames;
	}
	bool getShowTargetNames(){
		return g_showTargetNames;
	}
	void setShowAngles( bool showAngles ){
		g_showAngles = showAngles;
	}
	bool getShowAngles(){
		return g_showAngles;
	}

	void printStatistics() const {
		StringPool_analyse( EntityKeyValues::getPool() );
	}
};

Quake3EntityCreator g_Quake3EntityCreator;

EntityCreator& GetEntityCreator(){
	return g_Quake3EntityCreator;
}



class filter_entity_classname : public EntityFilter
{
	const char* m_classname;
public:
	filter_entity_classname( const char* classname ) : m_classname( classname ){
	}
	bool filter( const Entity& entity ) const {
		return string_equal( entity.getClassName(), m_classname );
	}
};

class filter_entity_classgroup : public EntityFilter
{
	const char* m_classgroup;
	std::size_t m_length;
public:
	filter_entity_classgroup( const char* classgroup ) : m_classgroup( classgroup ), m_length( string_length( m_classgroup ) ){
	}
	bool filter( const Entity& entity ) const {
		return string_equal_n( entity.getClassName(), m_classgroup, m_length );
	}
};

filter_entity_classname g_filter_entity_func_group( "func_group" );
filter_entity_classgroup g_filter_entity_func_detail( "func_detail" );
filter_entity_classname g_filter_entity_light( "light" );
filter_entity_classgroup g_filter_entity_trigger( "trigger_" );
filter_entity_classgroup g_filter_entity_path( "path_" );

class filter_entity_misc_model : public EntityFilter
{
public:
	bool filter( const Entity& entity ) const {
		return entity.getEntityClass().miscmodel_is;
	}
};
filter_entity_misc_model g_filter_entity_misc_model;

class filter_entity_doom3model : public EntityFilter
{
public:
	bool filter( const Entity& entity ) const {
		return string_equal( entity.getClassName(), "func_static" )
		    && !string_equal( entity.getKeyValue( "model" ), entity.getKeyValue( "name" ) );
	}
};

filter_entity_doom3model g_filter_entity_doom3model;


class filter_entity_not_func_detail : public EntityFilter
{
public:
	bool filter( const Entity& entity ) const {
		return entity.isContainer()
		    && !string_equal_n( entity.getClassName(), "func_detail", 11 );
	}
};

filter_entity_not_func_detail g_filter_entity_not_func_detail;

class filter_entity_world : public EntityFilter
{
public:
	bool filter( const Entity& entity ) const {
		const char* value = entity.getClassName();
		return string_equal( value, "worldspawn" )
		    || string_equal( value, "func_group" )
		    || string_equal_n( value, "func_detail", 11 );
	}
};

filter_entity_world g_filter_entity_world;

class filter_entity_point : public EntityFilter
{
public:
	bool filter( const Entity& entity ) const {
		return !entity.isContainer()
		    && !entity.getEntityClass().miscmodel_is
		    && !string_equal_prefix( entity.getClassName(), "light" );
	}
};

filter_entity_point g_filter_entity_point;

#include "qerplugin.h"

void Entity_InitFilters(){
	add_entity_filter( g_filter_entity_world, EXCLUDE_WORLD );
	add_entity_filter( g_filter_entity_func_group, EXCLUDE_FUNC_GROUPS );
	if( string_equal( GlobalRadiant().getRequiredGameDescriptionKeyValue( "brushtypes" ), "quake" ) ){
		add_entity_filter( g_filter_entity_func_detail, EXCLUDE_DETAILS );
		add_entity_filter( g_filter_entity_not_func_detail, EXCLUDE_STRUCTURAL );
	}
	add_entity_filter( g_filter_entity_world, EXCLUDE_ENT, true );
	add_entity_filter( g_filter_entity_point, EXCLUDE_POINT_ENT );
	add_entity_filter( g_filter_entity_trigger, EXCLUDE_TRIGGERS );
	add_entity_filter( g_filter_entity_misc_model, EXCLUDE_MODELS );
	add_entity_filter( g_filter_entity_doom3model, EXCLUDE_MODELS );
	add_entity_filter( g_filter_entity_light, EXCLUDE_LIGHTS );
	add_entity_filter( g_filter_entity_path, EXCLUDE_PATHS );
}


#include "preferencesystem.h"

void Entity_Construct( EGameType gameType ){
	g_gameType = gameType;
	if ( g_gameType == eGameTypeDoom3 ) {
		g_targetable_nameKey = "name";

		Static<KeyIsName>::instance().m_keyIsName = keyIsNameDoom3;
		Static<KeyIsName>::instance().m_nameKey = "name";
	}
	else
	{
		Static<KeyIsName>::instance().m_keyIsName = keyIsNameQuake3;
		Static<KeyIsName>::instance().m_nameKey = "targetname";
	}

	g_stupidQuakeBug = ( g_gameType == eGameTypeQuake1 );

	GlobalPreferenceSystem().registerPreference( "SI_ShowNames", BoolImportStringCaller( g_showNames ), BoolExportStringCaller( g_showNames ) );
	GlobalPreferenceSystem().registerPreference( "SI_ShowBboxes", BoolImportStringCaller( g_showBboxes ), BoolExportStringCaller( g_showBboxes ) );
	GlobalPreferenceSystem().registerPreference( "SI_ShowConnections", BoolImportStringCaller( g_showConnections ), BoolExportStringCaller( g_showConnections ) );
	GlobalPreferenceSystem().registerPreference( "SI_ShowNamesDist", IntImportStringCaller( g_showNamesDist ), IntExportStringCaller( g_showNamesDist ) );
	GlobalPreferenceSystem().registerPreference( "SI_ShowNamesRatio", IntImportStringCaller( g_showNamesRatio ), IntExportStringCaller( g_showNamesRatio ) );
	GlobalPreferenceSystem().registerPreference( "SI_ShowTargetNames", BoolImportStringCaller( g_showTargetNames ), BoolExportStringCaller( g_showTargetNames ) );
	GlobalPreferenceSystem().registerPreference( "SI_ShowAngles", BoolImportStringCaller( g_showAngles ), BoolExportStringCaller( g_showAngles ) );
	GlobalPreferenceSystem().registerPreference( "LightRadiuses", BoolImportStringCaller( g_lightRadii ), BoolExportStringCaller( g_lightRadii ) );

	if( !g_showTargetNames ){
		Static<KeyIsName>::instance().m_nameKey = "classname";
	}

	Entity_InitFilters();
	const LightType lightType = g_gameType == eGameTypeRTCW? LIGHTTYPE_RTCW : g_gameType == eGameTypeDoom3? LIGHTTYPE_DOOM3 : LIGHTTYPE_DEFAULT;
	Light_Construct( lightType );
	MiscModel_construct();
	Doom3Group_construct();

	RenderablePivot::StaticShader::instance() = GlobalShaderCache().capture( "$PIVOT" );
	RenderableNamedEntity::StaticShader::instance() = GlobalShaderCache().capture( "$TEXT" );

	GlobalShaderCache().attachRenderable( StaticRenderableConnectionLines::instance() );
}

void Entity_Destroy(){
	GlobalShaderCache().detachRenderable( StaticRenderableConnectionLines::instance() );

	GlobalShaderCache().release( "$PIVOT" );
	GlobalShaderCache().release( "$TEXT" );

	Doom3Group_destroy();
	MiscModel_destroy();
	Light_Destroy();
}
