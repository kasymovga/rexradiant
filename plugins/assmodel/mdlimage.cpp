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

#include "mdlimage.h"

#include "imagelib.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"


Image* LoadMDLImage( Assimp::Importer& importer, ArchiveFile& file ){
	const aiScene *scene = importer.GetScene();
	if( scene != nullptr && scene->HasTextures() ){
		const aiTexture *tex = scene->mTextures[0];
		if( tex->mWidth != 0 ){ // not compressed
			RGBAImage* image = new RGBAImage( tex->mWidth, tex->mHeight );
			unsigned char* pRGBA = image->getRGBAPixels();
			const aiTexel *texel = tex->pcData;
			for( size_t i = 0; i < tex->mWidth * tex->mHeight; ++i, ++texel ){
				*pRGBA++ = texel->r;
				*pRGBA++ = texel->g;
				*pRGBA++ = texel->b;
				*pRGBA++ = texel->a;
			}
			return image;
		}
	}
	return nullptr;
}

