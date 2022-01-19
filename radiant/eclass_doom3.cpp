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

#include "eclass_doom3.h"

#include "debugging/debugging.h"

#include <map>

#include "ifilesystem.h"
#include "iscriplib.h"
#include "iarchive.h"
#include "qerplugin.h"

#include "generic/callback.h"
#include "string/string.h"
#include "eclasslib.h"
#include "os/path.h"
#include "os/dir.h"
#include "stream/stringstream.h"
#include "moduleobservers.h"
#include "stringio.h"

class RawString
{
	const char* m_value;
public:
	RawString( const char* value ) : m_value( value ){
	}
	const char* c_str() const {
		return m_value;
	}
};

inline bool operator<( const RawString& self, const RawString& other ){
	return string_less_nocase( self.c_str(), other.c_str() );
}

typedef std::map<RawString, EntityClass*> EntityClasses;
EntityClasses g_EntityClassDoom3_classes;
EntityClass *g_EntityClassDoom3_bad = 0;


void EntityClassDoom3_clear(){
	for ( EntityClasses::iterator i = g_EntityClassDoom3_classes.begin(); i != g_EntityClassDoom3_classes.end(); ++i )
	{
		( *i ).second->free( ( *i ).second );
	}
	g_EntityClassDoom3_classes.clear();
}

// entityClass will be inserted only if another of the same name does not already exist.
// if entityClass was inserted, the same object is returned, otherwise the already-existing object is returned.
EntityClass* EntityClassDoom3_insertUnique( EntityClass* entityClass ){
	return ( *g_EntityClassDoom3_classes.insert( EntityClasses::value_type( entityClass->name(), entityClass ) ).first ).second;
}

void EntityClassDoom3_forEach( EntityClassVisitor& visitor ){
	for ( EntityClasses::iterator i = g_EntityClassDoom3_classes.begin(); i != g_EntityClassDoom3_classes.end(); ++i )
	{
		visitor.visit( ( *i ).second );
	}
}

inline void printParseError( const char* message ){
	globalErrorStream() << message;
}

//#define PARSE_RETURN_FALSE_IF_FAIL( expression ) if ( !( expression ) ) { printParseError( FILE_LINE "\nparse failed: " # expression "\n" ); return false; } else
#define PARSE_RETURN_FALSE_IF_FAIL( expression ) do{ if ( !( expression ) ) { printParseError( FILE_LINE "\nparse failed: " # expression "\n" ); return false; } }while( 0 )

bool EntityClassDoom3_parseToken( Tokeniser& tokeniser ){
	const char* token = tokeniser.getToken();
	PARSE_RETURN_FALSE_IF_FAIL( token != 0 );
	return true;
}

bool EntityClassDoom3_parseToken( Tokeniser& tokeniser, const char* string ){
	const char* token = tokeniser.getToken();
	PARSE_RETURN_FALSE_IF_FAIL( token != 0 );
	return string_equal( token, string );
}

bool EntityClassDoom3_parseString( Tokeniser& tokeniser, const char*& s ){
	const char* token = tokeniser.getToken();
	PARSE_RETURN_FALSE_IF_FAIL( token != 0 );
	s = token;
	return true;
}

bool EntityClassDoom3_parseString( Tokeniser& tokeniser, CopiedString& s ){
	const char* token = tokeniser.getToken();
	PARSE_RETURN_FALSE_IF_FAIL( token != 0 );
	s = token;
	return true;
}

bool EntityClassDoom3_parseString( Tokeniser& tokeniser, StringOutputStream& s ){
	const char* token = tokeniser.getToken();
	PARSE_RETURN_FALSE_IF_FAIL( token != 0 );
	s << token;
	return true;
}

bool EntityClassDoom3_parseUnknown( Tokeniser& tokeniser ){
	//const char* name =
	PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );

	//globalOutputStream() << "parsing unknown block " << makeQuoted(name) << "\n";

	PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser, "{" ) );
	tokeniser.nextLine();

	std::size_t depth = 1;
	for (;; )
	{
		const char* token;
		PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, token ) );
		if ( string_equal( token, "}" ) ) {
			if ( --depth == 0 ) {
				tokeniser.nextLine();
				break;
			}
		}
		else if ( string_equal( token, "{" ) ) {
			++depth;
		}
		tokeniser.nextLine();
	}
	return true;
}


class Model
{
public:
	bool m_resolved;
	CopiedString m_mesh;
	CopiedString m_skin;
	CopiedString m_parent;
	typedef std::map<CopiedString, CopiedString> Anims;
	Anims m_anims;
	Model() : m_resolved( false ){
	}
};

typedef std::map<CopiedString, Model> Models;

Models g_models;

void Model_resolveInheritance( const char* name, Model& model ){
	if ( model.m_resolved == false ) {
		model.m_resolved = true;

		if ( !model.m_parent.empty() ) {
			Models::iterator i = g_models.find( model.m_parent );
			if ( i == g_models.end() ) {
				globalErrorStream() << "model " << name << " inherits unknown model " << model.m_parent << "\n";
			}
			else
			{
				Model_resolveInheritance( ( *i ).first.c_str(), ( *i ).second );
				model.m_mesh = ( *i ).second.m_mesh;
				model.m_skin = ( *i ).second.m_skin;
			}
		}
	}
}

bool EntityClassDoom3_parseModel( Tokeniser& tokeniser ){
	const char* name;
	PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, name ) );

	Model& model = g_models[name];

	PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser, "{" ) );
	tokeniser.nextLine();

	for (;; )
	{
		const char* parameter;
		PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, parameter ) );
		if ( string_equal( parameter, "}" ) ) {
			tokeniser.nextLine();
			break;
		}
		else if ( string_equal( parameter, "inherit" ) ) {
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, model.m_parent ) );
			tokeniser.nextLine();
		}
		else if ( string_equal( parameter, "remove" ) ) {
			//const char* remove =
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
			tokeniser.nextLine();
		}
		else if ( string_equal( parameter, "mesh" ) ) {
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, model.m_mesh ) );
			tokeniser.nextLine();
		}
		else if ( string_equal( parameter, "skin" ) ) {
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, model.m_skin ) );
			tokeniser.nextLine();
		}
		else if ( string_equal( parameter, "offset" ) ) {
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser, "(" ) );
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser, ")" ) );
			tokeniser.nextLine();
		}
		else if ( string_equal( parameter, "channel" ) ) {
			//const char* channelName =
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser, "(" ) );
			for (;; )
			{
				const char* end;
				PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, end ) );
				if ( string_equal( end, ")" ) ) {
					tokeniser.nextLine();
					break;
				}
			}
		}
		else if ( string_equal( parameter, "anim" ) ) {
			CopiedString animName;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, animName ) );
			const char* animFile;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, animFile ) );
			model.m_anims.insert( Model::Anims::value_type( animName, animFile ) );

			const char* token;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, token ) );

			while ( string_equal( token, "," ) )
			{
				PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, animFile ) );
				PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, token ) );
			}

			if ( string_equal( token, "{" ) ) {
				for (;; )
				{
					const char* end;
					PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, end ) );
					if ( string_equal( end, "}" ) ) {
						tokeniser.nextLine();
						break;
					}
					tokeniser.nextLine();
				}
			}
			else
			{
				tokeniser.ungetToken();
			}
		}
		else
		{
			globalErrorStream() << "unknown model parameter: " << makeQuoted( parameter ) << "\n";
			return false;
		}
		tokeniser.nextLine();
	}
	return true;
}

inline bool char_isSpaceOrTab( char c ){
	return c == ' ' || c == '\t';
}

inline bool char_isNotSpaceOrTab( char c ){
	return !char_isSpaceOrTab( c );
}

template<typename Predicate>
inline const char* string_find_if( const char* string, Predicate predicate ){
	for (; *string != 0; ++string )
	{
		if ( predicate( *string ) ) {
			return string;
		}
	}
	return string;
}

inline const char* string_findFirstSpaceOrTab( const char* string ){
	return string_find_if( string, char_isSpaceOrTab );
}

inline const char* string_findFirstNonSpaceOrTab( const char* string ){
	return string_find_if( string, char_isNotSpaceOrTab );
}


static bool EntityClass_parse( EntityClass& entityClass, Tokeniser& tokeniser ){
	const char* name;
	PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, name ) );
	entityClass.name_set( name );

	PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser, "{" ) );
	tokeniser.nextLine();

	StringOutputStream usage( 256 );
	StringOutputStream description( 256 );
	CopiedString* currentDescription = 0;
	StringOutputStream* currentString = 0;

	for (;; )
	{
		const char* key;
		PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, key ) );

		const char* last = string_findFirstSpaceOrTab( key );
		CopiedString first( StringRange( key, last ) );

		if ( !string_empty( last ) ) {
			last = string_findFirstNonSpaceOrTab( last );
		}

		if ( currentString != 0 && string_equal( key, "\\" ) ) {
			tokeniser.nextLine();
			*currentString << " ";
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, *currentString ) );
			continue;
		}

		if ( currentDescription != 0 ) {
			*currentDescription = description.c_str();
			description.clear();
			currentDescription = 0;
		}
		currentString = 0;

		if ( string_equal( key, "}" ) ) {
			tokeniser.nextLine();
			break;
		}
		else if ( string_equal( key, "model" ) ) {
			const char* token;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, token ) );
			entityClass.fixedsize = true;
			entityClass.m_modelpath = StringOutputStream( 256 )( PathCleaned( token ) ).c_str();
		}
		else if ( string_equal( key, "editor_color" ) ) {
			const char* value;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, value ) );
			if ( !string_empty( value ) ) {
				entityClass.colorSpecified = true;
				bool success = string_parse_vector3( value, entityClass.color );
				ASSERT_MESSAGE( success, "editor_color: parse error" );
			}
		}
		else if ( string_equal( key, "editor_ragdoll" ) ) {
			//bool ragdoll = atoi(tokeniser.getToken()) != 0;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
		}
		else if ( string_equal( key, "editor_mins" ) ) {
			entityClass.sizeSpecified = true;
			const char* value;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, value ) );
			if ( !string_empty( value ) && !string_equal( value, "?" ) ) {
				entityClass.fixedsize = true;
				bool success = string_parse_vector3( value, entityClass.mins );
				ASSERT_MESSAGE( success, "editor_mins: parse error" );
			}
		}
		else if ( string_equal( key, "editor_maxs" ) ) {
			entityClass.sizeSpecified = true;
			const char* value;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, value ) );
			if ( !string_empty( value ) && !string_equal( value, "?" ) ) {
				entityClass.fixedsize = true;
				bool success = string_parse_vector3( value, entityClass.maxs );
				ASSERT_MESSAGE( success, "editor_maxs: parse error" );
			}
		}
		else if ( string_equal( key, "editor_usage" ) ) {
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, usage ) );
			currentString = &usage;
		}
		else if ( string_equal_n( key, "editor_usage", 12 ) ) {
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, usage ) );
			currentString = &usage;
		}
		else if ( string_equal( key, "editor_rotatable" )
		       || string_equal( key, "editor_showangle" )
		       || string_equal( key, "editor_showangles" ) // typo? in prey movables.def
		       || string_equal( key, "editor_mover" )
		       || string_equal( key, "editor_model" )
		       || string_equal( key, "editor_material" )
		       || string_equal( key, "editor_combatnode" )
		       || ( !string_empty( last ) && string_equal( first.c_str(), "editor_gui" ) )
		       || string_equal_n( key, "editor_copy", 11 ) ) {
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
		}
		else if ( !string_empty( last ) && ( string_equal( first.c_str(), "editor_var" ) || string_equal( first.c_str(), "editor_string" ) ) ) {
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, last ).second;
			attribute.m_type = "string";
			currentDescription = &attribute.m_description;
			currentString = &description;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, description ) );
		}
		else if ( !string_empty( last ) && string_equal( first.c_str(), "editor_float" ) ) {
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, last ).second;
			attribute.m_type = "string";
			currentDescription = &attribute.m_description;
			currentString = &description;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, description ) );
		}
		else if ( !string_empty( last ) && string_equal( first.c_str(), "editor_snd" ) ) {
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, last ).second;
			attribute.m_type = "sound";
			currentDescription = &attribute.m_description;
			currentString = &description;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, description ) );
		}
		else if ( !string_empty( last ) && string_equal( first.c_str(), "editor_bool" ) ) {
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, last ).second;
			attribute.m_type = "boolean";
			currentDescription = &attribute.m_description;
			currentString = &description;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, description ) );
		}
		else if ( !string_empty( last ) && string_equal( first.c_str(), "editor_int" ) ) {
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, last ).second;
			attribute.m_type = "integer";
			currentDescription = &attribute.m_description;
			currentString = &description;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, description ) );
		}
		else if ( !string_empty( last ) && string_equal( first.c_str(), "editor_model" ) ) {
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, last ).second;
			attribute.m_type = "model";
			currentDescription = &attribute.m_description;
			currentString = &description;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, description ) );
		}
		else if ( !string_empty( last ) && string_equal( first.c_str(), "editor_color" ) ) {
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, last ).second;
			attribute.m_type = "color";
			currentDescription = &attribute.m_description;
			currentString = &description;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, description ) );
		}
		else if ( !string_empty( last ) && ( string_equal( first.c_str(), "editor_material" ) || string_equal( first.c_str(), "editor_mat" ) ) ) {
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, last ).second;
			attribute.m_type = "shader";
			currentDescription = &attribute.m_description;
			currentString = &description;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, description ) );
		}
		else if ( string_equal( key, "inherit" ) ) {
			entityClass.inheritanceResolved = false;
			ASSERT_MESSAGE( entityClass.m_parent.empty(), "only one 'inherit' supported per entityDef" );
			const char* token;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, token ) );
			entityClass.m_parent.push_back( token );
		}
		// begin quake4-specific keys
		else if ( string_equal( key, "editor_targetonsel" ) ) {
			//const char* value =
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
		}
		else if ( string_equal( key, "editor_menu" ) ) {
			//const char* value =
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
		}
		else if ( string_equal( key, "editor_ignore" ) ) {
			//const char* value =
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
		}
		// end quake4-specific keys
		// begin ignore prey (unknown/unused?) entity keys
		else if ( string_equal( key, "editor_light" )
		       || string_equal( key, "editor_def def_debrisspawner" )
		       || string_equal( key, "editor_def def_drop" )
		       || string_equal( key, "editor_def def_guihand" )
		       || string_equal( key, "editor_def def_mine" ) ) {
			//const char* value =
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseToken( tokeniser ) );
		}
		// end ignore prey entity keys
		else
		{
			CopiedString tmp( key );
			//ASSERT_MESSAGE( !string_equal_n( key, "editor_", 7 ), "unsupported editor key: " << makeQuoted( key ) );
			if ( string_equal_n( key, "editor_", 7 ) ) {
				globalErrorStream() << "unsupported editor key " << makeQuoted( key );
			}
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, key ).second;
			attribute.m_type = "string";
			const char* value;
			PARSE_RETURN_FALSE_IF_FAIL( EntityClassDoom3_parseString( tokeniser, value ) );
			if ( string_equal( value, "}" ) ) { // hack for quake4 powerups.def bug
				globalErrorStream() << "entityDef " << makeQuoted( entityClass.name() ) << " key " << makeQuoted( tmp ) << " has no value\n";
				break;
			}
			else
			{
				attribute.m_value = value;
			}
		}
		tokeniser.nextLine();
	}

	entityClass.m_comments = usage.c_str();

	if ( string_equal( entityClass.name(), "light" ) ) {
		{
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, "light_radius" ).second;
			attribute.m_type = "vector3";
			attribute.m_value = "300 300 300";
		}
		{
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, "light_center" ).second;
			attribute.m_type = "vector3";
		}
		{
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, "noshadows" ).second;
			attribute.m_type = "boolean";
			attribute.m_value = "0";
		}
		{
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, "nospecular" ).second;
			attribute.m_type = "boolean";
			attribute.m_value = "0";
		}
		{
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, "nodiffuse" ).second;
			attribute.m_type = "boolean";
			attribute.m_value = "0";
		}
		{
			EntityClassAttribute& attribute = EntityClass_insertAttribute( entityClass, "falloff" ).second;
			attribute.m_type = "real";
		}
	}

	return true;
}

bool EntityClassDoom3_parseEntityDef( Tokeniser& tokeniser ){
	EntityClass* entityClass = Eclass_Alloc();
	entityClass->free = &Eclass_Free;

	if ( !EntityClass_parse( *entityClass, tokeniser ) ) {
		eclass_capture_state( entityClass ); // finish constructing the entity so that it can be destroyed cleanly.
		entityClass->free( entityClass );
		return false;
	}

	EntityClass* inserted = EntityClassDoom3_insertUnique( entityClass );
	if ( inserted != entityClass ) {
		globalErrorStream() << "entityDef " << entityClass->name() << " is already defined, second definition ignored\n";
		eclass_capture_state( entityClass ); // finish constructing the entity so that it can be destroyed cleanly.
		entityClass->free( entityClass );
	}
	return true;
}

bool EntityClassDoom3_parseBlock( Tokeniser& tokeniser, const char* blockType ){
	if ( string_equal( blockType, "entityDef" ) ) {
		return EntityClassDoom3_parseEntityDef( tokeniser );
	}
	else if ( string_equal( blockType, "model" ) ) {
		return EntityClassDoom3_parseModel( tokeniser );
	}
	else
	{
		return EntityClassDoom3_parseUnknown( tokeniser );
	}
}

bool EntityClassDoom3_parse( TextInputStream& inputStream, const char* filename ){
	Tokeniser& tokeniser = GlobalScriptLibrary().m_pfnNewScriptTokeniser( inputStream );

	tokeniser.nextLine();

	for (;; )
	{
		const char* blockType = tokeniser.getToken();
		if ( blockType == 0 ) {
			return true;
		}
		CopiedString tmp( blockType );
		if ( !EntityClassDoom3_parseBlock( tokeniser, tmp.c_str() ) ) {
			globalErrorStream() << GlobalFileSystem().findFile( filename ) << filename << ":" << (unsigned int)tokeniser.getLine() << ": " << tmp << " parse failed, skipping rest of file\n";
			return false;
		}
	}

	tokeniser.release();
}


void EntityClassDoom3_loadFile( const char* filename ){
	globalOutputStream() << "parsing entity classes from " << makeQuoted( filename ) << "\n";

	StringOutputStream fullname( 256 );
	fullname << "def/" << filename;

	ArchiveTextFile* file = GlobalFileSystem().openTextFile( fullname.c_str() );
	if ( file != 0 ) {
		EntityClassDoom3_parse( file->getInputStream(), fullname.c_str() );
		file->release();
	}
}

EntityClass* EntityClassDoom3_findOrInsert( const char *name, bool has_brushes ){
	ASSERT_NOTNULL( name );

	if ( string_empty( name ) ) {
		return g_EntityClassDoom3_bad;
	}

	EntityClasses::iterator i = g_EntityClassDoom3_classes.find( name );
	if ( i != g_EntityClassDoom3_classes.end()
	     //&& string_equal((*i).first, name)
	   ) {
		return ( *i ).second;
	}

	EntityClass* e = EntityClass_Create_Default( name, has_brushes );
	EntityClass* inserted = EntityClassDoom3_insertUnique( e );
	ASSERT_MESSAGE( inserted == e, "" );
	return inserted;
}

const ListAttributeType* EntityClassDoom3_findListType( const char* name ){
	return 0;
}


void EntityClass_resolveInheritance( EntityClass* derivedClass ){
	if ( derivedClass->inheritanceResolved == false ) {
		derivedClass->inheritanceResolved = true;
		EntityClasses::iterator i = g_EntityClassDoom3_classes.find( derivedClass->m_parent.front().c_str() );
		if ( i == g_EntityClassDoom3_classes.end() ) {
			globalErrorStream() << "failed to find entityDef " << makeQuoted( derivedClass->m_parent.front() ) << " inherited by "  << makeQuoted( derivedClass->name() ) << "\n";
		}
		else
		{
			EntityClass* parentClass = ( *i ).second;
			EntityClass_resolveInheritance( parentClass );
			if ( !derivedClass->colorSpecified ) {
				derivedClass->colorSpecified = parentClass->colorSpecified;
				derivedClass->color = parentClass->color;
			}
			if ( !derivedClass->sizeSpecified ) {
				derivedClass->sizeSpecified = parentClass->sizeSpecified;
				derivedClass->mins = parentClass->mins;
				derivedClass->maxs = parentClass->maxs;
				derivedClass->fixedsize = parentClass->fixedsize;
			}

			for ( EntityClassAttributes::iterator j = parentClass->m_attributes.begin(); j != parentClass->m_attributes.end(); ++j )
			{
				EntityClass_insertAttribute( *derivedClass, ( *j ).first.c_str(), ( *j ).second );
			}
		}
	}
}

class EntityClassDoom3 : public ModuleObserver
{
	std::size_t m_unrealised;
	ModuleObservers m_observers;
public:
	EntityClassDoom3() : m_unrealised( 2 ){
	}
	void realise(){
		if ( --m_unrealised == 0 ) {
			globalOutputStream() << "searching vfs directory " << makeQuoted( "def" ) << " for *.def\n";
			GlobalFileSystem().forEachFile( "def/", "def", FreeCaller1<const char*, EntityClassDoom3_loadFile>() );

			{
				for ( Models::iterator i = g_models.begin(); i != g_models.end(); ++i )
				{
					Model_resolveInheritance( ( *i ).first.c_str(), ( *i ).second );
				}
			}
			{
				for ( EntityClasses::iterator i = g_EntityClassDoom3_classes.begin(); i != g_EntityClassDoom3_classes.end(); ++i )
				{
					EntityClass_resolveInheritance( ( *i ).second );
					if ( !( *i ).second->m_modelpath.empty() ) {
						Models::iterator j = g_models.find( ( *i ).second->m_modelpath );
						if ( j != g_models.end() ) {
							( *i ).second->m_modelpath = ( *j ).second.m_mesh;
							( *i ).second->m_skin = ( *j ).second.m_skin;
						}
					}
					eclass_capture_state( ( *i ).second );

					StringOutputStream usage( 256 );

					usage << "-------- NOTES --------\n";

					if ( !( *i ).second->m_comments.empty() ) {
						usage << ( *i ).second->m_comments << "\n";
					}

					usage << "\n-------- KEYS --------\n";

					for ( EntityClassAttributes::iterator j = ( *i ).second->m_attributes.begin(); j != ( *i ).second->m_attributes.end(); ++j )
					{
						const char* name = EntityClassAttributePair_getName( *j );
						const char* description = EntityClassAttributePair_getDescription( *j );
						if ( !string_equal( name, description ) ) {
							usage << EntityClassAttributePair_getName( *j ) << " : " << EntityClassAttributePair_getDescription( *j ) << "\n";
						}
					}

					( *i ).second->m_comments = usage.c_str();
				}
			}

			m_observers.realise();
		}
	}
	void unrealise(){
		if ( ++m_unrealised == 1 ) {
			m_observers.unrealise();
			EntityClassDoom3_clear();
		}
	}
	void attach( ModuleObserver& observer ){
		m_observers.attach( observer );
	}
	void detach( ModuleObserver& observer ){
		m_observers.detach( observer );
	}
};

EntityClassDoom3 g_EntityClassDoom3;

void EntityClassDoom3_attach( ModuleObserver& observer ){
	g_EntityClassDoom3.attach( observer );
}
void EntityClassDoom3_detach( ModuleObserver& observer ){
	g_EntityClassDoom3.detach( observer );
}

void EntityClassDoom3_realise(){
	g_EntityClassDoom3.realise();
}
void EntityClassDoom3_unrealise(){
	g_EntityClassDoom3.unrealise();
}

void EntityClassDoom3_construct(){
	GlobalFileSystem().attach( g_EntityClassDoom3 );

	// start by creating the default unknown eclass
	g_EntityClassDoom3_bad = EClass_Create( "UNKNOWN_CLASS", Vector3( 0.0f, 0.5f, 0.0f ), "" );

	EntityClassDoom3_realise();
}

void EntityClassDoom3_destroy(){
	EntityClassDoom3_unrealise();

	g_EntityClassDoom3_bad->free( g_EntityClassDoom3_bad );

	GlobalFileSystem().detach( g_EntityClassDoom3 );
}

class EntityClassDoom3Dependencies : public GlobalFileSystemModuleRef, public GlobalShaderCacheModuleRef
{
};

class EntityClassDoom3API
{
	EntityClassManager m_eclassmanager;
public:
	typedef EntityClassManager Type;
	STRING_CONSTANT( Name, "doom3" );

	EntityClassDoom3API(){
		EntityClassDoom3_construct();

		m_eclassmanager.findOrInsert = &EntityClassDoom3_findOrInsert;
		m_eclassmanager.findListType = &EntityClassDoom3_findListType;
		m_eclassmanager.forEach = &EntityClassDoom3_forEach;
		m_eclassmanager.attach = &EntityClassDoom3_attach;
		m_eclassmanager.detach = &EntityClassDoom3_detach;
		m_eclassmanager.realise = &EntityClassDoom3_realise;
		m_eclassmanager.unrealise = &EntityClassDoom3_unrealise;
	}
	~EntityClassDoom3API(){
		EntityClassDoom3_destroy();
	}
	EntityClassManager* getTable(){
		return &m_eclassmanager;
	}
};

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

typedef SingletonModule<EntityClassDoom3API, EntityClassDoom3Dependencies> EntityClassDoom3Module;
typedef Static<EntityClassDoom3Module> StaticEntityClassDoom3Module;
StaticRegisterModule staticRegisterEntityClassDoom3( StaticEntityClassDoom3Module::instance() );
