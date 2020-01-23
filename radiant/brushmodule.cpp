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

#include "brushmodule.h"

#include "qerplugin.h"

#include "brushnode.h"
#include "brushmanip.h"

#include "preferencesystem.h"
#include "stringio.h"

#include "map.h"
#include "qe3.h"
#include "mainframe.h"
#include "preferences.h"

#include "clippertool.h"

bool g_multipleBrushTypes = false;
EBrushType g_brushTypes[3];
int g_brushType;
bool g_brush_always_caulk = false;

bool getTextureLockEnabled(){
	return g_brush_texturelock_enabled;
}

const char* getTexdefTypeIdLabel(){
	return g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_QUAKE ? "AP" : g_bp_globals.m_texdefTypeId == TEXDEFTYPEID_BRUSHPRIMITIVES ? "BP" : "220";
}

const char* BrushType_getName( EBrushType type ){
	switch ( type )
	{
	case eBrushTypeQuake:
	case eBrushTypeQuake2:
	case eBrushTypeQuake3:
		return "Axial Projection";
	case eBrushTypeQuake3BP:
		return "Brush Primitives";
	case eBrushTypeQuake3Valve220:
	case eBrushTypeValve220:
		return "Valve 220";
	case eBrushTypeDoom3:
		return "Doom3";
	case eBrushTypeQuake4:
		return "Quake4";
	default:
		return "unknown";
	}
}

void Face_importSnapPlanes( bool value ){
	Face::m_quantise = value ? quantiseInteger : quantiseFloating;
}
typedef FreeCaller1<bool, Face_importSnapPlanes> FaceImportSnapPlanesCaller;

void Face_exportSnapPlanes( const BoolImportCallback& importer ){
	importer( Face::m_quantise == quantiseInteger );
}
typedef FreeCaller1<const BoolImportCallback&, Face_exportSnapPlanes> FaceExportSnapPlanesCaller;

void Brush_constructPreferences( PreferencesPage& page ){
	page.appendCheckBox(
		"", "Snap planes to integer grid",
		FaceImportSnapPlanesCaller(),
		FaceExportSnapPlanesCaller()
		);
	page.appendEntry(
		"Default texture scale",
		g_texdef_default_scale
		);
	if ( g_multipleBrushTypes ) {
		const char* names[] = { BrushType_getName( g_brushTypes[0] ), BrushType_getName( g_brushTypes[1] ), BrushType_getName( g_brushTypes[2] ) };
		page.appendCombo(
			"New map Brush Type",
			g_brushType,
			STRING_ARRAY_RANGE( names )
			);
	}
	// d1223m
	page.appendCheckBox( "",
						 "Always use caulk for new brushes",
						 g_brush_always_caulk
						 );
}
void Brush_constructPage( PreferenceGroup& group ){
	PreferencesPage page( group.createPage( "Brush", "Brush Settings" ) );
	Brush_constructPreferences( page );
}
void Brush_registerPreferencesPage(){
	PreferencesDialog_addSettingsPage( FreeCaller1<PreferenceGroup&, Brush_constructPage>() );
}

void Brush_toggleFormat( EBrushType type ){
	if( g_multipleBrushTypes ){
		Brush::destroyStatic();
		Brush::constructStatic( type );
	}
}

void Brush_unlatchPreferences(){
	if( g_multipleBrushTypes )
		Brush_toggleFormat( g_brushTypes[g_brushType] );
}

void Brush_Construct( EBrushType type ){
	if ( g_multipleBrushTypes ) {
		for ( int i = 0; i < 3; ++i ){
			if( g_brushTypes[i] == type ){
				g_brushType = i;
				break;
			}
		}
		GlobalPreferenceSystem().registerPreference(
			"BrushType",
			IntImportStringCaller( g_brushType ),
			IntExportStringCaller( g_brushType )
			);
		type = g_brushTypes[g_brushType];
	}
	// d1223m
	GlobalPreferenceSystem().registerPreference(
		"BrushAlwaysCaulk",
		BoolImportStringCaller( g_brush_always_caulk ),
		BoolExportStringCaller( g_brush_always_caulk ) );

	Brush_registerCommands();
	Brush_registerPreferencesPage();

	BrushFilters_construct();

	BrushClipPlane::constructStatic();
	BrushInstance::constructStatic();
	Brush::constructStatic( type );
	Face::m_quantise = quantiseFloating;

	Brush::m_maxWorldCoord = g_MaxWorldCoord;
	BrushInstance::m_counter = &g_brushCount;

	g_texdef_default_scale = 0.5f;
	const char* value = g_pGameDescription->getKeyValue( "default_scale" );
	if ( !string_empty( value ) ) {
		float scale = static_cast<float>( atof( value ) );
		if ( scale != 0 ) {
			g_texdef_default_scale = scale;
		}
		else
		{
			globalErrorStream() << "error parsing \"default_scale\" attribute\n";
		}
	}

	GlobalPreferenceSystem().registerPreference( "TextureLock", BoolImportStringCaller( g_brush_texturelock_enabled ), BoolExportStringCaller( g_brush_texturelock_enabled ) );
	GlobalPreferenceSystem().registerPreference( "TextureVertexLock", BoolImportStringCaller( g_brush_textureVertexlock_enabled ), BoolExportStringCaller( g_brush_textureVertexlock_enabled ) );
	GlobalPreferenceSystem().registerPreference( "BrushSnapPlanes", makeBoolStringImportCallback( FaceImportSnapPlanesCaller() ), makeBoolStringExportCallback( FaceExportSnapPlanesCaller() ) );
	GlobalPreferenceSystem().registerPreference( "TexdefDefaultScale", FloatImportStringCaller( g_texdef_default_scale ), FloatExportStringCaller( g_texdef_default_scale ) );

	GridStatus_getTextureLockEnabled = getTextureLockEnabled;
	GridStatus_getTexdefTypeIdLabel = getTexdefTypeIdLabel;
	g_texture_lock_status_changed = FreeCaller<GridStatus_changed>();

	Clipper_Construct();
}

void Brush_Destroy(){
	Clipper_Destroy();

	Brush::m_maxWorldCoord = 0;
	BrushInstance::m_counter = 0;

	Brush::destroyStatic();
	BrushInstance::destroyStatic();
	BrushClipPlane::destroyStatic();
}

void Brush_clipperColourChanged(){
	BrushClipPlane::destroyStatic();
	BrushClipPlane::constructStatic();
}

void BrushFaceData_fromFace( const BrushFaceDataCallback& callback, Face& face ){
	_QERFaceData faceData;
	faceData.m_p0 = face.getPlane().planePoints()[0];
	faceData.m_p1 = face.getPlane().planePoints()[1];
	faceData.m_p2 = face.getPlane().planePoints()[2];
	faceData.m_shader = face.GetShader();
	faceData.m_texdef = face.getTexdef().m_projection.m_texdef;
	faceData.contents = face.getShader().m_flags.m_contentFlags;
	faceData.flags = face.getShader().m_flags.m_surfaceFlags;
	faceData.value = face.getShader().m_flags.m_value;
	callback( faceData );
}
typedef ConstReferenceCaller1<BrushFaceDataCallback, Face&, BrushFaceData_fromFace> BrushFaceDataFromFaceCaller;
typedef Callback1<Face&> FaceCallback;

class Quake3BrushCreator : public BrushCreator
{
public:
scene::Node& createBrush(){
	return ( new BrushNode )->node();
}
EBrushType getFormat() const {
	return Brush::m_type;
}
void toggleFormat( EBrushType type ) const {
	Brush_toggleFormat( type );
}
void Brush_forEachFace( scene::Node& brush, const BrushFaceDataCallback& callback ){
	::Brush_forEachFace( *Node_getBrush( brush ), FaceCallback( BrushFaceDataFromFaceCaller( callback ) ) );
}
bool Brush_addFace( scene::Node& brush, const _QERFaceData& faceData ){
	Node_getBrush( brush )->undoSave();
	return Node_getBrush( brush )->addPlane( faceData.m_p0, faceData.m_p1, faceData.m_p2, faceData.m_shader, TextureProjection( faceData.m_texdef, brushprimit_texdef_t(), Vector3( 0, 0, 0 ), Vector3( 0, 0, 0 ) ) ) != 0;
}
};

Quake3BrushCreator g_Quake3BrushCreator;

BrushCreator& GetBrushCreator(){
	return g_Quake3BrushCreator;
}

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"


class BrushDependencies :
	public GlobalRadiantModuleRef,
	public GlobalSceneGraphModuleRef,
	public GlobalShaderCacheModuleRef,
	public GlobalSelectionModuleRef,
	public GlobalOpenGLModuleRef,
	public GlobalUndoModuleRef,
	public GlobalFilterModuleRef
{
};

class BrushDoom3API : public TypeSystemRef
{
BrushCreator* m_brushdoom3;
public:
typedef BrushCreator Type;
STRING_CONSTANT( Name, "doom3" );

BrushDoom3API(){
	Brush_Construct( eBrushTypeDoom3 );

	m_brushdoom3 = &GetBrushCreator();
}
~BrushDoom3API(){
	Brush_Destroy();
}
BrushCreator* getTable(){
	return m_brushdoom3;
}
};

typedef SingletonModule<BrushDoom3API, BrushDependencies> BrushDoom3Module;
typedef Static<BrushDoom3Module> StaticBrushDoom3Module;
StaticRegisterModule staticRegisterBrushDoom3( StaticBrushDoom3Module::instance() );


class BrushQuake4API : public TypeSystemRef
{
BrushCreator* m_brushquake4;
public:
typedef BrushCreator Type;
STRING_CONSTANT( Name, "quake4" );

BrushQuake4API(){
	Brush_Construct( eBrushTypeQuake4 );

	m_brushquake4 = &GetBrushCreator();
}
~BrushQuake4API(){
	Brush_Destroy();
}
BrushCreator* getTable(){
	return m_brushquake4;
}
};

typedef SingletonModule<BrushQuake4API, BrushDependencies> BrushQuake4Module;
typedef Static<BrushQuake4Module> StaticBrushQuake4Module;
StaticRegisterModule staticRegisterBrushQuake4( StaticBrushQuake4Module::instance() );


class BrushQuake3API : public TypeSystemRef
{
BrushCreator* m_brushquake3;
public:
typedef BrushCreator Type;
STRING_CONSTANT( Name, "quake3" );

BrushQuake3API(){
	g_multipleBrushTypes = true;
	g_brushTypes[0] = eBrushTypeQuake3;
	g_brushTypes[1] = eBrushTypeQuake3BP;
	g_brushTypes[2] = eBrushTypeQuake3Valve220;

	Brush_Construct( eBrushTypeQuake3BP );

	m_brushquake3 = &GetBrushCreator();
}
~BrushQuake3API(){
	Brush_Destroy();
}
BrushCreator* getTable(){
	return m_brushquake3;
}
};

typedef SingletonModule<BrushQuake3API, BrushDependencies> BrushQuake3Module;
typedef Static<BrushQuake3Module> StaticBrushQuake3Module;
StaticRegisterModule staticRegisterBrushQuake3( StaticBrushQuake3Module::instance() );


class BrushQuake2API : public TypeSystemRef
{
BrushCreator* m_brushquake2;
public:
typedef BrushCreator Type;
STRING_CONSTANT( Name, "quake2" );

BrushQuake2API(){
	g_multipleBrushTypes = true;
	g_brushTypes[0] = eBrushTypeQuake2;
	g_brushTypes[1] = eBrushTypeQuake3BP;
	g_brushTypes[2] = eBrushTypeQuake3Valve220;

	Brush_Construct( eBrushTypeQuake2 );

	m_brushquake2 = &GetBrushCreator();
}
~BrushQuake2API(){
	Brush_Destroy();
}
BrushCreator* getTable(){
	return m_brushquake2;
}
};

typedef SingletonModule<BrushQuake2API, BrushDependencies> BrushQuake2Module;
typedef Static<BrushQuake2Module> StaticBrushQuake2Module;
StaticRegisterModule staticRegisterBrushQuake2( StaticBrushQuake2Module::instance() );


class BrushQuake1API : public TypeSystemRef
{
BrushCreator* m_brushquake1;
public:
typedef BrushCreator Type;
STRING_CONSTANT( Name, "quake" );

BrushQuake1API(){
	g_multipleBrushTypes = true;
	g_brushTypes[0] = eBrushTypeQuake;
	g_brushTypes[1] = eBrushTypeQuake3BP;
	g_brushTypes[2] = eBrushTypeValve220;

	Brush_Construct( eBrushTypeQuake );

	m_brushquake1 = &GetBrushCreator();
}
~BrushQuake1API(){
	Brush_Destroy();
}
BrushCreator* getTable(){
	return m_brushquake1;
}
};

typedef SingletonModule<BrushQuake1API, BrushDependencies> BrushQuake1Module;
typedef Static<BrushQuake1Module> StaticBrushQuake1Module;
StaticRegisterModule staticRegisterBrushQuake1( StaticBrushQuake1Module::instance() );


class BrushHalfLifeAPI : public TypeSystemRef
{
BrushCreator* m_brushhalflife;
public:
typedef BrushCreator Type;
STRING_CONSTANT( Name, "halflife" );

BrushHalfLifeAPI(){
	Brush_Construct( eBrushTypeValve220 );

	m_brushhalflife = &GetBrushCreator();
}
~BrushHalfLifeAPI(){
	Brush_Destroy();
}
BrushCreator* getTable(){
	return m_brushhalflife;
}
};

typedef SingletonModule<BrushHalfLifeAPI, BrushDependencies> BrushHalfLifeModule;
typedef Static<BrushHalfLifeModule> StaticBrushHalfLifeModule;
StaticRegisterModule staticRegisterBrushHalfLife( StaticBrushHalfLifeModule::instance() );
