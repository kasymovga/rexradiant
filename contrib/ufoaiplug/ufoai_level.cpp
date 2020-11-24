/*
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

#include "ufoai_level.h"
#include "ufoai_filters.h"

#include "ibrush.h"
#include "ientity.h"
#include "iscenegraph.h"

#include "string/string.h"
#include <list>

class Level;

/**
 * @brief find entities by class
 * @note from radiant/map.cpp
 */
class EntityFindByClassname : public scene::Graph::Walker
{
const char* m_name;
Entity*& m_entity;
public:
EntityFindByClassname( const char* name, Entity*& entity ) : m_name( name ), m_entity( entity ){
	m_entity = 0;
}
bool pre( const scene::Path& path, scene::Instance& instance ) const {
	if ( m_entity == 0 ) {
		Entity* entity = Node_getEntity( path.top() );
		if ( entity != 0 && string_equal( m_name, entity->getKeyValue( "classname" ) ) ) {
			m_entity = entity;
		}
	}
	return true;
}
};

/**
 * @brief
 */
Entity* Scene_FindEntityByClass( const char* name ){
	Entity* entity = NULL;
	GlobalSceneGraph().traverse( EntityFindByClassname( name, entity ) );
	return entity;
}

/**
 * @brief finds start positions
 */
class EntityFindFlags : public scene::Graph::Walker
{
const char *m_classname;
const char *m_flag;
int *m_count;

public:
EntityFindFlags( const char *classname, const char *flag, int *count ) : m_classname( classname ), m_flag( flag ), m_count( count ){
}
bool pre( const scene::Path& path, scene::Instance& instance ) const {
	const char *str;
	Entity* entity = Node_getEntity( path.top() );
	if ( entity != 0 && string_equal( m_classname, entity->getKeyValue( "classname" ) ) ) {
		str = entity->getKeyValue( m_flag );
		if ( string_empty( str ) ) {
			( *m_count )++;
		}
	}
	return true;
}
};


/**
 * @brief finds start positions
 */
class EntityFindTeams : public scene::Graph::Walker
{
const char *m_classname;
int *m_count;
int *m_team;

public:
EntityFindTeams( const char *classname, int *count, int *team ) : m_classname( classname ), m_count( count ), m_team( team ){
}
bool pre( const scene::Path& path, scene::Instance& instance ) const {
	const char *str;
	Entity* entity = Node_getEntity( path.top() );
	if ( entity != 0 && string_equal( m_classname, entity->getKeyValue( "classname" ) ) ) {
		if ( m_count ) {
			( *m_count )++;
		}
		// now get the highest teamnum
		if ( m_team ) {
			str = entity->getKeyValue( "team" );
			if ( !string_empty( str ) ) {
				if ( atoi( str ) > *m_team ) {
					( *m_team ) = atoi( str );
				}
			}
		}
	}
	return true;
}
};

/**
 * @brief
 */
void get_team_count( const char *classname, int *count, int *team ){
	GlobalSceneGraph().traverse( EntityFindTeams( classname, count, team ) );
	globalOutputStream() << "UFO:AI: classname: " << classname << ": #" << ( *count ) << "\n";
}

/**
 * @brief Some default values to worldspawn like maxlevel and so on
 */
void assign_default_values_to_worldspawn( bool override, const char **returnMsg ){
	static char message[1024];
	Entity* worldspawn;
	int teams = 0;
	int count = 0;
	char str[64];

	worldspawn = Scene_FindEntityByClass( "worldspawn" );
	if ( !worldspawn ) {
		globalWarningStream() << "UFO:AI: Could not find worldspawn.\n";
		*returnMsg = "Could not find worldspawn";
		return;
	}

	*message = '\0';
	*str = '\0';

	if ( override || string_empty( worldspawn->getKeyValue( "maxlevel" ) ) ) {
		// TODO: Get highest brush - a level has 64 units
		worldspawn->setKeyValue( "maxlevel", "5" );
		snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "Set maxlevel to: %s", worldspawn->getKeyValue( "maxlevel" ) );
	}

	if ( override || string_empty( worldspawn->getKeyValue( "maxteams" ) ) ) {
		get_team_count( "info_player_start", &count, &teams );
		if ( teams ) {
			snprintf( str, sizeof( str ) - 1, "%i", teams );
			worldspawn->setKeyValue( "maxteams", str );
			snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "Set maxteams to: %s", worldspawn->getKeyValue( "maxteams" ) );
		}
		if ( count < 16 ) {
			snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "You should at least place 16 info_player_start" );
		}
	}

	// no errors - no warnings
	if ( !strlen( message ) ) {
		return;
	}

	*returnMsg = message;
}

/**
 * @brief
 */
int check_entity_flags( const char *classname, const char *flag ){
	int count;

	/* init this with 0 every time we browse the tree */
	count = 0;

	GlobalSceneGraph().traverse( EntityFindFlags( classname, flag, &count ) );
	return count;
}

/**
 * @brief Will check e.g. the map entities for valid values
 * @todo: check for maxlevel
 */
void check_map_values( const char **returnMsg ){
	static char message[1024];
	int count = 0;
	int teams = 0;
	int ent_flags;
	Entity* worldspawn;
	char str[64];

	worldspawn = Scene_FindEntityByClass( "worldspawn" );
	if ( !worldspawn ) {
		globalWarningStream() << "UFO:AI: Could not find worldspawn.\n";
		*returnMsg = "Could not find worldspawn";
		return;
	}

	*message = '\0';
	*str = '\0';

	// multiplayer start positions
	get_team_count( "info_player_start", &count, &teams );
	if ( !count ) {
		strncat( message, "No multiplayer start positions (info_player_start)\n", sizeof( message ) - 1 );
	}

	// singleplayer map?
	count = 0;
	get_team_count( "info_human_start", &count, NULL );
	if ( !count ) {
		strncat( message, "No singleplayer start positions (info_human_start)\n", sizeof( message ) - 1 );
	}

	// singleplayer map?
	count = 0;
	get_team_count( "info_2x2_start", &count, NULL );
	if ( !count ) {
		strncat( message, "No singleplayer start positions for 2x2 units (info_2x2_start)\n", sizeof( message ) - 1 );
	}

	// search for civilians
	count = 0;
	get_team_count( "info_civilian_start", &count, NULL );
	if ( !count ) {
		strncat( message, "No civilian start positions (info_civilian_start)\n", sizeof( message ) - 1 );
	}

	// check maxlevel
	if ( string_empty( worldspawn->getKeyValue( "maxlevel" ) ) ) {
		strncat( message, "Worldspawn: No maxlevel defined\n", sizeof( message ) - 1 );
	}
	else if ( atoi( worldspawn->getKeyValue( "maxlevel" ) ) > 8 ) {
		strncat( message, "Worldspawn: Highest maxlevel is 8\n", sizeof( message ) - 1 );
		worldspawn->setKeyValue( "maxlevel", "8" );
	}

	ent_flags = check_entity_flags( "func_door", "spawnflags" );
	if ( ent_flags ) {
		snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "Found %i func_door with no spawnflags\n", ent_flags );
	}
	ent_flags = check_entity_flags( "func_breakable", "spawnflags" );
	if ( ent_flags ) {
		snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "Found %i func_breakable with no spawnflags\n", ent_flags );
	}
	ent_flags = check_entity_flags( "misc_sound", "spawnflags" );
	if ( ent_flags ) {
		snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "Found %i misc_sound with no spawnflags\n", ent_flags );
	}
	ent_flags = check_entity_flags( "misc_model", "spawnflags" );
	if ( ent_flags ) {
		snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "Found %i misc_model with no spawnflags\n", ent_flags );
	}
	ent_flags = check_entity_flags( "misc_particle", "spawnflags" );
	if ( ent_flags ) {
		snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "Found %i misc_particle with no spawnflags\n", ent_flags );
	}
	ent_flags = check_entity_flags( "info_player_start", "team" );
	if ( ent_flags ) {
		snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "Found %i info_player_start with no team assigned\n!!Teamcount may change after you've fixed this\n", ent_flags );
	}
	ent_flags = check_entity_flags( "light", "color" );
	ent_flags = check_entity_flags( "light", "_color" );
	if ( ent_flags ) {
		snprintf( &message[strlen( message )], sizeof( message ) - 1 - strlen( message ), "Found %i lights with no color value\n", ent_flags );
	}

	// no errors found
	if ( !strlen( message ) ) {
		snprintf( message, sizeof( message ) - 1, "No errors found - you are ready to compile the map now\n" );
	}

	*returnMsg = message;
}
