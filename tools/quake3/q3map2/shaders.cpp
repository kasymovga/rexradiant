/* -------------------------------------------------------------------------------

   Copyright (C) 1999-2007 id Software, Inc. and contributors.
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

   ----------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */



/* dependencies */
#include "q3map2.h"



/*
   ColorMod()
   routines for dealing with vertex color/alpha modification
 */

void ColorMod( colorMod_t *cm, int numVerts, bspDrawVert_t *drawVerts ){
	int i, j;
	float c;
	Color4f mult, add;
	bspDrawVert_t   *dv;
	colorMod_t      *cm2;


	/* dummy check */
	if ( cm == NULL || numVerts < 1 || drawVerts == NULL ) {
		return;
	}


	/* walk vertex list */
	for ( i = 0; i < numVerts; i++ )
	{
		/* get vertex */
		dv = &drawVerts[ i ];

		/* walk colorMod list */
		for ( cm2 = cm; cm2 != NULL; cm2 = cm2->next )
		{
			/* default */
			mult.set( 1 );
			add.set( 0 );

			const Vector3 cm2_vec3 = vector3_from_array( cm2->data );
			/* switch on type */
			switch ( cm2->type )
			{
			case EColorMod::ColorSet:
				mult.rgb().set( 0 );
				add.rgb() = cm2_vec3 * 255.0f;
				break;

			case EColorMod::AlphaSet:
				mult.alpha() = 0.0f;
				add.alpha() = cm2->data[ 0 ] * 255.0f;
				break;

			case EColorMod::ColorScale:
				mult.rgb() = cm2_vec3;
				break;

			case EColorMod::AlphaScale:
				mult.alpha() = cm2->data[ 0 ];
				break;

			case EColorMod::ColorDotProduct:
				c = vector3_dot( dv->normal, cm2_vec3 );
				mult.rgb().set( c );
				break;

			case EColorMod::ColorDotProductScale:
				c = vector3_dot( dv->normal, cm2_vec3 );
				c = ( c - cm2->data[3] ) / ( cm2->data[4] - cm2->data[3] );
				mult.rgb().set( c );
				break;

			case EColorMod::AlphaDotProduct:
				mult.alpha() = vector3_dot( dv->normal, cm2_vec3 );
				break;

			case EColorMod::AlphaDotProductScale:
				c = vector3_dot( dv->normal, cm2_vec3 );
				c = ( c - cm2->data[3] ) / ( cm2->data[4] - cm2->data[3] );
				mult.alpha() = c;
				break;

			case EColorMod::ColorDotProduct2:
				c = vector3_dot( dv->normal, cm2_vec3 );
				c *= c;
				mult.rgb().set( c );
				break;

			case EColorMod::ColorDotProduct2Scale:
				c = vector3_dot( dv->normal, cm2_vec3 );
				c *= c;
				c = ( c - cm2->data[3] ) / ( cm2->data[4] - cm2->data[3] );
				mult.rgb().set( c );
				break;

			case EColorMod::AlphaDotProduct2:
				mult.alpha() = vector3_dot( dv->normal, cm2_vec3 );
				mult.alpha() *= mult.alpha();
				break;

			case EColorMod::AlphaDotProduct2Scale:
				c = vector3_dot( dv->normal, cm2_vec3 );
				c *= c;
				c = ( c - cm2->data[3] ) / ( cm2->data[4] - cm2->data[3] );
				mult.alpha() = c;
				break;

			default:
				break;
			}

			/* apply mod */
			for ( j = 0; j < MAX_LIGHTMAPS; j++ )
			{
				dv->color[ j ] = color_to_byte( mult * static_cast<BasicVector4<byte>>( dv->color[ j ] ) + add );
			}
		}
	}
}



/*
   TCMod*()
   routines for dealing with a 3x3 texture mod matrix
 */

void TCMod( const tcMod_t& mod, Vector2& st ){
	const Vector2 old = st;

	st[ 0 ] = ( mod[ 0 ][ 0 ] * old[ 0 ] ) + ( mod[ 0 ][ 1 ] * old[ 1 ] ) + mod[ 0 ][ 2 ];
	st[ 1 ] = ( mod[ 1 ][ 0 ] * old[ 0 ] ) + ( mod[ 1 ][ 1 ] * old[ 1 ] ) + mod[ 1 ][ 2 ];
}


void TCModIdentity( tcMod_t& mod ){
	mod[ 0 ][ 0 ] = 1.0f;   mod[ 0 ][ 1 ] = 0.0f;   mod[ 0 ][ 2 ] = 0.0f;
	mod[ 1 ][ 0 ] = 0.0f;   mod[ 1 ][ 1 ] = 1.0f;   mod[ 1 ][ 2 ] = 0.0f;
	mod[ 2 ][ 0 ] = 0.0f;   mod[ 2 ][ 1 ] = 0.0f;   mod[ 2 ][ 2 ] = 1.0f;   /* this row is only used for multiples, not transformation */
}


void TCModMultiply( const tcMod_t& a, const tcMod_t& b, tcMod_t& out ){
	for ( int i = 0; i < 3; i++ )
	{
		out[ i ][ 0 ] = ( a[ i ][ 0 ] * b[ 0 ][ 0 ] ) + ( a[ i ][ 1 ] * b[ 1 ][ 0 ] ) + ( a[ i ][ 2 ] * b[ 2 ][ 0 ] );
		out[ i ][ 1 ] = ( a[ i ][ 0 ] * b[ 0 ][ 1 ] ) + ( a[ i ][ 1 ] * b[ 1 ][ 1 ] ) + ( a[ i ][ 2 ] * b[ 2 ][ 1 ] );
		out[ i ][ 2 ] = ( a[ i ][ 0 ] * b[ 0 ][ 2 ] ) + ( a[ i ][ 1 ] * b[ 1 ][ 2 ] ) + ( a[ i ][ 2 ] * b[ 2 ][ 2 ] );
	}
}


void TCModTranslate( tcMod_t& mod, float s, float t ){
	mod[ 0 ][ 2 ] += s;
	mod[ 1 ][ 2 ] += t;
}


void TCModScale( tcMod_t& mod, float s, float t ){
	mod[ 0 ][ 0 ] *= s;
	mod[ 1 ][ 1 ] *= t;
}


void TCModRotate( tcMod_t& mod, float euler ){
	tcMod_t old, temp;
	float radians, sinv, cosv;


	memcpy( old, mod, sizeof( tcMod_t ) );
	TCModIdentity( temp );

	radians = degrees_to_radians( euler );
	sinv = sin( radians );
	cosv = cos( radians );

	temp[ 0 ][ 0 ] = cosv;  temp[ 0 ][ 1 ] = -sinv;
	temp[ 1 ][ 0 ] = sinv;  temp[ 1 ][ 1 ] = cosv;

	TCModMultiply( old, temp, mod );
}



/*
   ApplySurfaceParm() - ydnar
   applies a named surfaceparm to the supplied flags
 */

bool ApplySurfaceParm( const char *name, int *contentFlags, int *surfaceFlags, int *compileFlags ){
	int i, fake;
	surfaceParm_t   *sp;


	/* dummy check */
	if ( name == NULL ) {
		name = "";
	}
	if ( contentFlags == NULL ) {
		contentFlags = &fake;
	}
	if ( surfaceFlags == NULL ) {
		surfaceFlags = &fake;
	}
	if ( compileFlags == NULL ) {
		compileFlags = &fake;
	}

	/* walk the current game's surfaceparms */
	sp = game->surfaceParms;
	while ( sp->name != NULL )
	{
		/* match? */
		if ( striEqual( name, sp->name ) ) {
			/* clear and set flags */
			*contentFlags &= ~( sp->contentFlagsClear );
			*contentFlags |= sp->contentFlags;
			*surfaceFlags &= ~( sp->surfaceFlagsClear );
			*surfaceFlags |= sp->surfaceFlags;
			*compileFlags &= ~( sp->compileFlagsClear );
			*compileFlags |= sp->compileFlags;

			/* return ok */
			return true;
		}

		/* next */
		sp++;
	}

	/* check custom info parms */
	for ( i = 0; i < numCustSurfaceParms; i++ )
	{
		/* get surfaceparm */
		sp = &custSurfaceParms[ i ];

		/* match? */
		if ( striEqual( name, sp->name ) ) {
			/* clear and set flags */
			*contentFlags &= ~( sp->contentFlagsClear );
			*contentFlags |= sp->contentFlags;
			*surfaceFlags &= ~( sp->surfaceFlagsClear );
			*surfaceFlags |= sp->surfaceFlags;
			*compileFlags &= ~( sp->compileFlagsClear );
			*compileFlags |= sp->compileFlags;

			/* return ok */
			return true;
		}
	}

	/* no matching surfaceparm found */
	return false;
}



/*
   BeginMapShaderFile() - ydnar
   erases and starts a new map shader script
 */

void BeginMapShaderFile( const char *mapFile ){
	/* dummy check */
	mapName.clear();
	mapShaderFile = "";
	if ( strEmptyOrNull( mapFile ) ) {
		return;
	}

	/* extract map name */
	mapName( PathFilename( mapFile ) );

	/* append ../scripts/q3map2_<mapname>.shader */
	mapShaderFile = StringOutputStream( 256 )( PathFilenameless( mapFile ), "../", game->shaderPath, "/q3map2_", mapName.c_str(), ".shader" );
	Sys_FPrintf( SYS_VRB, "Map has shader script %s\n", mapShaderFile.c_str() );

	/* remove it */
	remove( mapShaderFile.c_str() );

	/* stop making warnings about missing images */
	warnImage = false;
}



/*
   WriteMapShaderFile() - ydnar
   writes a shader to the map shader script
 */

void WriteMapShaderFile( void ){
	FILE            *file;
	shaderInfo_t    *si;
	int i, num;


	/* dummy check */
	if ( mapShaderFile.empty() ) {
		return;
	}

	/* are there any custom shaders? */
	for ( i = 0, num = 0; i < numShaderInfo; i++ )
	{
		if ( shaderInfo[ i ].custom ) {
			break;
		}
	}
	if ( i == numShaderInfo ) {
		return;
	}

	/* note it */
	Sys_FPrintf( SYS_VRB, "--- WriteMapShaderFile ---\n" );
	Sys_FPrintf( SYS_VRB, "Writing %s", mapShaderFile.c_str() );

	/* open shader file */
	file = fopen( mapShaderFile.c_str(), "wt" );
	if ( file == NULL ) {
		Sys_Warning( "Unable to open map shader file %s for writing\n", mapShaderFile.c_str() );
		return;
	}

	/* print header */
	fprintf( file,
	         "// Custom shader file for %s.bsp\n"
	         "// Generated by Q3Map2 (ydnar)\n"
	         "// Do not edit! This file is overwritten on recompiles.\n\n",
	         mapName.c_str() );

	/* walk the shader list */
	for ( i = 0, num = 0; i < numShaderInfo; i++ )
	{
		/* get the shader and print it */
		si = &shaderInfo[ i ];
		if ( !si->custom || strEmptyOrNull( si->shaderText ) ) {
			continue;
		}
		num++;

		/* print it to the file */
		fprintf( file, "%s%s\n", si->shader.c_str(), si->shaderText );
		//Sys_Printf( "%s%s\n", si->shader, si->shaderText ); /* FIXME: remove debugging code */

		Sys_FPrintf( SYS_VRB, "." );
	}

	/* close the shader */
	fflush( file );
	fclose( file );

	Sys_FPrintf( SYS_VRB, "\n" );

	/* print some stats */
	Sys_Printf( "%9d custom shaders emitted\n", num );
}



/*
   CustomShader() - ydnar
   sets up a custom map shader
 */

shaderInfo_t *CustomShader( shaderInfo_t *si, const char *find, char *replace ){
	shaderInfo_t    *csi;
	char shader[ MAX_QPATH ];
	char            *s;
	int loc;
	byte digest[ 16 ];
	char            *srcShaderText, temp[ 8192 ], shaderText[ 8192 ];   /* ydnar: fixme (make this bigger?) */


	/* dummy check */
	if ( si == NULL ) {
		return ShaderInfoForShader( "default" );
	}

	/* default shader text source */
	srcShaderText = si->shaderText;

	/* et: implicitMap */
	if ( si->implicitMap == EImplicitMap::Opaque ) {
		srcShaderText = temp;
		sprintf( temp, "\n"
		               "{ // Q3Map2 defaulted (implicitMap)\n"
		               "\t{\n"
		               "\t\tmap $lightmap\n"
		               "\t\trgbGen identity\n"
		               "\t}\n"
		               "\tq3map_styleMarker\n"
		               "\t{\n"
		               "\t\tmap %s\n"
		               "\t\tblendFunc GL_DST_COLOR GL_ZERO\n"
		               "\t\trgbGen identity\n"
		               "\t}\n"
		               "}\n",
		         si->implicitImagePath.c_str() );
	}

	/* et: implicitMask */
	else if ( si->implicitMap == EImplicitMap::Masked ) {
		srcShaderText = temp;
		sprintf( temp, "\n"
		               "{ // Q3Map2 defaulted (implicitMask)\n"
		               "\tcull none\n"
		               "\t{\n"
		               "\t\tmap %s\n"
		               "\t\talphaFunc GE128\n"
		               "\t\tdepthWrite\n"
		               "\t}\n"
		               "\t{\n"
		               "\t\tmap $lightmap\n"
		               "\t\trgbGen identity\n"
		               "\t\tdepthFunc equal\n"
		               "\t}\n"
		               "\tq3map_styleMarker\n"
		               "\t{\n"
		               "\t\tmap %s\n"
		               "\t\tblendFunc GL_DST_COLOR GL_ZERO\n"
		               "\t\tdepthFunc equal\n"
		               "\t\trgbGen identity\n"
		               "\t}\n"
		               "}\n",
		         si->implicitImagePath.c_str(),
		         si->implicitImagePath.c_str() );
	}

	/* et: implicitBlend */
	else if ( si->implicitMap == EImplicitMap::Blend ) {
		srcShaderText = temp;
		sprintf( temp, "\n"
		               "{ // Q3Map2 defaulted (implicitBlend)\n"
		               "\tcull none\n"
		               "\t{\n"
		               "\t\tmap %s\n"
		               "\t\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n"
		               "\t}\n"
		               "\t{\n"
		               "\t\tmap $lightmap\n"
		               "\t\trgbGen identity\n"
		               "\t\tblendFunc GL_DST_COLOR GL_ZERO\n"
		               "\t}\n"
		               "\tq3map_styleMarker\n"
		               "}\n",
		         si->implicitImagePath.c_str() );
	}

	/* default shader text */
	else if ( srcShaderText == NULL ) {
		srcShaderText = temp;
		sprintf( temp, "\n"
		               "{ // Q3Map2 defaulted\n"
		               "\t{\n"
		               "\t\tmap $lightmap\n"
		               "\t\trgbGen identity\n"
		               "\t}\n"
		               "\tq3map_styleMarker\n"
		               "\t{\n"
		               "\t\tmap %s.tga\n"
		               "\t\tblendFunc GL_DST_COLOR GL_ZERO\n"
		               "\t\trgbGen identity\n"
		               "\t}\n"
		               "}\n",
		         si->shader.c_str() );
	}

	/* error check */
	if ( ( strlen( mapName ) + 1 + 32 ) >= MAX_QPATH ) {
		Error( "Custom shader name length (%d) exceeded. Shorten your map name.\n", MAX_QPATH - 1 );
	}

	/* do some bad find-replace */
	s = strIstr( srcShaderText, find );
	if ( s == NULL ) {
		//%	strcpy( shaderText, srcShaderText );
		return si;  /* testing just using the existing shader if this fails */
	}
	else
	{
		/* substitute 'find' with 'replace' */
		loc = s - srcShaderText;
		strcpy( shaderText, srcShaderText );
		shaderText[ loc ] = '\0';
		strcat( shaderText, replace );
		strcat( shaderText, &srcShaderText[ loc + strlen( find ) ] );
	}

	/* make md4 hash of the shader text */
	Com_BlockFullChecksum( shaderText, strlen( shaderText ), digest );

	/* mangle hash into a shader name */
	sprintf( shader, "%s/%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", mapName.c_str(),
	         digest[ 0 ], digest[ 1 ], digest[ 2 ], digest[ 3 ], digest[ 4 ], digest[ 5 ], digest[ 6 ], digest[ 7 ],
	         digest[ 8 ], digest[ 9 ], digest[ 10 ], digest[ 11 ], digest[ 12 ], digest[ 13 ], digest[ 14 ], digest[ 15 ] );

	/* get shader */
	csi = ShaderInfoForShader( shader );

	/* might be a preexisting shader */
	if ( csi->custom ) {
		return csi;
	}

	/* clone the existing shader and rename */
	*csi = *si;
	csi->shader = shader;
	csi->custom = true;

	/* store new shader text */
	csi->shaderText = copystring( shaderText );  /* LEAK! */

	/* return it */
	return csi;
}



/*
   EmitVertexRemapShader()
   adds a vertexremapshader key/value pair to worldspawn
 */

void EmitVertexRemapShader( char *from, char *to ){
	byte digest[ 16 ];
	char key[ 64 ], value[ 256 ];


	/* dummy check */
	if ( strEmptyOrNull( from ) || strEmptyOrNull( to ) ) {
		return;
	}

	/* build value */
	sprintf( value, "%s;%s", from, to );

	/* make md4 hash */
	Com_BlockFullChecksum( value, strlen( value ), digest );

	/* make key (this is annoying, as vertexremapshader is precisely 17 characters,
	   which is one too long, so we leave off the last byte of the md5 digest) */
	sprintf( key, "vertexremapshader%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
	         digest[ 0 ], digest[ 1 ], digest[ 2 ], digest[ 3 ], digest[ 4 ], digest[ 5 ], digest[ 6 ], digest[ 7 ],
	         digest[ 8 ], digest[ 9 ], digest[ 10 ], digest[ 11 ], digest[ 12 ], digest[ 13 ], digest[ 14 ] ); /* no: digest[ 15 ] */

	/* add key/value pair to worldspawn */
	entities[ 0 ].setKeyValue( key, value );
}



/*
   AllocShaderInfo()
   allocates and initializes a new shader
 */

static shaderInfo_t *AllocShaderInfo( void ){
	shaderInfo_t    *si;


	/* allocate? */
	if ( shaderInfo == NULL ) {
		shaderInfo = safe_malloc( sizeof( shaderInfo_t ) * MAX_SHADER_INFO );
		numShaderInfo = 0;
	}

	/* bounds check */
	if ( numShaderInfo == MAX_SHADER_INFO ) {
		Error( "MAX_SHADER_INFO exceeded. Remove some PK3 files or shader scripts from shaderlist.txt and try again." );
	}
	si = &shaderInfo[ numShaderInfo ];
	numShaderInfo++;

	/* ydnar: clear to 0 first */
//	memset( si, 0, sizeof( shaderInfo_t ) );
	new (si) shaderInfo_t{}; // placement new

	/* set defaults */
	ApplySurfaceParm( "default", &si->contentFlags, &si->surfaceFlags, &si->compileFlags );

	si->backsplashFraction = DEF_BACKSPLASH_FRACTION * g_backsplashFractionScale;
	si->backsplashDistance = g_backsplashDistance < -900.0f ? DEF_BACKSPLASH_DISTANCE : g_backsplashDistance;

	si->bounceScale = DEF_RADIOSITY_BOUNCE;

	si->lightStyle = LS_NORMAL;

	si->polygonOffset = false;

	si->shadeAngleDegrees = 0.0f;
	si->lightmapSampleSize = 0;
	si->lightmapSampleOffset = DEFAULT_LIGHTMAP_SAMPLE_OFFSET;
	si->patchShadows = false;
	si->vertexShadows = true;  /* ydnar: changed default behavior */
	si->forceSunlight = false;
	si->lmBrightness = lightmapBrightness;
	si->vertexScale = vertexglobalscale;
	si->notjunc = false;

	/* ydnar: set texture coordinate transform matrix to identity */
	TCModIdentity( si->mod );

	/* ydnar: lightmaps can now be > 128x128 in certain games or an externally generated tga */
	si->lmCustomWidth = lmCustomSizeW;
	si->lmCustomHeight = lmCustomSizeH;

	/* return to sender */
	return si;
}



/*
   FinishShader() - ydnar
   sets a shader's width and height among other things
 */

void FinishShader( shaderInfo_t *si ){
	int x, y;
	Vector2 st;


	/* don't double-dip */
	if ( si->finished ) {
		return;
	}

	/* if they're explicitly set, copy from image size */
	if ( si->shaderWidth == 0 && si->shaderHeight == 0 ) {
		si->shaderWidth = si->shaderImage->width;
		si->shaderHeight = si->shaderImage->height;
	}

	/* legacy terrain has explicit image-sized texture projection */
	if ( si->legacyTerrain && !si->tcGen ) {
		/* set xy texture projection */
		si->tcGen = true;
		si->vecs[ 0 ] = { ( 1.0f / ( si->shaderWidth * 0.5f ) ), 0, 0 };
		si->vecs[ 1 ] = { 0, ( 1.0f / ( si->shaderHeight * 0.5f ) ), 0 };
	}

	/* find pixel coordinates best matching the average color of the image */
	float bestDist = 99999999;
	const Vector2 o( 1.0f / si->shaderImage->width, 1.0f / si->shaderImage->height );
	for ( y = 0, st[ 1 ] = 0.0f; y < si->shaderImage->height; y++, st[ 1 ] += o[ 1 ] )
	{
		for ( x = 0, st[ 0 ] = 0.0f; x < si->shaderImage->width; x++, st[ 0 ] += o[ 0 ] )
		{
			/* sample the shader image */
			Color4f color;
			RadSampleImage( si->shaderImage->pixels, si->shaderImage->width, si->shaderImage->height, st, color );

			/* determine error squared */
			const Color4f delta = color - si->averageColor;
			const float dist = vector4_dot( delta, delta );
			if ( dist < bestDist ) {
				si->stFlat = st;
			}
		}
	}

	if( noob && !( si->compileFlags & C_OB ) ){
		ApplySurfaceParm( "noob", &si->contentFlags, &si->surfaceFlags, &si->compileFlags );
	}

	/* set to finished */
	si->finished = true;
}



/*
   LoadShaderImages()
   loads a shader's images
   ydnar: image.c made this a bit simpler
 */

static void LoadShaderImages( shaderInfo_t *si ){

	/* nodraw shaders don't need images */
	if ( si->compileFlags & C_NODRAW ) {
		si->shaderImage = ImageLoad( DEFAULT_IMAGE );
	}
	else
	{
		/* try to load editor image first */
		si->shaderImage = ImageLoad( si->editorImagePath );

		/* then try shadername */
		if ( si->shaderImage == NULL ) {
			si->shaderImage = ImageLoad( si->shader );
		}

		/* then try implicit image path (note: new behavior!) */
		if ( si->shaderImage == NULL ) {
			si->shaderImage = ImageLoad( si->implicitImagePath );
		}

		/* then try lightimage (note: new behavior!) */
		if ( si->shaderImage == NULL ) {
			si->shaderImage = ImageLoad( si->lightImagePath );
		}

		/* otherwise, use default image */
		if ( si->shaderImage == NULL ) {
			si->shaderImage = ImageLoad( DEFAULT_IMAGE );
			if ( warnImage && !strEqual( si->shader, "noshader" ) ) {
				Sys_Warning( "Couldn't find image for shader %s\n", si->shader.c_str() );
			}
		}

		/* load light image */
		si->lightImage = ImageLoad( si->lightImagePath );

		/* load normalmap image (ok if this is NULL) */
		si->normalImage = ImageLoad( si->normalImagePath );
		if ( si->normalImage != NULL ) {
			Sys_FPrintf( SYS_VRB, "Shader %s has\n"
			                      "    NM %s\n", si->shader.c_str(), si->normalImagePath.c_str() );
		}
	}

	/* if no light image, reuse shader image */
	if ( si->lightImage == NULL ) {
		si->lightImage = si->shaderImage;
		si->lightImage->refCount++;
	}

	/* create default and average colors */
	const int count = si->lightImage->width * si->lightImage->height;
	Color4f color( 0, 0, 0, 0 );
	for ( int i = 0; i < count; i++ )
	{
		color[ 0 ] += si->lightImage->pixels[ i * 4 + 0 ];
		color[ 1 ] += si->lightImage->pixels[ i * 4 + 1 ];
		color[ 2 ] += si->lightImage->pixels[ i * 4 + 2 ];
		color[ 3 ] += si->lightImage->pixels[ i * 4 + 3 ];
	}

	if ( vector3_length( si->color ) == 0.0f ) {
		si->color = color.rgb();
		ColorNormalize( si->color );
		si->averageColor = color / count;
	}
	else
	{
		si->averageColor.rgb() = si->color;
		si->averageColor.alpha() = 1.0f;
	}
}



/*
   ShaderInfoForShader()
   finds a shaderinfo for a named shader
 */

#define MAX_SHADER_DEPRECATION_DEPTH 16

shaderInfo_t *ShaderInfoForShaderNull( const char *shaderName ){
	if ( strEqual( shaderName, "noshader" ) ) {
		return NULL;
	}
	return ShaderInfoForShader( shaderName );
}

shaderInfo_t *ShaderInfoForShader( const char *shaderName ){
	int i;
	int deprecationDepth;
	shaderInfo_t    *si;

	/* dummy check */
	if ( strEmptyOrNull( shaderName ) ) {
		Sys_Warning( "Null or empty shader name\n" );
		shaderName = "missing";
	}

	/* strip off extension */
	auto shader = String64()( PathExtensionless( shaderName ) );

	/* search for it */
	deprecationDepth = 0;
	for ( i = 0; i < numShaderInfo; i++ )
	{
		si = &shaderInfo[ i ];
		if ( striEqual( shader, si->shader ) ) {
			/* check if shader is deprecated */
			if ( deprecationDepth < MAX_SHADER_DEPRECATION_DEPTH && !strEmptyOrNull( si->deprecateShader ) ) {
				/* override name */
				shader( PathExtensionless( si->deprecateShader ) );
				/* increase deprecation depth */
				deprecationDepth++;
				if ( deprecationDepth == MAX_SHADER_DEPRECATION_DEPTH ) {
					Sys_Warning( "Max deprecation depth of %i is reached on shader '%s'\n", MAX_SHADER_DEPRECATION_DEPTH, shader.c_str() );
				}
				/* search again from beginning */
				i = -1;
				continue;
			}

			/* load image if necessary */
			if ( !si->finished ) {
				LoadShaderImages( si );
				FinishShader( si );
			}

			/* return it */
			return si;
		}
	}

	/* allocate a default shader */
	si = AllocShaderInfo();
	si->shader = shader;
	LoadShaderImages( si );
	FinishShader( si );

	/* return it */
	return si;
}



/*
   GetTokenAppend() - ydnar
   gets a token and appends its text to the specified buffer
 */

static int oldScriptLine = 0;
static int tabDepth = 0;

bool GetTokenAppend( char *buffer, bool crossline ){
	bool r;
	int i;


	/* get the token */
	r = GetToken( crossline );
	if ( !r || buffer == NULL || strEmpty( token ) ) {
		return r;
	}

	/* pre-tabstops */
	if ( token[ 0 ] == '}' ) {
		tabDepth--;
	}

	/* append? */
	if ( oldScriptLine != scriptline ) {
		strcat( buffer, "\n" );
		for ( i = 0; i < tabDepth; i++ )
			strcat( buffer, "\t" );
	}
	else{
		strcat( buffer, " " );
	}
	oldScriptLine = scriptline;
	strcat( buffer, token );

	/* post-tabstops */
	if ( token[ 0 ] == '{' ) {
		tabDepth++;
	}

	/* return */
	return r;
}


void Parse1DMatrixAppend( char *buffer, int x, float *m ){

	if ( !GetTokenAppend( buffer, true ) || !strEqual( token, "(" ) ) {
		Error( "Parse1DMatrixAppend(): line %d: ( not found!\nFile location be: %s\n", scriptline, g_strLoadedFileLocation );
	}
	for ( int i = 0; i < x; i++ )
	{
		if ( !GetTokenAppend( buffer, false ) ) {
			Error( "Parse1DMatrixAppend(): line %d: Number not found!\nFile location be: %s\n", scriptline, g_strLoadedFileLocation );
		}
		m[ i ] = atof( token );
	}
	if ( !GetTokenAppend( buffer, true ) || !strEqual( token, ")" ) ) {
		Error( "Parse1DMatrixAppend(): line %d: ) not found!\nFile location be: %s\n", scriptline, g_strLoadedFileLocation );
	}
}




/*
   ParseShaderFile()
   parses a shader file into discrete shaderInfo_t
 */

static void ParseShaderFile( const char *filename ){
	int i;
	shaderInfo_t    *si;
	char shaderText[ 8192 ];            /* ydnar: fixme (make this bigger?) */


	/* init */
	si = NULL;
	strClear( shaderText );

	/* load the shader */
	LoadScriptFile( filename, 0 );

	/* tokenize it */
	while ( 1 )
	{
		/* copy shader text to the shaderinfo */
		if ( si != NULL && !strEmpty( shaderText ) ) {
			strcat( shaderText, "\n" );
			si->shaderText = copystring( shaderText );
			//%	if( vector3_length( si->vecs[ 0 ] ) )
			//%		Sys_Printf( "%s\n", shaderText );
		}

		/* ydnar: clear shader text buffer */
		strClear( shaderText );

		/* test for end of file */
		if ( !GetToken( true ) ) {
			break;
		}

		/* shader name is initial token */
		si = AllocShaderInfo();

		/* ignore ":q3map" suffix */
		if( striEqualSuffix( token, ":q3map" ) )
			si->shader << StringRange( token, token + strlen( token ) - strlen( ":q3map" ) );
		else
			si->shader << token;

		/* handle { } section */
		if ( !GetTokenAppend( shaderText, true ) ) {
			break;
		}
		if ( !strEqual( token, "{" ) ) {
			if ( si != NULL ) {
				Error( "ParseShaderFile(): %s, line %d: { not found!\nFound instead: %s\nLast known shader: %s\nFile location be: %s\n",
				       filename, scriptline, token, si->shader.c_str(), g_strLoadedFileLocation );
			}
			else{
				Error( "ParseShaderFile(): %s, line %d: { not found!\nFound instead: %s\nFile location be: %s\n",
				       filename, scriptline, token, g_strLoadedFileLocation );
			}
		}

		while ( 1 )
		{
			/* get the next token */
			if ( !GetTokenAppend( shaderText, true ) ) {
				break;
			}
			if ( strEqual( token, "}" ) ) {
				break;
			}


			/* -----------------------------------------------------------------
			   shader stages (passes)
			   ----------------------------------------------------------------- */

			/* parse stage directives */
			if ( strEqual( token, "{" ) ) {
				si->hasPasses = true;
				while ( 1 )
				{
					if ( !GetTokenAppend( shaderText, true ) ) {
						break;
					}
					if ( strEqual( token, "}" ) ) {
						break;
					}

					/* only care about images if we don't have a editor/light image */
					if ( si->editorImagePath.empty() && si->lightImagePath.empty() && si->implicitImagePath.empty() ) {
						/* digest any images */
						if ( striEqual( token, "map" ) ||
						     striEqual( token, "clampMap" ) ||
						     striEqual( token, "animMap" ) ||
						     striEqual( token, "clampAnimMap" ) ||
						     striEqual( token, "mapComp" ) ||
						     striEqual( token, "mapNoComp" ) ) {
							/* skip one token for animated stages */
							if ( striEqual( token, "animMap" ) || striEqual( token, "clampAnimMap" ) ) {
								GetTokenAppend( shaderText, false );
							}

							/* get an image */
							GetTokenAppend( shaderText, false );
							if ( token[ 0 ] != '*' && token[ 0 ] != '$' ) {
								si->lightImagePath( PathExtensionless( token ) );

								/* debug code */
								//%	Sys_FPrintf( SYS_VRB, "Deduced shader image: %s\n", si->lightImagePath );
							}
						}
					}
				}
			}


			/* -----------------------------------------------------------------
			   surfaceparm * directives
			   ----------------------------------------------------------------- */

			/* match surfaceparm */
			else if ( striEqual( token, "surfaceparm" ) ) {
				GetTokenAppend( shaderText, false );
				if ( !ApplySurfaceParm( token, &si->contentFlags, &si->surfaceFlags, &si->compileFlags ) ) {
					Sys_Warning( "Unknown surfaceparm: \"%s\"\n", token );
				}
			}


			/* -----------------------------------------------------------------
			   game-related shader directives
			   ----------------------------------------------------------------- */

			/* ydnar: fogparms (for determining fog volumes) */
			else if ( striEqual( token, "fogparms" ) ) {
				si->fogParms = true;
			}

			/* ydnar: polygonoffset (for no culling) */
			else if ( striEqual( token, "polygonoffset" ) ) {
				si->polygonOffset = true;
			}

			/* tesssize is used to force liquid surfaces to subdivide */
			else if ( striEqual( token, "tessSize" ) || striEqual( token, "q3map_tessSize" ) /* sof2 */ ) {
				GetTokenAppend( shaderText, false );
				si->subdivisions = atof( token );
			}

			/* cull none will set twoSided (ydnar: added disable too) */
			else if ( striEqual( token, "cull" ) ) {
				GetTokenAppend( shaderText, false );
				if ( striEqual( token, "none" ) || striEqual( token, "disable" ) || striEqual( token, "twosided" ) ) {
					si->twoSided = true;
				}
			}

			/* deformVertexes autosprite[ 2 ]
			   we catch this so autosprited surfaces become point
			   lights instead of area lights */
			else if ( striEqual( token, "deformVertexes" ) ) {
				GetTokenAppend( shaderText, false );

				/* deformVertexes autosprite(2) */
				if ( striEqualPrefix( token, "autosprite" ) ) {
					/* set it as autosprite and detail */
					si->autosprite = true;
					ApplySurfaceParm( "detail", &si->contentFlags, &si->surfaceFlags, &si->compileFlags );

					/* ydnar: gs mods: added these useful things */
					si->noClip = true;
					si->notjunc = true;
				}

				/* deformVertexes move <x> <y> <z> <func> <base> <amplitude> <phase> <freq> (ydnar: for particle studio support) */
				if ( striEqual( token, "move" ) ) {
					Vector3 amt;
					float base, amp;


					/* get move amount */
					GetTokenAppend( shaderText, false );   amt[ 0 ] = atof( token );
					GetTokenAppend( shaderText, false );   amt[ 1 ] = atof( token );
					GetTokenAppend( shaderText, false );   amt[ 2 ] = atof( token );

					/* skip func */
					GetTokenAppend( shaderText, false );

					/* get base and amplitude */
					GetTokenAppend( shaderText, false );   base = atof( token );
					GetTokenAppend( shaderText, false );   amp = atof( token );

					/* calculate */
					si->minmax.mins = amt * base;
					si->minmax.maxs = amt * amp + si->minmax.mins;
				}
			}

			/* light <value> (old-style flare specification) */
			else if ( striEqual( token, "light" ) ) {
				GetTokenAppend( shaderText, false );
				si->flareShader = game->flareShader;
			}

			/* ydnar: damageShader <shader> <health> (sof2 mods) */
			else if ( striEqual( token, "damageShader" ) ) {
				GetTokenAppend( shaderText, false );
				if ( !strEmpty( token ) ) {
					si->damageShader = copystring( token );
				}
				GetTokenAppend( shaderText, false );   /* don't do anything with health */
			}

			/* ydnar: enemy territory implicit shaders */
			else if ( striEqual( token, "implicitMap" ) ) {
				si->implicitMap = EImplicitMap::Opaque;
				GetTokenAppend( shaderText, false );
				if ( strEqual( token, "-" ) ) {
					si->implicitImagePath = si->shader;
				}
				else{
					si->implicitImagePath = token;
				}
			}

			else if ( striEqual( token, "implicitMask" ) ) {
				si->implicitMap = EImplicitMap::Masked;
				GetTokenAppend( shaderText, false );
				if ( strEqual( token, "-" ) ) {
					si->implicitImagePath = si->shader;
				}
				else{
					si->implicitImagePath = token;
				}
			}

			else if ( striEqual( token, "implicitBlend" ) ) {
				si->implicitMap = EImplicitMap::Blend;
				GetTokenAppend( shaderText, false );
				if ( strEqual( token, "-" ) ) {
					si->implicitImagePath = si->shader;
				}
				else{
					si->implicitImagePath = token;
				}
			}


			/* -----------------------------------------------------------------
			   image directives
			   ----------------------------------------------------------------- */

			/* qer_editorimage <image> */
			else if ( striEqual( token, "qer_editorImage" ) ) {
				GetTokenAppend( shaderText, false );
				si->editorImagePath( PathExtensionless( token ) );
			}

			/* ydnar: q3map_normalimage <image> (bumpmapping normal map) */
			else if ( striEqual( token, "q3map_normalImage" ) ) {
				GetTokenAppend( shaderText, false );
				si->normalImagePath( PathExtensionless( token ) );
			}

			/* q3map_lightimage <image> */
			else if ( striEqual( token, "q3map_lightImage" ) ) {
				GetTokenAppend( shaderText, false );
				si->lightImagePath( PathExtensionless( token ) );
			}

			/* ydnar: skyparms <outer image> <cloud height> <inner image> */
			else if ( striEqual( token, "skyParms" ) ) {
				/* get image base */
				GetTokenAppend( shaderText, false );

				/* ignore bogus paths */
				if ( !strEqual( token, "-" ) && !striEqual( token, "full" ) ) {
					si->skyParmsImageBase = token;

					/* use top image as sky light image */
					if ( si->lightImagePath.empty() ) {
						si->lightImagePath( si->skyParmsImageBase, "_up" );
					}
				}

				/* skip rest of line */
				GetTokenAppend( shaderText, false );
				GetTokenAppend( shaderText, false );
			}

			/* -----------------------------------------------------------------
			   q3map_* directives
			   ----------------------------------------------------------------- */

			/* q3map_sun <red> <green> <blue> <intensity> <degrees> <elevation>
			   color will be normalized, so it doesn't matter what range you use
			   intensity falls off with angle but not distance 100 is a fairly bright sun
			   degree of 0 = from the east, 90 = north, etc.  altitude of 0 = sunrise/set, 90 = noon
			   ydnar: sof2map has bareword 'sun' token, so we support that as well */
			else if ( striEqual( token, "sun" ) /* sof2 */ || striEqual( token, "q3map_sun" ) || striEqual( token, "q3map_sunExt" ) ) {
				sun_t       *sun;
				/* ydnar: extended sun directive? */
				const bool ext = striEqual( token, "q3map_sunext" );

				/* allocate sun */
				sun = safe_calloc( sizeof( *sun ) );

				/* set style */
				sun->style = si->lightStyle;

				/* get color */
				GetTokenAppend( shaderText, false );
				sun->color[ 0 ] = atof( token );
				GetTokenAppend( shaderText, false );
				sun->color[ 1 ] = atof( token );
				GetTokenAppend( shaderText, false );
				sun->color[ 2 ] = atof( token );

				if ( colorsRGB ) {
					sun->color[0] = Image_LinearFloatFromsRGBFloat( sun->color[0] );
					sun->color[1] = Image_LinearFloatFromsRGBFloat( sun->color[1] );
					sun->color[2] = Image_LinearFloatFromsRGBFloat( sun->color[2] );
				}

				/* normalize it */
				ColorNormalize( sun->color );

				/* scale color by brightness */
				GetTokenAppend( shaderText, false );
				sun->photons = atof( token );

				/* get sun angle/elevation */
				GetTokenAppend( shaderText, false );
				const double a = degrees_to_radians( atof( token ) );

				GetTokenAppend( shaderText, false );
				const double b = degrees_to_radians( atof( token ) );

				sun->direction = vector3_for_spherical( a, b );

				/* get filter radius from shader */
				sun->filterRadius = si->lightFilterRadius;

				/* ydnar: get sun angular deviance/samples */
				if ( ext && TokenAvailable() ) {
					GetTokenAppend( shaderText, false );
					sun->deviance = degrees_to_radians( atof( token ) );

					GetTokenAppend( shaderText, false );
					sun->numSamples = atoi( token );
				}

				/* store sun */
				sun->next = si->sun;
				si->sun = sun;

				/* apply sky surfaceparm */
				ApplySurfaceParm( "sky", &si->contentFlags, &si->surfaceFlags, &si->compileFlags );

				/* don't process any more tokens on this line */
				continue;
			}

			/* match q3map_ */
			else if ( striEqualPrefix( token, "q3map_" ) ) {
				/* ydnar: q3map_baseShader <shader> (inherit this shader's parameters) */
				if ( striEqual( token, "q3map_baseShader" ) ) {
					shaderInfo_t    *si2;
					bool oldWarnImage;


					/* get shader */
					GetTokenAppend( shaderText, false );
					//%	Sys_FPrintf( SYS_VRB, "Shader %s has base shader %s\n", si->shader, token );
					oldWarnImage = warnImage;
					warnImage = false;
					si2 = ShaderInfoForShader( token );
					warnImage = oldWarnImage;

					/* subclass it */
					if ( si2 != NULL ) {
						/* preserve name */
						const String64 temp = si->shader;

						/* copy shader */
						*si = *si2;

						/* restore name and set to unfinished */
						si->shader = temp;
						si->shaderWidth = 0;
						si->shaderHeight = 0;
						si->finished = false;
					}
				}

				/* ydnar: q3map_surfacemodel <path to model> <density> <min scale> <max scale> <min angle> <max angle> <oriented (0 or 1)> */
				else if ( striEqual( token, "q3map_surfacemodel" ) ) {
					/* allocate new model and attach it */
					surfaceModel_t& model = si->surfaceModels.emplace_back();

					/* get parameters */
					GetTokenAppend( shaderText, false );
					model.model = token;

					GetTokenAppend( shaderText, false );
					model.density = atof( token );
					GetTokenAppend( shaderText, false );
					model.odds = atof( token );

					GetTokenAppend( shaderText, false );
					model.minScale = atof( token );
					GetTokenAppend( shaderText, false );
					model.maxScale = atof( token );

					GetTokenAppend( shaderText, false );
					model.minAngle = atof( token );
					GetTokenAppend( shaderText, false );
					model.maxAngle = atof( token );

					GetTokenAppend( shaderText, false );
					model.oriented = ( token[ 0 ] == '1' );
				}

				/* ydnar/sd: q3map_foliage <path to model> <scale> <density> <odds> <invert alpha (1 or 0)> */
				else if ( striEqual( token, "q3map_foliage" ) ) {
					/* allocate new foliage struct and attach it */
					foliage_t& foliage = si->foliage.emplace_back();

					/* get parameters */
					GetTokenAppend( shaderText, false );
					foliage.model = token;

					GetTokenAppend( shaderText, false );
					foliage.scale = atof( token );
					GetTokenAppend( shaderText, false );
					foliage.density = atof( token );
					GetTokenAppend( shaderText, false );
					foliage.odds = atof( token );
					GetTokenAppend( shaderText, false );
					foliage.inverseAlpha = atoi( token );
				}

				/* ydnar: q3map_bounce <value> (fraction of light to re-emit during radiosity passes) */
				else if ( striEqual( token, "q3map_bounce" ) || striEqual( token, "q3map_bounceScale" ) ) {
					GetTokenAppend( shaderText, false );
					si->bounceScale = atof( token );
				}

				/* ydnar/splashdamage: q3map_skyLight <value> <iterations> */
				else if ( striEqual( token, "q3map_skyLight" )  ) {
					GetTokenAppend( shaderText, false );
					si->skyLightValue = atof( token );
					GetTokenAppend( shaderText, false );
					si->skyLightIterations = atoi( token );

					/* clamp */
					value_maximize( si->skyLightValue, 0.0f );
					value_maximize( si->skyLightIterations, 2 );
				}

				/* q3map_surfacelight <value> */
				else if ( striEqual( token, "q3map_surfacelight" )  ) {
					GetTokenAppend( shaderText, false );
					si->value = atof( token );
				}

				/* q3map_lightStyle (sof2/jk2 lightstyle) */
				else if ( striEqual( token, "q3map_lightStyle" ) ) {
					GetTokenAppend( shaderText, false );
					si->lightStyle = std::clamp( atoi( token ), 0, LS_NONE );
				}

				/* wolf: q3map_lightRGB <red> <green> <blue> */
				else if ( striEqual( token, "q3map_lightRGB" ) ) {
					si->color.set( 0 );
					GetTokenAppend( shaderText, false );
					si->color[ 0 ] = atof( token );
					GetTokenAppend( shaderText, false );
					si->color[ 1 ] = atof( token );
					GetTokenAppend( shaderText, false );
					si->color[ 2 ] = atof( token );
					if ( colorsRGB ) {
						si->color[0] = Image_LinearFloatFromsRGBFloat( si->color[0] );
						si->color[1] = Image_LinearFloatFromsRGBFloat( si->color[1] );
						si->color[2] = Image_LinearFloatFromsRGBFloat( si->color[2] );
					}
					ColorNormalize( si->color );
				}

				/* q3map_lightSubdivide <value> */
				else if ( striEqual( token, "q3map_lightSubdivide" )  ) {
					GetTokenAppend( shaderText, false );
					si->lightSubdivide = atoi( token );
				}

				/* q3map_backsplash <percent> <distance> */
				else if ( striEqual( token, "q3map_backsplash" ) ) {
					GetTokenAppend( shaderText, false );
					si->backsplashFraction = atof( token ) * 0.01f;
					GetTokenAppend( shaderText, false );
					si->backsplashDistance = atof( token );
				}

				/* q3map_floodLight <r> <g> <b> <diste> <intensity> <light_direction_power> */
				else if ( striEqual( token, "q3map_floodLight" ) ) {
					/* get color */
					GetTokenAppend( shaderText, false );
					si->floodlightRGB[ 0 ] = atof( token );
					GetTokenAppend( shaderText, false );
					si->floodlightRGB[ 1 ] = atof( token );
					GetTokenAppend( shaderText, false );
					si->floodlightRGB[ 2 ] = atof( token );
					GetTokenAppend( shaderText, false );
					si->floodlightDistance = atof( token );
					GetTokenAppend( shaderText, false );
					si->floodlightIntensity = atof( token );
					GetTokenAppend( shaderText, false );
					si->floodlightDirectionScale = atof( token );
					if ( colorsRGB ) {
						si->floodlightRGB[0] = Image_LinearFloatFromsRGBFloat( si->floodlightRGB[0] );
						si->floodlightRGB[1] = Image_LinearFloatFromsRGBFloat( si->floodlightRGB[1] );
						si->floodlightRGB[2] = Image_LinearFloatFromsRGBFloat( si->floodlightRGB[2] );
					}
					ColorNormalize( si->floodlightRGB );
				}

				/* jal: q3map_nodirty : skip dirty */
				else if ( striEqual( token, "q3map_nodirty" ) ) {
					si->noDirty = true;
				}

				/* q3map_lightmapSampleSize <value> */
				else if ( striEqual( token, "q3map_lightmapSampleSize" ) ) {
					GetTokenAppend( shaderText, false );
					si->lightmapSampleSize = atoi( token );
				}

				/* q3map_lightmapSampleOffset <value> */
				else if ( striEqual( token, "q3map_lightmapSampleOffset" ) ) {
					GetTokenAppend( shaderText, false );
					si->lightmapSampleOffset = atof( token );
				}

				/* ydnar: q3map_lightmapFilterRadius <self> <other> */
				else if ( striEqual( token, "q3map_lightmapFilterRadius" ) ) {
					GetTokenAppend( shaderText, false );
					si->lmFilterRadius = atof( token );
					GetTokenAppend( shaderText, false );
					si->lightFilterRadius = atof( token );
				}

				/* ydnar: q3map_lightmapAxis [xyz] */
				else if ( striEqual( token, "q3map_lightmapAxis" ) ) {
					GetTokenAppend( shaderText, false );
					if ( striEqual( token, "x" ) ) {
						si->lightmapAxis = g_vector3_axis_x;
					}
					else if ( striEqual( token, "y" ) ) {
						si->lightmapAxis = g_vector3_axis_y;
					}
					else if ( striEqual( token, "z" ) ) {
						si->lightmapAxis = g_vector3_axis_z;
					}
					else
					{
						Sys_Warning( "Unknown value for lightmap axis: %s\n", token );
						si->lightmapAxis.set( 0 );
					}
				}

				/* ydnar: q3map_lightmapSize <width> <height> (for autogenerated shaders + external tga lightmaps) */
				else if ( striEqual( token, "q3map_lightmapSize" ) ) {
					GetTokenAppend( shaderText, false );
					si->lmCustomWidth = atoi( token );
					GetTokenAppend( shaderText, false );
					si->lmCustomHeight = atoi( token );

					/* must be a power of 2 */
					if ( ( ( si->lmCustomWidth - 1 ) & si->lmCustomWidth ) ||
					     ( ( si->lmCustomHeight - 1 ) & si->lmCustomHeight ) ) {
						Sys_Warning( "Non power-of-two lightmap size specified (%d, %d)\n",
						             si->lmCustomWidth, si->lmCustomHeight );
						si->lmCustomWidth = lmCustomSizeW;
						si->lmCustomHeight = lmCustomSizeH;
					}
				}

				/* ydnar: q3map_lightmapBrightness N (for autogenerated shaders + external tga lightmaps) */
				else if ( striEqual( token, "q3map_lightmapBrightness" ) || striEqual( token, "q3map_lightmapGamma" ) ) {
					GetTokenAppend( shaderText, false );
					si->lmBrightness *= atof( token );
					if ( si->lmBrightness < 0 ) {
						si->lmBrightness = 1.0;
					}
				}

				/* q3map_vertexScale (scale vertex lighting by this fraction) */
				else if ( striEqual( token, "q3map_vertexScale" ) ) {
					GetTokenAppend( shaderText, false );
					si->vertexScale *= atof( token );
				}

				/* q3map_noVertexLight */
				else if ( striEqual( token, "q3map_noVertexLight" ) ) {
					si->noVertexLight = true;
				}

				/* q3map_flare[Shader] <shader> */
				else if ( striEqual( token, "q3map_flare" ) || striEqual( token, "q3map_flareShader" ) ) {
					GetTokenAppend( shaderText, false );
					if ( !strEmpty( token ) ) {
						si->flareShader = copystring( token );
					}
				}

				/* q3map_backShader <shader> */
				else if ( striEqual( token, "q3map_backShader" ) ) {
					GetTokenAppend( shaderText, false );
					if ( !strEmpty( token ) ) {
						si->backShader = copystring( token );
					}
				}

				/* ydnar: q3map_cloneShader <shader> */
				else if ( striEqual( token, "q3map_cloneShader" ) ) {
					GetTokenAppend( shaderText, false );
					if ( !strEmpty( token ) ) {
						si->cloneShader = copystring( token );
					}
				}

				/* q3map_remapShader <shader> */
				else if ( striEqual( token, "q3map_remapShader" ) ) {
					GetTokenAppend( shaderText, false );
					if ( !strEmpty( token ) ) {
						si->remapShader = copystring( token );
					}
				}

				/* q3map_deprecateShader <shader> */
				else if ( striEqual( token, "q3map_deprecateShader" ) ) {
					GetTokenAppend( shaderText, false );
					if ( !strEmpty( token ) ) {
						si->deprecateShader = copystring( token );
					}
				}

				/* ydnar: q3map_offset <value> */
				else if ( striEqual( token, "q3map_offset" ) ) {
					GetTokenAppend( shaderText, false );
					si->offset = atof( token );
				}

				/* ydnar: q3map_fur <numlayers> <offset> <fade> */
				else if ( striEqual( token, "q3map_fur" ) ) {
					GetTokenAppend( shaderText, false );
					si->furNumLayers = atoi( token );
					GetTokenAppend( shaderText, false );
					si->furOffset = atof( token );
					GetTokenAppend( shaderText, false );
					si->furFade = atof( token );
				}

				/* ydnar: gs mods: legacy support for terrain/terrain2 shaders */
				else if ( striEqual( token, "q3map_terrain" ) ) {
					/* team arena terrain is assumed to be nonplanar, with full normal averaging,
					   passed through the metatriangle surface pipeline, with a lightmap axis on z */
					si->legacyTerrain = true;
					si->noClip = true;
					si->notjunc = true;
					si->indexed = true;
					si->nonplanar = true;
					si->forceMeta = true;
					si->shadeAngleDegrees = 179.0f;
					//%	si->lightmapAxis = g_vector3_axis_z;	/* ydnar 2002-09-21: turning this off for better lightmapping of cliff faces */
				}

				/* ydnar: picomodel: q3map_forceMeta (forces brush faces and/or triangle models to go through the metasurface pipeline) */
				else if ( striEqual( token, "q3map_forceMeta" ) ) {
					si->forceMeta = true;
				}

				/* ydnar: gs mods: q3map_shadeAngle <degrees> */
				else if ( striEqual( token, "q3map_shadeAngle" ) ) {
					GetTokenAppend( shaderText, false );
					si->shadeAngleDegrees = atof( token );
				}

				/* ydnar: q3map_textureSize <width> <height> (substitute for q3map_lightimage derivation for terrain) */
				else if ( striEqual( token, "q3map_textureSize" ) ) {
					GetTokenAppend( shaderText, false );
					si->shaderWidth = atoi( token );
					GetTokenAppend( shaderText, false );
					si->shaderHeight = atoi( token );
				}

				/* ydnar: gs mods: q3map_tcGen <style> <parameters> */
				else if ( striEqual( token, "q3map_tcGen" ) ) {
					si->tcGen = true;
					GetTokenAppend( shaderText, false );

					/* q3map_tcGen vector <s vector> <t vector> */
					if ( striEqual( token, "vector" ) ) {
						Parse1DMatrixAppend( shaderText, 3, si->vecs[ 0 ].data() );
						Parse1DMatrixAppend( shaderText, 3, si->vecs[ 1 ].data() );
					}

					/* q3map_tcGen ivector <1.0/s vector> <1.0/t vector> (inverse vector, easier for mappers to understand) */
					else if ( striEqual( token, "ivector" ) ) {
						Parse1DMatrixAppend( shaderText, 3, si->vecs[ 0 ].data() );
						Parse1DMatrixAppend( shaderText, 3, si->vecs[ 1 ].data() );
						for ( i = 0; i < 3; i++ )
						{
							si->vecs[ 0 ][ i ] = si->vecs[ 0 ][ i ] ? 1.0 / si->vecs[ 0 ][ i ] : 0;
							si->vecs[ 1 ][ i ] = si->vecs[ 1 ][ i ] ? 1.0 / si->vecs[ 1 ][ i ] : 0;
						}
					}
					else
					{
						Sys_Warning( "Unknown q3map_tcGen method: %s\n", token );
						si->vecs[ 0 ].set( 0 );
						si->vecs[ 1 ].set( 0 );
					}
				}

				/* ydnar: gs mods: q3map_[color|rgb|alpha][Gen|Mod] <style> <parameters> */
				else if ( striEqual( token, "q3map_colorGen" ) || striEqual( token, "q3map_colorMod" ) ||
				          striEqual( token, "q3map_rgbGen" ) || striEqual( token, "q3map_rgbMod" ) ||
				          striEqual( token, "q3map_alphaGen" ) || striEqual( token, "q3map_alphaMod" ) ) {
					colorMod_t  *cm, *cm2;


					/* alphamods are colormod + 1 */
					const bool alpha = striEqual( token, "q3map_alphaGen" ) || striEqual( token, "q3map_alphaMod" );

					/* allocate new colormod */
					cm = safe_calloc( sizeof( *cm ) );

					/* attach to shader */
					if ( si->colorMod == NULL ) {
						si->colorMod = cm;
					}
					else
					{
						for ( cm2 = si->colorMod; cm2 != NULL; cm2 = cm2->next )
						{
							if ( cm2->next == NULL ) {
								cm2->next = cm;
								break;
							}
						}
					}

					/* get type */
					GetTokenAppend( shaderText, false );

					/* alpha set|const A */
					if ( alpha && ( striEqual( token, "set" ) || striEqual( token, "const" ) ) ) {
						cm->type = EColorMod::AlphaSet;
						GetTokenAppend( shaderText, false );
						cm->data[ 0 ] = atof( token );
					}

					/* color|rgb set|const ( X Y Z ) */
					else if ( striEqual( token, "set" ) || striEqual( token, "const" ) ) {
						cm->type = EColorMod::ColorSet;
						Parse1DMatrixAppend( shaderText, 3, cm->data );
						if ( colorsRGB ) {
							cm->data[0] = Image_LinearFloatFromsRGBFloat( cm->data[0] );
							cm->data[1] = Image_LinearFloatFromsRGBFloat( cm->data[1] );
							cm->data[2] = Image_LinearFloatFromsRGBFloat( cm->data[2] );
						}
					}

					/* alpha scale A */
					else if ( alpha && striEqual( token, "scale" ) ) {
						cm->type = EColorMod::AlphaScale;
						GetTokenAppend( shaderText, false );
						cm->data[ 0 ] = atof( token );
					}

					/* color|rgb scale ( X Y Z ) */
					else if ( striEqual( token, "scale" ) ) {
						cm->type = EColorMod::ColorScale;
						Parse1DMatrixAppend( shaderText, 3, cm->data );
					}

					/* dotProduct ( X Y Z ) */
					else if ( striEqual( token, "dotProduct" ) ) {
						cm->type = alpha? EColorMod::AlphaDotProduct : EColorMod::ColorDotProduct;
						Parse1DMatrixAppend( shaderText, 3, cm->data );
					}

					/* dotProductScale ( X Y Z MIN MAX ) */
					else if ( striEqual( token, "dotProductScale" ) ) {
						cm->type = alpha? EColorMod::AlphaDotProductScale : EColorMod::ColorDotProductScale;
						Parse1DMatrixAppend( shaderText, 5, cm->data );
					}

					/* dotProduct2 ( X Y Z ) */
					else if ( striEqual( token, "dotProduct2" ) ) {
						cm->type = alpha? EColorMod::AlphaDotProduct2 : EColorMod::ColorDotProduct2;
						Parse1DMatrixAppend( shaderText, 3, cm->data );
					}

					/* dotProduct2scale ( X Y Z MIN MAX ) */
					else if ( striEqual( token, "dotProduct2scale" ) ) {
						cm->type = alpha? EColorMod::AlphaDotProduct2Scale : EColorMod::ColorDotProduct2Scale;
						Parse1DMatrixAppend( shaderText, 5, cm->data );
					}

					/* volume */
					else if ( striEqual( token, "volume" ) ) {
						/* special stub mode for flagging volume brushes */
						cm->type = EColorMod::Volume;
					}

					/* unknown */
					else{
						Sys_Warning( "Unknown colorMod method: %s\n", token );
					}
				}

				/* ydnar: gs mods: q3map_tcMod <style> <parameters> */
				else if ( striEqual( token, "q3map_tcMod" ) ) {
					float a, b;


					GetTokenAppend( shaderText, false );

					/* q3map_tcMod [translate | shift | offset] <s> <t> */
					if ( striEqual( token, "translate" ) || striEqual( token, "shift" ) || striEqual( token, "offset" ) ) {
						GetTokenAppend( shaderText, false );
						a = atof( token );
						GetTokenAppend( shaderText, false );
						b = atof( token );

						TCModTranslate( si->mod, a, b );
					}

					/* q3map_tcMod scale <s> <t> */
					else if ( striEqual( token, "scale" ) ) {
						GetTokenAppend( shaderText, false );
						a = atof( token );
						GetTokenAppend( shaderText, false );
						b = atof( token );

						TCModScale( si->mod, a, b );
					}

					/* q3map_tcMod rotate <s> <t> (fixme: make this communitive) */
					else if ( striEqual( token, "rotate" ) ) {
						GetTokenAppend( shaderText, false );
						a = atof( token );
						TCModRotate( si->mod, a );
					}
					else{
						Sys_Warning( "Unknown q3map_tcMod method: %s\n", token );
					}
				}

				/* q3map_fogDir (direction a fog shader fades from transparent to opaque) */
				else if ( striEqual( token, "q3map_fogDir" ) ) {
					Parse1DMatrixAppend( shaderText, 3, si->fogDir.data() );
					VectorNormalize( si->fogDir );
				}

				/* q3map_globaltexture */
				else if ( striEqual( token, "q3map_globaltexture" )  ) {
					si->globalTexture = true;
				}

				/* ydnar: gs mods: q3map_nonplanar (make it a nonplanar merge candidate for meta surfaces) */
				else if ( striEqual( token, "q3map_nonplanar" ) ) {
					si->nonplanar = true;
				}

				/* ydnar: gs mods: q3map_noclip (preserve original face winding, don't clip by bsp tree) */
				else if ( striEqual( token, "q3map_noclip" ) ) {
					si->noClip = true;
				}

				/* q3map_notjunc */
				else if ( striEqual( token, "q3map_notjunc" ) ) {
					si->notjunc = true;
				}

				/* q3map_nofog */
				else if ( striEqual( token, "q3map_nofog" ) ) {
					si->noFog = true;
				}

				/* ydnar: gs mods: q3map_indexed (for explicit terrain-style indexed mapping) */
				else if ( striEqual( token, "q3map_indexed" ) ) {
					si->indexed = true;
				}

				/* ydnar: q3map_invert (inverts a drawsurface's facing) */
				else if ( striEqual( token, "q3map_invert" ) ) {
					si->invert = true;
				}

				/* ydnar: gs mods: q3map_lightmapMergable (ok to merge non-planar */
				else if ( striEqual( token, "q3map_lightmapMergable" ) ) {
					si->lmMergable = true;
				}

				/* ydnar: q3map_nofast */
				else if ( striEqual( token, "q3map_noFast" ) ) {
					si->noFast = true;
				}

				/* q3map_patchshadows */
				else if ( striEqual( token, "q3map_patchShadows" ) ) {
					si->patchShadows = true;
				}

				/* q3map_vertexshadows */
				else if ( striEqual( token, "q3map_vertexShadows" ) ) {
					si->vertexShadows = true;  /* ydnar */

				}
				/* q3map_novertexshadows */
				else if ( striEqual( token, "q3map_noVertexShadows" ) ) {
					si->vertexShadows = false; /* ydnar */

				}
				/* q3map_splotchfix (filter dark lightmap luxels on lightmapped models) */
				else if ( striEqual( token, "q3map_splotchfix" ) ) {
					si->splotchFix = true; /* ydnar */

				}
				/* q3map_forcesunlight */
				else if ( striEqual( token, "q3map_forceSunlight" ) ) {
					si->forceSunlight = true;
				}

				/* q3map_onlyvertexlighting (sof2) */
				else if ( striEqual( token, "q3map_onlyVertexLighting" ) ) {
					ApplySurfaceParm( "pointlight", &si->contentFlags, &si->surfaceFlags, &si->compileFlags );
				}

				/* q3map_material (sof2) */
				else if ( striEqual( token, "q3map_material" ) ) {
					GetTokenAppend( shaderText, false );
					if ( !ApplySurfaceParm( StringOutputStream( 64 )( "*mat_", token ), &si->contentFlags, &si->surfaceFlags, &si->compileFlags ) ) {
						Sys_Warning( "Unknown material \"%s\"\n", token );
					}
				}

				/* ydnar: q3map_clipmodel (autogenerate clip brushes for model triangles using this shader) */
				else if ( striEqual( token, "q3map_clipmodel" )  ) {
					si->clipModel = true;
				}

				/* ydnar: q3map_styleMarker[2] */
				else if ( striEqual( token, "q3map_styleMarker" ) ) {
					si->styleMarker = 1;
				}
				else if ( striEqual( token, "q3map_styleMarker2" ) ) {  /* uses depthFunc equal */
					si->styleMarker = 2;
				}

				/* ydnar: default to searching for q3map_<surfaceparm> */
#if 1
				else
				{
					//%	Sys_FPrintf( SYS_VRB, "Attempting to match %s with a known surfaceparm\n", token );
					if ( !ApplySurfaceParm( &token[ 6 ], &si->contentFlags, &si->surfaceFlags, &si->compileFlags ) ) {
						Sys_Warning( "Unknown q3map_* directive \"%s\"\n", token );
					}
				}
#endif
			}


			/* -----------------------------------------------------------------
			   skip
			   ----------------------------------------------------------------- */

			/* ignore all other tokens on the line */
			while ( TokenAvailable() && GetTokenAppend( shaderText, false ) ) ;
		}
	}
}



/*
   ParseCustomInfoParms() - rr2do2
   loads custom info parms file for mods
 */

static void ParseCustomInfoParms( void ){
	bool parsedContent, parsedSurface;


	/* file exists? */
	if ( vfsGetFileCount( "scripts/custinfoparms.txt" ) == 0 ) {
		return;
	}

	/* load it */
	LoadScriptFile( "scripts/custinfoparms.txt", 0 );

	/* clear the array */
	memset( custSurfaceParms, 0, sizeof( custSurfaceParms ) );
	numCustSurfaceParms = 0;
	parsedContent = parsedSurface = false;

	/* parse custom contentflags */
	MatchToken( "{" );
	while ( 1 )
	{
		if ( !GetToken( true ) ) {
			break;
		}

		if ( strEqual( token, "}" ) ) {
			parsedContent = true;
			break;
		}

		custSurfaceParms[ numCustSurfaceParms ].name = copystring( token );
		GetToken( false );
		sscanf( token, "%x", &custSurfaceParms[ numCustSurfaceParms ].contentFlags );
		numCustSurfaceParms++;
	}

	/* any content? */
	if ( !parsedContent ) {
		Sys_Warning( "Couldn't find valid custom contentsflag section\n" );
		return;
	}

	/* parse custom surfaceflags */
	MatchToken( "{" );
	while ( 1 )
	{
		if ( !GetToken( true ) ) {
			break;
		}

		if ( strEqual( token, "}" ) ) {
			parsedSurface = true;
			break;
		}

		custSurfaceParms[ numCustSurfaceParms ].name = copystring( token );
		GetToken( false );
		sscanf( token, "%x", &custSurfaceParms[ numCustSurfaceParms ].surfaceFlags );
		numCustSurfaceParms++;
	}

	/* any content? */
	if ( !parsedContent ) {
		Sys_Warning( "Couldn't find valid custom surfaceflag section\n" );
	}
}



#define MAX_SHADER_FILES    1024

struct StrList
{
	int n;
	char* s[MAX_SHADER_FILES];
};

void pushShaderCallback( StrList* list, const char* string ){
	char* shader = copystring( string );
	strClear( shader + strlen( shader ) - strlen( ".shader" ) );
	/* check for duplicate entries */
	for ( int i = 0; i < list->n; i++ )
		if ( striEqual( list->s[ i ], shader ) ){
			free( shader );
			return;
		}

	/* test limit */
	if ( list->n >= MAX_SHADER_FILES )
		Error( "MAX_SHADER_FILES (%d) reached!", (int) MAX_SHADER_FILES );

	/* new shader file */
	list->s[ list->n++ ] = shader;
}

/*
   LoadShaderInfo()
   the shaders are parsed out of shaderlist.txt from a main directory
   that is, if using -fs_game we ignore the shader scripts that might be in baseq3/
   on linux there's an additional twist, we actually merge the stuff from ~/.q3a/ and from the base dir
 */

void LoadShaderInfo( void ){
	int i, j, count;
	char filename[ 1024 ];
	StrList shaderFiles = { .n = 0 };


	/* rr2do2: parse custom infoparms first */
	if ( useCustomInfoParms ) {
		ParseCustomInfoParms();
	}

	/* we can pile up several shader files, the one in baseq3 and ones in the mod dir or other spots */
	sprintf( filename, "%s/shaderlist.txt", game->shaderPath );
	count = vfsGetFileCount( filename );

	/* load them all */
	for ( i = 0; i < count; i++ )
	{
		/* load shader list */
		sprintf( filename, "%s/shaderlist.txt", game->shaderPath );
		LoadScriptFile( filename, i );

		/* parse it */
		while ( GetToken( true ) )
		{
			/* check for duplicate entries */
			for ( j = 0; j < shaderFiles.n; j++ )
				if ( strEqual( shaderFiles.s[ j ], token ) ) {
					break;
				}

			/* test limit */
			if ( j >= MAX_SHADER_FILES ) {
				Error( "MAX_SHADER_FILES (%d) reached, trim your shaderlist.txt!", (int) MAX_SHADER_FILES );
			}

			/* new shader file */
			if ( j == shaderFiles.n ) {
				shaderFiles.s[ shaderFiles.n++ ] = copystring( token );
			}
		}
	}

	if( shaderFiles.n == 0 ){
		Sys_Printf( "%s", "No shaderlist.txt found: loading all shaders\n" );
		vfsListShaderFiles( &shaderFiles, pushShaderCallback );
	}

	/* parse the shader files */
	for ( i = 0; i < shaderFiles.n; i++ )
	{
		sprintf( filename, "%s/%s.shader", game->shaderPath, shaderFiles.s[ i ] );
		ParseShaderFile( filename );
		free( shaderFiles.s[ i ] );
	}

	/* emit some statistics */
	Sys_FPrintf( SYS_VRB, "%9d shaderInfo\n", numShaderInfo );
}
