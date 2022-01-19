/*
   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
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
 */

#pragma once

#include <cstddef>
#include "string/stringfwd.h"
#include "generic/callbackfwd.h"

enum EBrushPrefab
{
	eBrushCuboid,
	eBrushPrism,
	eBrushCone,
	eBrushSphere,
	eBrushRock,
	eBrushIcosahedron,
};

class TextureProjection;
class ContentsFlagsValue;
namespace scene
{
class Graph;
class Node;
}
void Scene_BrushConstructPrefab( scene::Graph& graph, EBrushPrefab type, std::size_t sides, bool option, const char* shader );
class AABB;
void Scene_BrushResize_Cuboid( scene::Node*& node, const AABB& bounds );
void Brush_ConstructPlacehoderCuboid( scene::Node& node, const AABB& bounds );
void Scene_BrushSetTexdef_Selected( scene::Graph& graph, const TextureProjection& projection, bool setBasis, bool resetBasis );
void Scene_BrushSetTexdef_Component_Selected( scene::Graph& graph, const TextureProjection& projection, bool setBasis, bool resetBasis );
void Scene_BrushSetTexdef_Selected( scene::Graph& graph, const float* hShift, const float* vShift, const float* hScale, const float* vScale, const float* rotation );
void Scene_BrushSetTexdef_Component_Selected( scene::Graph& graph, const float* hShift, const float* vShift, const float* hScale, const float* vScale, const float* rotation );
void Scene_BrushGetTexdef_Selected( scene::Graph& graph, TextureProjection& projection );
void Scene_BrushGetTexdef_Component_Selected( scene::Graph& graph, TextureProjection& projection );
void Scene_BrushGetShaderSize_Component_Selected( scene::Graph& graph, size_t& width, size_t& height );
void Scene_BrushSetFlags_Selected( scene::Graph& graph, const ContentsFlagsValue& flags );
void Scene_BrushSetFlags_Component_Selected( scene::Graph& graph, const ContentsFlagsValue& flags );
void Scene_BrushGetFlags_Selected( scene::Graph& graph, ContentsFlagsValue& flags );
void Scene_BrushGetFlags_Component_Selected( scene::Graph& graph, ContentsFlagsValue& flags );
void Scene_BrushShiftTexdef_Selected( scene::Graph& graph, float s, float t );
void Scene_BrushShiftTexdef_Component_Selected( scene::Graph& graph, float s, float t );
void Scene_BrushScaleTexdef_Selected( scene::Graph& graph, float s, float t );
void Scene_BrushScaleTexdef_Component_Selected( scene::Graph& graph, float s, float t );
void Scene_BrushRotateTexdef_Selected( scene::Graph& graph, float angle );
void Scene_BrushRotateTexdef_Component_Selected( scene::Graph& graph, float angle );
void Scene_BrushSetShader_Selected( scene::Graph& graph, const char* name );
void Scene_BrushSetShader_Component_Selected( scene::Graph& graph, const char* name );
void Scene_BrushGetShader_Selected( scene::Graph& graph, CopiedString& shader );
void Scene_BrushGetShader_Component_Selected( scene::Graph& graph, CopiedString& shader );
void Scene_BrushFindReplaceShader( scene::Graph& graph, const char* find, const char* replace );
void Scene_BrushFindReplaceShader_Selected( scene::Graph& graph, const char* find, const char* replace );
void Scene_BrushFindReplaceShader_Component_Selected( scene::Graph& graph, const char* find, const char* replace );
void Scene_BrushSelectByShader( scene::Graph& graph, const char* name );
void Scene_BrushSelectByShader_Component( scene::Graph& graph, const char* name );
void Scene_BrushFacesSelectByShader( scene::Graph& graph, const char* name );

#include "itexdef.h"
template<typename Element> class BasicVector3;
typedef BasicVector3<float> Vector3;
void Scene_BrushProjectTexture_Selected( scene::Graph& graph, const texdef_t& texdef, const Vector3* direction );
void Scene_BrushProjectTexture_Component_Selected( scene::Graph& graph, const texdef_t& texdef, const Vector3* direction );
void Scene_BrushProjectTexture_Selected( scene::Graph& graph, const TextureProjection& projection, const Vector3& normal );
void Scene_BrushProjectTexture_Component_Selected( scene::Graph& graph, const TextureProjection& projection, const Vector3& normal );

void Scene_BrushFitTexture_Selected( scene::Graph& graph, float s_repeat, float t_repeat, bool only_dimension );
void Scene_BrushFitTexture_Component_Selected( scene::Graph& graph, float s_repeat, float t_repeat, bool only_dimension );

typedef struct _GtkMenu GtkMenu;
void Brush_constructMenu( GtkMenu* menu );

extern Callback g_texture_lock_status_changed;

void BrushFilters_construct();
void Brush_registerCommands();
