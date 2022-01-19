/*
   BobToolz plugin for GtkRadiant
   Copyright (C) 2001 Gordon Biggans

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// DPatch.cpp: implementation of the DPatch class.
//
//////////////////////////////////////////////////////////////////////

#include "DPatch.h"

#include <list>
#include "str.h"
#include "scenelib.h"

#include "ipatch.h"

#include "misc.h"
#include "./dialogs/dialogs-gtk.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//		Added patch merging, wahey!

//
//		problem is, you cant put patches into entities as yet :(
//

DPatch::DPatch(){
	width = MIN_PATCH_WIDTH;
	height = MIN_PATCH_HEIGHT;
	QER_entity = NULL;
	QER_brush = NULL;
}

DPatch::~DPatch(){

}

void DPatch::SetTexture( const char *textureName ){
	strcpy( texture, textureName );
}

void CopyDrawVert( const drawVert_t* in, drawVert_t* out ){
	out->lightmap[0] = in->lightmap[0];
	out->lightmap[1] = in->lightmap[1];
	out->st[0] = in->st[0];
	out->st[1] = in->st[1];
	VectorCopy( in->normal, out->normal );
	VectorCopy( in->xyz, out->xyz );
}

void DPatch::BuildInRadiant( scene::Node* entity ){
	NodeSmartReference patch( GlobalPatchCreator().createPatch() );

	scene::Node& parent = entity != 0 ? *entity : GlobalRadiant().getMapWorldEntity();
	Node_getTraversable( parent )->insert( patch );

	GlobalPatchCreator().Patch_setShader( patch, texture );
	GlobalPatchCreator().Patch_resize( patch, height, width );
	PatchControlMatrix matrix = GlobalPatchCreator().Patch_getControlPoints( patch );
	for ( int x = 0; x < width; x++ )
	{
		for ( int y = 0; y < height; y++ )
		{
			PatchControl& p = matrix( x, y );
			p.m_vertex[0] = points[x][y].xyz[0];
			p.m_vertex[1] = points[x][y].xyz[1];
			p.m_vertex[2] = points[x][y].xyz[2];
			p.m_texcoord[0] = points[x][y].st[0];
			p.m_texcoord[1] = points[x][y].st[1];
		}
	}
	GlobalPatchCreator().Patch_controlPointsChanged( patch );

	QER_entity = entity;
	QER_brush = patch.get_pointer();


#if 0
	int nIndex = g_FuncTable.m_pfnCreatePatchHandle();
	//$ FIXME: m_pfnGetPatchHandle
	patchMesh_t* pm = g_FuncTable.m_pfnGetPatchData( nIndex );

	b->patchBrush = true;
	b->pPatch = Patch_Alloc();
	b->pPatch->setDims( width,height );

	for ( int x = 0; x < width; x++ )
		for ( int y = 0; y < height; y++ )
			CopyDrawVert( &points[x][y], &pm->ctrl[x][y] );

/*	if(entity)
	{
	//	strcpy(pm->d_texture->name, texture);

		brush_t* brush = (brush_t*)g_FuncTable.m_pfnCreateBrushHandle();
		brush->patchBrush = true;
		brush->pPatch = pm;

		pm->pSymbiot = brush;
		pm->bSelected = false;
		pm->bOverlay = false;	// bleh, f*cks up, just have to wait for a proper function
		pm->bDirty = true;		// or get my own patch out....
		pm->nListID = -1;

		g_FuncTable.m_pfnCommitBrushHandleToEntity(brush, entity);
	}
	else*/              // patch to entity just plain dont work atm

	if ( entity ) {
		g_FuncTable.m_pfnCommitPatchHandleToEntity( nIndex, pm, texture, entity );
	}
	else{
		g_FuncTable.m_pfnCommitPatchHandleToMap( nIndex, pm, texture );
	}

	QER_brush = pm->pSymbiot;
#endif
}

void DPatch::LoadFromPatch( scene::Instance& patch ){
	QER_entity = patch.path().parent().get_pointer();
	QER_brush = patch.path().top().get_pointer();

	PatchControlMatrix matrix = GlobalPatchCreator().Patch_getControlPoints( patch.path().top() );

	width = static_cast<int>( matrix.x() );
	height = static_cast<int>( matrix.y() );

	for ( int x = 0; x < width; x++ )
	{
		for ( int y = 0; y < height; y++ )
		{
			PatchControl& p = matrix( x, y );
			points[x][y].xyz[0] = p.m_vertex[0];
			points[x][y].xyz[1] = p.m_vertex[1];
			points[x][y].xyz[2] = p.m_vertex[2];
			points[x][y].st[0] = p.m_texcoord[0];
			points[x][y].st[1] = p.m_texcoord[1];
		}
	}
	SetTexture( GlobalPatchCreator().Patch_getShader( patch.path().top() ) );

#if 0
	SetTexture( brush->pPatch->GetShader() );

	width = brush->pPatch->getWidth();
	height = brush->pPatch->getHeight();

	for ( int x = 0; x < height; x++ )
	{
		for ( int y = 0; y < width; y++ )
		{
			float *p = brush->pPatch->ctrlAt( ROW,x,y );
			p[0] = points[x][y].xyz[0];
			p[1] = points[x][y].xyz[1];
			p[2] = points[x][y].xyz[2];
			p[3] = points[x][y].st[0];
			p[4] = points[x][y].st[1];
		}
	}
#endif
}

bool DPatch::ResetTextures( const char *oldTextureName, const char *newTextureName ){
	if ( !oldTextureName || !strcmp( texture, oldTextureName ) ) {
		strcpy( texture, newTextureName );
		return true;
	}

	return false;
}

void Build1dArray( vec3_t* array, const drawVert_t points[MAX_PATCH_WIDTH][MAX_PATCH_HEIGHT],
                   int startX, int startY, int number, bool horizontal, bool inverse ){
	int x = startX, y = startY, i, step;

	if ( inverse ) {
		step = -1;
	}
	else{
		step = 1;
	}

	for ( i = 0; i < number; i++ )
	{
		VectorCopy( points[x][y].xyz, array[i] );

		if ( horizontal ) {
			x += step;
		}
		else{
			y += step;
		}
	}
}

void Print1dArray( vec3_t* array, int size ){
	for ( int i = 0; i < size; i++ )
		globalOutputStream() << "(" << array[i][0] << " " << array[i][1] << " " << array[i][2] << ")\t";
	globalOutputStream() << "\n";
}

bool Compare1dArrays( vec3_t* a1, vec3_t* a2, int size ){
	int i;
	bool equal = true;

	for ( i = 0; i < size; i++ )
	{
		if ( !VectorCompare( a1[i], a2[size - i - 1] ) ) {
			equal = false;
			break;
		}
	}
	return equal;
}

patch_merge_t DPatch::IsMergable( const DPatch& other ){
	int i, j;
	vec3_t p1Array[4][MAX_PATCH_HEIGHT];
	vec3_t p2Array[4][MAX_PATCH_HEIGHT];

	int p1ArraySizes[4];
	int p2ArraySizes[4];

	patch_merge_t merge_info;

	Build1dArray( p1Array[0], this->points, 0,               0,                this->width,    true,   false );
	Build1dArray( p1Array[1], this->points, this->width - 1, 0,                this->height,   false,  false );
	Build1dArray( p1Array[2], this->points, this->width - 1, this->height - 1, this->width,    true,   true );
	Build1dArray( p1Array[3], this->points, 0,               this->height - 1, this->height,   false,  true );

	Build1dArray( p2Array[0], other.points, 0,               0,                other.width,   true,   false );
	Build1dArray( p2Array[1], other.points, other.width - 1, 0,                other.height,  false,  false );
	Build1dArray( p2Array[2], other.points, other.width - 1, other.height - 1, other.width,   true,   true );
	Build1dArray( p2Array[3], other.points, 0,               other.height - 1, other.height,  false,  true );

	p1ArraySizes[0] = this->width;
	p1ArraySizes[1] = this->height;
	p1ArraySizes[2] = this->width;
	p1ArraySizes[3] = this->height;

	p2ArraySizes[0] = other.width;
	p2ArraySizes[1] = other.height;
	p2ArraySizes[2] = other.width;
	p2ArraySizes[3] = other.height;

	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < 4; j++ )
		{
			if ( p1ArraySizes[i] == p2ArraySizes[j] ) {
				if ( Compare1dArrays( p1Array[i], p2Array[j], p1ArraySizes[i] ) ) {
					merge_info.pos1 = i;
					merge_info.pos2 = j;
					merge_info.mergable = true;
					return merge_info;
				}
			}
		}
	}

	merge_info.mergable = false;
	return merge_info;
}

DPatch* DPatch::MergePatches( patch_merge_t merge_info, DPatch& p1, DPatch& p2 ){
	while ( merge_info.pos1 != 2 )
	{
		p1.Transpose();
		merge_info.pos1--;
		if ( merge_info.pos1 < 0 ) {
			merge_info.pos1 += 4;
		}
	}

	while ( merge_info.pos2 != 0 )
	{
		p2.Transpose();
		merge_info.pos2--;
		if ( merge_info.pos2 < 0 ) {
			merge_info.pos2 += 3;
		}
	}

	const int newHeight = p1.height + p2.height - 1;
	if ( newHeight > MAX_PATCH_HEIGHT ) {
		return 0;
	}

	DPatch* newPatch = new DPatch();

	newPatch->height    = newHeight;
	newPatch->width     = p1.width;
	newPatch->SetTexture( p1.texture );

	for ( int y = 0; y < p1.height; y++ )
		for ( int x = 0; x < p1.width; x++ )
			newPatch->points[x][y] = p1.points[x][y];

	for ( int y = 1; y < p2.height; y++ )
		for ( int x = 0; x < p2.width; x++ )
			newPatch->points[x][( y + p1.height - 1 )] = p2.points[x][y];

//	newPatch->Invert();
	return newPatch;
}

void DPatch::Invert(){
	int i, j;

	for ( i = 0; i < width; i++ )
	{
		for ( j = 0; j < height / 2; j++ )
		{
			std::swap( points[i][height - 1 - j], points[i][j] );
		}
	}
}
/*
   //Was used for debugging, obsolete function
   DPatch* DPatch::TransposePatch(DPatch *p1)
   {
    globalOutputStream() << "Source patch ";
    p1->DebugPrint();
    p1->Transpose();
    globalOutputStream() << "Transposed";
    p1->DebugPrint();

    DPatch* newPatch = new DPatch();
    newPatch->height	= p1->height;
    newPatch->width		= p1->width;
    newPatch->SetTexture(p1->texture);

    for(int x = 0; x < p1->height; x++)
    {
        for(int y = 0; y < p1->width; y++)
        {
            newPatch->points[x][y] = p1->points[x][y];
        }
    }
    return newPatch;
   }

   //Function to figure out what is actually going wrong.
   void DPatch::DebugPrint()
   {
    globalOutputStream() << "width: " << width << "\theight: " << height << "\n";
    for(int x = 0; x < height; x++)
    {
        for(int y = 0; y < width; y++)
        {
            globalOutputStream() << "\t(" << points[x][y].xyz[0] << " " << points[x][y].xyz[1] << " " << points[x][y].xyz[2] << ")\t";
        }
        globalOutputStream() << "\n";
    }
   }
 */

void DPatch::Transpose(){
	int i, j;

	if ( width > height ) {
		for ( i = 0; i < height; i++ )
		{
			for ( j = i + 1; j < width; j++ )
			{
				if ( j < height ) {
					// swap the value
					std::swap( points[j][i], points[i][j] );
				}
				else
				{
					// just copy
					points[i][j] = points[j][i];
				}
			}
		}
	}
	else
	{
		for ( i = 0; i < width; i++ )
		{
			for ( j = i + 1; j < height; j++ )
			{
				if ( j < width ) {
					// swap the value
					std::swap( points[i][j], points[j][i] );
				}
				else
				{
					// just copy
					points[j][i] = points[i][j];
				}
			}
		}
	}

	std::swap( width, height );

	Invert();
}

std::list<DPatch> DPatch::SplitCols(){
	std::list<DPatch> patchList;
	int i;
	int x, y;

	if ( height >= 5 ) {
		for ( i = 0; i < ( height - 1 ) / 2; i++ )
		{
			DPatch p;

			p.width = width;
			p.height = MIN_PATCH_HEIGHT;
			p.SetTexture( texture );
			for ( x = 0; x < p.width; x++ )
			{
				for ( y = 0; y < MIN_PATCH_HEIGHT; y++ )
				{
					p.points[x][y] = points[x][( i * 2 ) + y];
				}
			}
			patchList.push_back( p );
		}
	}
	else {
		globalWarningStream() << "bobToolz SplitPatchCols: Patch has not enough columns for splitting.\n";
		patchList.push_back( *this );
	}
	return patchList;
}

std::list<DPatch> DPatch::SplitRows(){
	std::list<DPatch> patchList;
	int i;
	int x, y;

	if ( width >= 5 ) {
		for ( i = 0; i < ( width - 1 ) / 2; i++ )
		{
			DPatch p;

			p.width = MIN_PATCH_WIDTH;
			p.height = height;
			p.SetTexture( texture );

			for ( x = 0; x < MIN_PATCH_WIDTH; x++ )
			{
				for ( y = 0; y < p.height; y++ )
				{
					p.points[x][y] = points[( i * 2 ) + x][y];
				}
			}
			patchList.push_back( p );
		}
	}
	else
	{
		globalWarningStream() << "bobToolz SplitPatchRows: Patch has not enough rows for splitting.\n";
		patchList.push_back( *this );
	}
	return patchList;
}

std::list<DPatch> DPatch::Split(){
	std::list<DPatch> patchList;

	if ( height >= 5 ) {
		std::list<DPatch> patchColList = SplitCols();
		if( width >= 5 ){
			for( auto&& patchesCol : patchColList )
				patchList.splice( patchList.cend(), patchesCol.SplitRows() );
		}
		else{
			patchList.swap( patchColList );
		}
	}
	else if ( width >= 5 ) {
		patchList = SplitRows();
	}
	else
	{
		globalWarningStream() << "bobToolz SplitPatch: Patch has not enough rows and columns for splitting.\n";
		patchList.push_back( *this );
	}
	return patchList;
}
