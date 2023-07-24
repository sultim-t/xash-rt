/*
gl_rmain.c - renderer main loop
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "gl_local.h"
#include "xash3d_mathlib.h"
#include "library.h"
#include "beamdef.h"
#include "particledef.h"
#include "entity_types.h"

#define IsLiquidContents( cnt )	( cnt == CONTENTS_WATER || cnt == CONTENTS_SLIME || cnt == CONTENTS_LAVA )

float		gldepthmin, gldepthmax;
ref_instance_t	RI;

static int R_RankForRenderMode( int rendermode )
{
	switch( rendermode )
	{
	case kRenderTransTexture:
		return 1;	// draw second
	case kRenderTransAdd:
		return 2;	// draw third
	case kRenderGlow:
		return 3;	// must be last!
	}
	return 0;
}

void R_AllowFog( qboolean allowed )
{
	if( allowed )
	{
		if( glState.isFogEnabled )
			pglEnable( GL_FOG );
	}
	else
	{
		if( glState.isFogEnabled )
			pglDisable( GL_FOG );
	}
}

/*
===============
R_OpaqueEntity

Opaque entity can be brush or studio model but sprite
===============
*/
static qboolean R_OpaqueEntity( cl_entity_t *ent )
{
	if( R_GetEntityRenderMode( ent ) == kRenderNormal )
		return true;
	return false;
}

/*
===============
R_TransEntityCompare

Sorting translucent entities by rendermode then by distance
===============
*/
static int R_TransEntityCompare( const void *a, const void *b )
{
	cl_entity_t	*ent1, *ent2;
	vec3_t		vecLen, org;
	float		dist1, dist2;
	int		rendermode1;
	int		rendermode2;

	ent1 = *(cl_entity_t **)a;
	ent2 = *(cl_entity_t **)b;
	rendermode1 = R_GetEntityRenderMode( ent1 );
	rendermode2 = R_GetEntityRenderMode( ent2 );

	// sort by distance
	if( ent1->model->type != mod_brush || rendermode1 != kRenderTransAlpha )
	{
		VectorAverage( ent1->model->mins, ent1->model->maxs, org );
		VectorAdd( ent1->origin, org, org );
		VectorSubtract( RI.vieworg, org, vecLen );
		dist1 = DotProduct( vecLen, vecLen );
	}
	else dist1 = 1000000000;

	if( ent2->model->type != mod_brush || rendermode2 != kRenderTransAlpha )
	{
		VectorAverage( ent2->model->mins, ent2->model->maxs, org );
		VectorAdd( ent2->origin, org, org );
		VectorSubtract( RI.vieworg, org, vecLen );
		dist2 = DotProduct( vecLen, vecLen );
	}
	else dist2 = 1000000000;

	if( dist1 > dist2 )
		return -1;
	if( dist1 < dist2 )
		return 1;

	// then sort by rendermode
	if( R_RankForRenderMode( rendermode1 ) > R_RankForRenderMode( rendermode2 ))
		return 1;
	if( R_RankForRenderMode( rendermode1 ) < R_RankForRenderMode( rendermode2 ))
		return -1;

	return 0;
}

/*
===============
R_WorldToScreen

Convert a given point from world into screen space
Returns true if we behind to screen
===============
*/
int R_WorldToScreen( const vec3_t point, vec3_t screen )
{
	matrix4x4	worldToScreen;
	qboolean	behind;
	float	w;

	if( !point || !screen )
		return true;

	Matrix4x4_Copy( worldToScreen, RI.worldviewProjectionMatrix );
	screen[0] = worldToScreen[0][0] * point[0] + worldToScreen[0][1] * point[1] + worldToScreen[0][2] * point[2] + worldToScreen[0][3];
	screen[1] = worldToScreen[1][0] * point[0] + worldToScreen[1][1] * point[1] + worldToScreen[1][2] * point[2] + worldToScreen[1][3];
	w = worldToScreen[3][0] * point[0] + worldToScreen[3][1] * point[1] + worldToScreen[3][2] * point[2] + worldToScreen[3][3];
	screen[2] = 0.0f; // just so we have something valid here

	if( w < 0.001f )
	{
		behind = true;
	}
	else
	{
		float invw = 1.0f / w;
		screen[0] *= invw;
		screen[1] *= invw;
		behind = false;
	}

	return behind;
}

/*
===============
R_ScreenToWorld

Convert a given point from screen into world space
===============
*/
void R_ScreenToWorld( const vec3_t screen, vec3_t point )
{
	matrix4x4	screenToWorld;
	float	w;

	if( !point || !screen )
		return;

	Matrix4x4_Invert_Full( screenToWorld, RI.worldviewProjectionMatrix );

	point[0] = screen[0] * screenToWorld[0][0] + screen[1] * screenToWorld[0][1] + screen[2] * screenToWorld[0][2] + screenToWorld[0][3];
	point[1] = screen[0] * screenToWorld[1][0] + screen[1] * screenToWorld[1][1] + screen[2] * screenToWorld[1][2] + screenToWorld[1][3];
	point[2] = screen[0] * screenToWorld[2][0] + screen[1] * screenToWorld[2][1] + screen[2] * screenToWorld[2][2] + screenToWorld[2][3];
	w = screen[0] * screenToWorld[3][0] + screen[1] * screenToWorld[3][1] + screen[2] * screenToWorld[3][2] + screenToWorld[3][3];
	if( w != 0.0f ) VectorScale( point, ( 1.0f / w ), point );
}

/*
===============
R_PushScene
===============
*/
void R_PushScene( void )
{
	if( ++tr.draw_stack_pos >= MAX_DRAW_STACK )
		gEngfuncs.Host_Error( "draw stack overflow\n" );

	tr.draw_list = &tr.draw_stack[tr.draw_stack_pos];
}

/*
===============
R_PopScene
===============
*/
void R_PopScene( void )
{
	if( --tr.draw_stack_pos < 0 )
		gEngfuncs.Host_Error( "draw stack underflow\n" );
	tr.draw_list = &tr.draw_stack[tr.draw_stack_pos];
}

/*
===============
R_ClearScene
===============
*/
void R_ClearScene( void )
{
	tr.draw_list->num_solid_entities = 0;
	tr.draw_list->num_trans_entities = 0;
	tr.draw_list->num_beam_entities = 0;

	// clear the scene befor start new frame
	if( gEngfuncs.drawFuncs->R_ClearScene != NULL )
		gEngfuncs.drawFuncs->R_ClearScene();

}

/*
===============
R_AddEntity
===============
*/
qboolean R_AddEntity( struct cl_entity_s *clent, int type )
{
	if( !r_drawentities->value )
		return false; // not allow to drawing

	if( !clent || !clent->model )
		return false; // if set to invisible, skip

	if( FBitSet( clent->curstate.effects, EF_NODRAW ))
		return false; // done

	if( !R_ModelOpaque( clent->curstate.rendermode ) && CL_FxBlend( clent ) <= 0 )
		return true; // invisible

	switch( type )
	{
	case ET_FRAGMENTED:
		r_stats.c_client_ents++;
		break;
	case ET_TEMPENTITY:
		r_stats.c_active_tents_count++;
		break;
	default: break;
	}

	if( R_OpaqueEntity( clent ))
	{
		// opaque
		if( tr.draw_list->num_solid_entities >= MAX_VISIBLE_PACKET )
			return false;

		tr.draw_list->solid_entities[tr.draw_list->num_solid_entities] = clent;
		tr.draw_list->num_solid_entities++;
	}
	else
	{
		// translucent
		if( tr.draw_list->num_trans_entities >= MAX_VISIBLE_PACKET )
			return false;

		tr.draw_list->trans_entities[tr.draw_list->num_trans_entities] = clent;
		tr.draw_list->num_trans_entities++;
	}

	return true;
}

/*
=============
R_Clear
=============
*/
static void R_Clear( int bitMask )
{
	int	bits;

	if( ENGINE_GET_PARM( PARM_DEV_OVERVIEW ))
		pglClearColor( 0.0f, 1.0f, 0.0f, 1.0f ); // green background (Valve rules)
	else pglClearColor( 0.5f, 0.5f, 0.5f, 1.0f );

	bits = GL_DEPTH_BUFFER_BIT;

	if( glState.stencilEnabled )
		bits |= GL_STENCIL_BUFFER_BIT;

	bits &= bitMask;

	pglClear( bits );

	// change ordering for overview
	if( RI.drawOrtho )
	{
		gldepthmin = 1.0f;
		gldepthmax = 0.0f;
	}
	else
	{
		gldepthmin = 0.0f;
		gldepthmax = 1.0f;
	}

	pglDepthFunc( GL_LEQUAL );
	pglDepthRange( gldepthmin, gldepthmax );
}

//=============================================================================
/*
===============
R_GetFarClip
===============
*/
static float R_GetFarClip( void )
{
	if( WORLDMODEL && RI.drawWorld )
		return MOVEVARS->zmax * 1.73f;
	return 2048.0f;
}

#if XASH_RAYTRACING
static float R_GetNearClip( void )
{
	return 4.0f;
}
#endif

/*
===============
R_SetupFrustum
===============
*/
void R_SetupFrustum( void )
{
	const ref_overview_t	*ov = gEngfuncs.GetOverviewParms();

	if( RP_NORMALPASS() && ( ENGINE_GET_PARM( PARM_WATER_LEVEL ) >= 3 ))
	{
		RI.fov_x = atan( tan( DEG2RAD( RI.fov_x ) / 2 ) * ( 0.97f + sin( gpGlobals->time * 1.5f ) * 0.03f )) * 2 / (M_PI_F / 180.0f);
		RI.fov_y = atan( tan( DEG2RAD( RI.fov_y ) / 2 ) * ( 1.03f - sin( gpGlobals->time * 1.5f ) * 0.03f )) * 2 / (M_PI_F / 180.0f);
	}

	// build the transformation matrix for the given view angles
	AngleVectors( RI.viewangles, RI.vforward, RI.vright, RI.vup );

	if( !r_lockfrustum->value )
	{
		VectorCopy( RI.vieworg, RI.cullorigin );
		VectorCopy( RI.vforward, RI.cull_vforward );
		VectorCopy( RI.vright, RI.cull_vright );
		VectorCopy( RI.vup, RI.cull_vup );
	}

	if( RI.drawOrtho )
		GL_FrustumInitOrtho( &RI.frustum, ov->xLeft, ov->xRight, ov->yTop, ov->yBottom, ov->zNear, ov->zFar );
	else GL_FrustumInitProj( &RI.frustum, 0.0f, R_GetFarClip(), RI.fov_x, RI.fov_y ); // NOTE: we ignore nearplane here (mirrors only)
}

/*
=============
R_SetupProjectionMatrix
=============
*/
static void R_SetupProjectionMatrix( matrix4x4 m )
{
	GLfloat	xMin, xMax, yMin, yMax, zNear, zFar;

	if( RI.drawOrtho )
	{
		const ref_overview_t *ov = gEngfuncs.GetOverviewParms();
		Matrix4x4_CreateOrtho( m, ov->xLeft, ov->xRight, ov->yTop, ov->yBottom, ov->zNear, ov->zFar );
		return;
	}

	RI.farClip = R_GetFarClip();

#if !XASH_RAYTRACING
	zNear = 4.0f;
#else
	zNear = R_GetNearClip();
#endif

	zFar = Q_max( 256.0f, RI.farClip );

	yMax = zNear * tan( RI.fov_y * M_PI_F / 360.0f );
	yMin = -yMax;

	xMax = zNear * tan( RI.fov_x * M_PI_F / 360.0f );
	xMin = -xMax;

	Matrix4x4_CreateProjection( m, xMax, xMin, yMax, yMin, zNear, zFar );
}

/*
=============
R_SetupModelviewMatrix
=============
*/
static void R_SetupModelviewMatrix( matrix4x4 m )
{
	Matrix4x4_CreateModelview( m );
	Matrix4x4_ConcatRotate( m, -RI.viewangles[2], 1, 0, 0 );
	Matrix4x4_ConcatRotate( m, -RI.viewangles[0], 0, 1, 0 );
	Matrix4x4_ConcatRotate( m, -RI.viewangles[1], 0, 0, 1 );
	Matrix4x4_ConcatTranslate( m, -RI.vieworg[0], -RI.vieworg[1], -RI.vieworg[2] );
}

/*
=============
R_LoadIdentity
=============
*/
void R_LoadIdentity( void )
{
	if( tr.modelviewIdentity ) return;

	Matrix4x4_LoadIdentity( RI.objectMatrix );
	Matrix4x4_Copy( RI.modelviewMatrix, RI.worldviewMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.modelviewMatrix );
	tr.modelviewIdentity = true;
}

/*
=============
R_RotateForEntity
=============
*/
void R_RotateForEntity( cl_entity_t *e )
{
	float	scale = 1.0f;

	if( e == gEngfuncs.GetEntityByIndex( 0 ) )
	{
		R_LoadIdentity();
		return;
	}

	if( e->model->type != mod_brush && e->curstate.scale > 0.0f )
		scale = e->curstate.scale;

	Matrix4x4_CreateFromEntity( RI.objectMatrix, e->angles, e->origin, scale );
	Matrix4x4_ConcatTransforms( RI.modelviewMatrix, RI.worldviewMatrix, RI.objectMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.modelviewMatrix );
	tr.modelviewIdentity = false;
}

/*
=============
R_TranslateForEntity
=============
*/
void R_TranslateForEntity( cl_entity_t *e )
{
	float	scale = 1.0f;

	if( e == gEngfuncs.GetEntityByIndex( 0 ) )
	{
		R_LoadIdentity();
		return;
	}

	if( e->model->type != mod_brush && e->curstate.scale > 0.0f )
		scale = e->curstate.scale;

	Matrix4x4_CreateFromEntity( RI.objectMatrix, vec3_origin, e->origin, scale );
	Matrix4x4_ConcatTransforms( RI.modelviewMatrix, RI.worldviewMatrix, RI.objectMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.modelviewMatrix );
	tr.modelviewIdentity = false;
}

/*
===============
R_FindViewLeaf
===============
*/
void R_FindViewLeaf( void )
{
	RI.oldviewleaf = RI.viewleaf;
	RI.viewleaf = gEngfuncs.Mod_PointInLeaf( RI.pvsorigin, WORLDMODEL->nodes );
}

/*
===============
R_SetupFrame
===============
*/
static void R_SetupFrame( void )
{
	// setup viewplane dist
	RI.viewplanedist = DotProduct( RI.vieworg, RI.vforward );

	// NOTE: this request is the fps-killer on some NVidia drivers
	glState.isFogEnabled = pglIsEnabled( GL_FOG );

	if( !gl_nosort->value )
	{
		// sort translucents entities by rendermode and distance
		qsort( tr.draw_list->trans_entities, tr.draw_list->num_trans_entities, sizeof( cl_entity_t* ), R_TransEntityCompare );
	}

	// current viewleaf
	if( RI.drawWorld )
	{
		RI.isSkyVisible = false; // unknown at this moment
		R_FindViewLeaf();
	}
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL( qboolean set_gl_state )
{
	R_SetupModelviewMatrix( RI.worldviewMatrix );
	R_SetupProjectionMatrix( RI.projectionMatrix );

	Matrix4x4_Concat( RI.worldviewProjectionMatrix, RI.projectionMatrix, RI.worldviewMatrix );

	if( !set_gl_state ) return;

	if( RP_NORMALPASS( ))
	{
		int	x, x2, y, y2;

		// set up viewport (main, playersetup)
		x = floor( RI.viewport[0] * gpGlobals->width / gpGlobals->width );
		x2 = ceil(( RI.viewport[0] + RI.viewport[2] ) * gpGlobals->width / gpGlobals->width );
		y = floor( gpGlobals->height - RI.viewport[1] * gpGlobals->height / gpGlobals->height );
		y2 = ceil( gpGlobals->height - ( RI.viewport[1] + RI.viewport[3] ) * gpGlobals->height / gpGlobals->height );

		pglViewport( x, y2, x2 - x, y - y2 );
	}
	else
	{
		// envpass, mirrorpass
		pglViewport( RI.viewport[0], RI.viewport[1], RI.viewport[2], RI.viewport[3] );
	}

	pglMatrixMode( GL_PROJECTION );
	GL_LoadMatrix( RI.projectionMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.worldviewMatrix );

	if( FBitSet( RI.params, RP_CLIPPLANE ))
	{
		GLdouble	clip[4];
		mplane_t	*p = &RI.clipPlane;

		clip[0] = p->normal[0];
		clip[1] = p->normal[1];
		clip[2] = p->normal[2];
		clip[3] = -p->dist;

		pglClipPlane( GL_CLIP_PLANE0, clip );
		pglEnable( GL_CLIP_PLANE0 );
	}

	GL_Cull( GL_FRONT );

	pglDisable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
}

/*
=============
R_EndGL
=============
*/
static void R_EndGL( void )
{
	if( RI.params & RP_CLIPPLANE )
		pglDisable( GL_CLIP_PLANE0 );
}

/*
=============
R_RecursiveFindWaterTexture

using to find source waterleaf with
watertexture to grab fog values from it
=============
*/
static gl_texture_t *R_RecursiveFindWaterTexture( const mnode_t *node, const mnode_t *ignore, qboolean down )
{
	gl_texture_t *tex = NULL;

	// assure the initial node is not null
	// we could check it here, but we would rather check it
	// outside the call to get rid of one additional recursion level
	Assert( node != NULL );

	// ignore solid nodes
	if( node->contents == CONTENTS_SOLID )
		return NULL;

	if( node->contents < 0 )
	{
		mleaf_t		*pleaf;
		msurface_t	**mark;
		int		i, c;

		// ignore non-liquid leaves
		if( node->contents != CONTENTS_WATER && node->contents != CONTENTS_LAVA && node->contents != CONTENTS_SLIME )
			 return NULL;

		// find texture
		pleaf = (mleaf_t *)node;
		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		for( i = 0; i < c; i++, mark++ )
		{
			if( (*mark)->flags & SURF_DRAWTURB && (*mark)->texinfo && (*mark)->texinfo->texture )
				return R_GetTexture( (*mark)->texinfo->texture->gl_texturenum );
		}

		// texture not found
		return NULL;
	}

	// this is a regular node
	// traverse children
	if( node->children[0] && ( node->children[0] != ignore ))
	{
		tex = R_RecursiveFindWaterTexture( node->children[0], node, true );
		if( tex ) return tex;
	}

	if( node->children[1] && ( node->children[1] != ignore ))
	{
		tex = R_RecursiveFindWaterTexture( node->children[1], node, true );
		if( tex )	return tex;
	}

	// for down recursion, return immediately
	if( down ) return NULL;

	// texture not found, step up if any
	if( node->parent )
		return R_RecursiveFindWaterTexture( node->parent, node, false );

	// top-level node, bail out
	return NULL;
}

/*
=============
R_CheckFog

check for underwater fog
Using backward recursion to find waterline leaf
from underwater leaf (idea: XaeroX)
=============
*/
static void R_CheckFog( void )
{
	cl_entity_t	*ent;
	gl_texture_t	*tex;
	int		i, cnt, count;

	// quake global fog
	if( ENGINE_GET_PARM( PARM_QUAKE_COMPATIBLE ))
	{
		if( !MOVEVARS->fog_settings )
		{
			if( pglIsEnabled( GL_FOG ))
				pglDisable( GL_FOG );
			RI.fogEnabled = false;
			return;
		}

		// quake-style global fog
		RI.fogColor[0] = ((MOVEVARS->fog_settings & 0xFF000000) >> 24) / 255.0f;
		RI.fogColor[1] = ((MOVEVARS->fog_settings & 0xFF0000) >> 16) / 255.0f;
		RI.fogColor[2] = ((MOVEVARS->fog_settings & 0xFF00) >> 8) / 255.0f;
		RI.fogDensity = ((MOVEVARS->fog_settings & 0xFF) / 255.0f) * 0.01f;
		RI.fogStart = RI.fogEnd = 0.0f;
		RI.fogColor[3] = 1.0f;
		RI.fogCustom = false;
		RI.fogEnabled = true;
		RI.fogSkybox = true;
		return;
	}

	RI.fogEnabled = false;

	if( RI.onlyClientDraw || ENGINE_GET_PARM( PARM_WATER_LEVEL ) < 3 || !RI.drawWorld || !RI.viewleaf )
	{
		if( RI.cached_waterlevel == 3 )
		{
			// in some cases waterlevel jumps from 3 to 1. Catch it
			RI.cached_waterlevel = ENGINE_GET_PARM( PARM_WATER_LEVEL );
			RI.cached_contents = CONTENTS_EMPTY;
			if( !RI.fogCustom )
			{
				glState.isFogEnabled = false;
				pglDisable( GL_FOG );
			}
		}
		return;
	}

	ent = gEngfuncs.CL_GetWaterEntity( RI.vieworg );
	if( ent && ent->model && ent->model->type == mod_brush && ent->curstate.skin < 0 )
		cnt = ent->curstate.skin;
	else cnt = RI.viewleaf->contents;

	RI.cached_waterlevel = ENGINE_GET_PARM( PARM_WATER_LEVEL );

	if( !IsLiquidContents( RI.cached_contents ) && IsLiquidContents( cnt ))
	{
		tex = NULL;

		// check for water texture
		if( ent && ent->model && ent->model->type == mod_brush )
		{
			msurface_t	*surf;

			count = ent->model->nummodelsurfaces;

			for( i = 0, surf = &ent->model->surfaces[ent->model->firstmodelsurface]; i < count; i++, surf++ )
			{
				if( surf->flags & SURF_DRAWTURB && surf->texinfo && surf->texinfo->texture )
				{
					tex = R_GetTexture( surf->texinfo->texture->gl_texturenum );
					RI.cached_contents = ent->curstate.skin;
					break;
				}
			}
		}
		else
		{
			tex = R_RecursiveFindWaterTexture( RI.viewleaf->parent, NULL, false );
			if( tex ) RI.cached_contents = RI.viewleaf->contents;
		}

		if( !tex ) return;	// no valid fogs

		// copy fog params
		RI.fogColor[0] = tex->fogParams[0] / 255.0f;
		RI.fogColor[1] = tex->fogParams[1] / 255.0f;
		RI.fogColor[2] = tex->fogParams[2] / 255.0f;
		RI.fogDensity = tex->fogParams[3] * 0.000025f;
		RI.fogStart = RI.fogEnd = 0.0f;
		RI.fogColor[3] = 1.0f;
		RI.fogCustom = false;
		RI.fogEnabled = true;
		RI.fogSkybox = true;
	}
	else
	{
		RI.fogCustom = false;
		RI.fogEnabled = true;
		RI.fogSkybox = true;
	}
}

/*
=============
R_CheckGLFog

special condition for Spirit 1.9
that used direct calls of glFog-functions
=============
*/
static void R_CheckGLFog( void )
{
#ifdef HACKS_RELATED_HLMODS
	if(( !RI.fogEnabled && !RI.fogCustom ) && pglIsEnabled( GL_FOG ) && VectorIsNull( RI.fogColor ))
	{
		// fill the fog color from GL-state machine
		pglGetFloatv( GL_FOG_COLOR, RI.fogColor );
		RI.fogSkybox = true;
	}
#endif
}

/*
=============
R_DrawFog

=============
*/
void R_DrawFog( void )
{
	if( !RI.fogEnabled ) return;

	pglEnable( GL_FOG );
	if( ENGINE_GET_PARM( PARM_QUAKE_COMPATIBLE ))
		pglFogi( GL_FOG_MODE, GL_EXP2 );
	else pglFogi( GL_FOG_MODE, GL_EXP );
	pglFogf( GL_FOG_DENSITY, RI.fogDensity );
	pglFogfv( GL_FOG_COLOR, RI.fogColor );
	pglHint( GL_FOG_HINT, GL_NICEST );
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList( void )
{
	int	i;

	tr.blend = 1.0f;
	GL_CheckForErrors();

	// first draw solid entities
	for( i = 0; i < tr.draw_list->num_solid_entities && !RI.onlyClientDraw; i++ )
	{
		RI.currententity = tr.draw_list->solid_entities[i];
		RI.currentmodel = RI.currententity->model;

		Assert( RI.currententity != NULL );
		Assert( RI.currentmodel != NULL );

		switch( RI.currentmodel->type )
		{
		case mod_brush:
			R_DrawBrushModel( RI.currententity );
			break;
		case mod_alias:
			R_DrawAliasModel( RI.currententity );
			break;
		case mod_studio:
			R_DrawStudioModel( RI.currententity );
			break;
		default:
			break;
		}
	}

	GL_CheckForErrors();

	// quake-specific feature
	R_DrawAlphaTextureChains();

	GL_CheckForErrors();

	// draw sprites seperately, because of alpha blending
	for( i = 0; i < tr.draw_list->num_solid_entities && !RI.onlyClientDraw; i++ )
	{
		RI.currententity = tr.draw_list->solid_entities[i];
		RI.currentmodel = RI.currententity->model;

		Assert( RI.currententity != NULL );
		Assert( RI.currentmodel != NULL );

		switch( RI.currentmodel->type )
		{
		case mod_sprite:
			R_DrawSpriteModel( RI.currententity );
			break;
		}
	}

	GL_CheckForErrors();

	if( !RI.onlyClientDraw )
	{
		gEngfuncs.CL_DrawEFX( tr.frametime, false );
	}

	GL_CheckForErrors();

	if( RI.drawWorld )
		gEngfuncs.pfnDrawNormalTriangles();

	GL_CheckForErrors();

	// then draw translucent entities
	for( i = 0; i < tr.draw_list->num_trans_entities && !RI.onlyClientDraw; i++ )
	{
		RI.currententity = tr.draw_list->trans_entities[i];
		RI.currentmodel = RI.currententity->model;

		// handle studiomodels with custom rendermodes on texture
		if( RI.currententity->curstate.rendermode != kRenderNormal )
			tr.blend = CL_FxBlend( RI.currententity ) / 255.0f;
		else tr.blend = 1.0f; // draw as solid but sorted by distance

		if( tr.blend <= 0.0f ) continue;

		Assert( RI.currententity != NULL );
		Assert( RI.currentmodel != NULL );

		switch( RI.currentmodel->type )
		{
		case mod_brush:
			R_DrawBrushModel( RI.currententity );
			break;
		case mod_alias:
			R_DrawAliasModel( RI.currententity );
			break;
		case mod_studio:
			R_DrawStudioModel( RI.currententity );
			break;
		case mod_sprite:
			R_DrawSpriteModel( RI.currententity );
			break;
		default:
			break;
		}
	}

	GL_CheckForErrors();

	if( RI.drawWorld )
	{
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		gEngfuncs.pfnDrawTransparentTriangles ();
	}

	GL_CheckForErrors();

	if( !RI.onlyClientDraw )
	{
		R_AllowFog( false );
		gEngfuncs.CL_DrawEFX( tr.frametime, true );
		R_AllowFog( true );
	}

	GL_CheckForErrors();

	pglDisable( GL_BLEND );	// Trinity Render issues

	if( !RI.onlyClientDraw )
		R_DrawViewModel();
	gEngfuncs.CL_ExtraUpdate();

	GL_CheckForErrors();
}

/*
================
R_RenderScene

R_SetupRefParams must be called right before
================
*/
void R_RenderScene( void )
{
	if( !WORLDMODEL && RI.drawWorld )
		gEngfuncs.Host_Error( "R_RenderView: NULL worldmodel\n" );

	// frametime is valid only for normal pass
	if( RP_NORMALPASS( ))
		tr.frametime = gpGlobals->time -   gpGlobals->oldtime;
	else tr.frametime = 0.0;

	// begin a new frame
	tr.framecount++;

	R_PushDlights();

	R_SetupFrustum();
	R_SetupFrame();
	R_SetupGL( true );
	R_Clear( ~0 );

#if XASH_RAYTRACING
    {
        RgCameraInfo info = {
            .sType       = RG_STRUCTURE_TYPE_CAMERA_INFO,
            .pNext       = NULL,
            .fovYRadians = DEG2RAD( RI.fov_y ),
            .aspect      = ( float )gpGlobals->width / ( float )gpGlobals->height,
            .cameraNear  = R_GetNearClip(),
            .cameraFar   = R_GetFarClip(),
        };
        // reinterpret cast to make matrices column-major
        matrix4x4* v = ( matrix4x4* )&info.view;
        Matrix4x4_Transpose( *v, RI.worldviewMatrix );

		RgResult r = rgUploadCamera( rg_instance, &info );
        RG_CHECK( r );
    }
#endif

	R_MarkLeaves();
	R_DrawFog ();

	R_CheckGLFog();
	R_DrawWorld();
	R_CheckFog();

	gEngfuncs.CL_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList();

	R_DrawWaterSurfaces();

	R_EndGL();
}

/*
===============
R_CheckGamma
===============
*/
void R_CheckGamma( void )
{
	if( gEngfuncs.R_DoResetGamma( ))
	{
		// paranoia cubemaps uses this
		gEngfuncs.BuildGammaTable( 1.8f, 0.0f );

		// paranoia cubemap rendering
		if( gEngfuncs.drawFuncs->GL_BuildLightmaps )
			gEngfuncs.drawFuncs->GL_BuildLightmaps( );
	}
	else if( FBitSet( vid_gamma->flags, FCVAR_CHANGED ) || FBitSet( vid_brightness->flags, FCVAR_CHANGED ))
	{
		gEngfuncs.BuildGammaTable( vid_gamma->value, vid_brightness->value );
		glConfig.softwareGammaUpdate = true;
		GL_RebuildLightmaps();
		glConfig.softwareGammaUpdate = false;
	}
}

#if XASH_RAYTRACING
extern const float* rt_portal_posteffect_position;

static qboolean RT_IsPortalPosteffectActive()
{
    if( rt_portal_posteffect_position != NULL )
    {
        const float threshold = METRIC_TO_QUAKEUNIT( 2.0f );

        return VectorDistance2( rt_portal_posteffect_position, RI.vieworg ) < threshold * threshold;
    }

    return false;
}

static float RT_LerpAlpha( float a, float b, float lerp )
{
    float r = a + bound( 0.0f, lerp, 1.0f ) * ( b - a );

	r = powf( r, 2.2f );
    return bound( 0.0f, r, 1.0f );
}

static const char* RT_TryUploadChapterIntroTexture()
{
    static char tex[ 256 ]                      = "";
    static char tex_prev[ RT_ARRAYSIZE( tex ) ] = "";

    const char* chaptername = CVAR_TO_STR( rt_cvars._rt_chapter );
    if( !chaptername )
    {
        if( Q_strlen( tex_prev ) > 0 )
        {
            RgResult r = rgMarkOriginalTextureAsDeleted( rg_instance, tex_prev );
            RG_CHECK( r );

            tex_prev[ 0 ] = '\0';
        }

        return NULL;
    }

	Q_strncpy( tex, "resource/ch/", RT_ARRAYSIZE( tex ) );
    Q_strncat( tex, chaptername, RT_ARRAYSIZE( tex ) - RT_ARRAYSIZE( "resource/ch/" ) - 1 );

	// same as previous
    if( Q_strncmp( tex, tex_prev, RT_ARRAYSIZE( tex ) ) == 0 )
    {
        return tex;
    }

    {
        RgResult r = rgMarkOriginalTextureAsDeleted( rg_instance, tex_prev );
        RG_CHECK( r );
    }
    {
        const uint32_t pix = 0xFF000000;

        RgOriginalTextureInfo info = {
            .sType        = RG_STRUCTURE_TYPE_ORIGINAL_TEXTURE_INFO,
            .pTextureName = tex,
            .pPixels      = &pix,
            .size         = { 1, 1 },
            .filter       = RG_SAMPLER_FILTER_LINEAR,
            .addressModeU = RG_SAMPLER_ADDRESS_MODE_CLAMP,
            .addressModeV = RG_SAMPLER_ADDRESS_MODE_CLAMP,
        };
        RgResult r = rgProvideOriginalTexture( rg_instance, &info );
        RG_CHECK( r );
    }

    Q_strncpy( tex_prev, tex, RT_ARRAYSIZE( tex ) );
    return tex;
}

static float rt_chaptertime_start     = -1;
static float rt_chaptertime_beginfade = -1;
static float rt_chaptertime_end       = -1;
void RT_ResetChapterLogo( void )
{
    rt_chaptertime_start     = -1;
    rt_chaptertime_beginfade = -1;
    rt_chaptertime_end       = -1;
}

static void RT_TryDrawCustomChapterIntro()
{
    const char* texname = RT_TryUploadChapterIntroTexture();

    if( !texname )
    {
        return;
    }
	
    const float duration_solid   = 3.0f;
    const float duration_fadeout = 2.0f;
    Assert( duration_fadeout > 0 );
    const float background_opacity = 0.3f;
	
    if( RT_CVAR_TO_BOOL( _rt_chaptershow ) )
    {
        rt_chaptertime_start     = gpGlobals->time;
        rt_chaptertime_beginfade = rt_chaptertime_start + duration_solid;
        rt_chaptertime_end       = rt_chaptertime_start + duration_solid + duration_fadeout;

		gEngfuncs.Cvar_Set( rt_cvars._rt_chaptershow->name, "0" );
    }

    float curtime = gpGlobals->time;

    if( curtime < rt_chaptertime_start )
    {
        return;
    }

	if( curtime > rt_chaptertime_end )
    {
        RT_ResetChapterLogo();
        return;
    }

    float alpha = RT_LerpAlpha(
        1.0f, 0.0f, Q_max( 0, curtime - rt_chaptertime_beginfade ) / duration_fadeout );

    if( alpha < 1.0f / 255.0f )
    {
        return;
    }

    static const uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };

    static const RgPrimitiveVertex verts_fullscreen[] = {
        { .position = { -1, +1, 0 }, .texCoord = { 0, 1 }, .color = 0xFFFFFFFF },
        { .position = { -1, -1, 0 }, .texCoord = { 0, 0 }, .color = 0xFFFFFFFF },
        { .position = { +1, -1, 0 }, .texCoord = { 1, 0 }, .color = 0xFFFFFFFF },
        { .position = { +1, +1, 0 }, .texCoord = { 1, 1 }, .color = 0xFFFFFFFF },
    };

    RgPrimitiveVertex verts_16by9[] = {
        { .position = { 0 }, .texCoord = { 0, 1 }, .color = 0xFFFFFFFF },
        { .position = { 0 }, .texCoord = { 0, 0 }, .color = 0xFFFFFFFF },
        { .position = { 0 }, .texCoord = { 1, 0 }, .color = 0xFFFFFFFF },
        { .position = { 0 }, .texCoord = { 1, 1 }, .color = 0xFFFFFFFF },
    };

    {
        float xwin = ( float )gpGlobals->width / ( float )gpGlobals->height;
        float ximg = 16.0f / 9.0f;
		
		float tx, ty;

        if( ximg < xwin )
        {
            tx = ximg / xwin;
            ty = 1.0f;
        }
        else
        {
            tx = 1.0f;
            ty = xwin / ximg;
        }

		VectorSet( verts_16by9[ 0 ].position, -tx, +ty, 0 );
        VectorSet( verts_16by9[ 1 ].position, -tx, -ty, 0 );
		VectorSet( verts_16by9[ 2 ].position, +tx, -ty, 0 );
		VectorSet( verts_16by9[ 3 ].position, +tx, +ty, 0 );
    }

    // background
    {
        RgMeshPrimitiveForceRasterizedEXT raster = {
            .sType           = RG_STRUCTURE_TYPE_MESH_PRIMITIVE_FORCE_RASTERIZED_EXT,
            .pNext           = NULL,
            .pViewport       = NULL,
            .pViewProjection = ( const float* )matrix4x4_identity,
        };
        RgMeshPrimitiveInfo prim = {
            .sType                = RG_STRUCTURE_TYPE_MESH_PRIMITIVE_INFO,
            .pNext                = &raster,
            .pPrimitiveNameInMesh = NULL,
            .primitiveIndexInMesh = 0,
            .flags                = RG_MESH_PRIMITIVE_TRANSLUCENT,
            .pVertices            = verts_fullscreen,
            .vertexCount          = RT_ARRAYSIZE( verts_fullscreen ),
            .pIndices             = indices,
            .indexCount           = RT_ARRAYSIZE( indices ),
            .pTextureName         = NULL,
            .textureFrame         = 0,
            .color    = rgUtilPackColorFloat4D( 0.0f, 0.0f, 0.0f, alpha * background_opacity ),
            .emissive = 0,
        };
        RgResult r = rgUploadMeshPrimitive( rg_instance, NULL, &prim );
        RG_CHECK( r );
    }
    // text
    {
        RgMeshPrimitiveForceRasterizedEXT raster = {
            .sType           = RG_STRUCTURE_TYPE_MESH_PRIMITIVE_FORCE_RASTERIZED_EXT,
            .pNext           = NULL,
            .pViewport       = NULL,
            .pViewProjection = ( const float* )matrix4x4_identity,
        };
        RgMeshPrimitiveInfo prim = {
            .sType                = RG_STRUCTURE_TYPE_MESH_PRIMITIVE_INFO,
            .pNext                = &raster,
            .pPrimitiveNameInMesh = NULL,
            .primitiveIndexInMesh = 0,
            .flags                = RG_MESH_PRIMITIVE_TRANSLUCENT,
            .pVertices            = verts_16by9,
            .vertexCount          = RT_ARRAYSIZE( verts_16by9 ),
            .pIndices             = indices,
            .indexCount           = RT_ARRAYSIZE( indices ),
            .pTextureName         = texname,
            .textureFrame         = 0,
            .color                = rgUtilPackColorFloat4D( 1.0f, 1.0f, 1.0f, alpha ),
            .emissive             = 0,
        };
        RgResult r = rgUploadMeshPrimitive( rg_instance, NULL, &prim );
        RG_CHECK( r );
    }
}
#endif

/*
===============
R_BeginFrame
===============
*/
void R_BeginFrame( qboolean clearScene )
{
	glConfig.softwareGammaUpdate = false;	// in case of possible fails

	if(( gl_clear->value || ENGINE_GET_PARM( PARM_DEV_OVERVIEW )) &&
		clearScene && ENGINE_GET_PARM( PARM_CONNSTATE ) != ca_cinematic )
	{
		pglClear( GL_COLOR_BUFFER_BIT );
	}

	R_CheckGamma();

	R_Set2DMode( true );

	// draw buffer stuff
	pglDrawBuffer( GL_BACK );

#if XASH_RAYTRACING

    if( RT_CVAR_TO_BOOL( rt_forcecvars ) )
    {
		// CVAR_TO_BOOL but without NULL check
    #define TO_BOOL( x ) ( ( ( x )->value != 0.0f ) ? true : false )

        // if classic present, use slower pipeline
        if( RT_CVAR_TO_FLOAT( rt_classic ) > 0.01f )
        {
            if( CVAR_TO_BOOL( r_fullbright ) )
            {
                // disable fullbright
                gEngfuncs.Cvar_Set( r_fullbright->name, "0" );
            }
        }
        else
        {
            if( !TO_BOOL( r_fullbright ) )
            {
                // enable fullbright to reduce CPU-side calculations
                gEngfuncs.Cvar_Set( r_fullbright->name, "1" );
            }
        }

        // dynamic lightmaps not supported
        if( CVAR_TO_BOOL( r_dynamic ) )
        {
            gEngfuncs.Cvar_Set( r_dynamic->name, "0" );
        }
        // always disable culling / PVS
        if( !TO_BOOL( r_nocull ) )
        {
            gEngfuncs.Cvar_Set( r_nocull->name, "1" );
        }
        if( !TO_BOOL( r_novis ) )
        {
            gEngfuncs.Cvar_Set( r_novis->name, "1" );
        }

    #undef TO_BOOL
    }

    {
        char mapname_storage[ 64 ] = "";

        char* mapname = NULL;
        if( WORLDMODEL )
        {
            {
                assert( sizeof( WORLDMODEL->name ) == sizeof( mapname_storage ) );
                memcpy( mapname_storage, WORLDMODEL->name, sizeof( mapname_storage ) );
                mapname_storage[ sizeof( mapname_storage ) - 1 ] = '\0';
            }
            mapname = mapname_storage;

            // try skip "maps/"
            if( Q_strncmp( mapname, "maps/", 5 ) == 0 )
            {
                mapname += 5;
            }

            // try remove ".bsp" at the end
            char* e = Q_strstr( mapname, ".bsp" );
            if( e != NULL )
            {
                *e = '\0';
            }
        }
		
        RgStartFrameInfo info = {
            .sType                  = RG_STRUCTURE_TYPE_START_FRAME_INFO,
            .pNext                  = NULL,
            .pMapName               = mapname,
            .ignoreExternalGeometry = RT_CVAR_TO_FLOAT( rt_classic ) > 0.5f,
            .vsync                  = RT_CVAR_TO_BOOL( rt_vsync ),
        };
		
        RgResult r = rgStartFrame( rg_instance, &info );
        RG_CHECK( r );
    }
#endif

	// update texture parameters
	if( FBitSet( gl_texture_nearest->flags|gl_lightmap_nearest->flags|gl_texture_anisotropy->flags|gl_texture_lodbias->flags, FCVAR_CHANGED ))
		R_SetTextureParameters();

	gEngfuncs.CL_ExtraUpdate ();
}

/*
===============
R_SetupRefParams

set initial params for renderer
===============
*/
void R_SetupRefParams( const ref_viewpass_t *rvp )
{
	RI.params = RP_NONE;
	RI.drawWorld = FBitSet( rvp->flags, RF_DRAW_WORLD );
	RI.onlyClientDraw = FBitSet( rvp->flags, RF_ONLY_CLIENTDRAW );
	RI.farClip = 0;

	if( !FBitSet( rvp->flags, RF_DRAW_CUBEMAP ))
		RI.drawOrtho = FBitSet( rvp->flags, RF_DRAW_OVERVIEW );
	else RI.drawOrtho = false;

	// setup viewport
	RI.viewport[0] = rvp->viewport[0];
	RI.viewport[1] = rvp->viewport[1];
	RI.viewport[2] = rvp->viewport[2];
	RI.viewport[3] = rvp->viewport[3];

	// calc FOV
	RI.fov_x = rvp->fov_x;
	RI.fov_y = rvp->fov_y;

	VectorCopy( rvp->vieworigin, RI.vieworg );
	VectorCopy( rvp->viewangles, RI.viewangles );
	VectorCopy( rvp->vieworigin, RI.pvsorigin );
}

/*
===============
R_RenderFrame
===============
*/
void R_RenderFrame( const ref_viewpass_t *rvp )
{
	if( r_norefresh->value )
		return;

	// setup the initial render params
	R_SetupRefParams( rvp );

	if( gl_finish->value && RI.drawWorld )
		pglFinish();

	if( glConfig.max_multisamples > 1 && FBitSet( gl_msaa->flags, FCVAR_CHANGED ))
	{
		if( CVAR_TO_BOOL( gl_msaa ))
			pglEnable( GL_MULTISAMPLE_ARB );
		else pglDisable( GL_MULTISAMPLE_ARB );
		ClearBits( gl_msaa->flags, FCVAR_CHANGED );
	}

	// completely override rendering
	if( gEngfuncs.drawFuncs->GL_RenderFrame != NULL )
	{
		tr.fCustomRendering = true;

		if( gEngfuncs.drawFuncs->GL_RenderFrame( rvp ))
		{
			R_GatherPlayerLight();
			tr.realframecount++;
			tr.fResetVis = true;
			return;
		}
	}

	tr.fCustomRendering = false;
	if( !RI.onlyClientDraw )
		R_RunViewmodelEvents();

	tr.realframecount++; // right called after viewmodel events
	R_RenderScene();

#if XASH_RAYTRACING
    RT_TryDrawCustomChapterIntro();
#endif

	return;
}

#if XASH_RAYTRACING
enum
{
    RT_VINTAGE_OFF,
    RT_VINTAGE_CRT,
    RT_VINTAGE_200,
    RT_VINTAGE_480,
    RT_VINTAGE_720,

    RT_VINTAGE__COUNT
};
// this enum must match pVintageNames from VideoModes.cpp

static void ResolutionToRtgl( RgDrawFrameRenderResolutionParams* dst,
                              const RgExtent2D                   winsize,
                              RgExtent2D*                        storage )
{
    const float aspect = ( float )winsize.width / ( float )winsize.height;

    if( RT_CVAR_TO_INT32( rt_renderscale ) > 0 )
    {
        float scale = ( float )RT_CVAR_TO_INT32( rt_renderscale ) / 100.0f;
        scale       = RT_CLAMP( scale, 0.2f, 1.0f );

        dst->customRenderSize.width  = ( uint32_t )( scale * ( float )winsize.width );
        dst->customRenderSize.height = ( uint32_t )( scale * ( float )winsize.height );
        dst->pPixelizedRenderSize    = NULL;

        return;
    }
    else
    {
        if( RT_CVAR_TO_INT32( rt_ef_vintage ) != RT_VINTAGE_OFF )
        {
            uint32_t h_pixelized = 0;
            uint32_t h_render    = 0;

            switch( RT_CVAR_TO_INT32( rt_ef_vintage ) )
            {
                case RT_VINTAGE_200:
                    h_pixelized = 200;
                    h_render    = 400;
                    break;

                case RT_VINTAGE_480:
                    h_pixelized = 480;
                    h_render    = 600;
                    break;

                case RT_VINTAGE_CRT:
                    h_pixelized = 480;
                    h_render    = 480;
                    break;

                case RT_VINTAGE_720:
                    h_pixelized = 720;
                    h_render    = 720;
                    break;

                default:
                    gEngfuncs.Cvar_SetValue( rt_cvars.rt_ef_vintage->name, 0 );
                    dst->customRenderSize     = winsize;
                    dst->pPixelizedRenderSize = NULL;
                    return;
            }

            assert( h_render > 0 && h_pixelized > 0 );

            storage->height              = h_pixelized;
            storage->width               = ( uint32_t )( h_pixelized * aspect );
            dst->pPixelizedRenderSize    = storage;
            dst->customRenderSize.height = h_render;
            dst->customRenderSize.width  = ( uint32_t )( h_render * aspect );

            return;
        }
    }

    dst->customRenderSize     = winsize;
    dst->pPixelizedRenderSize = NULL;
}

static RgRenderSharpenTechnique GetSharpenTechniqueFromCvar()
{
    int vintage = RT_CVAR_TO_INT32( rt_ef_vintage );
    int t       = RT_CVAR_TO_INT32( rt_sharpen );

    switch( t )
    {
        case 2: return RG_RENDER_SHARPEN_TECHNIQUE_AMD_CAS;
        case 1: return RG_RENDER_SHARPEN_TECHNIQUE_NAIVE;
        default:
            // to accentuate a chunky look, because of the linear (not nearest) downscale mode
            if( vintage == RT_VINTAGE_200 || vintage == RT_VINTAGE_480 )
            {
                return RG_RENDER_SHARPEN_TECHNIQUE_AMD_CAS;
            }
            return RG_RENDER_SHARPEN_TECHNIQUE_NONE;
    }
}

static void UpscaleCvarsToRtgl( RgDrawFrameRenderResolutionParams* pDst )
{
    RgBool32 dlss_ok = rgUtilIsUpscaleTechniqueAvailable( rg_instance, RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS );
    RgBool32 fsr2_ok = rgUtilIsUpscaleTechniqueAvailable( rg_instance, RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2 );
	gEngfuncs.Cvar_Set( rt_cvars._rt_dlss_available->name, dlss_ok ? "1" : "0" );

    int nvDlss = dlss_ok ? RT_CVAR_TO_INT32( rt_upscale_dlss ) : 0;
    int amdFsr = fsr2_ok ? RT_CVAR_TO_INT32( rt_upscale_fsr2 ) : 0;

    switch( nvDlss )
    {
        case 1:
            // start with Quality
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_QUALITY;
            break;
        case 2:
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_BALANCED;
            break;
        case 3:
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_PERFORMANCE;
            break;
        case 4:
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_ULTRA_PERFORMANCE;
            break;

        case 5:
            // use DLSS with rt_renderscale
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_CUSTOM;
            break;

        default: nvDlss = 0; break;
    }

    switch( amdFsr )
    {
        case 1:
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_QUALITY;
            break;
        case 2:
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_BALANCED;
            break;
        case 3:
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_PERFORMANCE;
            break;
        case 4:
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_ULTRA_PERFORMANCE;
            break;

        case 5:
            // use FSR2 with rt_renderscale
            pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2;
            pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_CUSTOM;
            break;

        default: amdFsr = 0; break;
    }

    // both disabled
    if( nvDlss == 0 && amdFsr == 0 )
    {
        pDst->upscaleTechnique = RG_RENDER_UPSCALE_TECHNIQUE_NEAREST;
        pDst->resolutionMode   = RG_RENDER_RESOLUTION_MODE_CUSTOM;
    }

    if( amdFsr )
    {
        pDst->sharpenTechnique = RG_RENDER_SHARPEN_TECHNIQUE_AMD_CAS;
    }
    else
    {
        pDst->sharpenTechnique = GetSharpenTechniqueFromCvar();
    }
}
#endif

/*
===============
R_EndFrame
===============
*/
void R_EndFrame( void )
{
	// flush any remaining 2D bits
	R_Set2DMode( false );
	gEngfuncs.GL_SwapBuffers();

#if XASH_RAYTRACING
    RT_OnBeforeDrawFrame();
    RT_UploadAllLights();

    RgExtent2D       pixstorage = { 0 };
    const RgExtent2D winsize    = { .width = gpGlobals->width, .height = gpGlobals->height };

    RgDrawFrameRenderResolutionParams resolution_params = {
        .sType = RG_STRUCTURE_TYPE_DRAW_FRAME_RENDER_RESOLUTION_PARAMS,
        .pNext = NULL,
    };
    ResolutionToRtgl( &resolution_params, winsize, &pixstorage );
    UpscaleCvarsToRtgl( &resolution_params );

    {
        static float prevvalue = 0;

        if( fabsf( RT_CVAR_TO_FLOAT( rt_classic ) - prevvalue ) > 0.9f )
        {
            resolution_params.resetUpscalerHistory = true;
        }
        prevvalue = RT_CVAR_TO_FLOAT( rt_classic );
    }

	assert( tr.lightstylevalue[ 0 ] == 0 || tr.lightstylevalue[ 0 ] == 158 );

	float lightstyles[ RT_ARRAYSIZE( tr.lightstylevalue ) ];
    for( uint32_t i = 0; i < RT_ARRAYSIZE( tr.lightstylevalue ); i++ )
    {
        lightstyles[ i ] = ( float )bound( 0, tr.lightstylevalue[ i ], 255 ) / 255.0f;
    }

    RgDrawFrameIlluminationParams illum_params = {
        .sType                              = RG_STRUCTURE_TYPE_DRAW_FRAME_ILLUMINATION_PARAMS,
        .pNext                              = NULL,
        .maxBounceShadows                   = RT_CVAR_TO_UINT32( rt_shadowrays ),
        .enableSecondBounceForIndirect      = RT_CVAR_TO_BOOL( rt_indir2bounces ),
        .cellWorldSize                      = METRIC_TO_QUAKEUNIT( 2.0f ),
        .directDiffuseSensitivityToChange   = 1.0f,
        .indirectDiffuseSensitivityToChange = 0.75f,
        .specularSensitivityToChange        = 1.0f,
        .polygonalLightSpotlightFactor      = 2.0f,
        .lightUniqueIdIgnoreFirstPersonViewerShadows =
            rt_state.flashlight_uniqueid ? &rt_state.flashlight_uniqueid : NULL,
        .lightstyleValuesCount = RT_ARRAYSIZE( lightstyles ),
        .pLightstyleValues     = lightstyles,
    };

    RgDrawFrameTonemappingParams tnmp_params = {
        .sType                = RG_STRUCTURE_TYPE_DRAW_FRAME_TONEMAPPING_PARAMS,
        .pNext                = &illum_params,
        .disableEyeAdaptation = false,
        .ev100Min             = RT_CVAR_TO_FLOAT( rt_tnmp_ev100_min ),
        .ev100Max             = RT_CVAR_TO_FLOAT( rt_tnmp_ev100_max ),
        .luminanceWhitePoint  = RT_CVAR_TO_FLOAT( rt_classic_white ),
        .saturation           = { RT_CVAR_TO_FLOAT( rt_tnmp_saturation_r ),
                                  RT_CVAR_TO_FLOAT( rt_tnmp_saturation_g ),
                                  RT_CVAR_TO_FLOAT( rt_tnmp_saturation_b ) },
        .crosstalk            = { RT_CVAR_TO_FLOAT( rt_tnmp_crosstalk_r ),
                                  RT_CVAR_TO_FLOAT( rt_tnmp_crosstalk_g ),
                                  RT_CVAR_TO_FLOAT( rt_tnmp_crosstalk_b ) },
    };

    RgDrawFrameBloomParams bloom_params = {
        .sType          = RG_STRUCTURE_TYPE_DRAW_FRAME_BLOOM_PARAMS,
        .pNext          = &tnmp_params,
        .bloomIntensity = RT_CVAR_TO_BOOL( rt_bloom ) && RT_CVAR_TO_FLOAT( rt_classic ) < 0.5f
                              ? RT_CVAR_TO_FLOAT( rt_bloom_intensity )
                              : 0.0f,
        .inputThreshold = RT_CVAR_TO_FLOAT( rt_bloom_threshold ),
        .bloomEmissionMultiplier = RT_CVAR_TO_FLOAT( rt_bloom_emis_mult ),
        .lensDirtIntensity =
            RT_CVAR_TO_BOOL( rt_bloom_dirt_enable ) ? RT_CVAR_TO_FLOAT( rt_bloom_dirt ) : 0.0f,
    };

    RgMediaType cameramedia =
        ENGINE_GET_PARM( PARM_WATER_LEVEL ) > 2 ? RG_MEDIA_TYPE_WATER : RG_MEDIA_TYPE_VACUUM;

    RgDrawFrameReflectRefractParams refl_refr_params = {
        .sType                   = RG_STRUCTURE_TYPE_DRAW_FRAME_REFLECT_REFRACT_PARAMS,
        .pNext                   = &bloom_params,
        .maxReflectRefractDepth  = RT_CVAR_TO_UINT32( rt_reflrefr_depth ),
        .typeOfMediaAroundCamera = cameramedia,
        .indexOfRefractionGlass  = RT_CVAR_TO_FLOAT( rt_refr_glass ),
        .indexOfRefractionWater  = RT_CVAR_TO_FLOAT( rt_refr_water ),
        .thinMediaWidth          = METRIC_TO_QUAKEUNIT( RT_CVAR_TO_FLOAT( rt_refr_thinwidth ) ),
        .waterWaveSpeed          = METRIC_TO_QUAKEUNIT( 0.4f ),
        .waterWaveNormalStrength = RT_CVAR_TO_FLOAT( rt_normalmap_stren_water ),
        .waterColor              = { RT_CVAR_TO_FLOAT( rt_me_water_r ) / 255.0f,
                                     RT_CVAR_TO_FLOAT( rt_me_water_g ) / 255.0f,
                                     RT_CVAR_TO_FLOAT( rt_me_water_b ) / 255.0f },
        .acidColor               = { 0.1f, 1.0f, 0.01f },
        .acidDensity             = 100,
        .waterWaveTextureDerivativesMultiplier = 5,
        .waterTextureAreaScale                 = METRIC_TO_QUAKEUNIT( 1.0f ),
        .portalNormalTwirl                     = 0,
    };

    #define VectorPow( v, a ) \
        ( ( v )[ 0 ] = powf( ( v )[ 0 ], ( a ) ), \
          ( v )[ 1 ] = powf( ( v )[ 1 ], ( a ) ), \
          ( v )[ 2 ] = powf( ( v )[ 2 ], ( a ) ) )

    // because 1 quake unit is not 1 meter
    VectorPow( refl_refr_params.waterColor.data, QUAKEUNIT_IN_METERS );

    RgDrawFrameSkyParams sky_params = {
        .sType              = RG_STRUCTURE_TYPE_DRAW_FRAME_SKY_PARAMS,
        .pNext              = &refl_refr_params,
        .skyType            = RI.isSkyVisible ? RG_SKY_TYPE_RASTERIZED_GEOMETRY : RG_SKY_TYPE_COLOR,
        .skyColorDefault    = { 0, 0, 0 },
        .skyColorMultiplier = RT_CVAR_TO_FLOAT( rt_sky ),
        .skyColorSaturation = RT_CVAR_TO_FLOAT( rt_sky_saturation ),
        .skyViewerPosition  = RT_VEC3( RI.vieworg ),
    };
	
    RgDrawFrameVolumetricParams volumetric_params = {
        .sType  = RG_STRUCTURE_TYPE_DRAW_FRAME_VOLUMETRIC_PARAMS,
        .pNext  = &sky_params,
        .enable = RT_CVAR_TO_UINT32( rt_volume_type ) != 0 && RT_CVAR_TO_FLOAT( rt_classic ) < 0.5f,
        .maxHistoryLength =
            RT_CVAR_TO_UINT32( rt_volume_type ) == 1 ? RT_CVAR_TO_FLOAT( rt_volume_history ) : 0,
        .useSimpleDepthBased     = RT_CVAR_TO_UINT32( rt_volume_type ) == 2,
        .volumetricFar           = RT_CVAR_TO_FLOAT( rt_volume_far ),
        .ambientColor            = { RT_CVAR_TO_FLOAT( rt_volume_ambient ),
                                     RT_CVAR_TO_FLOAT( rt_volume_ambient ),
                                     RT_CVAR_TO_FLOAT( rt_volume_ambient ) },
        .scaterring              = RT_CVAR_TO_FLOAT( rt_volume_scatter ),
        .assymetry               = RT_CVAR_TO_FLOAT( rt_volume_lassymetry ),
        .useIlluminationVolume   = RT_CVAR_TO_BOOL( rt_volume_illumgrid ),
        .fallbackSourceColor     = { 0, 0, 0 },
        .fallbackSourceDirection = { 0, -1, 0 },
        .lightMultiplier         = RT_CVAR_TO_FLOAT( rt_volume_lintensity ),
    };

    RgDrawFrameTexturesParams texture_params = {
        .sType                  = RG_STRUCTURE_TYPE_DRAW_FRAME_TEXTURES_PARAMS,
        .pNext                  = &volumetric_params,
        .dynamicSamplerFilter   = RT_CVAR_TO_BOOL( rt_texture_nearest ) ? RG_SAMPLER_FILTER_NEAREST
                                                                        : RG_SAMPLER_FILTER_LINEAR,
        .normalMapStrength      = RT_CVAR_TO_FLOAT( rt_normalmap_stren ),
        .emissionMapBoost       = RT_CVAR_TO_FLOAT( rt_emis_mapboost ),
        .emissionMaxScreenColor = RT_CVAR_TO_FLOAT( rt_emis_maxscrcolor ),
    };

    RgDrawFrameLightmapParams lightmap_params = {
        .sType                  = RG_STRUCTURE_TYPE_DRAW_FRAME_LIGHTMAP_PARAMS,
        .pNext                  = &texture_params,
        .lightmapScreenCoverage = RT_CVAR_TO_FLOAT( rt_classic ),
    };

	qboolean skipframe = false;
    if( rt_cvars._rt_skipframe )
    {
        skipframe = RT_CVAR_TO_BOOL( _rt_skipframe );
        gEngfuncs.Cvar_Set( rt_cvars._rt_skipframe->name, "0" );
    }

    RgPostEffectCRT crt_effect = {
        .isActive =
            RT_CVAR_TO_BOOL( rt_ef_crt ) || RT_CVAR_TO_INT32( rt_ef_vintage ) == RT_VINTAGE_CRT,
    };

    RgPostEffectChromaticAberration chromatic_aberration_effect = {
        .isActive              = RT_CVAR_TO_FLOAT( rt_ef_chraber ) > 0.0f,
        .transitionDurationIn  = 0,
        .transitionDurationOut = 0,
        .intensity             = RT_CVAR_TO_FLOAT( rt_ef_chraber ),
    };

    RgPostEffectWaves waves_effect = {
        .isActive              = cameramedia != RG_MEDIA_TYPE_VACUUM,
        .transitionDurationIn  = 0.1f,
        .transitionDurationOut = 0.75f,
        .amplitude             = RT_CVAR_TO_FLOAT( rt_ef_water ) * 0.01f,
        .speed                 = 1.0f,
        .xMultiplier           = 0.5f,
    };
	
	RgPostEffectTeleport teleport_effect = {
        .isActive              = true,
        .transitionDurationIn  = 1.5f,
        .transitionDurationOut = 0.1f,
    };

    RgDrawFramePostEffectsParams posteffect_params = {
        .sType                = RG_STRUCTURE_TYPE_DRAW_FRAME_POST_EFFECTS_PARAMS,
        .pNext                = &lightmap_params,
        .pChromaticAberration = &chromatic_aberration_effect,
        .pTeleport            = RT_IsPortalPosteffectActive() ? &teleport_effect : NULL,
        .pWaves = ENGINE_GET_PARM( PARM_CONNSTATE ) == ca_active ? &waves_effect : NULL,
        .pCRT   = &crt_effect,
    };

    RgDrawFrameInfo info = {
        .sType            = RG_STRUCTURE_TYPE_DRAW_FRAME_INFO,
        .pNext            = &posteffect_params,
        .rayLength        = R_GetFarClip(),
        .rayCullMaskWorld = RG_DRAW_FRAME_RAY_CULL_WORLD_0_BIT |
                            RG_DRAW_FRAME_RAY_CULL_WORLD_1_BIT | RG_DRAW_FRAME_RAY_CULL_SKY_BIT,
        .presentPrevFrame = skipframe,
        .currentTime      = gpGlobals->realtime,
    };

    RgResult r = rgDrawFrame( rg_instance, &info );
    RG_CHECK( r );
#endif
}

/*
===============
R_DrawCubemapView
===============
*/
void R_DrawCubemapView( const vec3_t origin, const vec3_t angles, int size )
{
	ref_viewpass_t rvp;

	// basic params
	rvp.flags = rvp.viewentity = 0;
	SetBits( rvp.flags, RF_DRAW_WORLD );
	SetBits( rvp.flags, RF_DRAW_CUBEMAP );

	rvp.viewport[0] = rvp.viewport[1] = 0;
	rvp.viewport[2] = rvp.viewport[3] = size;
	rvp.fov_x = rvp.fov_y = 90.0f; // this is a final fov value

	// setup origin & angles
	VectorCopy( origin, rvp.vieworigin );
	VectorCopy( angles, rvp.viewangles );

	R_RenderFrame( &rvp );

	RI.viewleaf = NULL;		// force markleafs next frame
}

/*
===============
CL_FxBlend
===============
*/
int CL_FxBlend( cl_entity_t *e )
{
	int	blend = 0;
	float	offset, dist;
	vec3_t	tmp;

	offset = ((int)e->index ) * 363.0f; // Use ent index to de-sync these fx

	switch( e->curstate.renderfx )
	{
	case kRenderFxPulseSlowWide:
		blend = e->curstate.renderamt + 0x40 * sin( gpGlobals->time * 2 + offset );
		break;
	case kRenderFxPulseFastWide:
		blend = e->curstate.renderamt + 0x40 * sin( gpGlobals->time * 8 + offset );
		break;
	case kRenderFxPulseSlow:
		blend = e->curstate.renderamt + 0x10 * sin( gpGlobals->time * 2 + offset );
		break;
	case kRenderFxPulseFast:
		blend = e->curstate.renderamt + 0x10 * sin( gpGlobals->time * 8 + offset );
		break;
	case kRenderFxFadeSlow:
		if( RP_NORMALPASS( ))
		{
			if( e->curstate.renderamt > 0 )
				e->curstate.renderamt -= 1;
			else e->curstate.renderamt = 0;
		}
		blend = e->curstate.renderamt;
		break;
	case kRenderFxFadeFast:
		if( RP_NORMALPASS( ))
		{
			if( e->curstate.renderamt > 3 )
				e->curstate.renderamt -= 4;
			else e->curstate.renderamt = 0;
		}
		blend = e->curstate.renderamt;
		break;
	case kRenderFxSolidSlow:
		if( RP_NORMALPASS( ))
		{
			if( e->curstate.renderamt < 255 )
				e->curstate.renderamt += 1;
			else e->curstate.renderamt = 255;
		}
		blend = e->curstate.renderamt;
		break;
	case kRenderFxSolidFast:
		if( RP_NORMALPASS( ))
		{
			if( e->curstate.renderamt < 252 )
				e->curstate.renderamt += 4;
			else e->curstate.renderamt = 255;
		}
		blend = e->curstate.renderamt;
		break;
	case kRenderFxStrobeSlow:
		blend = 20 * sin( gpGlobals->time * 4 + offset );
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxStrobeFast:
		blend = 20 * sin( gpGlobals->time * 16 + offset );
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxStrobeFaster:
		blend = 20 * sin( gpGlobals->time * 36 + offset );
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxFlickerSlow:
		blend = 20 * (sin( gpGlobals->time * 2 ) + sin( gpGlobals->time * 17 + offset ));
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxFlickerFast:
		blend = 20 * (sin( gpGlobals->time * 16 ) + sin( gpGlobals->time * 23 + offset ));
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxHologram:
	case kRenderFxDistort:
		VectorCopy( e->origin, tmp );
		VectorSubtract( tmp, RI.vieworg, tmp );
		dist = DotProduct( tmp, RI.vforward );

		// turn off distance fade
		if( e->curstate.renderfx == kRenderFxDistort )
			dist = 1;

		if( dist <= 0 )
		{
			blend = 0;
		}
		else
		{
			e->curstate.renderamt = 180;
			if( dist <= 100 ) blend = e->curstate.renderamt;
			else blend = (int) ((1.0f - ( dist - 100 ) * ( 1.0f / 400.0f )) * e->curstate.renderamt );
			blend += gEngfuncs.COM_RandomLong( -32, 31 );
		}
		break;
	default:
		blend = e->curstate.renderamt;
		break;
	}

	blend = bound( 0, blend, 255 );

	return blend;
}
