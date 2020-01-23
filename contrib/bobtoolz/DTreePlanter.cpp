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

#include "DTreePlanter.h"

#include <list>
#include "str.h"

#include "DPoint.h"
#include "DPlane.h"
#include "DBrush.h"
#include "DEPair.h"
#include "DPatch.h"
#include "DEntity.h"

#include "ScriptParser.h"
#include "misc.h"
#include "scenelib.h"



#include "funchandlers.h"

SignalHandlerResult DTreePlanter::mouseDown( const WindowVector& position, ButtonIdentifier button, ModifierFlags modifiers ){
	if ( button != c_buttonLeft || GlobalRadiant().XYWindow_getViewType() != XY ) {
		return SIGNAL_CONTINUE_EMISSION;
	}

	Vector3 pt, vhit;

	pt = vector3_snapped( GlobalRadiant().XYWindow_windowToWorld( position ), GlobalRadiant().getGridSize() );

	if ( FindDropPoint( vector3_to_array( pt ), vector3_to_array( vhit ) ) ) {
		vhit[2] += m_offset;

		char buffer[128];
		DEntity e( m_entType );

		sprintf( buffer, "%i %i %i", (int)vhit[0], (int)vhit[1], (int)vhit[2] );
		e.AddEPair( "origin", buffer );

		if ( m_autoLink ) {

			const scene::Path* pLastEntity = NULL;
			const scene::Path* pThisEntity = NULL;

			int entpos;
			for ( int i = 0; i < 256; i++ ) {
				sprintf( buffer, m_linkName, i );
				pThisEntity = FindEntityFromTargetname( buffer );

				if ( pThisEntity ) {
					entpos = i;
					pLastEntity = pThisEntity;
				}
			}

			if ( !pLastEntity ) {
				sprintf( buffer, m_linkName, 0 );
			}
			else {
				sprintf( buffer, m_linkName, entpos + 1 );
			}

			e.AddEPair( "targetname", buffer );

			if ( pLastEntity ) {
				DEntity e2;
				e2.LoadFromEntity( pLastEntity->top(), true );
				e2.AddEPair( "target", buffer );
				e2.RemoveFromRadiant();
				e2.BuildInRadiant( false );
			}

		}

		if ( m_setAngles ) {
			int angleYaw = ( rand() % ( m_maxYaw - m_minYaw + 1 ) ) + m_minYaw;
			int anglePitch = ( rand() % ( m_maxPitch - m_minPitch + 1 ) ) + m_minPitch;

			sprintf( buffer, "%i %i 0", anglePitch, angleYaw );
			e.AddEPair( "angles", buffer );
		}

		if ( m_numModels ) {
			int treetype = rand() % m_numModels;
			e.AddEPair( "model", m_trees[treetype].name );
		}

		if ( m_useScale ) {
			float scale = ( ( ( rand() % 1000 ) * 0.001f ) * ( m_maxScale - m_minScale ) ) + m_minScale;

			sprintf( buffer, "%f", scale );
			e.AddEPair( "modelscale", buffer );
		}

		e.BuildInRadiant( false );
	}

	if ( m_autoLink ) {
		DoTrainPathPlot();
	}

	return SIGNAL_STOP_EMISSION;
}

bool DTreePlanter::FindDropPoint( vec3_t in, vec3_t out ) {
	DPlane p1;
	DPlane p2;

	vec3_t vUp =        { 0, 0, 1 };
	vec3_t vForward =   { 0, 1, 0 };
	vec3_t vLeft =      { 1, 0, 0 };

	in[2] = 65535;

	VectorCopy( in, p1.points[0] );
	VectorCopy( in, p1.points[1] );
	VectorCopy( in, p1.points[2] );
	VectorMA( p1.points[1], 20, vUp,         p1.points[1] );
	VectorMA( p1.points[1], 20, vLeft,       p1.points[2] );

	VectorCopy( in, p2.points[0] );
	VectorCopy( in, p2.points[1] );
	VectorCopy( in, p2.points[2] );
	VectorMA( p1.points[1], 20, vUp,         p2.points[1] );
	VectorMA( p1.points[1], 20, vForward,    p2.points[2] );

	p1.Rebuild();
	p2.Rebuild();

	bool found = false;
	vec3_t temp;
	vec_t dist;
	int cnt = m_world.GetIDMax();
	for ( int i = 0; i < cnt; i++ ) {
		DBrush* pBrush = m_world.GetBrushForID( i );

		if ( pBrush->IntersectsWith( &p1, &p2, temp ) ) {
			vec3_t diff;
			vec_t tempdist;
			VectorSubtract( in, temp, diff );
			tempdist = VectorLength( diff );
			if ( !found || ( tempdist < dist ) ) {
				dist = tempdist;
				VectorCopy( temp, out );
				found  = true;
			}
		}
	}

	return found;
}

class TreePlanterDropEntityIfSelected
{
mutable DEntity ent;
DTreePlanter& planter;
public:
TreePlanterDropEntityIfSelected( DTreePlanter& planter ) : planter( planter ){
}
void operator()( scene::Instance& instance ) const {
	if ( !instance.isSelected() ) {
		return;
	}
	ent.LoadFromEntity( instance.path().top() );

	DEPair* pEpair = ent.FindEPairByKey( "origin" );
	if ( !pEpair ) {
		return;
	}

	vec3_t vec, out;
	sscanf( pEpair->value.GetBuffer(), "%f %f %f", &vec[0], &vec[1], &vec[2] );

	planter.FindDropPoint( vec, out );

	char buffer[256];
	sprintf( buffer, "%f %f %f", out[0], out[1], out[2] );
	ent.AddEPair( "origin", buffer );
	ent.RemoveFromRadiant();
	ent.BuildInRadiant( false );
}
};

void DTreePlanter::DropEntsToGround( void ) {
	Scene_forEachEntity( TreePlanterDropEntityIfSelected( *this ) );
}

void DTreePlanter::MakeChain( int linkNum, const char* linkName ) {
	char buffer[256];
	int i;
	for ( i = 0; i < linkNum; i++ ) {
		DEntity e( "info_train_spline_main" );

		sprintf( buffer, "%s_pt%i", linkName, i );
		e.AddEPair( "targetname", buffer );

		sprintf( buffer, "0 %i 0", i * 64 );
		e.AddEPair( "origin", buffer );

		if ( i != m_linkNum - 1 ) {
			sprintf( buffer, "%s_pt%i", linkName, i + 1 );
			e.AddEPair( "target", buffer );

			sprintf( buffer, "%s_ctl%i", linkName, i );
			e.AddEPair( "control", buffer );
		}
		e.BuildInRadiant( false );
	}

	for ( i = 0; i < linkNum - 1; i++ ) {
		DEntity e( "info_train_spline_control" );

		sprintf( buffer, "%s_ctl%i", linkName, i );
		e.AddEPair( "targetname", buffer );

		sprintf( buffer, "0 %i 0", ( i * 64 ) + 32 );
		e.AddEPair( "origin", buffer );

		e.BuildInRadiant( false );
	}
}

void DTreePlanter::SelectChain( void ) {
/*	char buffer[256];

    for(int i = 0; i < m_linkNum; i++) {
        DEntity e("info_train_spline_main");

        sprintf( buffer, "%s_pt%i", m_linkName, i );
        e.AddEPair( "targetname", buffer );

        sprintf( buffer, "0 %i 0", i * 64 );
        e.AddEPair( "origin", buffer );

        if(i != m_linkNum-1) {
            sprintf( buffer, "%s_pt%i", m_linkName, i+1 );
            e.AddEPair( "target", buffer );

            sprintf( buffer, "%s_ctl%i", m_linkName, i );
            e.AddEPair( "control", buffer );
        }

        e.BuildInRadiant( false );
    }

    for(int i = 0; i < m_linkNum-1; i++) {
        DEntity e("info_train_spline_control");

        sprintf( buffer, "%s_ctl%i", m_linkName, i );
        e.AddEPair( "targetname", buffer );

        sprintf( buffer, "0 %i 0", (i * 64) + 32);
        e.AddEPair( "origin", buffer );

        e.BuildInRadiant( false );
    }*/
}
