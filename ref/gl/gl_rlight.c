/*
gl_rlight.c - dynamic and static lights
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
#include "pm_local.h"
#include "studio.h"
#include "xash3d_mathlib.h"
#include "ref_params.h"

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/
/*
==================
CL_RunLightStyles

==================
*/
void CL_RunLightStyles( void )
{
	int		i, k, flight, clight;
	float		l, lerpfrac, backlerp;
	float		frametime = (gpGlobals->time -   gpGlobals->oldtime);
	float		scale;
	lightstyle_t	*ls;

	if( !WORLDMODEL ) return;

	scale = r_lighting_modulate->value;

	// light animations
	// 'm' is normal light, 'a' is no light, 'z' is double bright
	for( i = 0; i < MAX_LIGHTSTYLES; i++ )
	{
		ls = gEngfuncs.GetLightStyle( i );
		if( !WORLDMODEL->lightdata )
		{
			tr.lightstylevalue[i] = 256 * 256;
			continue;
		}

		if( !ENGINE_GET_PARM( PARAM_GAMEPAUSED ) && frametime <= 0.1f )
			ls->time += frametime; // evaluate local time

		flight = (int)Q_floor( ls->time * 10 );
		clight = (int)Q_ceil( ls->time * 10 );
		lerpfrac = ( ls->time * 10 ) - flight;
		backlerp = 1.0f - lerpfrac;

		if( !ls->length )
		{
			tr.lightstylevalue[i] = 256 * scale;
			continue;
		}
		else if( ls->length == 1 )
		{
			// single length style so don't bother interpolating
			tr.lightstylevalue[i] = ls->map[0] * 22 * scale;
			continue;
		}
		else if( !ls->interp || !CVAR_TO_BOOL( cl_lightstyle_lerping ))
		{
			tr.lightstylevalue[i] = ls->map[flight%ls->length] * 22 * scale;
			continue;
		}

		// interpolate animating light
		// frame just gone
		k = ls->map[flight % ls->length];
		l = (float)( k * 22.0f ) * backlerp;

		// upcoming frame
		k = ls->map[clight % ls->length];
		l += (float)( k * 22.0f ) * lerpfrac;

		tr.lightstylevalue[i] = (int)l * scale;
	}
}

/*
=============
R_MarkLights
=============
*/
void R_MarkLights( dlight_t *light, int bit, mnode_t *node )
{
	float		dist;
	msurface_t	*surf;
	int		i;

	if( !node || node->contents < 0 )
		return;

	dist = PlaneDiff( light->origin, node->plane );

	if( dist > light->radius )
	{
		R_MarkLights( light, bit, node->children[0] );
		return;
	}
	if( dist < -light->radius )
	{
		R_MarkLights( light, bit, node->children[1] );
		return;
	}

	// mark the polygons
	surf = RI.currentmodel->surfaces + node->firstsurface;

	for( i = 0; i < node->numsurfaces; i++, surf++ )
	{
		if( !BoundsAndSphereIntersect( surf->info->mins, surf->info->maxs, light->origin, light->radius ))
			continue;	// no intersection

		if( surf->dlightframe != tr.dlightframecount )
		{
			surf->dlightbits = 0;
			surf->dlightframe = tr.dlightframecount;
		}
		surf->dlightbits |= bit;
	}

	R_MarkLights( light, bit, node->children[0] );
	R_MarkLights( light, bit, node->children[1] );
}

/*
=============
R_PushDlights
=============
*/
void R_PushDlights( void )
{
	dlight_t	*l;
	int	i;

	tr.dlightframecount = tr.framecount;

	RI.currententity = gEngfuncs.GetEntityByIndex( 0 );
	if( RI.currententity )
		RI.currentmodel = RI.currententity->model;

	for( i = 0; i < MAX_DLIGHTS; i++, l++ )
	{
		l = gEngfuncs.GetDynamicLight( i );

		if( l->die < gpGlobals->time || !l->radius )
			continue;

		if( GL_FrustumCullSphere( &RI.frustum, l->origin, l->radius, 15 ))
			continue;

		if( RI.currententity )
			R_MarkLights( l, 1<<i, RI.currentmodel->nodes );
	}
}

/*
=============
R_CountDlights
=============
*/
int R_CountDlights( void )
{
	dlight_t	*l;
	int	i, numDlights = 0;

	for( i = 0; i < MAX_DLIGHTS; i++ )
	{
		l = gEngfuncs.GetDynamicLight( i );

		if( l->die < gpGlobals->time || !l->radius )
			continue;

		numDlights++;
	}

	return numDlights;
}

/*
=============
R_CountSurfaceDlights
=============
*/
int R_CountSurfaceDlights( msurface_t *surf )
{
	int	i, numDlights = 0;

	for( i = 0; i < MAX_DLIGHTS; i++ )
	{
		if(!( surf->dlightbits & BIT( i )))
			continue;	// not lit by this light

		numDlights++;
	}

	return numDlights;
}

/*
=======================================================================

	AMBIENT LIGHTING

=======================================================================
*/
static vec3_t	g_trace_lightspot;
static vec3_t	g_trace_lightvec;
static float	g_trace_fraction;

/*
=================
R_RecursiveLightPoint
=================
*/
static qboolean R_RecursiveLightPoint( model_t *model, mnode_t *node, float p1f, float p2f, colorVec *cv, const vec3_t start, const vec3_t end )
{
	float		front, back, frac, midf;
	int		i, map, side, size;
	float		ds, dt, s, t;
	int		sample_size;
	color24		*lm, *dm;
	mextrasurf_t	*info;
	msurface_t	*surf;
	mtexinfo_t	*tex;
	matrix3x4		tbn;
	vec3_t		mid;

	// didn't hit anything
	if( !node || node->contents < 0 )
	{
		cv->r = cv->g = cv->b = cv->a = 0;
		return false;
	}

	// calculate mid point
	front = PlaneDiff( start, node->plane );
	back = PlaneDiff( end, node->plane );

	side = front < 0;
	if(( back < 0 ) == side )
		return R_RecursiveLightPoint( model, node->children[side], p1f, p2f, cv, start, end );

	frac = front / ( front - back );

	VectorLerp( start, frac, end, mid );
	midf = p1f + ( p2f - p1f ) * frac;

	// co down front side
	if( R_RecursiveLightPoint( model, node->children[side], p1f, midf, cv, start, mid ))
		return true; // hit something

	if(( back < 0 ) == side )
	{
		cv->r = cv->g = cv->b = cv->a = 0;
		return false; // didn't hit anything
	}

	// check for impact on this node
	surf = model->surfaces + node->firstsurface;
	VectorCopy( mid, g_trace_lightspot );

	for( i = 0; i < node->numsurfaces; i++, surf++ )
	{
		int	smax, tmax;

		tex = surf->texinfo;
		info = surf->info;

		if( FBitSet( surf->flags, SURF_DRAWTILED ))
			continue;	// no lightmaps

		s = DotProduct( mid, info->lmvecs[0] ) + info->lmvecs[0][3];
		t = DotProduct( mid, info->lmvecs[1] ) + info->lmvecs[1][3];

		if( s < info->lightmapmins[0] || t < info->lightmapmins[1] )
			continue;

		ds = s - info->lightmapmins[0];
		dt = t - info->lightmapmins[1];

		if ( ds > info->lightextents[0] || dt > info->lightextents[1] )
			continue;

		cv->r = cv->g = cv->b = cv->a = 0;

		if( !surf->samples )
			return true;

		sample_size = gEngfuncs.Mod_SampleSizeForFace( surf );
		smax = (info->lightextents[0] / sample_size) + 1;
		tmax = (info->lightextents[1] / sample_size) + 1;
		ds /= sample_size;
		dt /= sample_size;

		lm = surf->samples + Q_rint( dt ) * smax + Q_rint( ds );
		g_trace_fraction = midf;
		size = smax * tmax;
		dm = NULL;

		if( surf->info->deluxemap )
		{
			vec3_t	faceNormal;

			if( FBitSet( surf->flags, SURF_PLANEBACK ))
				VectorNegate( surf->plane->normal, faceNormal );
			else VectorCopy( surf->plane->normal, faceNormal );

			// compute face TBN
#if 1
			Vector4Set( tbn[0], surf->info->lmvecs[0][0], surf->info->lmvecs[0][1], surf->info->lmvecs[0][2], 0.0f );
			Vector4Set( tbn[1], -surf->info->lmvecs[1][0], -surf->info->lmvecs[1][1], -surf->info->lmvecs[1][2], 0.0f );
			Vector4Set( tbn[2], faceNormal[0], faceNormal[1], faceNormal[2], 0.0f );
#else
			Vector4Set( tbn[0], surf->info->lmvecs[0][0], -surf->info->lmvecs[1][0], faceNormal[0], 0.0f );
			Vector4Set( tbn[1], surf->info->lmvecs[0][1], -surf->info->lmvecs[1][1], faceNormal[1], 0.0f );
			Vector4Set( tbn[2], surf->info->lmvecs[0][2], -surf->info->lmvecs[1][2], faceNormal[2], 0.0f );
#endif
			VectorNormalize( tbn[0] );
			VectorNormalize( tbn[1] );
			VectorNormalize( tbn[2] );
			dm = surf->info->deluxemap + Q_rint( dt ) * smax + Q_rint( ds );
		}

		for( map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++ )
		{
			uint	scale = tr.lightstylevalue[surf->styles[map]];

			if( tr.ignore_lightgamma )
			{
				cv->r += lm->r * scale;
				cv->g += lm->g * scale;
				cv->b += lm->b * scale;
			}
			else
			{
				cv->r += gEngfuncs.LightToTexGamma( lm->r ) * scale;
				cv->g += gEngfuncs.LightToTexGamma( lm->g ) * scale;
				cv->b += gEngfuncs.LightToTexGamma( lm->b ) * scale;
			}
			lm += size; // skip to next lightmap

			if( dm != NULL )
			{
				vec3_t	srcNormal, lightNormal;
				float	f = (1.0f / 128.0f);

				VectorSet( srcNormal, ((float)dm->r - 128.0f) * f, ((float)dm->g - 128.0f) * f, ((float)dm->b - 128.0f) * f );
				Matrix3x4_VectorIRotate( tbn, srcNormal, lightNormal );		// turn to world space
				VectorScale( lightNormal, (float)scale * -1.0f, lightNormal );	// turn direction from light
				VectorAdd( g_trace_lightvec, lightNormal, g_trace_lightvec );
				dm += size; // skip to next deluxmap
			}
		}

		return true;
	}

	// go down back side
	return R_RecursiveLightPoint( model, node->children[!side], midf, p2f, cv, mid, end );
}

/*
=================
R_LightVec

check bspmodels to get light from
=================
*/
colorVec R_LightVecInternal( const vec3_t start, const vec3_t end, vec3_t lspot, vec3_t lvec )
{
	float	last_fraction;
	int	i, maxEnts = 1;
	colorVec	light, cv;

	if( lspot ) VectorClear( lspot );
	if( lvec ) VectorClear( lvec );

	if( WORLDMODEL && WORLDMODEL->lightdata )
	{
		light.r = light.g = light.b = light.a = 0;
		last_fraction = 1.0f;

		// get light from bmodels too
		if( CVAR_TO_BOOL( r_lighting_extended ))
			maxEnts = MAX_PHYSENTS;

		// check all the bsp-models
		for( i = 0; i < maxEnts; i++ )
		{
			physent_t	*pe = gEngfuncs.EV_GetPhysent( i );
			vec3_t	offset, start_l, end_l;
			mnode_t	*pnodes;
			matrix4x4	matrix;

			if( !pe )
				break;

			if( !pe->model || pe->model->type != mod_brush )
				continue; // skip non-bsp models

			pnodes = &pe->model->nodes[pe->model->hulls[0].firstclipnode];
			VectorSubtract( pe->model->hulls[0].clip_mins, vec3_origin, offset );
			VectorAdd( offset, pe->origin, offset );
			VectorSubtract( start, offset, start_l );
			VectorSubtract( end, offset, end_l );

			// rotate start and end into the models frame of reference
			if( !VectorIsNull( pe->angles ))
			{
				Matrix4x4_CreateFromEntity( matrix, pe->angles, offset, 1.0f );
				Matrix4x4_VectorITransform( matrix, start, start_l );
				Matrix4x4_VectorITransform( matrix, end, end_l );
			}

			VectorClear( g_trace_lightspot );
			VectorClear( g_trace_lightvec );
			g_trace_fraction = 1.0f;

			if( !R_RecursiveLightPoint( pe->model, pnodes, 0.0f, 1.0f, &cv, start_l, end_l ))
				continue;	// didn't hit anything

			if( g_trace_fraction < last_fraction )
			{
				if( lspot ) VectorCopy( g_trace_lightspot, lspot );
				if( lvec ) VectorNormalize2( g_trace_lightvec, lvec );
				light.r = Q_min(( cv.r >> 7 ), 255 );
				light.g = Q_min(( cv.g >> 7 ), 255 );
				light.b = Q_min(( cv.b >> 7 ), 255 );
				last_fraction = g_trace_fraction;

				if(( light.r + light.g + light.b ) != 0 )
					break; // we get light now
			}
		}
	}
	else
	{
		light.r = light.g = light.b = 255;
		light.a = 0;
	}

	return light;
}

/*
=================
R_LightVec

check bspmodels to get light from
=================
*/
colorVec R_LightVec( const vec3_t start, const vec3_t end, vec3_t lspot, vec3_t lvec )
{
	colorVec	light = R_LightVecInternal( start, end, lspot, lvec );

	if( CVAR_TO_BOOL( r_lighting_extended ) && lspot != NULL && lvec != NULL )
	{
		// trying to get light from ceiling (but ignore gradient analyze)
		if(( light.r + light.g + light.b ) == 0 )
			return R_LightVecInternal( end, start, lspot, lvec );
	}

	return light;
}

/*
=================
R_LightPoint

light from floor
=================
*/
colorVec R_LightPoint( const vec3_t p0 )
{
	vec3_t	p1;

	VectorSet( p1, p0[0], p0[1], p0[2] - 2048.0f );

	return R_LightVec( p0, p1, NULL, NULL );
}





#if XASH_RAYTRACING

typedef struct rt_sun_light_t
{
    RgColor4DPacked32 pcolor;
    float             intensity;
    float             pitch;
    float             angle;
} rt_sun_light_t;

typedef struct rt_static_light_t
{
    vec3_t            abs_position;
    vec3_t            dir;
    RgColor4DPacked32 pcolor;
    float             intensity;
    uint32_t          index;
    int               light_style;
    qboolean          is_spot;
    float             spot_inner_cone_rad;
    float             spot_outer_cone_rad;
} rt_static_light_t;

#define RT_MAX_STATIC_LIGHTS        256
#define RT_MAX_POTENTIAL_SUN_LIGHTS 16

struct
{
    // Need to save on the map start to add them in each frame
    rt_static_light_t static_lights[ RT_MAX_STATIC_LIGHTS ];
    int               static_lights_count;

    rt_sun_light_t potential_sun[ RT_MAX_POTENTIAL_SUN_LIGHTS ];
    uint32_t       potential_sun_count;

    rt_sun_light_t sun;
    qboolean       sun_exists;
} g_lights;


static void AddStaticLight( const vec3_t      abs_position,
                            const vec3_t      dir,
                            RgColor4DPacked32 pcolor,
                            float             intensity,
                            int               light_style,
                            qboolean          is_spot,
                            float             spot_cone,
                            float             spot_cone2 )
{
    uint32_t index = g_lights.static_lights_count;
    ASSERT( index < RT_MAX_STATIC_LIGHTS );

    rt_static_light_t* dst = &g_lights.static_lights[ index ];
    {
        VectorCopy( abs_position, dst->abs_position );
        VectorCopy( dir, dst->dir );
        dst->pcolor      = pcolor;
        dst->intensity   = intensity;
        dst->index       = index;
        dst->light_style = light_style;
        dst->is_spot     = is_spot;

        dst->spot_inner_cone_rad = DEG2RAD( Q_min( spot_cone, spot_cone2 ) );
        dst->spot_outer_cone_rad = DEG2RAD( Q_max( spot_cone, spot_cone2 ) );
    }

    g_lights.static_lights_count++;
}


static void AddPotentialSun( float pitch, float angle, RgColor4DPacked32 pcolor, float intensity )
{
    uint32_t index = g_lights.potential_sun_count;
    ASSERT( index < RT_MAX_POTENTIAL_SUN_LIGHTS );

    rt_sun_light_t* dst = &g_lights.potential_sun[ index ];
    {
        dst->pcolor    = pcolor;
        dst->intensity = intensity;
        dst->pitch     = pitch;
        dst->angle     = angle;
    }

    g_lights.potential_sun_count++;
}


static void FilterTheSunFromPotential( const char* mapname )
{
    g_lights.sun_exists = false;

    // for now, just get the first one
    if( g_lights.potential_sun_count > 0 )
    {
        g_lights.sun_exists = true;
        memcpy( &g_lights.sun, &g_lights.potential_sun[ 0 ], sizeof( rt_sun_light_t ) );
    }
}


void RT_ParseStaticLightEntities()
{
    g_lights.static_lights_count = 0;
    g_lights.potential_sun_count = 0;
    g_lights.sun_exists          = 0;

    const model_t* const world   = WORLDMODEL;
    ASSERT( world );

    enum
    {
        LT_CLASS_UNKNOWN,
        LT_CLASS_POINT,
        LT_CLASS_SPOT,
        LT_CLASS_SUN_POTENTIAL,
    } classname = LT_CLASS_UNKNOWN;

    struct
    {
        vec3_t            origin;
        vec3_t            allangles;
        RgColor4DPacked32 pcolor;
        float             intensity;
        float             pitch;
        float             angle;
        int               style;
        float             cone;
        float             cone2;
    } light = { 0 };

    enum
    {
        LT_HAS_CLASS     = 1,
        LT_HAS_ORIGIN    = 2,
        LT_HAS_COLOR     = 4,
        LT_HAS_PITCH     = 8,
        LT_HAS_ANGLE     = 16,
        LT_HAS_SKY       = 32,
        LT_HAS_STYLE     = 64,
        LT_HAS_ALLANGLES = 128,
        LT_HAS_CONE      = 256,
        LT_HAS_CONE2     = 512,
    };


    uint32_t has = 0;
    char*    pos = world->entities;

    for( ;; )
    {
        string key, value;

        pos = COM_ParseFile( pos, key, sizeof( key ) );
        if( pos == NULL )
        {
            break;
        }

        if( key[ 0 ] == '{' )
        {
            classname = LT_CLASS_UNKNOWN;
            has       = 0;
            continue;
        }

        if( key[ 0 ] == '}' )
        {
            if( !( has & LT_HAS_CLASS ) )
            {
                continue;
            }
			
            if( ( classname == LT_CLASS_POINT || classname == LT_CLASS_SPOT ) &&
                !( has & LT_HAS_SKY ) )
            {
                qboolean has_info = !!( has & LT_HAS_ORIGIN ) && !!( has & LT_HAS_COLOR );

                vec3_t direction = { 0 };

                if( classname == LT_CLASS_SPOT )
                {
                    if( has & LT_HAS_ALLANGLES )
                    {
                        AngleVectors( light.allangles, direction, NULL, NULL );
                    }
                    else if( ( has & LT_HAS_PITCH ) && ( has & LT_HAS_ANGLE ) )
                    {
                        vec3_t angles = { -light.pitch, light.angle, 0 };
                        AngleVectors( angles, direction, NULL, NULL );
                    }
                    else
                    {
                        has_info = false;
                    }

					if( !( has & LT_HAS_CONE ) )
                    {
                        has_info = false;
                    }
                }

                if( has_info )
                {
                    AddStaticLight( light.origin,
                                    direction,
                                    light.pcolor,
                                    light.intensity,
                                    has & LT_HAS_STYLE ? light.style : 0,
                                    classname == LT_CLASS_SPOT,
                                    has & LT_HAS_CONE ? light.cone : 0,
                                    has & LT_HAS_CONE2 ? light.cone2 : 90 );
                }
            }
            else if( classname == LT_CLASS_SUN_POTENTIAL || ( has & LT_HAS_SKY ) )
            {
                qboolean has_info = !!( has & LT_HAS_COLOR );

                float pitch = 0;
                float angle = 0;

                if( has & LT_HAS_PITCH )
                {
                    pitch = light.pitch;
                }

                if( has & LT_HAS_ANGLE )
                {
                    angle = light.angle;
                }

                if( has_info )
                {
                    AddPotentialSun( pitch, angle, light.pcolor, light.intensity );
                }
            }

            continue;
        }

        pos = COM_ParseFile( pos, value, sizeof( value ) );
        if( pos == NULL )
        {
            break;
        }

        if( Q_strcmp( key, "origin" ) == 0 )
        {
            vec3_t    tmp_origin;
            const int components =
                sscanf( value, "%f %f %f", &tmp_origin[ 0 ], &tmp_origin[ 1 ], &tmp_origin[ 2 ] );
            if( components == 3 )
            {
                VectorCopy( tmp_origin, light.origin );
                has |= LT_HAS_ORIGIN;
            }
        }
        else if( Q_strcmp( key, "angles" ) == 0 )
        {
            vec3_t    tmp_allangles;
            const int components = sscanf(
                value, "%f %f %f", &tmp_allangles[ 0 ], &tmp_allangles[ 1 ], &tmp_allangles[ 2 ] );
            if( components == 3 )
            {
                VectorCopy( tmp_allangles, light.allangles );
                has |= LT_HAS_ALLANGLES;
            }
        }
        else if( Q_strcmp( key, "_light" ) == 0 )
        {
            vec3_t    tmp_color;
            float     scale;
            const int components = sscanf(
                value, "%f %f %f %f", &tmp_color[ 0 ], &tmp_color[ 1 ], &tmp_color[ 2 ], &scale );
            if( components == 1 )
            {
                float val = tmp_color[ 0 ];

                light.pcolor =
                    rgUtilPackColorFloat4D( val / 255.0f, val / 255.0f, val / 255.0f, 1.0f );
                light.intensity = 1.0f;

                has |= LT_HAS_COLOR;
            }
            else if( components == 3 )
            {
                light.pcolor    = rgUtilPackColorFloat4D( tmp_color[ 0 ] / 255.0f,
                                                       tmp_color[ 1 ] / 255.0f,
                                                       tmp_color[ 2 ] / 255.0f,
                                                       1.0f );
                light.intensity = 1.0f;

                has |= LT_HAS_COLOR;
            }
            else if( components == 4 )
            {
                light.pcolor    = rgUtilPackColorFloat4D( tmp_color[ 0 ] / 255.0f,
                                                       tmp_color[ 1 ] / 255.0f,
                                                       tmp_color[ 2 ] / 255.0f,
                                                       1.0f );
                light.intensity = scale / 255.0f;

                has |= LT_HAS_COLOR;
            }
        }
        else if( Q_strcmp( key, "classname" ) == 0 )
        {
            if( Q_strcmp( value, "light" ) == 0 )
            {
                classname = LT_CLASS_POINT;
            }
            else if( Q_strcmp( value, "light_spot" ) == 0 )
            {
                classname = LT_CLASS_SPOT;
            }
            else if( Q_strcmp( value, "light_environment" ) == 0 )
            {
                classname = LT_CLASS_SUN_POTENTIAL;
            }

            has |= LT_HAS_CLASS;
        }
        else if( Q_strcmp( key, "pitch" ) == 0 )
        {
            float     tmp;
            const int components = sscanf( value, "%f", &tmp );

            if( components == 1 )
            {
                light.pitch = tmp;
                has |= LT_HAS_PITCH;
            }
        }
        else if( Q_strcmp( key, "angle" ) == 0 )
        {
            float     tmp;
            const int components = sscanf( value, "%f", &tmp );

            if( components == 1 )
            {
                light.angle = tmp;
                has |= LT_HAS_ANGLE;
            }
        }
        else if( Q_strcmp( key, "_sky" ) == 0 )
        {
            int       tmp;
            const int components = sscanf( value, "%i", &tmp );

            if( components == 1 && tmp != 0 )
            {
                has |= LT_HAS_SKY;
            }
        }
        else if( Q_strcmp( key, "style" ) == 0 )
        {
            int       tmp;
            const int components = sscanf( value, "%i", &tmp );

            if( components == 1 )
            {
                light.style = tmp;
                has |= LT_HAS_STYLE;
            }
        }
        else if( Q_strcmp( key, "_cone" ) == 0 )
        {
            float     tmp;
            const int components = sscanf( value, "%f", &tmp );

            if( components == 1 )
            {
                light.cone = tmp;
                has |= LT_HAS_CONE;
            }
        }
        else if( Q_strcmp( key, "_cone2" ) == 0 )
        {
            float     tmp;
            const int components = sscanf( value, "%f", &tmp );

            if( components == 1 )
            {
                light.cone2 = tmp;
                has |= LT_HAS_CONE2;
            }
        }
    }

    FilterTheSunFromPotential( WORLDMODEL->name );
}


#define RT_ARRAYSIZE( arr ) ( sizeof( arr ) / sizeof( arr[ 0 ] ) )


static float GetLightStyleIntensity( int light_style )
{
    ASSERT( light_style >= 0 && light_style < ( int )RT_ARRAYSIZE( tr.lightstylevalue ) );
    // from CL_LightVecInternal and R_RecursiveLightPoint, assuming lm = (1, 1, 1)

    uint style_scale = tr.lightstylevalue[ light_style ];

    return ( float )bound( 0, style_scale, 255 ) / 255.0f;
}


static qboolean IsPlayerFlashlight( const dlight_t* l )
{
    int flashlight_key = RT_CVAR_TO_INT32( _rt_flsh_key );
    return l->key == flashlight_key;
}


static void CalculateFlaslightPosition( vec3_t out_position )
{
    vec3_t u;
    VectorCopy( RI.vup, u );
    VectorScale( u, METRIC_TO_QUAKEUNIT( RT_CVAR_TO_FLOAT( rt_flsh_u ) ), u );

    vec3_t r;
    VectorCopy( RI.vright, r );
    VectorScale( r, METRIC_TO_QUAKEUNIT( RT_CVAR_TO_FLOAT( rt_flsh_r ) ), r );

    VectorCopy( RI.vieworg, out_position );
    VectorAdd( out_position, u, out_position );
    VectorAdd( out_position, r, out_position );
}

    #define VectorPow( in, pw, out )                  \
        ( ( out )[ 0 ] = powf( ( in )[ 0 ], ( pw ) ), \
          ( out )[ 1 ] = powf( ( in )[ 1 ], ( pw ) ), \
          ( out )[ 2 ] = powf( ( in )[ 2 ], ( pw ) ) )


    #define RT_IDBASE_SUN         0
    #define RT_IDBASE_FLASHLIGHT  256
    #define RT_IDBASE_DLIGHT      512
    #define RT_IDBASE_ELIGHT      768
    #define RT_IDBASE_STATICLIGHT 1024


void RT_UploadAllLights()
{
    rt_state.flashlight_uniqueid = 0;

    if( g_lights.sun_exists && RI.isSkyVisible )
    {
        rt_sun_light_t* sun = &g_lights.sun;

        vec3_t angles = { -sun->pitch, sun->angle, 0 };
        vec3_t direction;
        AngleVectors( angles, direction, NULL, NULL );

        RgDirectionalLightUploadInfo info = {
            .uniqueID               = RT_IDBASE_SUN,
            .isExportable           = true,
            .color                  = sun->pcolor,
            .intensity              = RT_CVAR_TO_FLOAT( rt_sun ) * sun->intensity,
            .direction              = RT_VEC3( direction ),
            .angularDiameterDegrees = RT_CVAR_TO_FLOAT( rt_sun_diameter ),
        };

        RgResult r = rgUploadDirectionalLight( rg_instance, &info );
        RG_CHECK( r );
    }

    for( int i = 0; i < g_lights.static_lights_count; i++ )
    {
        const rt_static_light_t* src = &g_lights.static_lights[ i ];

        if( src->is_spot )
        {
            RgSpotLightUploadInfo info = {
                .uniqueID     = RT_IDBASE_STATICLIGHT + src->index,
                .isExportable = true,
                .color        = src->pcolor,
                .intensity    = RT_CVAR_TO_FLOAT( rt_light_s ) * src->intensity *
                             GetLightStyleIntensity( src->light_style ),
                .position   = RT_VEC3( src->abs_position ),
                .direction  = RT_VEC3( src->dir ),
                .radius     = METRIC_TO_QUAKEUNIT( RT_CVAR_TO_FLOAT( rt_light_radius ) ),
                .angleOuter = src->spot_outer_cone_rad,
                .angleInner = src->spot_inner_cone_rad,
            };

            RgResult r = rgUploadSpotLight( rg_instance, &info );
            RG_CHECK( r );
        }
        else
        {
            RgSphericalLightUploadInfo info = {
                .uniqueID     = RT_IDBASE_STATICLIGHT + src->index,
                .isExportable = true,
                .color        = src->pcolor,
                .intensity    = RT_CVAR_TO_FLOAT( rt_light_s ) * src->intensity *
                             GetLightStyleIntensity( src->light_style ),
                .position = RT_VEC3( src->abs_position ),
                .radius   = METRIC_TO_QUAKEUNIT( RT_CVAR_TO_FLOAT( rt_light_radius ) ),
            };

            RgResult r = rgUploadSphericalLight( rg_instance, &info );
            RG_CHECK( r );
        }
    }

    for( qboolean is_e_light = false; is_e_light <= true; is_e_light++ )
    {
        for( int i = 0; i < ( is_e_light ? MAX_ELIGHTS : MAX_DLIGHTS ); i++ )
        {
            dlight_t* l =
                is_e_light ? gEngfuncs.GetEntityLight( i ) : gEngfuncs.GetDynamicLight( i );

            if( !l || l->dark )
            {
                continue;
            }

            if( l->die < gpGlobals->time || !l->radius )
            {
                continue;
            }


            if( IsPlayerFlashlight( l ) )
            {
                vec3_t pos;
                CalculateFlaslightPosition( pos );

                RgSpotLightUploadInfo info = {
                    .uniqueID     = RT_IDBASE_FLASHLIGHT + i,
                    .isExportable = false,
                    .color        = rgUtilPackColorByte4D( 220, 243, 255, 255 ),
                    .intensity    = RT_CVAR_TO_FLOAT( rt_flsh ),
                    .position     = RT_VEC3( pos ),
                    .direction    = RT_VEC3( RI.vforward ),
                    .radius       = METRIC_TO_QUAKEUNIT( RT_CVAR_TO_FLOAT( rt_flsh_radius ) ),
                    .angleOuter   = DEG2RAD( RT_CVAR_TO_FLOAT( rt_flsh_angle ) ),
                    .angleInner   = 0,
                };

                RgResult r = rgUploadSpotLight( rg_instance, &info );
                RG_CHECK( r );

                // uploaded flashlight, reset the index, for future reuse
                rt_cvars._rt_flsh_key->value = -1;
                rt_state.flashlight_uniqueid = info.uniqueID;
            }
            else
            {
                float falloff_mult = QUAKEUNIT_TO_METRIC( l->radius );
                // Lerp( 1.0f,
                //       GOLDSRCUNIT_TO_METRIC( l->radius ),
                //       CVAR_TO_FLOAT( rt_cvars.rt_fLightDlightFalloffToIntensity ) );

                RgSphericalLightUploadInfo info = {
                    .uniqueID     = is_e_light ? RT_IDBASE_ELIGHT + i : RT_IDBASE_DLIGHT + i,
                    .isExportable = false,
                    .color     = rgUtilPackColorByte4D( l->color.r, l->color.g, l->color.b, 255 ),
                    .intensity = RT_CVAR_TO_FLOAT( rt_light_d ) * falloff_mult,
                    .position  = RT_VEC3( l->origin ),
                    .radius    = METRIC_TO_QUAKEUNIT( RT_CVAR_TO_FLOAT( rt_light_radius ) ),
                };

                RgResult r = rgUploadSphericalLight( rg_instance, &info );
                RG_CHECK( r );
            }
        }
    }
}
#endif // XASH_RAYTRACING
