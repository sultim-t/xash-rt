
#include "gl_local.h"
#if XASH_GL4ES
#include "gl4es/include/gl4esinit.h"
#include "gl4es/include/gl4eshint.h"
#endif // XASH_GL4ES

cvar_t	*gl_extensions;
cvar_t	*gl_texture_anisotropy;
cvar_t	*gl_texture_lodbias;
cvar_t	*gl_texture_nearest;
cvar_t	*gl_lightmap_nearest;
cvar_t	*gl_keeptjunctions;
cvar_t	*gl_check_errors;
cvar_t	*gl_polyoffset;
cvar_t	*gl_wireframe;
cvar_t	*gl_finish;
cvar_t	*gl_nosort;
cvar_t	*gl_vsync;
cvar_t	*gl_clear;
cvar_t	*gl_test;
cvar_t	*gl_msaa;
cvar_t	*gl_stencilbits;
cvar_t	*r_lighting_extended;
cvar_t	*r_lighting_modulate;
cvar_t	*r_lighting_ambient;
cvar_t	*r_detailtextures;
cvar_t	*r_adjust_fov;
cvar_t	*r_decals;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lockpvs;
cvar_t	*r_lockfrustum;
cvar_t	*r_traceglow;
cvar_t	*r_dynamic;
cvar_t	*gl_round_down;
cvar_t	*r_vbo;
cvar_t	*r_vbo_dlightmode;

cvar_t	*tracerred;
cvar_t	*tracergreen;
cvar_t	*tracerblue;
cvar_t	*traceralpha;

DEFINE_ENGINE_SHARED_CVAR_LIST()

poolhandle_t r_temppool;

gl_globals_t	tr;
glconfig_t	glConfig;
glstate_t	glState;
glwstate_t	glw_state;

#ifdef XASH_GL_STATIC
#define GL_CALL( x ) #x, NULL
#else
#define GL_CALL( x ) #x, (void**)&p##x
#endif
static dllfunc_t opengl_110funcs[] =
{
	{ GL_CALL( glClearColor ) },
	{ GL_CALL( glClear ) },
	{ GL_CALL( glAlphaFunc ) },
	{ GL_CALL( glBlendFunc ) },
	{ GL_CALL( glCullFace ) },
	{ GL_CALL( glDrawBuffer ) },
	{ GL_CALL( glReadBuffer ) },
	{ GL_CALL( glAccum ) },
	{ GL_CALL( glEnable ) },
	{ GL_CALL( glDisable ) },
	{ GL_CALL( glEnableClientState ) },
	{ GL_CALL( glDisableClientState ) },
	{ GL_CALL( glGetBooleanv ) },
	{ GL_CALL( glGetDoublev ) },
	{ GL_CALL( glGetFloatv ) },
	{ GL_CALL( glGetIntegerv ) },
	{ GL_CALL( glGetError ) },
	{ GL_CALL( glGetString ) },
	{ GL_CALL( glFinish ) },
	{ GL_CALL( glFlush ) },
	{ GL_CALL( glClearDepth ) },
	{ GL_CALL( glDepthFunc ) },
	{ GL_CALL( glDepthMask ) },
	{ GL_CALL( glDepthRange ) },
	{ GL_CALL( glFrontFace ) },
	{ GL_CALL( glDrawElements ) },
	{ GL_CALL( glDrawArrays ) },
	{ GL_CALL( glColorMask ) },
	{ GL_CALL( glIndexPointer ) },
	{ GL_CALL( glVertexPointer ) },
	{ GL_CALL( glNormalPointer ) },
	{ GL_CALL( glColorPointer ) },
	{ GL_CALL( glTexCoordPointer ) },
	{ GL_CALL( glArrayElement ) },
	{ GL_CALL( glColor3f ) },
	{ GL_CALL( glColor3fv ) },
	{ GL_CALL( glColor4f ) },
	{ GL_CALL( glColor4fv ) },
	{ GL_CALL( glColor3ub ) },
	{ GL_CALL( glColor4ub ) },
	{ GL_CALL( glColor4ubv ) },
	{ GL_CALL( glTexCoord1f ) },
	{ GL_CALL( glTexCoord2f ) },
	{ GL_CALL( glTexCoord3f ) },
	{ GL_CALL( glTexCoord4f ) },
	{ GL_CALL( glTexCoord1fv ) },
	{ GL_CALL( glTexCoord2fv ) },
	{ GL_CALL( glTexCoord3fv ) },
	{ GL_CALL( glTexCoord4fv ) },
	{ GL_CALL( glTexGenf ) },
	{ GL_CALL( glTexGenfv ) },
	{ GL_CALL( glTexGeni ) },
	{ GL_CALL( glVertex2f ) },
	{ GL_CALL( glVertex3f ) },
	{ GL_CALL( glVertex3fv ) },
	{ GL_CALL( glNormal3f ) },
	{ GL_CALL( glNormal3fv ) },
	{ GL_CALL( glBegin ) },
	{ GL_CALL( glEnd ) },
	{ GL_CALL( glLineWidth ) },
	{ GL_CALL( glPointSize ) },
	{ GL_CALL( glMatrixMode ) },
	{ GL_CALL( glOrtho ) },
	{ GL_CALL( glRasterPos2f ) },
	{ GL_CALL( glFrustum ) },
	{ GL_CALL( glViewport ) },
	{ GL_CALL( glPushMatrix ) },
	{ GL_CALL( glPopMatrix ) },
	{ GL_CALL( glPushAttrib ) },
	{ GL_CALL( glPopAttrib ) },
	{ GL_CALL( glLoadIdentity ) },
	{ GL_CALL( glLoadMatrixd ) },
	{ GL_CALL( glLoadMatrixf ) },
	{ GL_CALL( glMultMatrixd ) },
	{ GL_CALL( glMultMatrixf ) },
	{ GL_CALL( glRotated ) },
	{ GL_CALL( glRotatef ) },
	{ GL_CALL( glScaled ) },
	{ GL_CALL( glScalef ) },
	{ GL_CALL( glTranslated ) },
	{ GL_CALL( glTranslatef ) },
	{ GL_CALL( glReadPixels ) },
	{ GL_CALL( glDrawPixels ) },
	{ GL_CALL( glStencilFunc ) },
	{ GL_CALL( glStencilMask ) },
	{ GL_CALL( glStencilOp ) },
	{ GL_CALL( glClearStencil ) },
	{ GL_CALL( glIsEnabled ) },
	{ GL_CALL( glIsList ) },
	{ GL_CALL( glIsTexture ) },
	{ GL_CALL( glTexEnvf ) },
	{ GL_CALL( glTexEnvfv ) },
	{ GL_CALL( glTexEnvi ) },
	{ GL_CALL( glTexParameterf ) },
	{ GL_CALL( glTexParameterfv ) },
	{ GL_CALL( glTexParameteri ) },
	{ GL_CALL( glHint ) },
	{ GL_CALL( glPixelStoref ) },
	{ GL_CALL( glPixelStorei ) },
	{ GL_CALL( glGenTextures ) },
	{ GL_CALL( glDeleteTextures ) },
	{ GL_CALL( glBindTexture ) },
	{ GL_CALL( glTexImage1D ) },
	{ GL_CALL( glTexImage2D ) },
	{ GL_CALL( glTexSubImage1D ) },
	{ GL_CALL( glTexSubImage2D ) },
	{ GL_CALL( glCopyTexImage1D ) },
	{ GL_CALL( glCopyTexImage2D ) },
	{ GL_CALL( glCopyTexSubImage1D ) },
	{ GL_CALL( glCopyTexSubImage2D ) },
	{ GL_CALL( glScissor ) },
	{ GL_CALL( glGetTexImage ) },
	{ GL_CALL( glGetTexEnviv ) },
	{ GL_CALL( glPolygonOffset ) },
	{ GL_CALL( glPolygonMode ) },
	{ GL_CALL( glPolygonStipple ) },
	{ GL_CALL( glClipPlane ) },
	{ GL_CALL( glGetClipPlane ) },
	{ GL_CALL( glShadeModel ) },
	{ GL_CALL( glGetTexLevelParameteriv ) },
	{ GL_CALL( glGetTexLevelParameterfv ) },
	{ GL_CALL( glFogfv ) },
	{ GL_CALL( glFogf ) },
	{ GL_CALL( glFogi ) },
	{ NULL					, NULL }
};

static dllfunc_t debugoutputfuncs[] =
{
	{ GL_CALL( glDebugMessageControlARB ) },
	{ GL_CALL( glDebugMessageInsertARB ) },
	{ GL_CALL( glDebugMessageCallbackARB ) },
	{ GL_CALL( glGetDebugMessageLogARB ) },
	{ NULL					, NULL }
};

static dllfunc_t multitexturefuncs[] =
{
	{ GL_CALL( glMultiTexCoord1f ) },
	{ GL_CALL( glMultiTexCoord2f ) },
	{ GL_CALL( glMultiTexCoord3f ) },
	{ GL_CALL( glMultiTexCoord4f ) },
	{ GL_CALL( glActiveTexture ) },
	{ GL_CALL( glActiveTextureARB ) },
	{ GL_CALL( glClientActiveTexture ) },
	{ GL_CALL( glClientActiveTextureARB ) },
	{ NULL					, NULL }
};

static dllfunc_t texture3dextfuncs[] =
{
	{ GL_CALL( glTexImage3D ) },
	{ GL_CALL( glTexSubImage3D ) },
	{ GL_CALL( glCopyTexSubImage3D ) },
	{ NULL					, NULL }
};

static dllfunc_t texturecompressionfuncs[] =
{
	{ GL_CALL( glCompressedTexImage3DARB ) },
	{ GL_CALL( glCompressedTexImage2DARB ) },
	{ GL_CALL( glCompressedTexImage1DARB ) },
	{ GL_CALL( glCompressedTexSubImage3DARB ) },
	{ GL_CALL( glCompressedTexSubImage2DARB ) },
	{ GL_CALL( glCompressedTexSubImage1DARB ) },
	{ GL_CALL( glGetCompressedTexImage ) },
	{ NULL					, NULL }
};

static dllfunc_t vbofuncs[] =
{
	{ GL_CALL( glBindBufferARB ) },
	{ GL_CALL( glDeleteBuffersARB ) },
	{ GL_CALL( glGenBuffersARB ) },
	{ GL_CALL( glIsBufferARB ) },
	{ GL_CALL( glMapBufferARB ) },
	{ GL_CALL( glUnmapBufferARB ) }, // ,
	{ GL_CALL( glBufferDataARB ) },
	{ GL_CALL( glBufferSubDataARB ) },
	{ NULL, NULL }
};

static dllfunc_t multisampletexfuncs[] =
{
	{ GL_CALL(glTexImage2DMultisample) },
	{ NULL, NULL }
};

static dllfunc_t drawrangeelementsfuncs[] =
{
{ GL_CALL( glDrawRangeElements ) },
{ NULL, NULL }
};

static dllfunc_t drawrangeelementsextfuncs[] =
{
{ GL_CALL( glDrawRangeElementsEXT ) },
{ NULL, NULL }
};

/*
========================
DebugCallback

For ARB_debug_output
========================
*/
static void APIENTRY GL_DebugOutput( GLuint source, GLuint type, GLuint id, GLuint severity, GLint length, const GLcharARB *message, GLvoid *userParam )
{
	switch( type )
	{
	case GL_DEBUG_TYPE_ERROR_ARB:
		gEngfuncs.Con_Printf( S_OPENGL_ERROR "%s\n", message );
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
		gEngfuncs.Con_Printf( S_OPENGL_WARN "%s\n", message );
		break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB:
		gEngfuncs.Con_Reportf( S_OPENGL_WARN "%s\n", message );
		break;
	case GL_DEBUG_TYPE_PERFORMANCE_ARB:
	case GL_DEBUG_TYPE_OTHER_ARB:
	default:
		gEngfuncs.Con_Printf( S_OPENGL_NOTE "%s\n", message );
		break;
	}
}

/*
=================
GL_SetExtension
=================
*/
void GL_SetExtension( int r_ext, int enable )
{
	if( r_ext >= 0 && r_ext < GL_EXTCOUNT )
		glConfig.extension[r_ext] = enable ? GL_TRUE : GL_FALSE;
	else gEngfuncs.Con_Printf( S_ERROR "GL_SetExtension: invalid extension %d\n", r_ext );
}

/*
=================
GL_Support
=================
*/
qboolean GL_Support( int r_ext )
{
#if XASH_RAYTRACING
    switch(r_ext)
    {
        case GL_OPENGL_110:
        case GL_TEXTURE_2D_RECT_EXT:
            return true;

        case GL_ARB_VERTEX_BUFFER_OBJECT_EXT:
        default:
			return false;
    }

#else
	if( r_ext >= 0 && r_ext < GL_EXTCOUNT )
		return glConfig.extension[r_ext] ? true : false;
	gEngfuncs.Con_Printf( S_ERROR "GL_Support: invalid extension %d\n", r_ext );

	return false;
#endif
}

/*
=================
GL_MaxTextureUnits
=================
*/
int GL_MaxTextureUnits( void )
{
	if( GL_Support( GL_SHADER_GLSL100_EXT ))
		return Q_min( Q_max( glConfig.max_texture_coords, glConfig.max_teximage_units ), MAX_TEXTURE_UNITS );
	return glConfig.max_texture_units;
}

/*
=================
GL_CheckExtension
=================
*/
qboolean GL_CheckExtension( const char *name, const dllfunc_t *funcs, const char *cvarname, int r_ext )
{
	const dllfunc_t	*func;
	cvar_t		*parm = NULL;
	const char	*extensions_string;

	gEngfuncs.Con_Reportf( "GL_CheckExtension: %s ", name );
	GL_SetExtension( r_ext, true );

	if( cvarname )
	{
		// system config disable extensions
		parm = gEngfuncs.Cvar_Get( cvarname, "1", FCVAR_GLCONFIG|FCVAR_READ_ONLY, va( CVAR_GLCONFIG_DESCRIPTION, name ));
	}

	if(( parm && !CVAR_TO_BOOL( parm )) || ( !CVAR_TO_BOOL( gl_extensions ) && r_ext != GL_OPENGL_110 ))
	{
		gEngfuncs.Con_Reportf( "- disabled\n" );
		GL_SetExtension( r_ext, false );
		return false; // nothing to process at
	}

	extensions_string = glConfig.extensions_string;

	if(( name[2] == '_' || name[3] == '_' ) && !Q_strstr( extensions_string, name ))
	{
		GL_SetExtension( r_ext, false );	// update render info
		gEngfuncs.Con_Reportf( "- ^1failed\n" );
		return false;
	}

#ifndef XASH_GL_STATIC
	// clear exports
	for( func = funcs; func && func->name; func++ )
		*func->func = NULL;

	for( func = funcs; func && func->name; func++ )
	{
		// functions are cleared before all the extensions are evaluated
		if((*func->func = (void *)gEngfuncs.GL_GetProcAddress( func->name )) == NULL )
			GL_SetExtension( r_ext, false ); // one or more functions are invalid, extension will be disabled
	}
#endif

	if( GL_Support( r_ext ))
	{
		gEngfuncs.Con_Reportf( "- ^2enabled\n" );
		return true;
	}

	gEngfuncs.Con_Reportf( "- ^1failed\n" );
	return false;
}

/*
==============
GL_GetProcAddress

defined just for nanogl/glwes, so it don't link to SDL2 directly, nor use dlsym
==============
*/
void GAME_EXPORT *GL_GetProcAddress( const char *name )
{
	return gEngfuncs.GL_GetProcAddress( name );
}

/*
===============
GL_SetDefaultTexState
===============
*/
static void GL_SetDefaultTexState( void )
{

	int	i;

	memset( glState.currentTextures, -1, MAX_TEXTURE_UNITS * sizeof( *glState.currentTextures ));
	memset( glState.texCoordArrayMode, 0, MAX_TEXTURE_UNITS * sizeof( *glState.texCoordArrayMode ));
	memset( glState.genSTEnabled, 0, MAX_TEXTURE_UNITS * sizeof( *glState.genSTEnabled ));

	for( i = 0; i < MAX_TEXTURE_UNITS; i++ )
	{
		glState.currentTextureTargets[i] = GL_NONE;
		glState.texIdentityMatrix[i] = true;
	}
}

/*
===============
GL_SetDefaultState
===============
*/
static void GL_SetDefaultState( void )
{
	memset( &glState, 0, sizeof( glState ));
	GL_SetDefaultTexState ();

	// init draw stack
	tr.draw_list = &tr.draw_stack[0];
	tr.draw_stack_pos = 0;
}

/*
===============
GL_SetDefaults
===============
*/
static void GL_SetDefaults( void )
{
	pglFinish();

	pglClearColor( 0.5f, 0.5f, 0.5f, 1.0f );

	pglDisable( GL_DEPTH_TEST );
	pglDisable( GL_CULL_FACE );
	pglDisable( GL_SCISSOR_TEST );
	pglDepthFunc( GL_LEQUAL );
	pglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	if( glState.stencilEnabled )
	{
		pglDisable( GL_STENCIL_TEST );
		pglStencilMask( ( GLuint ) ~0 );
		pglStencilFunc( GL_EQUAL, 0, ~0 );
		pglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
	}

	pglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	pglPolygonOffset( -1.0f, -2.0f );

	GL_CleanupAllTextureUnits();

	pglDisable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglDisable( GL_POLYGON_OFFSET_FILL );
	pglAlphaFunc( GL_GREATER, DEFAULT_ALPHATEST );
	pglEnable( GL_TEXTURE_2D );
	pglShadeModel( GL_SMOOTH );
	pglFrontFace( GL_CCW );

	pglPointSize( 1.2f );
	pglLineWidth( 1.2f );

	GL_Cull( GL_NONE );
}


/*
=================
R_RenderInfo_f
=================
*/
void R_RenderInfo_f( void )
{
	gEngfuncs.Con_Printf( "\n" );
	gEngfuncs.Con_Printf( "GL_VENDOR: %s\n", glConfig.vendor_string );
	gEngfuncs.Con_Printf( "GL_RENDERER: %s\n", glConfig.renderer_string );
	gEngfuncs.Con_Printf( "GL_VERSION: %s\n", glConfig.version_string );

	// don't spam about extensions
	gEngfuncs.Con_Reportf( "GL_EXTENSIONS: %s\n", glConfig.extensions_string );

	if( glConfig.wrapper == GLES_WRAPPER_GL4ES )
	{
		const char *vendor = (const char *)pglGetString( GL_VENDOR | 0x10000 );
		const char *renderer = (const char *)pglGetString( GL_RENDERER | 0x10000 );
		const char *version = (const char *)pglGetString( GL_VERSION | 0x10000 );
		const char *extensions = (const char *)pglGetString( GL_EXTENSIONS | 0x10000 );

		if( vendor )
			gEngfuncs.Con_Printf( "GL4ES_VENDOR: %s\n", vendor );
		if( renderer )
			gEngfuncs.Con_Printf( "GL4ES_RENDERER: %s\n", renderer );
		if( version )
			gEngfuncs.Con_Printf( "GL4ES_VERSION: %s\n", version );
		if( extensions )
			gEngfuncs.Con_Reportf( "GL4ES_EXTENSIONS: %s\n", extensions );

	}


	gEngfuncs.Con_Printf( "GL_MAX_TEXTURE_SIZE: %i\n", glConfig.max_2d_texture_size );

	if( GL_Support( GL_ARB_MULTITEXTURE ))
		gEngfuncs.Con_Printf( "GL_MAX_TEXTURE_UNITS_ARB: %i\n", glConfig.max_texture_units );
	if( GL_Support( GL_TEXTURE_CUBEMAP_EXT ))
		gEngfuncs.Con_Printf( "GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB: %i\n", glConfig.max_cubemap_size );
	if( GL_Support( GL_ANISOTROPY_EXT ))
		gEngfuncs.Con_Printf( "GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: %.1f\n", glConfig.max_texture_anisotropy );
	if( GL_Support( GL_TEXTURE_2D_RECT_EXT ))
		gEngfuncs.Con_Printf( "GL_MAX_RECTANGLE_TEXTURE_SIZE: %i\n", glConfig.max_2d_rectangle_size );
	if( GL_Support( GL_TEXTURE_ARRAY_EXT ))
		gEngfuncs.Con_Printf( "GL_MAX_ARRAY_TEXTURE_LAYERS_EXT: %i\n", glConfig.max_2d_texture_layers );
	if( GL_Support( GL_SHADER_GLSL100_EXT ))
	{
		gEngfuncs.Con_Printf( "GL_MAX_TEXTURE_COORDS_ARB: %i\n", glConfig.max_texture_coords );
		gEngfuncs.Con_Printf( "GL_MAX_TEXTURE_IMAGE_UNITS_ARB: %i\n", glConfig.max_teximage_units );
		gEngfuncs.Con_Printf( "GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB: %i\n", glConfig.max_vertex_uniforms );
		gEngfuncs.Con_Printf( "GL_MAX_VERTEX_ATTRIBS_ARB: %i\n", glConfig.max_vertex_attribs );
	}

	gEngfuncs.Con_Printf( "\n" );
	gEngfuncs.Con_Printf( "MODE: %ix%i\n", gpGlobals->width, gpGlobals->height );
	gEngfuncs.Con_Printf( "\n" );
	gEngfuncs.Con_Printf( "VERTICAL SYNC: %s\n", gl_vsync->value ? "enabled" : "disabled" );
	gEngfuncs.Con_Printf( "Color %d bits, Alpha %d bits, Depth %d bits, Stencil %d bits\n", glConfig.color_bits,
		glConfig.alpha_bits, glConfig.depth_bits, glConfig.stencil_bits );
}

#ifdef XASH_GLES
void GL_InitExtensionsGLES( void )
{
	int extid;

	// intialize wrapper type
#ifdef XASH_NANOGL
	glConfig.context = CONTEXT_TYPE_GLES_1_X;
	glConfig.wrapper = GLES_WRAPPER_NANOGL;
#elif defined( XASH_WES )
	glConfig.context = CONTEXT_TYPE_GLES_2_X;
	glConfig.wrapper = GLES_WRAPPER_WES;
#else
#error "unknown gles wrapper"
#endif

	glConfig.hardware_type = GLHW_GENERIC;

	for( extid = GL_OPENGL_110 + 1; extid < GL_EXTCOUNT; extid++ )
	{
		switch( extid )
		{
		case GL_ARB_VERTEX_BUFFER_OBJECT_EXT:
			GL_SetExtension( extid, true );
			break;
		case GL_ARB_MULTITEXTURE:
			GL_SetExtension( extid, true ); // required to be supported by wrapper

			pglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glConfig.max_texture_units );
			if( glConfig.max_texture_units <= 1 )
			{
				GL_SetExtension( extid, false );
				glConfig.max_texture_units = 1;
			}

			glConfig.max_texture_coords = glConfig.max_teximage_units = glConfig.max_texture_units;
			break;
		case GL_TEXTURE_CUBEMAP_EXT:
			if( GL_CheckExtension( "GL_OES_texture_cube_map", NULL, "gl_texture_cubemap", extid ))
				pglGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.max_cubemap_size );
			break;
		case GL_ANISOTROPY_EXT:
			glConfig.max_texture_anisotropy = 0.0f;
			if( GL_CheckExtension( "GL_EXT_texture_filter_anisotropic", NULL, "gl_ext_anisotropic_filter", extid ))
				pglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.max_texture_anisotropy );
			break;
		case GL_TEXTURE_LOD_BIAS:
			if( GL_CheckExtension( "GL_EXT_texture_lod_bias", NULL, "gl_texture_mipmap_biasing", extid ))
				pglGetFloatv( GL_MAX_TEXTURE_LOD_BIAS_EXT, &glConfig.max_texture_lod_bias );
			break;
		case GL_ARB_TEXTURE_NPOT_EXT:
			GL_CheckExtension( "GL_OES_texture_npot", NULL, "gl_texture_npot", extid );
			break;
		case GL_DEBUG_OUTPUT:
			if( glw_state.extended )
				GL_CheckExtension( "GL_KHR_debug", NULL, NULL, extid );
			break;
		// case GL_TEXTURE_COMPRESSION_EXT: NOPE
		// case GL_SHADER_GLSL100_EXT: NOPE
		// case GL_TEXTURE_2D_RECT_EXT: NOPE
		// case GL_TEXTURE_ARRAY_EXT: NOPE
		// case GL_TEXTURE_3D_EXT: NOPE
		// case GL_CLAMPTOEDGE_EXT: NOPE
		// case GL_CLAMP_TEXBORDER_EXT: NOPE
		// case GL_ARB_TEXTURE_FLOAT_EXT: NOPE
		// case GL_ARB_DEPTH_FLOAT_EXT: NOPE
		// case GL_ARB_SEAMLESS_CUBEMAP: NOPE
		// case GL_EXT_GPU_SHADER4: NOPE
		// case GL_DEPTH_TEXTURE: NOPE
		// case GL_DRAWRANGEELEMENTS_EXT: NOPE
		default:
			GL_SetExtension( extid, false );
		}
	}
}
#else
void GL_InitExtensionsBigGL( void )
{
	// intialize wrapper type
	glConfig.context = CONTEXT_TYPE_GL;
	glConfig.wrapper = GLES_WRAPPER_NONE;

	if( Q_stristr( glConfig.renderer_string, "geforce" ))
		glConfig.hardware_type = GLHW_NVIDIA;
	else if( Q_stristr( glConfig.renderer_string, "quadro fx" ))
		glConfig.hardware_type = GLHW_NVIDIA;
	else if( Q_stristr(glConfig.renderer_string, "rv770" ))
		glConfig.hardware_type = GLHW_RADEON;
	else if( Q_stristr(glConfig.renderer_string, "radeon hd" ))
		glConfig.hardware_type = GLHW_RADEON;
	else if( Q_stristr( glConfig.renderer_string, "eah4850" ) || Q_stristr( glConfig.renderer_string, "eah4870" ))
		glConfig.hardware_type = GLHW_RADEON;
	else if( Q_stristr( glConfig.renderer_string, "radeon" ))
		glConfig.hardware_type = GLHW_RADEON;
	else if( Q_stristr( glConfig.renderer_string, "intel" ))
		glConfig.hardware_type = GLHW_INTEL;
	else glConfig.hardware_type = GLHW_GENERIC;

	// gl4es may be used system-wide
	if( Q_stristr( glConfig.renderer_string, "gl4es" ))
	{
		const char *vendor = (const char *)pglGetString( GL_VENDOR | 0x10000 );
		const char *renderer = (const char *)pglGetString( GL_RENDERER | 0x10000 );
		const char *version = (const char *)pglGetString( GL_VERSION | 0x10000 );
		const char *extensions = (const char *)pglGetString( GL_EXTENSIONS | 0x10000 );
		glConfig.wrapper = GLES_WRAPPER_GL4ES;
	}

	// multitexture
	glConfig.max_texture_units = glConfig.max_texture_coords = glConfig.max_teximage_units = 1;
	if( GL_CheckExtension( "GL_ARB_multitexture", multitexturefuncs, "gl_arb_multitexture", GL_ARB_MULTITEXTURE ))
	{
		pglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glConfig.max_texture_units );
	}

	if( glConfig.max_texture_units == 1 )
		GL_SetExtension( GL_ARB_MULTITEXTURE, false );

	// 3d texture support
	if( GL_CheckExtension( "GL_EXT_texture3D", texture3dextfuncs, "gl_texture_3d", GL_TEXTURE_3D_EXT ))
	{
		pglGetIntegerv( GL_MAX_3D_TEXTURE_SIZE, &glConfig.max_3d_texture_size );

		if( glConfig.max_3d_texture_size < 32 )
		{
			GL_SetExtension( GL_TEXTURE_3D_EXT, false );
			gEngfuncs.Con_Printf( S_ERROR "GL_EXT_texture3D reported bogus GL_MAX_3D_TEXTURE_SIZE, disabled\n" );
		}
	}

	// 2d texture array support
	if( GL_CheckExtension( "GL_EXT_texture_array", texture3dextfuncs, "gl_texture_2d_array", GL_TEXTURE_ARRAY_EXT ))
		pglGetIntegerv( GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, &glConfig.max_2d_texture_layers );

	// cubemaps support
	if( GL_CheckExtension( "GL_ARB_texture_cube_map", NULL, "gl_texture_cubemap", GL_TEXTURE_CUBEMAP_EXT ))
	{
		pglGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.max_cubemap_size );

		// check for seamless cubemaps too
		GL_CheckExtension( "GL_ARB_seamless_cube_map", NULL, "gl_texture_cubemap_seamless", GL_ARB_SEAMLESS_CUBEMAP );
	}

	GL_CheckExtension( "GL_ARB_texture_non_power_of_two", NULL, "gl_texture_npot", GL_ARB_TEXTURE_NPOT_EXT );
	GL_CheckExtension( "GL_ARB_texture_compression", texturecompressionfuncs, "gl_texture_dxt_compression", GL_TEXTURE_COMPRESSION_EXT );
	if( !GL_CheckExtension( "GL_EXT_texture_edge_clamp", NULL, "gl_clamp_to_edge", GL_CLAMPTOEDGE_EXT ))
		GL_CheckExtension( "GL_SGIS_texture_edge_clamp", NULL, "gl_clamp_to_edge", GL_CLAMPTOEDGE_EXT );

	glConfig.max_texture_anisotropy = 0.0f;
	if( GL_CheckExtension( "GL_EXT_texture_filter_anisotropic", NULL, "gl_texture_anisotropic_filter", GL_ANISOTROPY_EXT ))
		pglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.max_texture_anisotropy );

#if XASH_WIN32 // Win32 only drivers?
	// g-cont. because lodbias it too glitchy on Intel's cards
	if( glConfig.hardware_type != GLHW_INTEL )
#endif
	{
		if( GL_CheckExtension( "GL_EXT_texture_lod_bias", NULL, "gl_texture_mipmap_biasing", GL_TEXTURE_LOD_BIAS ))
			pglGetFloatv( GL_MAX_TEXTURE_LOD_BIAS_EXT, &glConfig.max_texture_lod_bias );
	}

	GL_CheckExtension( "GL_ARB_texture_border_clamp", NULL, NULL, GL_CLAMP_TEXBORDER_EXT );

	GL_CheckExtension( "GL_ARB_depth_texture", NULL, NULL, GL_DEPTH_TEXTURE );
	GL_CheckExtension( "GL_ARB_texture_float", NULL, "gl_texture_float", GL_ARB_TEXTURE_FLOAT_EXT );
	GL_CheckExtension( "GL_ARB_depth_buffer_float", NULL, "gl_texture_depth_float", GL_ARB_DEPTH_FLOAT_EXT );
	GL_CheckExtension( "GL_EXT_gpu_shader4", NULL, NULL, GL_EXT_GPU_SHADER4 ); // don't confuse users
	GL_CheckExtension( "GL_ARB_vertex_buffer_object", vbofuncs, "gl_vertex_buffer_object", GL_ARB_VERTEX_BUFFER_OBJECT_EXT );
	GL_CheckExtension( "GL_ARB_texture_multisample", multisampletexfuncs, "gl_texture_multisample", GL_TEXTURE_MULTISAMPLE );
	GL_CheckExtension( "GL_ARB_texture_compression_bptc", NULL, "gl_texture_bptc_compression", GL_ARB_TEXTURE_COMPRESSION_BPTC );
	if( GL_CheckExtension( "GL_ARB_shading_language_100", NULL, NULL, GL_SHADER_GLSL100_EXT ))
	{
		pglGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, &glConfig.max_texture_coords );
		pglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &glConfig.max_teximage_units );

		// check for hardware skinning
		pglGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig.max_vertex_uniforms );
		pglGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig.max_vertex_attribs );

#if XASH_WIN32 // Win32 only drivers?
		if( glConfig.hardware_type == GLHW_RADEON && glConfig.max_vertex_uniforms > 512 )
			glConfig.max_vertex_uniforms /= 4; // radeon returns not correct info
#endif
	}
	else
	{
		// just get from multitexturing
		glConfig.max_texture_coords = glConfig.max_teximage_units = glConfig.max_texture_units;
	}

	// rectangle textures support
	GL_CheckExtension( "GL_ARB_texture_rectangle", NULL, "gl_texture_rectangle", GL_TEXTURE_2D_RECT_EXT );

	if( !GL_CheckExtension( "glDrawRangeElements", drawrangeelementsfuncs, "gl_drawrangeelements", GL_DRAW_RANGEELEMENTS_EXT ) )
	{
		if( GL_CheckExtension( "glDrawRangeElementsEXT", drawrangeelementsextfuncs,
			"gl_drawrangelements", GL_DRAW_RANGEELEMENTS_EXT ) )
		{
#ifndef XASH_GL_STATIC
			pglDrawRangeElements = pglDrawRangeElementsEXT;
#endif
		}
	}

	// this won't work without extended context
	if( glw_state.extended )
		GL_CheckExtension( "GL_ARB_debug_output", debugoutputfuncs, "gl_debug_output", GL_DEBUG_OUTPUT );
}
#endif

void GL_InitExtensions( void )
{
#if !XASH_RAYTRACING
	GL_OnContextCreated();

	// initialize gl extensions
	GL_CheckExtension( "OpenGL 1.1.0", opengl_110funcs, NULL, GL_OPENGL_110 );

	// get our various GL strings
	glConfig.vendor_string = (const char *)pglGetString( GL_VENDOR );
	glConfig.renderer_string = (const char *)pglGetString( GL_RENDERER );
	glConfig.version_string = (const char *)pglGetString( GL_VERSION );
	glConfig.extensions_string = (const char *)pglGetString( GL_EXTENSIONS );
	gEngfuncs.Con_Reportf( "^3Video^7: %s\n", glConfig.renderer_string );

#ifdef XASH_GLES
	GL_InitExtensionsGLES();
#else
	GL_InitExtensionsBigGL();
#endif

	pglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.max_2d_texture_size );
	if( glConfig.max_2d_texture_size <= 0 ) glConfig.max_2d_texture_size = 256;
#ifndef XASH_GL4ES
	// enable gldebug if allowed
	if( GL_Support( GL_DEBUG_OUTPUT ))
	{
		if( gpGlobals->developer )
		{
			gEngfuncs.Con_Reportf( "Installing GL_DebugOutput...\n");
			pglDebugMessageCallbackARB( GL_DebugOutput, NULL );

			// force everything to happen in the main thread instead of in a separate driver thread
			pglEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
		}

		// enable all the low priority messages
		pglDebugMessageControlARB( GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, NULL, true );
	}
#endif
	if( GL_Support( GL_TEXTURE_2D_RECT_EXT ))
		pglGetIntegerv( GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &glConfig.max_2d_rectangle_size );

	gEngfuncs.Cvar_Get( "gl_max_size", va( "%i", glConfig.max_2d_texture_size ), 0, "opengl texture max dims" );
	gEngfuncs.Cvar_Set( "gl_anisotropy", va( "%f", bound( 0, gl_texture_anisotropy->value, glConfig.max_texture_anisotropy )));

	if( GL_Support( GL_TEXTURE_COMPRESSION_EXT ))
		gEngfuncs.Image_AddCmdFlags( IL_DDS_HARDWARE );

	// MCD has buffering issues
#if XASH_WIN32
	if( Q_strstr( glConfig.renderer_string, "gdi" ))
		gEngfuncs.Cvar_SetValue( "gl_finish", 1 );
#endif

	R_RenderInfo_f();
#else
    glConfig.vendor_string     = "";
    glConfig.renderer_string   = "";
    glConfig.version_string    = "";
    glConfig.extensions_string = "";

    glConfig.max_2d_texture_size    = 4096;
    glConfig.max_2d_rectangle_size  = 4096;
    glConfig.max_texture_anisotropy = 8;
#endif

	tr.framecount = tr.visframecount = 1;
	glw_state.initialized = true;
}

void GL_ClearExtensions( void )
{
	// now all extensions are disabled
	memset( glConfig.extension, 0, sizeof( glConfig.extension ));
	glw_state.initialized = false;
}

//=======================================================================

/*
=================
GL_InitCommands
=================
*/
void GL_InitCommands( void )
{
	RETRIEVE_ENGINE_SHARED_CVAR_LIST();

	r_lighting_extended = gEngfuncs.Cvar_Get( "r_lighting_extended", "1", FCVAR_GLCONFIG, "allow to get lighting from world and bmodels" );
	r_lighting_modulate = gEngfuncs.Cvar_Get( "r_lighting_modulate", "0.6", FCVAR_GLCONFIG, "lightstyles modulate scale" );
	r_lighting_ambient = gEngfuncs.Cvar_Get( "r_lighting_ambient", "0.3", FCVAR_GLCONFIG, "map ambient lighting scale" );
	r_novis = gEngfuncs.Cvar_Get( "r_novis", "0", 0, "ignore vis information (perfomance test)" );
	r_nocull = gEngfuncs.Cvar_Get( "r_nocull", "0", 0, "ignore frustrum culling (perfomance test)" );
	r_detailtextures = gEngfuncs.Cvar_Get( "r_detailtextures", "1", FCVAR_ARCHIVE, "enable detail textures support" );
	r_lockpvs = gEngfuncs.Cvar_Get( "r_lockpvs", "0", FCVAR_CHEAT, "lockpvs area at current point (pvs test)" );
	r_lockfrustum = gEngfuncs.Cvar_Get( "r_lockfrustum", "0", FCVAR_CHEAT, "lock frustrum area at current point (cull test)" );
	r_traceglow = gEngfuncs.Cvar_Get( "r_traceglow", "1", FCVAR_GLCONFIG, "cull flares behind models" );

	gl_extensions = gEngfuncs.Cvar_Get( "gl_allow_extensions", "1", FCVAR_GLCONFIG|FCVAR_READ_ONLY, "allow gl_extensions" );
	gl_texture_nearest = gEngfuncs.Cvar_Get( "gl_texture_nearest", "0", FCVAR_GLCONFIG, "disable texture filter" );
	gl_lightmap_nearest = gEngfuncs.Cvar_Get( "gl_lightmap_nearest", "0", FCVAR_GLCONFIG, "disable lightmap filter" );
	gl_check_errors = gEngfuncs.Cvar_Get( "gl_check_errors", "1", FCVAR_GLCONFIG, "ignore video engine errors" );
	gl_texture_anisotropy = gEngfuncs.Cvar_Get( "gl_anisotropy", "8", FCVAR_GLCONFIG, "textures anisotropic filter" );
	gl_texture_lodbias =  gEngfuncs.Cvar_Get( "gl_texture_lodbias", "0.0", FCVAR_GLCONFIG, "LOD bias for mipmapped textures (perfomance|quality)" );
	gl_keeptjunctions = gEngfuncs.Cvar_Get( "gl_keeptjunctions", "1", FCVAR_GLCONFIG, "removing tjuncs causes blinking pixels" );
	gl_finish = gEngfuncs.Cvar_Get( "gl_finish", "0", FCVAR_GLCONFIG, "use glFinish instead of glFlush" );
	gl_nosort = gEngfuncs.Cvar_Get( "gl_nosort", "0", FCVAR_GLCONFIG, "disable sorting of translucent surfaces" );
	gl_test = gEngfuncs.Cvar_Get( "gl_test", "0", 0, "engine developer cvar for quick testing new features" );
	gl_wireframe = gEngfuncs.Cvar_Get( "gl_wireframe", "0", FCVAR_GLCONFIG|FCVAR_SPONLY, "show wireframe overlay" );
	gl_msaa = gEngfuncs.Cvar_Get( "gl_msaa", "1", FCVAR_GLCONFIG, "enable or disable multisample anti-aliasing" );
	gl_stencilbits = gEngfuncs.Cvar_Get( "gl_stencilbits", "8", FCVAR_GLCONFIG|FCVAR_READ_ONLY, "pixelformat stencil bits (0 - auto)" );
	gl_round_down = gEngfuncs.Cvar_Get( "gl_round_down", "2", FCVAR_GLCONFIG|FCVAR_READ_ONLY, "round texture sizes to nearest POT value" );

	// these cvar not used by engine but some mods requires this
	gl_polyoffset = gEngfuncs.Cvar_Get( "gl_polyoffset", "2.0", FCVAR_GLCONFIG, "polygon offset for decals" );

	// make sure gl_vsync is checked after vid_restart
	SetBits( gl_vsync->flags, FCVAR_CHANGED );

	gEngfuncs.Cmd_AddCommand( "r_info", R_RenderInfo_f, "display renderer info" );
	gEngfuncs.Cmd_AddCommand( "timerefresh", SCR_TimeRefresh_f, "turn quickly and print rendering statistcs" );
}

/*
===============
R_CheckVBO

register VBO cvars and get default value
===============
*/
static void R_CheckVBO( void )
{
	const char *def = "0";
	const char *dlightmode = "1";
	int flags = FCVAR_ARCHIVE;
	qboolean disable = false;

	// some bad GLES1 implementations breaks dlights completely
	if( glConfig.max_texture_units < 3 )
		disable = true;

#ifdef XASH_MOBILE_PLATFORM
	// VideoCore4 drivers have a problem with mixing VBO and client arrays
	// Disable it, as there is no suitable workaround here
	if( Q_stristr( glConfig.renderer_string, "VideoCore IV" ) || Q_stristr( glConfig.renderer_string, "vc4" ) )
		disable = true;

	// dlightmode 1 is not too much tested on android
	// so better to left it off
	dlightmode = "0";
#endif

	if( disable )
	{
		// do not keep in config unless dev > 3 and enabled
		flags = 0;
		def = "0";
	}

	r_vbo = gEngfuncs.Cvar_Get( "gl_vbo", def, flags, "draw world using VBO (known to be glitchy)" );
	r_vbo_dlightmode = gEngfuncs.Cvar_Get( "gl_vbo_dlightmode", dlightmode, FCVAR_ARCHIVE, "vbo dlight rendering mode(0-1)" );
}

/*
=================
GL_RemoveCommands
=================
*/
void GL_RemoveCommands( void )
{
	gEngfuncs.Cmd_RemoveCommand( "r_info" );
}


static void PrintMessage(const char* pMessage, RgMessageSeverityFlags severity, void* pUserData)
{
	if (severity & RG_MESSAGE_SEVERITY_ERROR)
	{
		gEngfuncs.Host_Error(pMessage);
	}

	gEngfuncs.Con_Printf(pMessage);
}

/*
===============
R_Init
===============
*/
qboolean R_Init( void )
{
	if( glw_state.initialized )
		return true;

	GL_InitCommands();
	GL_InitRandomTable();

	GL_SetDefaultState();

	// create the window and set up the context
    if( !gEngfuncs.R_Init_Video( REF_RT ) ) // request GL context
	{
		GL_RemoveCommands();
		gEngfuncs.R_Free_Video();
// Why? Host_Error again???
//		gEngfuncs.Host_Error( "Can't initialize video subsystem\nProbably driver was not installed" );
		return false;
	}

	{
		RgWin32SurfaceCreateInfo win32Info = {
			.hinstance = GetModuleHandle(NULL),
			.hwnd = gpGlobals->rtglHwnd,
		};

		#define ASSET_DIRECTORY "rt/"

        RgInstanceCreateInfo info = {
            .pAppName = "Xash",
            .pAppGUID = "986af412-bab4-4e44-a603-bfaf49e7ef4d",

            .pWin32SurfaceInfo = &win32Info,

            .pfnPrint = PrintMessage,

            .pShaderFolderPath  = ASSET_DIRECTORY "shaders/",
            .pBlueNoiseFilePath = ASSET_DIRECTORY "BlueNoise_LDR_RGBA_128.ktx2",

            .primaryRaysMaxAlbedoLayers          = 1,
            .indirectIlluminationMaxAlbedoLayers = 1,
            .rayCullBackFacingTriangles          = 0,
            .allowGeometryWithSkyFlag            = 1,

            .rasterizedMaxVertexCount = 1 << 20,
            .rasterizedMaxIndexCount  = 1 << 21,
            .rasterizedVertexColorGamma = true,

            .rasterizedSkyCubemapSize = 256,

            .maxTextureCount                             = MAX_TEXTURES,
            .textureSamplerForceMinificationFilterLinear = true,

            .pOverridenTexturesFolderPath = ASSET_DIRECTORY,

            .overridenAlbedoAlphaTextureIsSRGB               = true,
            .overridenRoughnessMetallicEmissionTextureIsSRGB = false,
            .overridenNormalTextureIsSRGB                    = false,

            .originalAlbedoAlphaTextureIsSRGB               = true,
            .originalRoughnessMetallicEmissionTextureIsSRGB = false,
            .originalNormalTextureIsSRGB                    = false,

            .pWaterNormalTexturePath = ASSET_DIRECTORY "WaterNormal_n.ktx2",

            // to match the GLTF standard
            .pbrTextureSwizzling = RG_TEXTURE_SWIZZLING_NULL_ROUGHNESS_METALLIC,
        };

		RgResult r = rgCreateInstance(&info, &rg_instance);
		RG_CHECK(r);

		{
            const rt_state_t nullstate = {
                .viewport        = { 0 },
                .projMatrixFor2D = { 0 },

                .curTexture2DName  = NULL,
                .curTextureNearest = false,
                .curTextureClamped = false,

                .curIsRasterized = false,

                .curStudioBodyPart = -1,
                .curStudioSubmodel = -1,
                .curStudioMesh     = -1,
                .curStudioGlend    = -1,

                .curBrushSurface = -1,
            };
			memcpy( &rt_state, &nullstate, sizeof( rt_state ) );
		}
	}

	r_temppool = Mem_AllocPool( "Render Zone" );

	GL_SetDefaults();
	R_CheckVBO();
	R_InitImages();
	R_SpriteInit();
	R_StudioInit();
	R_AliasInit();
	R_ClearDecals();
	R_ClearScene();

#if XASH_RAYTRACING
	glw_state.initialized = true;
#endif
	return true;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown( void )
{
	if( !glw_state.initialized )
		return;

	GL_RemoveCommands();
	R_ShutdownImages();

	Mem_FreePool( &r_temppool );

#ifdef XASH_GL4ES
	close_gl4es();
#endif // XASH_GL4ES

	// shut down OS specific OpenGL stuff like contexts, etc.
	gEngfuncs.R_Free_Video();

	if( rg_instance )
	{
		rgDestroyInstance( rg_instance );
	}
}

/*
=================
GL_ErrorString
convert errorcode to string
=================
*/
const char *GL_ErrorString( int err )
{
	switch( err )
	{
	case GL_STACK_OVERFLOW:
		return "GL_STACK_OVERFLOW";
	case GL_STACK_UNDERFLOW:
		return "GL_STACK_UNDERFLOW";
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	default:
		return "UNKNOWN ERROR";
	}
}

/*
=================
GL_CheckForErrors
obsolete
=================
*/
void GL_CheckForErrors_( const char *filename, const int fileline )
{
#if !XASH_RAYTRACING
	int	err;

	if( !CVAR_TO_BOOL( gl_check_errors ))
		return;

	if(( err = pglGetError( )) == GL_NO_ERROR )
		return;

	gEngfuncs.Con_Printf( S_OPENGL_ERROR "%s (at %s:%i)\n", GL_ErrorString( err ), filename, fileline );
#endif
}

void GL_SetupAttributes( int safegl )
{
	int context_flags = 0; // REFTODO!!!!!
	int samples = 0;

#ifdef XASH_GLES
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_PROFILE_MASK, REF_GL_CONTEXT_PROFILE_ES );
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_EGL, 1 );
#ifdef XASH_NANOGL
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_MAJOR_VERSION, 1 );
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_MINOR_VERSION, 1 );
#elif defined( XASH_WES ) || defined( XASH_REGAL )
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_MAJOR_VERSION, 2 );
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_MINOR_VERSION, 0 );
#endif
#elif defined XASH_GL4ES
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_PROFILE_MASK, REF_GL_CONTEXT_PROFILE_ES );
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_EGL, 1 );
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_MAJOR_VERSION, 2 );
	gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_MINOR_VERSION, 0 );
#else // GL1.x
	if( gEngfuncs.Sys_CheckParm( "-glcore" ))
	{
		SetBits( context_flags, FCONTEXT_CORE_PROFILE );

		gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_PROFILE_MASK, REF_GL_CONTEXT_PROFILE_CORE );
	}
	else
	{
		gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_PROFILE_MASK, REF_GL_CONTEXT_PROFILE_COMPATIBILITY );
	}
#endif // XASH_GLES

	if( gEngfuncs.Sys_CheckParm( "-gldebug" ) )
	{
		gEngfuncs.Con_Reportf( "Creating an extended GL context for debug...\n" );
		SetBits( context_flags, FCONTEXT_DEBUG_ARB );
		gEngfuncs.GL_SetAttribute( REF_GL_CONTEXT_FLAGS, REF_GL_CONTEXT_DEBUG_FLAG );
		glw_state.extended = true;
	}

	if( safegl > SAFE_DONTCARE )
	{
		safegl = -1; // can't retry anymore, can only shutdown engine
		return;
	}

	gEngfuncs.Con_Printf( "Trying safe opengl mode %d\n", safegl );

	if( safegl == SAFE_DONTCARE )
		return;

	gEngfuncs.GL_SetAttribute( REF_GL_DOUBLEBUFFER, 1 );

	if( safegl < SAFE_NOACC )
		gEngfuncs.GL_SetAttribute( REF_GL_ACCELERATED_VISUAL, 1 );

	gEngfuncs.Con_Printf( "bpp %d\n", gpGlobals->desktopBitsPixel );

	if( safegl < SAFE_NOSTENCIL )
		gEngfuncs.GL_SetAttribute( REF_GL_STENCIL_SIZE, gl_stencilbits->value );

	if( safegl < SAFE_NOALPHA )
		gEngfuncs.GL_SetAttribute( REF_GL_ALPHA_SIZE, 8 );

	if( safegl < SAFE_NODEPTH )
		gEngfuncs.GL_SetAttribute( REF_GL_DEPTH_SIZE, 24 );
	else
		gEngfuncs.GL_SetAttribute( REF_GL_DEPTH_SIZE, 8 );

	if( safegl < SAFE_NOCOLOR )
	{
		if( gpGlobals->desktopBitsPixel >= 24 )
		{
			gEngfuncs.GL_SetAttribute( REF_GL_RED_SIZE, 8 );
			gEngfuncs.GL_SetAttribute( REF_GL_GREEN_SIZE, 8 );
			gEngfuncs.GL_SetAttribute( REF_GL_BLUE_SIZE, 8 );
		}
		else if( gpGlobals->desktopBitsPixel >= 16 )
		{
			gEngfuncs.GL_SetAttribute( REF_GL_RED_SIZE, 5 );
			gEngfuncs.GL_SetAttribute( REF_GL_GREEN_SIZE, 6 );
			gEngfuncs.GL_SetAttribute( REF_GL_BLUE_SIZE, 5 );
		}
		else
		{
			gEngfuncs.GL_SetAttribute( REF_GL_RED_SIZE, 3 );
			gEngfuncs.GL_SetAttribute( REF_GL_GREEN_SIZE, 3 );
			gEngfuncs.GL_SetAttribute( REF_GL_BLUE_SIZE, 2 );
		}
	}

	if( safegl < SAFE_NOMSAA )
	{
		switch( (int)gEngfuncs.pfnGetCvarFloat( "gl_msaa_samples" ))
		{
		case 2:
		case 4:
		case 8:
		case 16:
			samples = gEngfuncs.pfnGetCvarFloat( "gl_msaa_samples" );
			break;
		default:
			samples = 0; // don't use, because invalid parameter is passed
		}

		if( samples )
		{
			gEngfuncs.GL_SetAttribute( REF_GL_MULTISAMPLEBUFFERS, 1 );
			gEngfuncs.GL_SetAttribute( REF_GL_MULTISAMPLESAMPLES, samples );

			glConfig.max_multisamples = samples;
		}
		else
		{
			gEngfuncs.GL_SetAttribute( REF_GL_MULTISAMPLEBUFFERS, 0 );
			gEngfuncs.GL_SetAttribute( REF_GL_MULTISAMPLESAMPLES, 0 );

			glConfig.max_multisamples = 0;
		}
	}
	else
	{
		gEngfuncs.Cvar_Set( "gl_msaa_samples", "0" );
	}
}

void wes_init( const char *gles2 );
int nanoGL_Init( void );
#ifdef XASH_GL4ES
void GL4ES_GetMainFBSize( int *width, int *height )
{
	*width = gpGlobals->width;
	*height = gpGlobals->height;
}
void *GL4ES_GetProcAddress( const char *name )
{
	if( !Q_strcmp(name, "glShadeModel") )
		// combined gles/gles2/gl implementation exports this, but it is invalid
		return NULL;
	return gEngfuncs.GL_GetProcAddress( name );
}
#endif

void GL_OnContextCreated( void )
{
	int colorBits[3];
#ifdef XASH_NANOGL
	nanoGL_Init();
#endif

	gEngfuncs.GL_GetAttribute( REF_GL_RED_SIZE, &colorBits[0] );
	gEngfuncs.GL_GetAttribute( REF_GL_GREEN_SIZE, &colorBits[1] );
	gEngfuncs.GL_GetAttribute( REF_GL_BLUE_SIZE, &colorBits[2] );
	glConfig.color_bits = colorBits[0] + colorBits[1] + colorBits[2];

	gEngfuncs.GL_GetAttribute( REF_GL_ALPHA_SIZE, &glConfig.alpha_bits );
	gEngfuncs.GL_GetAttribute( REF_GL_DEPTH_SIZE, &glConfig.depth_bits );
	gEngfuncs.GL_GetAttribute( REF_GL_STENCIL_SIZE, &glConfig.stencil_bits );
	glState.stencilEnabled = glConfig.stencil_bits ? true : false;

	gEngfuncs.GL_GetAttribute( REF_GL_MULTISAMPLESAMPLES, &glConfig.msaasamples );

#ifdef XASH_WES
	wes_init( "" );
#endif
#ifdef XASH_GL4ES
	set_getprocaddress( GL4ES_GetProcAddress );
	set_getmainfbsize( GL4ES_GetMainFBSize );
	initialize_gl4es();

	// merge glBegin/glEnd in beams and console
	pglHint( GL_BEGINEND_HINT_GL4ES, 1 );
	// dxt unpacked to 16-bit looks ugly
	pglHint( GL_AVOID16BITS_HINT_GL4ES, 1 );
#endif
}


#if XASH_RAYTRACING
#define EMPTY_LINKAGE extern
#define EMPTY_FUNCTION( name ) p##name
EMPTY_LINKAGE GLenum EMPTY_FUNCTION( glGetError )(void){ return 0; }
EMPTY_LINKAGE const GLubyte * EMPTY_FUNCTION( glGetString )(GLenum name){ return ""; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glAccum )(GLenum op, GLfloat value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glAlphaFunc )(GLenum func, GLclampf ref){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glArrayElement )(GLint i){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCallList )(GLuint list){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCallLists )(GLsizei n, GLenum type, const GLvoid *lists){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glClear )(GLbitfield mask){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glClearDepth )(GLclampd depth){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glClearIndex )(GLfloat c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glClearStencil )(GLint s){}
EMPTY_LINKAGE GLboolean EMPTY_FUNCTION( glIsEnabled )( GLenum cap ){ return 0; }
EMPTY_LINKAGE GLboolean EMPTY_FUNCTION( glIsList )( GLuint list ){ return 0; }
EMPTY_LINKAGE GLboolean EMPTY_FUNCTION( glIsTexture )( GLuint texture ){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glClipPlane )(GLenum plane, const GLdouble *equation){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3b )( GLbyte red, GLbyte green, GLbyte blue ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3bv )( const GLbyte* v ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3i )( GLint red, GLint green, GLint blue ) { assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3iv )( const GLint* v ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3s )( GLshort red, GLshort green, GLshort blue ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3sv )( const GLshort* v ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3ui )( GLuint red, GLuint green, GLuint blue ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3uiv )( const GLuint* v ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3us )( GLushort red, GLushort green, GLushort blue ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor3usv )( const GLushort* v ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4b )( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4bv )( const GLbyte* v ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4i )( GLint red, GLint green, GLint blue, GLint alpha ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4iv )( const GLint* v ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4s )( GLshort red, GLshort green, GLshort blue, GLshort alpha ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4sv )( const GLshort* v ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4ui )( GLuint red, GLuint green, GLuint blue, GLuint alpha ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4uiv )( const GLuint* v ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4us )( GLushort red, GLushort green, GLushort blue, GLushort alpha ){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColor4usv )(const GLushort *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColorMaterial )(GLenum face, GLenum mode){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCullFace )(GLenum mode){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDeleteLists )(GLuint list, GLsizei range){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDeleteTextures )(GLsizei n, const GLuint *textures){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDepthFunc )(GLenum func){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDepthMask )(GLboolean flag){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDisableClientState )(GLenum array){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDrawArrays )(GLenum mode, GLint first, GLsizei count){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDrawBuffer )(GLenum mode){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEdgeFlag )(GLboolean flag){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEdgeFlagv )(const GLboolean *flag){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEnableClientState )(GLenum array){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEndList )(void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalCoord1d )(GLdouble u){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalCoord1dv )(const GLdouble *u){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalCoord1f )(GLfloat u){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalCoord1fv )(const GLfloat *u){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalCoord2d )(GLdouble u, GLdouble v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalCoord2dv )(const GLdouble *u){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalCoord2f )(GLfloat u, GLfloat v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalCoord2fv )(const GLfloat *u){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalMesh1 )(GLenum mode, GLint i1, GLint i2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalPoint1 )(GLint i){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEvalPoint2 )(GLint i, GLint j){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFinish )(void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFlush )(void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFogf )(GLenum pname, GLfloat param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFogfv )(GLenum pname, const GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFogi )(GLenum pname, GLint param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFogiv )(GLenum pname, const GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFrontFace )(GLenum mode){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGenTextures )(GLsizei n, GLuint *textures){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetBooleanv )(GLenum pname, GLboolean *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetClipPlane )(GLenum plane, GLdouble *equation){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetDoublev )(GLenum pname, GLdouble *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetFloatv )(GLenum pname, GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetIntegerv )(GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetLightfv )(GLenum light, GLenum pname, GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetLightiv )(GLenum light, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetMapdv )(GLenum target, GLenum query, GLdouble *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetMapfv )(GLenum target, GLenum query, GLfloat *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetMapiv )(GLenum target, GLenum query, GLint *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetMaterialiv )(GLenum face, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetPixelMapfv )(GLenum map, GLfloat *values){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetPixelMapuiv )(GLenum map, GLuint *values){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetPixelMapusv )(GLenum map, GLushort *values){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetPointerv )(GLenum pname, GLvoid* *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetPolygonStipple )(GLubyte *mask){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexEnviv )(GLenum target, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexGeniv )(GLenum coord, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetTexParameteriv )(GLenum target, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glHint )(GLenum target, GLenum mode){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexMask )(GLuint mask){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexd )(GLdouble c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexdv )(const GLdouble *c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexf )(GLfloat c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexfv )(const GLfloat *c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexi )(GLint c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexiv )(const GLint *c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexs )(GLshort c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexsv )(const GLshort *c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexub )(GLubyte c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glIndexubv )(const GLubyte *c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glInitNames )(void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLightModelf )(GLenum pname, GLfloat param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLightModelfv )(GLenum pname, const GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLightModeli )(GLenum pname, GLint param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLightModeliv )(GLenum pname, const GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLightf )(GLenum light, GLenum pname, GLfloat param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLightfv )(GLenum light, GLenum pname, const GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLighti )(GLenum light, GLenum pname, GLint param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLightiv )(GLenum light, GLenum pname, const GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLineStipple )(GLint factor, GLushort pattern){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLineWidth )(GLfloat width){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glListBase )(GLuint base){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLoadName )(GLuint name){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLogicOp )(GLenum opcode){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMapGrid1d )(GLint un, GLdouble u1, GLdouble u2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMapGrid1f )(GLint un, GLfloat u1, GLfloat u2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMaterialf )(GLenum face, GLenum pname, GLfloat param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMaterialfv )(GLenum face, GLenum pname, const GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMateriali )(GLenum face, GLenum pname, GLint param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMaterialiv )(GLenum face, GLenum pname, const GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMultMatrixd )(const GLdouble *m){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMultMatrixf )(const GLfloat *m){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNewList )(GLuint list, GLenum mode){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3bv )(const GLbyte *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3dv )(const GLdouble *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3fv )(const GLfloat *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3i )(GLint nx, GLint ny, GLint nz){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3iv )(const GLint *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3s )(GLshort nx, GLshort ny, GLshort nz){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormal3sv )(const GLshort *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPassThrough )(GLfloat token){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat *values){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint *values){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort *values){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPixelStoref )(GLenum pname, GLfloat param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPixelStorei )(GLenum pname, GLint param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPixelTransferf )(GLenum pname, GLfloat param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPixelTransferi )(GLenum pname, GLint param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPixelZoom )(GLfloat xfactor, GLfloat yfactor){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPointSize )(GLfloat size){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPolygonMode )(GLenum face, GLenum mode){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPolygonOffset )(GLfloat factor, GLfloat units){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPolygonStipple )(const GLubyte *mask){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPopAttrib )(void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPopClientAttrib )(void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPopMatrix )(void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPopName )(void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPushAttrib )(GLbitfield mask){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPushClientAttrib )(GLbitfield mask){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPushMatrix )(void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPushName )(GLuint name){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos2d )(GLdouble x, GLdouble y){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos2dv )(const GLdouble *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos2f )(GLfloat x, GLfloat y){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos2fv )(const GLfloat *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos2i )(GLint x, GLint y){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos2iv )(const GLint *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos2s )(GLshort x, GLshort y){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos2sv )(const GLshort *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos3d )(GLdouble x, GLdouble y, GLdouble z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos3dv )(const GLdouble *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos3f )(GLfloat x, GLfloat y, GLfloat z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos3fv )(const GLfloat *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos3i )(GLint x, GLint y, GLint z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos3iv )(const GLint *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos3s )(GLshort x, GLshort y, GLshort z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos3sv )(const GLshort *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos4dv )(const GLdouble *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos4fv )(const GLfloat *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos4i )(GLint x, GLint y, GLint z, GLint w){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos4iv )(const GLint *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRasterPos4sv )(const GLshort *v){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glReadBuffer )(GLenum mode){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRectdv )(const GLdouble *v1, const GLdouble *v2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRectfv )(const GLfloat *v1, const GLfloat *v2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRecti )(GLint x1, GLint y1, GLint x2, GLint y2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRectiv )(const GLint *v1, const GLint *v2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRectsv )(const GLshort *v1, const GLshort *v2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glScaled )(GLdouble x, GLdouble y, GLdouble z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glScalef )(GLfloat x, GLfloat y, GLfloat z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glScissor )(GLint x, GLint y, GLsizei width, GLsizei height){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glSelectBuffer )(GLsizei size, GLuint *buffer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glShadeModel )(GLenum mode){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glStencilFunc )(GLenum func, GLint ref, GLuint mask){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glStencilMask )(GLuint mask){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glStencilOp )(GLenum fail, GLenum zfail, GLenum zpass){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord1d )(GLdouble s){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord1dv )(const GLdouble *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord1f )(GLfloat s){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord1fv )(const GLfloat *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord1i )(GLint s){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord1iv )(const GLint *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord1s )(GLshort s){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord1sv )(const GLshort *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord2d )(GLdouble s, GLdouble t){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord2dv )(const GLdouble *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord2i )(GLint s, GLint t){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord2iv )(const GLint *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord2s )(GLshort s, GLshort t){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord2sv )(const GLshort *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord3d )(GLdouble s, GLdouble t, GLdouble r){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord3dv )(const GLdouble *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord3f )(GLfloat s, GLfloat t, GLfloat r){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord3fv )(const GLfloat *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord3i )(GLint s, GLint t, GLint r){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord3iv )(const GLint *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord3s )(GLshort s, GLshort t, GLshort r){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord3sv )(const GLshort *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord4dv )(const GLdouble *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord4fv )(const GLfloat *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord4i )(GLint s, GLint t, GLint r, GLint q){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord4iv )(const GLint *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoord4sv )(const GLshort *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexEnvf )(GLenum target, GLenum pname, GLfloat param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexEnvi )(GLenum target, GLenum pname, GLint param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexEnviv )(GLenum target, GLenum pname, const GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexGend )(GLenum coord, GLenum pname, GLdouble param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexGendv )(GLenum coord, GLenum pname, const GLdouble *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexGenf )(GLenum coord, GLenum pname, GLfloat param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexGeni )(GLenum coord, GLenum pname, GLint param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexGeniv )(GLenum coord, GLenum pname, const GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexParameterf )(GLenum target, GLenum pname, GLfloat param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexParameteri )(GLenum target, GLenum pname, GLint param){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexParameteriv )(GLenum target, GLenum pname, const GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTranslated )(GLdouble x, GLdouble y, GLdouble z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTranslatef )(GLfloat x, GLfloat y, GLfloat z){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex2d )(GLdouble x, GLdouble y){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex2dv )(const GLdouble *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex2fv )(const GLfloat *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex2i )(GLint x, GLint y){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex2iv )(const GLint *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex2s )(GLshort x, GLshort y){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex2sv )(const GLshort *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex3d )(GLdouble x, GLdouble y, GLdouble z){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex3dv )(const GLdouble *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex3i )(GLint x, GLint y, GLint z){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex3iv )(const GLint *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex3s )(GLshort x, GLshort y, GLshort z){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex3sv )(const GLshort *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex4dv )(const GLdouble *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex4fv )(const GLfloat *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex4i )(GLint x, GLint y, GLint z, GLint w){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex4iv )(const GLint *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertex4sv )(const GLshort *v){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){ assert( 0 ); }
EMPTY_LINKAGE void EMPTY_FUNCTION( glPointParameterfEXT )( GLenum param, GLfloat value ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glPointParameterfvEXT )( GLenum param, const GLfloat *value ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLockArraysEXT ) (int a, int b){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUnlockArraysEXT ) (void){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glActiveTextureARB )( GLenum e ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glClientActiveTextureARB )( GLenum e ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetCompressedTexImage )( GLenum target, GLint lod, const GLvoid* data ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDrawRangeElements )( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDrawRangeElementsEXT )( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMultiTexCoord1f) (GLenum e, GLfloat a){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMultiTexCoord2f) (GLenum e, GLfloat a, GLfloat b){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMultiTexCoord3f) (GLenum e, GLfloat a, GLfloat b, GLfloat c){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glMultiTexCoord4f) (GLenum e, GLfloat a, GLfloat b, GLfloat c, GLfloat d){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glActiveTexture) (GLenum e){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glClientActiveTexture) (GLenum e){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCompressedTexImage3DARB )(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCompressedTexImage2DARB )(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border,  GLsizei imageSize, const void *data){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCompressedTexImage1DARB )(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCompressedTexSubImage3DARB )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCompressedTexSubImage2DARB )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCompressedTexSubImage1DARB )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDeleteObjectARB )(GLhandleARB obj){}
EMPTY_LINKAGE GLhandleARB EMPTY_FUNCTION( glGetHandleARB )(GLenum pname){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glDetachObjectARB )(GLhandleARB containerObj, GLhandleARB attachedObj){}
EMPTY_LINKAGE GLhandleARB EMPTY_FUNCTION( glCreateShaderObjectARB )(GLenum shaderType){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glShaderSourceARB )(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCompileShaderARB )(GLhandleARB shaderObj){}
EMPTY_LINKAGE GLhandleARB EMPTY_FUNCTION( glCreateProgramObjectARB )(void){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glAttachObjectARB )(GLhandleARB containerObj, GLhandleARB obj){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glLinkProgramARB )(GLhandleARB programObj){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUseProgramObjectARB )(GLhandleARB programObj){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glValidateProgramARB )(GLhandleARB programObj){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glBindProgramARB )(GLenum target, GLuint program){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDeleteProgramsARB )(GLsizei n, const GLuint *programs){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGenProgramsARB )(GLsizei n, GLuint *programs){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glProgramStringARB )(GLenum target, GLenum format, GLsizei len, const GLvoid *string){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glProgramEnvParameter4fARB )(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glProgramLocalParameter4fARB )(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform1fARB )(GLint location, GLfloat v0){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform2fARB )(GLint location, GLfloat v0, GLfloat v1){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform3fARB )(GLint location, GLfloat v0, GLfloat v1, GLfloat v2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform4fARB )(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform1iARB )(GLint location, GLint v0){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform2iARB )(GLint location, GLint v0, GLint v1){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform3iARB )(GLint location, GLint v0, GLint v1, GLint v2){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform4iARB )(GLint location, GLint v0, GLint v1, GLint v2, GLint v3){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform1fvARB )(GLint location, GLsizei count, const GLfloat *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform2fvARB )(GLint location, GLsizei count, const GLfloat *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform3fvARB )(GLint location, GLsizei count, const GLfloat *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform4fvARB )(GLint location, GLsizei count, const GLfloat *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform1ivARB )(GLint location, GLsizei count, const GLint *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform2ivARB )(GLint location, GLsizei count, const GLint *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform3ivARB )(GLint location, GLsizei count, const GLint *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniform4ivARB )(GLint location, GLsizei count, const GLint *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniformMatrix2fvARB )(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniformMatrix3fvARB )(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glUniformMatrix4fvARB )(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetObjectParameterfvARB )(GLhandleARB obj, GLenum pname, GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetObjectParameterivARB )(GLhandleARB obj, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetInfoLogARB )(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetAttachedObjectsARB )(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj){}
EMPTY_LINKAGE GLint EMPTY_FUNCTION( glGetUniformLocationARB )(GLhandleARB programObj, const GLcharARB *name){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetActiveUniformARB )(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetUniformfvARB )(GLhandleARB programObj, GLint location, GLfloat *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetUniformivARB )(GLhandleARB programObj, GLint location, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetShaderSourceARB )(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexImage3D )( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexSubImage3D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glCopyTexSubImage3D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glBlendEquationEXT )(GLenum e){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glStencilOpSeparate )(GLenum a, GLenum b, GLenum c, GLenum d){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glStencilFuncSeparate )(GLenum a, GLenum b, GLint c, GLuint d){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glActiveStencilFaceEXT )(GLenum e){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertexAttribPointerARB )(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEnableVertexAttribArrayARB )(GLuint index){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDisableVertexAttribArrayARB )(GLuint index){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glBindAttribLocationARB )(GLhandleARB programObj, GLuint index, const GLcharARB *name){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetActiveAttribARB )(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name){}
EMPTY_LINKAGE GLint EMPTY_FUNCTION( glGetAttribLocationARB )(GLhandleARB programObj, const GLcharARB *name){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glBindFragDataLocation )(GLuint programObj, GLuint index, const GLcharARB *name){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertexAttrib2fARB )( GLuint index, GLfloat x, GLfloat y ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertexAttrib2fvARB )( GLuint index, const GLfloat *v ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glVertexAttrib3fvARB )( GLuint index, const GLfloat *v ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glBindBufferARB )(GLenum target, GLuint buffer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDeleteBuffersARB )(GLsizei n, const GLuint *buffers){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGenBuffersARB )(GLsizei n, GLuint *buffers){}
EMPTY_LINKAGE GLboolean EMPTY_FUNCTION( glIsBufferARB )(GLuint buffer){ return 0; }
EMPTY_LINKAGE GLvoid* EMPTY_FUNCTION( glMapBufferARB )(GLenum target, GLenum access){ return NULL; }
EMPTY_LINKAGE GLboolean EMPTY_FUNCTION( glUnmapBufferARB )(GLenum target){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glBufferDataARB )(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glBufferSubDataARB )(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGenQueriesARB )(GLsizei n, GLuint *ids){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDeleteQueriesARB )(GLsizei n, const GLuint *ids){}
EMPTY_LINKAGE GLboolean EMPTY_FUNCTION( glIsQueryARB )(GLuint id){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glBeginQueryARB )(GLenum target, GLuint id){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glEndQueryARB )(GLenum target){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetQueryivARB )(GLenum target, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetQueryObjectivARB )(GLuint id, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetQueryObjectuivARB )(GLuint id, GLenum pname, GLuint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDebugMessageControlARB )( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDebugMessageInsertARB )( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* buf ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDebugMessageCallbackARB )( GL_DEBUG_PROC_ARB callback, void* userParam ){}
EMPTY_LINKAGE GLuint EMPTY_FUNCTION( glGetDebugMessageLogARB )( GLuint count, GLsizei bufsize, GLenum* sources, GLenum* types, GLuint* ids, GLuint* severities, GLsizei* lengths, char* messageLog ){ return 0; }
EMPTY_LINKAGE GLboolean EMPTY_FUNCTION( glIsRenderbuffer )(GLuint renderbuffer){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glBindRenderbuffer )(GLenum target, GLuint renderbuffer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDeleteRenderbuffers )(GLsizei n, const GLuint *renderbuffers){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGenRenderbuffers )(GLsizei n, GLuint *renderbuffers){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRenderbufferStorage )(GLenum target, GLenum internalformat, GLsizei width, GLsizei height){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glRenderbufferStorageMultisample )(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetRenderbufferParameteriv )(GLenum target, GLenum pname, GLint *params){}
EMPTY_LINKAGE GLboolean EMPTY_FUNCTION( glIsFramebuffer )(GLuint framebuffer){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glBindFramebuffer )(GLenum target, GLuint framebuffer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDeleteFramebuffers )(GLsizei n, const GLuint *framebuffers){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGenFramebuffers )(GLsizei n, GLuint *framebuffers){}
EMPTY_LINKAGE GLenum EMPTY_FUNCTION( glCheckFramebufferStatus )(GLenum target){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glFramebufferTexture1D )(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFramebufferTexture2D )(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFramebufferTexture3D )(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFramebufferTextureLayer )(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glFramebufferRenderbuffer )(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGetFramebufferAttachmentParameteriv )(GLenum target, GLenum attachment, GLenum pname, GLint *params){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glBlitFramebuffer )(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDrawBuffersARB )( GLsizei n, const GLenum *bufs ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGenerateMipmap )( GLenum target ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glBindVertexArray )( GLuint array ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glDeleteVertexArrays )( GLsizei n, const GLuint *arrays ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glGenVertexArrays )( GLsizei n, const GLuint *arrays ){}
EMPTY_LINKAGE GLboolean EMPTY_FUNCTION( glIsVertexArray )( GLuint array ){ return 0; }
EMPTY_LINKAGE void EMPTY_FUNCTION( glSwapInterval ) ( int interval ){}
EMPTY_LINKAGE void EMPTY_FUNCTION( glTexImage2DMultisample )( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations ){}

#undef EMPTY_LINKAGE
#undef EMPTY_FUNCTION


static void TryBatch( qboolean glbegin, RgUtilImScratchTopology glbegin_topology );


void pglBegin( GLenum mode )
{
    RgUtilImScratchTopology topology = RG_UTIL_IM_SCRATCH_TOPOLOGY_TRIANGLES;
    switch( mode )
    {
        case GL_TRIANGLES: topology = RG_UTIL_IM_SCRATCH_TOPOLOGY_TRIANGLES; break;
        case GL_TRIANGLE_STRIP: topology = RG_UTIL_IM_SCRATCH_TOPOLOGY_TRIANGLE_STRIP; break;
        case GL_TRIANGLE_FAN: topology = RG_UTIL_IM_SCRATCH_TOPOLOGY_TRIANGLE_FAN; break;
        case GL_QUADS: topology = RG_UTIL_IM_SCRATCH_TOPOLOGY_QUADS; break;
        case GL_POLYGON: topology = RG_UTIL_IM_SCRATCH_TOPOLOGY_TRIANGLE_FAN; break;
        default: assert( 0 ); return;
    }
	
    TryBatch( true, topology );
}

void pglTexCoord2f( GLfloat s, GLfloat t )
{
    rgUtilImScratchTexCoord( rg_instance, s, t );
}
void pglTexCoord2fv( const GLfloat* v )
{
    rgUtilImScratchTexCoord( rg_instance, v[ 0 ], v[ 1 ] );
}

void pglVertex3f( GLfloat x, GLfloat y, GLfloat z )
{
    rgUtilImScratchVertex( rg_instance, x, y, z );
}
void pglVertex3fv( const GLfloat* v )
{
    rgUtilImScratchVertex( rg_instance, v[ 0 ], v[ 1 ], v[ 2 ] );
}
void pglVertex2f ( GLfloat x, GLfloat y )
{
    rgUtilImScratchVertex( rg_instance, x, y, 0.0f );
}

void pglColor3d( GLdouble red, GLdouble green, GLdouble blue )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorFloat4D( ( float )red, ( float )green, ( float )blue, 1.0f ) );
}
void pglColor3dv( const GLdouble* v )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorFloat4D( ( float )v[ 0 ], ( float )v[ 1 ], ( float )v[ 2 ], 1.0f ) );
}
void pglColor3f( GLfloat red, GLfloat green, GLfloat blue )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorFloat4D( red, green, blue, 1.0f ) );
}
void pglColor3fv( const GLfloat* v )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorFloat4D( v[ 0 ], v[ 1 ], v[ 2 ], 1.0f ) );
}
void pglColor3ub( GLubyte red, GLubyte green, GLubyte blue )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorByte4D( red, green, blue, 255 ) );
}
void pglColor3ubv( const GLubyte* v )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorByte4D( v[ 0 ], v[ 1 ], v[ 2 ], 255 ) );
}
void pglColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorFloat4D( ( float )red, ( float )green, ( float )blue, ( float )alpha ) );
}
void pglColor4dv( const GLdouble* v )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorFloat4D( ( float )v[ 0 ], ( float )v[ 1 ], ( float )v[ 2 ], ( float )v[ 3 ] ) );
}
void pglColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorFloat4D( red, green, blue, alpha ) );
}
void pglColor4fv( const GLfloat* v )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorFloat4D( v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ] ) );
}
void pglColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorByte4D( red, green, blue, alpha ) );
}
void pglColor4ubv( const GLubyte* v )
{
    rgUtilImScratchColor( rg_instance, rgUtilPackColorByte4D( v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ] ) );
}




RgInstance rg_instance = NULL;
rt_state_t rt_state = { 0 };

void pglBindTexture( GLenum target, GLuint texture, const char* textureName )
{
    if( target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE_EXT )
    {
        if( textureName && textureName[ 0 ] != '\0' )
        {
            rt_state.curTexture2DName = textureName;
        }
    }
}

void pglTexImage2D( GLenum        target,
                    GLint         level,
                    GLint         internalformat,
                    GLsizei       width,
                    GLsizei       height,
                    GLint         border,
                    GLenum        format,
                    GLenum        type,
                    const GLvoid* pixels )
{
    if( target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE_EXT )
    {
        if( rt_state.curTexture2DName )
        {
            if( level == 0 && format == GL_RGBA && type == GL_UNSIGNED_BYTE && pixels )
            {
                RgOriginalTextureInfo info = {
                    .pTextureName = rt_state.curTexture2DName,
                    .pPixels      = pixels,
                    .size         = { width, height },
                    .filter       = rt_state.curTextureNearest ? RG_SAMPLER_FILTER_NEAREST
                                                               : RG_SAMPLER_FILTER_AUTO,
                    .addressModeU = rt_state.curTextureClamped ? RG_SAMPLER_ADDRESS_MODE_CLAMP
                                                               : RG_SAMPLER_ADDRESS_MODE_REPEAT,
                    .addressModeV = rt_state.curTextureClamped ? RG_SAMPLER_ADDRESS_MODE_CLAMP
                                                               : RG_SAMPLER_ADDRESS_MODE_REPEAT,
                };

                RgResult r = rgProvideOriginalTexture( rg_instance, &info );
                RG_CHECK( r );
            }
        }
    }
}

static qboolean rt_raster_additive = false;
static qboolean rt_raster_blend    = false;
static qboolean rt_alphatest       = false;

void pglEnable( GLenum cap )
{
    switch( cap )
    {
        case GL_BLEND: rt_raster_blend = true; break;
        case GL_ALPHA_TEST: rt_alphatest = true; break;
        default: break;
    }
}
void pglDisable( GLenum cap )
{
    switch( cap )
    {
        case GL_BLEND: rt_raster_blend = false; break;
        case GL_ALPHA_TEST: rt_alphatest = false; break;
        default: break;
    }
}

void pglBlendFunc( GLenum sfactor, GLenum dfactor )
{
    rt_raster_additive = ( sfactor == GL_ONE || dfactor == GL_ONE );
}


static qboolean AreFloatsClose( const float a, const float b )
{
    const float eps = 0.001f;
    return fabsf( a - b ) < eps;
}

static qboolean AreTransformsClose( const RgTransform* a, const RgTransform* b )
{
    for( int i = 0; i < 3; i++ )
    {
        for( int j = 0; j < 4; j++ )
        {
            if( !AreFloatsClose( a->matrix[ i ][ j ], b->matrix[ i ][ j ] ) )
            {
                return false;
            }
        }
    }

    return true;
}

#define MATRIX4_TO_RGTRANSFORM( m )                                             \
    {{                                                                          \
        { ( m )[ 0 ][ 0 ], ( m )[ 0 ][ 1 ], ( m )[ 0 ][ 2 ], ( m )[ 0 ][ 3 ] }, \
        { ( m )[ 1 ][ 0 ], ( m )[ 1 ][ 1 ], ( m )[ 1 ][ 2 ], ( m )[ 1 ][ 3 ] }, \
        { ( m )[ 2 ][ 0 ], ( m )[ 2 ][ 1 ], ( m )[ 2 ][ 2 ], ( m )[ 2 ][ 3 ] }, \
    }}

static qboolean ArePrimitivesSame( const RgMeshInfo*          a_mesh,
                                   const RgMeshPrimitiveInfo* a_primitive,
                                   const RgMeshInfo*          b_mesh,
                                   const RgMeshPrimitiveInfo* b_primitive )
{
    if( a_mesh->uniqueObjectID == b_mesh->uniqueObjectID && a_mesh->pMeshName == b_mesh->pMeshName )
    {
        if( a_primitive->flags == b_primitive->flags && a_primitive->color == b_primitive->color &&
            a_primitive->pTextureName == b_primitive->pTextureName &&
            AreFloatsClose( a_primitive->emissive, b_primitive->emissive ) )
        {
            if( RI.currentmodel == WORLDMODEL ||
                AreTransformsClose( &a_primitive->transform, &b_primitive->transform ) )
            {
                return true;
            }
        }
    }

    return false;
}

static int Q_clamp_wassert( int x, int xmin, int xmax )
{
    if( x < xmin )
    {
        assert( 0 );
        return xmin;
    }
    if( x > xmax )
    {
        assert( 0 );
        return xmax;
    }
    return x;
}

static uint32_t hashStudioPrimitive( int bodypart, int submodel, int mesh, int weaponmodel, int glendIndex )
{
	// must be compact
    const uint32_t BODYPART_BITS = 5;
    const uint32_t SUBMODEL_BITS = 5;
    const uint32_t MESH_BITS = 8;
    const uint32_t WEAPON_BITS = 1;
    const uint32_t GLEND_BITS    = 32 - BODYPART_BITS - SUBMODEL_BITS - MESH_BITS - WEAPON_BITS;
    assert( BODYPART_BITS + SUBMODEL_BITS + MESH_BITS + WEAPON_BITS + GLEND_BITS == 32 );

	// must be same as engine limitations
    assert( ( 1 << BODYPART_BITS ) == MAXSTUDIOBODYPARTS ); // body parts per submodel
    assert( ( 1 << SUBMODEL_BITS ) == MAXSTUDIOMODELS );    // sub-models per model
    assert( ( 1 << MESH_BITS ) == MAXSTUDIOMESHES );        // max textures per model

	// must be within bounds
    bodypart    = Q_clamp_wassert( bodypart, 0, ( 1 << BODYPART_BITS ) - 1 );
    submodel    = Q_clamp_wassert( submodel, 0, ( 1 << SUBMODEL_BITS ) - 1 );
    mesh        = Q_clamp_wassert( mesh, 0, ( 1 << MESH_BITS ) - 1 );
    weaponmodel = Q_clamp_wassert( !!weaponmodel, 0, ( 1 << WEAPON_BITS ) - 1 );
    glendIndex  = Q_clamp_wassert( glendIndex, 0, ( 1 << GLEND_BITS ) - 1 );

	// combine
    return ( uint32_t )glendIndex << ( BODYPART_BITS + SUBMODEL_BITS + MESH_BITS + WEAPON_BITS ) |
           ( uint32_t )weaponmodel << ( BODYPART_BITS + SUBMODEL_BITS + MESH_BITS ) |
           ( uint32_t )mesh << ( BODYPART_BITS + SUBMODEL_BITS ) |
           ( uint32_t )submodel << ( BODYPART_BITS ) |
		   ( uint32_t )bodypart;
}

static struct
{
    qboolean            valid;
    RgMeshInfo          mesh;
    RgMeshPrimitiveInfo primitive;
} rt_batch = { 0 };

static void FlushResidue()
{
    if( rt_batch.valid )
    {
        rgUtilImScratchSetToPrimitive( rg_instance, &rt_batch.primitive );

        RgResult r = rgUploadMeshPrimitive( rg_instance, &rt_batch.mesh, &rt_batch.primitive );
        RG_CHECK( r );

        rgUtilImScratchClear( rg_instance );
    }
    rt_batch.valid = false;
}

static void TryBatch( qboolean glbegin, RgUtilImScratchTopology glbegin_topology )
{
    if( glState.in2DMode )
    {
        if( glbegin )
        {
            FlushResidue();

            rgUtilImScratchClear( rg_instance );
            rgUtilImScratchStart( rg_instance, glbegin_topology );
        }
        else
        {
            RgMeshPrimitiveInfo info = {
                .primitiveNameInMesh  = NULL,
                .primitiveIndexInMesh = 0,
                .flags                = ( rt_raster_blend ? RG_MESH_PRIMITIVE_TRANSLUCENT : 0 ) |
                         ( rt_alphatest ? RG_MESH_PRIMITIVE_ALPHA_TESTED : 0 ),
                .transform    = RG_TRANSFORM_IDENTITY,
                .pTextureName = rt_state.curTexture2DName,
                .textureFrame = 0,
                .color        = rgUtilPackColorByte4D( 255, 255, 255, 255 ),
                .emissive     = rt_raster_blend && rt_raster_additive ? 1.0f : 0.0f,
                .pEditorInfo  = NULL,
            };
            rgUtilImScratchEnd( rg_instance );
            rgUtilImScratchSetToPrimitive( rg_instance, &info );

            RgResult r = rgUploadNonWorldPrimitive(
                rg_instance, &info, rt_state.projMatrixFor2D, &rt_state.viewport );
            RG_CHECK( r );
        }

        return;
    }

	if( rt_state.curIsRasterized || rt_state.curIsSky )
    {
        if( glbegin )
        {
            FlushResidue();

            rgUtilImScratchClear( rg_instance );
            rgUtilImScratchStart( rg_instance, glbegin_topology );
        }
        else
	    {
            RgMeshInfo mesh = {
                .uniqueObjectID = UINT32_MAX,
                .pMeshName      = NULL,
                .isStatic       = false,
                .animationName  = NULL,
                .animationTime  = 0.0f,
            };

            RgMeshPrimitiveInfo info = {
                .primitiveNameInMesh  = NULL,
                .primitiveIndexInMesh = 0,
                .flags                = ( rt_raster_blend ? RG_MESH_PRIMITIVE_TRANSLUCENT : 0 ) |
                         ( rt_alphatest ? RG_MESH_PRIMITIVE_ALPHA_TESTED : 0 ) |
                         ( rt_state.curIsSky ? RG_MESH_PRIMITIVE_SKY : 0 ),
                .transform    = RG_TRANSFORM_IDENTITY,
                .pTextureName = rt_state.curTexture2DName,
                .textureFrame = 0,
                .color        = rgUtilPackColorByte4D( 255, 255, 255, 255 ),
                .emissive     = rt_raster_blend && rt_raster_additive ? 1.0f : 0.0f,
                .pEditorInfo  = NULL,
            };
            rgUtilImScratchEnd( rg_instance );
            rgUtilImScratchSetToPrimitive( rg_instance, &info );

            RgResult r = rgUploadMeshPrimitive( rg_instance, &mesh, &info );
            RG_CHECK( r );
        }

		return;
    }

    if( !RI.currententity || RI.currententity->index < 0 )
    {
        if( glbegin )
            FlushResidue();
        return;
    }

    if( !RI.currentmodel )
    {
        if( glbegin )
            FlushResidue();
        return;
    }

    qboolean isstudiomodel = rt_state.curStudioBodyPart >= 0 && rt_state.curStudioSubmodel >= 0 &&
                             rt_state.curStudioMesh >= 0 && rt_state.curStudioGlend >= 0;

    qboolean isbrush = rt_state.curBrushSurface >= 0;

    if( isstudiomodel )
    {
        if( glbegin )
        {
            FlushResidue();

            rgUtilImScratchClear( rg_instance );
            rgUtilImScratchStart( rg_instance, glbegin_topology );
        }
        else
        {
            qboolean isviewmodel    = ( RI.currententity == gEngfuncs.GetViewModel() );
            qboolean isplayerviewer = ( RI.currententity == gEngfuncs.GetLocalPlayer() &&
                                        !ENGINE_GET_PARM( PARM_THIRDPERSON ) );

            RgMeshInfo mesh = {
                .uniqueObjectID = RI.currententity->index,
                .pMeshName      = RI.currentmodel->name,
                .isStatic       = false,
                .animationName  = NULL,
                .animationTime  = 0.0f,
            };

            if( rt_state.curTempEntityIndex > 0 )
            {
                mesh.uniqueObjectID = rt_state.curTempEntityIndex;
            }

			assert( mesh.uniqueObjectID != 0 );

            if( RI.currententity->player )
            {
                assert( ( mesh.uniqueObjectID & ( 1u << 31u ) ) == 0 );
                mesh.uniqueObjectID |= 1u << 31u;
            }

            RgMeshPrimitiveInfo info = {
                .primitiveNameInMesh  = NULL,
                .primitiveIndexInMesh = hashStudioPrimitive( rt_state.curStudioBodyPart,
                                                             rt_state.curStudioSubmodel,
                                                             rt_state.curStudioMesh,
                                                             rt_state.curStudioWeaponModel,
                                                             rt_state.curStudioGlend ),
                .flags                = ( rt_alphatest ? RG_MESH_PRIMITIVE_ALPHA_TESTED : 0 ) |
                         ( isviewmodel
                               ? RG_MESH_PRIMITIVE_FIRST_PERSON
                               : ( isplayerviewer ? RG_MESH_PRIMITIVE_FIRST_PERSON_VIEWER : 0 ) ),
                .transform    = RG_TRANSFORM_IDENTITY,
                .pTextureName = rt_state.curTexture2DName,
                .textureFrame = 0,
                .color        = rgUtilPackColorByte4D( 255, 255, 255, 255 ),
                .emissive     = 0.0f,
                .pEditorInfo  = NULL,
            };
            rgUtilImScratchEnd( rg_instance );
            rgUtilImScratchSetToPrimitive( rg_instance, &info );

            RgResult r = rgUploadMeshPrimitive( rg_instance, &mesh, &info );
            RG_CHECK( r );
        }

		return;
    }

	if( isbrush )
    {
        Assert( rt_state.curTempEntityIndex == 0 );

        RgMeshInfo mesh = {
            .uniqueObjectID = RI.currententity->index,
            .pMeshName      = RI.currentmodel->name,
            .isStatic       = false,
            .animationName  = NULL,
            .animationTime  = 0.0f,
        };

        RgMeshPrimitiveInfo info = {
            .primitiveNameInMesh  = NULL,
            .primitiveIndexInMesh = rt_state.curBrushSurface,
            .flags                = rt_alphatest ? RG_MESH_PRIMITIVE_ALPHA_TESTED : 0,
            .transform            = MATRIX4_TO_RGTRANSFORM( RI.objectMatrix ),
            .pTextureName         = rt_state.curTexture2DName,
            .textureFrame         = 0,
            .color                = rgUtilPackColorByte4D( 255, 255, 255, 255 ),
            .emissive             = 0.0f,
            .pEditorInfo          = NULL,
        };

        if( glbegin )
        {
            if( rt_batch.valid )
            {
                if( ArePrimitivesSame( &mesh, &info, &rt_batch.mesh, &rt_batch.primitive ) )
                {
                    rgUtilImScratchStart( rg_instance, glbegin_topology );
                }
                else
                {
                    rgUtilImScratchSetToPrimitive( rg_instance, &rt_batch.primitive );

                    RgResult r =
                        rgUploadMeshPrimitive( rg_instance, &rt_batch.mesh, &rt_batch.primitive );
                    RG_CHECK( r );

                    // start new
                    rgUtilImScratchClear( rg_instance );
                    rt_batch.valid     = true;
                    rt_batch.mesh      = mesh;
                    rt_batch.primitive = info;
                    rgUtilImScratchStart( rg_instance, glbegin_topology );
                }
            }
            else
            {
                // start new
                rgUtilImScratchClear( rg_instance );
                rt_batch.valid     = true;
                rt_batch.mesh      = mesh;
                rt_batch.primitive = info;
                rgUtilImScratchStart( rg_instance, glbegin_topology );
            }
        }
        else
        {
            rgUtilImScratchEnd( rg_instance );
        }

        return;
    }
}

void pglEnd( void )
{
    TryBatch( false, 0 );
}

void RT_OnBeforeDrawFrame()
{
    if( rt_batch.valid )
    {
        rgUtilImScratchSetToPrimitive( rg_instance, &rt_batch.primitive );

        RgResult r = rgUploadMeshPrimitive( rg_instance, &rt_batch.mesh, &rt_batch.primitive );
        RG_CHECK( r );
		
        rgUtilImScratchClear( rg_instance );
    }
    rt_batch.valid = false;
}

void pglViewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
    rt_state.viewport.x      = ( float )x;
    rt_state.viewport.y      = ( float )y;
    rt_state.viewport.width  = ( float )width;
    rt_state.viewport.height = ( float )height;
}

void pglDepthRange( GLclampd zNear, GLclampd zFar )
{
    rt_state.viewport.minDepth = ( float )zNear;
    rt_state.viewport.maxDepth = ( float )zFar;
}


static GLenum    rt_matrix_mode = 0;
static matrix4x4 rt_matrix_proj = { 0 };

void pglMatrixMode( GLenum mode )
{
    rt_matrix_mode = mode;
}

void pglLoadIdentity( void )
{
    if( rt_matrix_mode == GL_PROJECTION )
    {
        Matrix4x4_LoadIdentity( rt_state.projMatrixFor2D );
        Matrix4x4_LoadIdentity( rt_matrix_proj );
    }
}

void pglLoadMatrixd( const GLdouble* m )
{
    assert( 0 );
}

void pglLoadMatrixf( const GLfloat* m )
{
    if( rt_matrix_mode == GL_PROJECTION )
    {
        memcpy( rt_state.projMatrixFor2D, m, 16 * sizeof( float ) );
        Matrix4x4_FromArrayFloatGL( rt_matrix_proj, m );
    }
}

void pglOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
	// vulkan: swap Y
    {
        GLdouble temp = bottom;

        bottom = top;
        top    = temp;
    }

    if( rt_matrix_mode == GL_PROJECTION )
    {
        GLdouble tx = -( right + left ) / ( right - left );
        GLdouble ty = -( top + bottom ) / ( top - bottom );
        GLdouble tz = -( zFar + zNear ) / ( zFar - zNear );

        matrix4x4 ortho = {
            { ( float )( 2.0 / ( right - left ) ), 0, 0, ( float )tx },
            { 0, ( float )( 2.0 / ( top - bottom ) ), 0, ( float )ty },
            { 0, 0, ( float )( -2.0 / ( zFar - zNear ) ), ( float )tz },
            { 0, 0, 0, 1 },
        };

        matrix4x4 prev;
        Matrix4x4_Copy( prev, rt_matrix_proj );

        Matrix4x4_Concat( rt_matrix_proj, ortho, prev );

        Matrix4x4_ToArrayFloatGL( rt_matrix_proj, rt_state.projMatrixFor2D );
    }
}

#endif // XASH_RAYTRACING


