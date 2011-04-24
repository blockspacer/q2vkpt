/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//
// cl_precache.c
//

#include "cl_local.h"

/*
================
CL_ParsePlayerSkin

Breaks up playerskin into name (optional), model and skin components.
If model or skin are found to be invalid, replaces them with sane defaults.
================
*/
void CL_ParsePlayerSkin( char *name, char *model, char *skin, const char *s ) {
    size_t len;
    char *t;

    // configstring parsing guarantees that playerskins can never
    // overflow, but still check the length to be entirely fool-proof
    len = strlen( s );
    if( len >= MAX_QPATH ) {
        Com_Error( ERR_DROP, "%s: oversize playerskin", __func__ );
    }

    // isolate the player's name
    t = strchr( s, '\\' );
    if( t ) {
        len = t - s;
        strcpy( model, t + 1 );
    } else {
        len = 0;
        strcpy( model, s );
    }

    // copy the player's name
    if( name ) {
        memcpy( name, s, len );
        name[len] = 0;
    }

    // isolate the model name
    t = strchr( model, '/' );
    if( !t )
        t = strchr( model, '\\' );
    if( !t )
        t = model;
    if( t == model )
        goto default_model;
    *t++ = 0;

    // apply restrictions on skins
    if( cl_noskins->integer == 2 || !COM_IsPath( t ) )
        goto default_skin;

    if( cl_noskins->integer || !COM_IsPath( model ) )
        goto default_model;

    // isolate the skin name
    strcpy( skin, t );
    return;

default_skin:
    if( !Q_stricmp( model, "female" ) ) {
        strcpy( model, "female" );
        strcpy( skin, "athena" );
    } else {
default_model:
        strcpy( model, "male" );
        strcpy( skin, "grunt" );
    }
}

/*
================
CL_LoadClientinfo

================
*/
void CL_LoadClientinfo( clientinfo_t *ci, const char *s ) {
    int         i;
    char        model_name[MAX_QPATH];
    char        skin_name[MAX_QPATH];
    char        model_filename[MAX_QPATH];
    char        skin_filename[MAX_QPATH];
    char        weapon_filename[MAX_QPATH];
    char        icon_filename[MAX_QPATH];

    CL_ParsePlayerSkin( ci->name, model_name, skin_name, s );

    // model file
    Q_concat( model_filename, sizeof( model_filename ),
        "players/", model_name, "/tris.md2", NULL );
    ci->model = R_RegisterModel( model_filename );
    if( !ci->model && Q_stricmp( model_name, "male" ) ) {
        strcpy( model_name, "male" );
        strcpy( model_filename, "players/male/tris.md2" );
        ci->model = R_RegisterModel( model_filename );
    }

    // skin file
    Q_concat( skin_filename, sizeof( skin_filename ),
        "players/", model_name, "/", skin_name, ".pcx", NULL );
    ci->skin = R_RegisterSkin( skin_filename );

    // if we don't have the skin and the model was female,
    // see if athena skin exists
    if( !ci->skin && !Q_stricmp( model_name, "female" ) ) {
        strcpy( skin_name, "athena" );
        strcpy( skin_filename, "players/female/athena.pcx" );
        ci->skin = R_RegisterSkin( skin_filename );
    }

    // if we don't have the skin and the model wasn't male,
    // see if the male has it (this is for CTF's skins)
    if( !ci->skin && Q_stricmp( model_name, "male" ) ) {
        // change model to male
        strcpy( model_name, "male" );
        strcpy( model_filename, "players/male/tris.md2" );
        ci->model = R_RegisterModel( model_filename );

        // see if the skin exists for the male model
        Q_concat( skin_filename, sizeof( skin_filename ),
            "players/male/", skin_name, ".pcx", NULL );
        ci->skin = R_RegisterSkin( skin_filename );
    }

    // if we still don't have a skin, it means that the male model
    // didn't have it, so default to grunt
    if( !ci->skin ) {
        // see if the skin exists for the male model
        strcpy( skin_name, "grunt" );
        strcpy( skin_filename, "players/male/grunt.pcx" );
        ci->skin = R_RegisterSkin( skin_filename );
    }

    // weapon file
    for( i = 0; i < cl.numWeaponModels; i++ ) {
        Q_concat( weapon_filename, sizeof( weapon_filename ),
            "players/", model_name, "/", cl.weaponModels[i], NULL );
        ci->weaponmodel[i] = R_RegisterModel( weapon_filename );
        if( !ci->weaponmodel[i] && Q_stricmp( model_name, "male" ) ) {
            // try male
            Q_concat( weapon_filename, sizeof( weapon_filename ),
                "players/male/", cl.weaponModels[i], NULL );
            ci->weaponmodel[i] = R_RegisterModel( weapon_filename );
        }
    }

    // icon file
    Q_concat( icon_filename, sizeof( icon_filename ),
        "/players/", model_name, "/", skin_name, "_i.pcx", NULL );
    ci->icon = R_RegisterPic( icon_filename );

    strcpy( ci->model_name, model_name );
    strcpy( ci->skin_name, skin_name );

    // must have loaded all data types to be valid
    if( !ci->skin || !ci->icon || !ci->model || !ci->weaponmodel[0] ) {
        ci->skin = 0;
        ci->icon = 0;
        ci->model = 0;
        ci->weaponmodel[0] = 0;
        ci->model_name[0] = 0;
        ci->skin_name[0] = 0;
    }
}

/*
=================
CL_LoadState
=================
*/
void CL_LoadState( load_state_t state ) {
    extern void VID_PumpEvents( void );
    const char *s;

    switch( state ) {
    case LOAD_MAP:
        s = cl.configstrings[ CS_MODELS + 1 ];
        break;
    case LOAD_MODELS:
        s = "models";
        break;
    case LOAD_IMAGES:
        s = "images";
        break;
    case LOAD_CLIENTS:
        s = "clients";
        break;
    case LOAD_SOUNDS:
        s = "sounds";
        break;
    case LOAD_FINISH:
        s = NULL;
        break;
    default:
        return;
    }

    if( s ) {
        Con_Printf( "Loading %s...\r", s );
    } else {
        Con_Print( "\r" );
    }

    SCR_UpdateScreen();
    VID_PumpEvents();
}

/*
=================
CL_RegisterSounds
=================
*/
void CL_RegisterSounds( void ) {
    int i;
    char    *s;

    S_BeginRegistration ();
    CL_RegisterTEntSounds ();
    for ( i = 1; i < MAX_SOUNDS; i++ ) {
        s = cl.configstrings[ CS_SOUNDS + i ];
        if ( !s[ 0 ] )
            break;
        cl.sound_precache[ i ] = S_RegisterSound( s );
    }
    S_EndRegistration ();
}

/*
=================
CL_RegisterBspModels

Registers main BSP file and inline models
=================
*/
void CL_RegisterBspModels( void ) {
    qerror_t ret;
    char *name;
    int i;

    ret = BSP_Load( cl.configstrings[ CS_MODELS + 1 ], &cl.bsp );
    if( cl.bsp == NULL ) {
        Com_Error( ERR_DROP, "Couldn't load %s: %s",
            cl.configstrings[ CS_MODELS + 1 ], Q_ErrorString( ret ) );
    }

#if USE_MAPCHECKSUM
    if( cl.bsp->checksum != atoi( cl.configstrings[ CS_MAPCHECKSUM ] ) ) {
        if( cls.demo.playback ) {
            Com_WPrintf( "Local map version differs from demo: %i != %s\n",
                cl.bsp->checksum, cl.configstrings[ CS_MAPCHECKSUM ] );
        } else {
            Com_Error( ERR_DROP, "Local map version differs from server: %i != %s",
                cl.bsp->checksum, cl.configstrings[ CS_MAPCHECKSUM ] );
        }
    }
#endif

    for ( i = 1; i < MAX_MODELS; i++ ) {
        name = cl.configstrings[CS_MODELS+i];
        if( !name[0] ) {
            break;
        }
        if( name[0] == '*' )
            cl.model_clip[i] = BSP_InlineModel( cl.bsp, name );
        else
            cl.model_clip[i] = NULL;
    }
}

/*
=================
CL_RegisterVWepModels

Builds a list of visual weapon models
=================
*/
void CL_RegisterVWepModels( void ) {
    int         i;
    char        *name;

    cl.numWeaponModels = 1;
    strcpy( cl.weaponModels[0], "weapon.md2" );

    // only default model when vwep is off
    if( !cl_vwep->integer ) {
        return;
    }

    for( i = 2; i < MAX_MODELS; i++ ) {
        name = cl.configstrings[ CS_MODELS + i ];
        if( !name[0] ) {
            break;
        }
        if( name[0] != '#' ) {
            continue;
        }

        // special player weapon model
        strcpy( cl.weaponModels[cl.numWeaponModels++], name + 1 );

        if( cl.numWeaponModels == MAX_CLIENTWEAPONMODELS ) {
            break;
        }
    }
}

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
void CL_PrepRefresh (void) {
    int         i;
    char        *name;
    float       rotate;
    vec3_t      axis;

    if( !cls.ref_initialized )
        return;
    if( !cl.mapname[0] )
        return;     // no map loaded

    // register models, pics, and skins
    R_BeginRegistration( cl.mapname );

    CL_LoadState( LOAD_MODELS );

    CL_RegisterTEntModels ();

    for( i = 2; i < MAX_MODELS; i++ ) {
        name = cl.configstrings[ CS_MODELS + i ];
        if( !name[0] ) {
            break;
        }
        if( name[0] == '#' ) {
            continue;
        }
        cl.model_draw[i] = R_RegisterModel( name );
    }

    CL_LoadState( LOAD_IMAGES );
    for( i = 1; i < MAX_IMAGES; i++ ) {
        name = cl.configstrings[ CS_IMAGES + i ];
        if( !name[0] ) {
            break;
        }
        cl.image_precache[i] = R_RegisterPic( name );
    }

    CL_LoadState( LOAD_CLIENTS );
    for(i = 0; i < MAX_CLIENTS; i++ ) {
        name = cl.configstrings[ CS_PLAYERSKINS + i ];
        if( !name[0] ) {
            continue;
        }
        CL_LoadClientinfo( &cl.clientinfo[i], name );
    }

    CL_LoadClientinfo( &cl.baseclientinfo, "unnamed\\male/grunt" );

    // set sky textures and speed
    rotate = atof( cl.configstrings[CS_SKYROTATE] );
    if( sscanf( cl.configstrings[CS_SKYAXIS], "%f %f %f",
        &axis[0], &axis[1], &axis[2]) != 3 )
    {
        Com_DPrintf( "Couldn't parse CS_SKYAXIS\n" );
        VectorClear( axis );
    }
    R_SetSky( cl.configstrings[CS_SKY], rotate, axis );

    // the renderer can now free unneeded stuff
    R_EndRegistration();

    // clear any lines of console text
    Con_ClearNotify_f();

    SCR_UpdateScreen();
}
