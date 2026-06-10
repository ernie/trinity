// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_bot.c

#include "g_local.h"


static int		g_numBots;
static char		*g_botInfos[MAX_BOTS];


int				g_numArenas;
static char		*g_arenaInfos[MAX_ARENAS];


#define BOT_BEGIN_DELAY_BASE		2000
#define BOT_BEGIN_DELAY_INCREMENT	1500

#define BOT_SPAWN_QUEUE_DEPTH	16

typedef struct {
	int		clientNum;
	int		spawnTime;
	char	botName[MAX_NETNAME];
} botSpawnQueue_t;

static botSpawnQueue_t	botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

// Track bots that have been selected for addition but whose addbot commands
// haven't executed yet. This prevents duplicates when G_AddRandomBot is called
// multiple times in the same frame.
#define PENDING_BOT_TIMEOUT		1000	// Clear pending entries after 1 second
#define MAX_PENDING_BOTS		16

typedef struct {
	char	name[MAX_NETNAME];
	int		addTime;
} pendingBot_t;

static pendingBot_t	pendingBots[MAX_PENDING_BOTS];

/*
===============
G_IsBotNameQueued

Check if a bot name is already in the spawn queue
===============
*/
static qboolean G_IsBotNameQueued( const char *name ) {
	int n;

	for ( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if ( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( !Q_stricmp( name, botSpawnQueue[n].botName ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
===============
G_IsBotPending

Check if a bot name is pending addition (addbot command queued but not yet executed)
===============
*/
static qboolean G_IsBotPending( const char *name ) {
	int n;

	for ( n = 0; n < MAX_PENDING_BOTS; n++ ) {
		if ( !pendingBots[n].addTime ) {
			continue;
		}
		// Clear expired entries
		if ( level.time - pendingBots[n].addTime > PENDING_BOT_TIMEOUT ) {
			pendingBots[n].addTime = 0;
			continue;
		}
		if ( !Q_stricmp( name, pendingBots[n].name ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
===============
G_MarkBotPending

Mark a bot as pending addition
===============
*/
static void G_MarkBotPending( const char *name ) {
	int n;

	// Find an empty slot
	for ( n = 0; n < MAX_PENDING_BOTS; n++ ) {
		if ( !pendingBots[n].addTime || level.time - pendingBots[n].addTime > PENDING_BOT_TIMEOUT ) {
			Q_strncpyz( pendingBots[n].name, name, sizeof(pendingBots[n].name) );
			pendingBots[n].addTime = level.time;
			return;
		}
	}
}

/*
===============
G_ClearBotPending

Remove a bot from the pending list (called when bot actually joins)
===============
*/
static void G_ClearBotPending( const char *name ) {
	int n;

	for ( n = 0; n < MAX_PENDING_BOTS; n++ ) {
		if ( pendingBots[n].addTime && !Q_stricmp( name, pendingBots[n].name ) ) {
			pendingBots[n].addTime = 0;
			return;
		}
	}
}

vmCvar_t bot_minplayers;

extern gentity_t	*podium1;
extern gentity_t	*podium2;
extern gentity_t	*podium3;

extern char mapname[ MAX_QPATH ];

float trap_Cvar_VariableValue( const char *var_name ) {
	char buf[128];

	trap_Cvar_VariableStringBuffer(var_name, buf, sizeof(buf));
	return atof(buf);
}



/*
===============
G_ParseInfos
===============
*/
int G_ParseInfos( char *buf, int max, char *infos[] ) {
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = G_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}


/*
===============
G_LoadArenasFromFile
===============
*/
static void G_LoadArenasFromFile( const char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( f == FS_INVALID_HANDLE ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	g_numArenas += G_ParseInfos( buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas] );
}


/*
===============
G_LoadArenas
===============
*/
static void G_LoadArenas( void ) {
	int			numdirs;
	vmCvar_t	arenasFile;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i, n;
	int			dirlen;

	g_numArenas = 0;

	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM );
	if( *arenasFile.string ) {
		G_LoadArenasFromFile(arenasFile.string);
	}
	else {
		G_LoadArenasFromFile("scripts/arenas.txt");
	}

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList( "scripts", ".arena", dirlist, sizeof( dirlist ) );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = (int)strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		G_LoadArenasFromFile(filename);
	}
	trap_Print( va( "%i arenas parsed\n", g_numArenas ) );
	
	for( n = 0; n < g_numArenas; n++ ) {
		Info_SetValueForKey( g_arenaInfos[n], "num", va( "%i", n ) );
	}
}


/*
===============
G_GetArenaInfoByNumber
===============
*/
const char *G_GetArenaInfoByMap( const char *map ) {
	int			n;

	for( n = 0; n < g_numArenas; n++ ) {
		if( Q_stricmp( Info_ValueForKey( g_arenaInfos[n], "map" ), map ) == 0 ) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}


/*
===============
G_IsBotInUse

Check if a bot is pending, in the spawn queue, or already connected/connecting
===============
*/
static qboolean G_IsBotInUse( const char *name ) {
	int i;
	gclient_t *cl;

	// Check pending bots (addbot command queued but not yet executed)
	if ( G_IsBotPending( name ) ) {
		return qtrue;
	}

	// Check spawn queue
	if ( G_IsBotNameQueued( name ) ) {
		return qtrue;
	}

	// Check connected/connecting clients
	for ( i = 0; i < level.maxclients; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( !Q_stricmp( name, cl->pers.netname ) ) {
			return qtrue;
		}
	}
	return qfalse;
}


/*
===============
G_RandomIndex

random() can return exactly 1.0, which would index one past the end
===============
*/
static int G_RandomIndex( int count ) {
	int		n = (int)( random() * count );

	return ( n < count ) ? n : count - 1;
}


#ifdef MISSIONPACK

/*
================================================================================

CLAN IDENTITY

teaminfo.txt defines the TA clans, their rosters, and the alias/base-character
table. Non-identity aliases get botinfos synthesized into g_aliasBotInfos:
resolvable by name, never drawn at random. Clanned teams fill from their roster.

================================================================================
*/

#define MAX_CLANS			16
#define MAX_CLAN_MEMBERS	5
#define MAX_CLAN_ALIASES	64
#define MAX_CLANNAME		64
#define MAX_ALIAS_BOTS		( MAX_CLANS * MAX_CLAN_MEMBERS )

typedef struct {
	char	alias[MAX_NETNAME];		// "Lionheart"
	char	base[MAX_NETNAME];		// "Morgan"
} clanMember_t;

typedef struct {
	char			name[MAX_CLANNAME];	// "Crusaders"
	clanMember_t	members[MAX_CLAN_MEMBERS];
} clan_t;

typedef struct {
	char	name[MAX_NETNAME];
	char	base[MAX_NETNAME];
} clanAlias_t;

static int		g_numClans;
static clan_t	g_clans[MAX_CLANS];
static int		g_numAliasBots;
static char		*g_aliasBotInfos[MAX_ALIAS_BOTS];


/*
===============
G_Clan_ParseString

String_Parse from ui_main.c, adapted to fixed storage
===============
*/
static qboolean G_Clan_ParseString( char **p, char *out, int size ) {
	char	*token;

	token = COM_ParseExt( p, qtrue );
	if ( !token[0] ) {
		return qfalse;
	}
	Q_strncpyz( out, token, size );
	return qtrue;
}


/*
===============
G_Clan_ParseTeams

Team_Parse from ui_main.c
===============
*/
static qboolean G_Clan_ParseTeams( char **p ) {
	char	*token;
	char	asset[MAX_QPATH];
	clan_t	*clan;
	int		i;

	token = COM_ParseExt( p, qtrue );
	if ( token[0] != '{' ) {
		return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt( p, qtrue );

		if ( token[0] == '}' ) {
			return qtrue;
		}
		if ( !token[0] ) {
			return qfalse;
		}
		if ( token[0] == '{' ) {
			// seven tokens per entry: team name, icon asset, and 5 member names
			clan = &g_clans[ ( g_numClans < MAX_CLANS ) ? g_numClans : MAX_CLANS - 1 ];
			if ( !G_Clan_ParseString( p, clan->name, sizeof( clan->name ) ) ||
					!G_Clan_ParseString( p, asset, sizeof( asset ) ) ) {
				return qfalse;
			}
			for ( i = 0; i < MAX_CLAN_MEMBERS; i++ ) {
				if ( !G_Clan_ParseString( p, clan->members[i].alias, sizeof( clan->members[0].alias ) ) ) {
					return qfalse;
				}
			}
			if ( g_numClans < MAX_CLANS ) {
				g_numClans++;
			}
			token = COM_ParseExt( p, qtrue );
			if ( token[0] != '}' ) {
				return qfalse;
			}
		}
	}
	return qfalse;	// not reached
}


/*
===============
G_Clan_ParseAliases

Alias_Parse from ui_main.c
===============
*/
static qboolean G_Clan_ParseAliases( char **p, clanAlias_t *aliases, int *numAliases ) {
	char		*token;
	char		action[8];
	clanAlias_t	*alias;

	token = COM_ParseExt( p, qtrue );
	if ( token[0] != '{' ) {
		return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt( p, qtrue );

		if ( token[0] == '}' ) {
			return qtrue;
		}
		if ( !token[0] ) {
			return qfalse;
		}
		if ( token[0] == '{' ) {
			// three tokens per entry: alias name, base character, preferred action
			alias = &aliases[ ( *numAliases < MAX_CLAN_ALIASES ) ? *numAliases : MAX_CLAN_ALIASES - 1 ];
			if ( !G_Clan_ParseString( p, alias->name, sizeof( alias->name ) ) ||
					!G_Clan_ParseString( p, alias->base, sizeof( alias->base ) ) ||
					!G_Clan_ParseString( p, action, sizeof( action ) ) ) {
				return qfalse;
			}
			if ( *numAliases < MAX_CLAN_ALIASES ) {
				(*numAliases)++;
			}
			token = COM_ParseExt( p, qtrue );
			if ( token[0] != '}' ) {
				return qfalse;
			}
		}
	}
	return qfalse;	// not reached
}


/*
===============
G_Clan_SkipBlock
===============
*/
static qboolean G_Clan_SkipBlock( char **p ) {
	char	*token;
	int		depth;

	depth = 0;
	while ( 1 ) {
		token = COM_ParseExt( p, qtrue );
		if ( !token[0] ) {
			return qfalse;
		}
		if ( token[0] == '{' ) {
			depth++;
		} else if ( token[0] == '}' ) {
			depth--;
			if ( depth <= 0 ) {
				return qtrue;
			}
		}
	}
	return qfalse;	// not reached
}


/*
===============
G_InitClans

Runs every level init, bots or not — rotation must work on botless servers.
===============
*/
void G_InitClans( void ) {
	int				len;
	fileHandle_t	f;
	char			buf[8192];
	char			*p;
	char			*token;
	clanAlias_t		aliases[MAX_CLAN_ALIASES];
	int				numAliases;
	clanMember_t	*member;
	int				c, m, a;

	g_numClans = 0;
	memset( g_clans, 0, sizeof( g_clans ) );
	// the alias pool points into the per-level G_Alloc arena
	g_numAliasBots = 0;
	numAliases = 0;

	len = trap_FS_FOpenFile( "teaminfo.txt", &f, FS_READ );
	if ( f == FS_INVALID_HANDLE ) {
		return;
	}
	if ( len >= sizeof( buf ) ) {
		trap_Print( va( S_COLOR_RED "file too large: teaminfo.txt is %i, max allowed is %i\n",
			len, (int)sizeof( buf ) ) );
		trap_FS_FCloseFile( f );
		return;
	}
	trap_FS_Read( buf, len, f );
	trap_FS_FCloseFile( f );
	buf[len] = '\0';

	p = buf;
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] || token[0] == '}' ) {
			break;
		}
		if ( !Q_stricmp( token, "teams" ) ) {
			if ( !G_Clan_ParseTeams( &p ) ) {
				break;
			}
		} else if ( !Q_stricmp( token, "aliases" ) ) {
			if ( !G_Clan_ParseAliases( &p, aliases, &numAliases ) ) {
				break;
			}
		} else if ( !Q_stricmp( token, "characters" ) ) {
			if ( !G_Clan_SkipBlock( &p ) ) {
				break;
			}
		}
	}

	// a roster name with no alias entry is its own base
	for ( c = 0; c < g_numClans; c++ ) {
		for ( m = 0; m < MAX_CLAN_MEMBERS; m++ ) {
			member = &g_clans[c].members[m];
			Q_strncpyz( member->base, member->alias, sizeof( member->base ) );
			for ( a = 0; a < numAliases; a++ ) {
				if ( !Q_stricmp( aliases[a].name, member->alias ) ) {
					Q_strncpyz( member->base, aliases[a].base, sizeof( member->base ) );
					break;
				}
			}
		}
	}
}


/*
===============
G_SynthesizeClanBots

An unresolvable base (the stock "Ursla" typo) costs only that one member.
===============
*/
static void G_SynthesizeClanBots( void ) {
	int				c, m, count;
	char			info[MAX_INFO_STRING];
	char			*base;
	clanMember_t	*member;

	count = 0;
	for ( c = 0; c < g_numClans; c++ ) {
		for ( m = 0; m < MAX_CLAN_MEMBERS; m++ ) {
			member = &g_clans[c].members[m];

			// identity alias: the real bot IS the roster member
			if ( !Q_stricmp( member->alias, member->base ) ) {
				if ( !G_GetBotInfoByName( member->alias ) ) {
					trap_Print( va( S_COLOR_YELLOW "WARNING: %s roster member %s has no botinfo\n",
						g_clans[c].name, member->alias ) );
				}
				continue;
			}

			// a bot already loaded under the alias name wins
			if ( G_GetBotInfoByName( member->alias ) ) {
				trap_Print( va( S_COLOR_YELLOW "WARNING: bot %s already exists, not synthesizing clan alias\n",
					member->alias ) );
				continue;
			}

			base = G_GetBotInfoByName( member->base );
			if ( !base ) {
				trap_Print( va( S_COLOR_YELLOW "WARNING: clan alias %s: base bot %s not found, skipping\n",
					member->alias, member->base ) );
				continue;
			}

			if ( g_numAliasBots >= MAX_ALIAS_BOTS ) {
				trap_Print( S_COLOR_YELLOW "WARNING: MAX_ALIAS_BOTS reached, clan alias synthesis stopped\n" );
				return;
			}

			Q_strncpyz( info, base, sizeof( info ) );
			Info_SetValueForKey( info, "name", member->alias );
			Info_SetValueForKey( info, "funname", member->alias );

			g_aliasBotInfos[g_numAliasBots] = G_Alloc( strlen( info ) + 1 );
			if ( !g_aliasBotInfos[g_numAliasBots] ) {
				return;
			}
			strcpy( g_aliasBotInfos[g_numAliasBots], info );
			g_numAliasBots++;
			count++;
		}
	}

	if ( count ) {
		trap_Print( va( "%i clan alias bots synthesized\n", count ) );
	}
}


/*
===============
G_FindClanIndexByName

Returns -1 if the team name isn't a parsed clan.
===============
*/
static int G_FindClanIndexByName( const char *teamName ) {
	char	clean[MAX_CLANNAME];
	int		c;

	Q_strncpyz( clean, teamName, sizeof( clean ) );
	Q_CleanStr( clean );

	for ( c = 0; c < g_numClans; c++ ) {
		if ( !Q_stricmp( clean, g_clans[c].name ) ) {
			return c;
		}
	}
	return -1;
}


/*
===============
G_IsClanBaseCharacter
===============
*/
static qboolean G_IsClanBaseCharacter( const char *name ) {
	int		c, m;

	for ( c = 0; c < g_numClans; c++ ) {
		for ( m = 0; m < MAX_CLAN_MEMBERS; m++ ) {
			if ( !Q_stricmp( name, g_clans[c].members[m].base ) ) {
				return qtrue;
			}
		}
	}
	return qfalse;
}


/*
===============
G_ClanMemberIndex

Roster slot of a (color-clean) name in the given clan, -1 if none.
===============
*/
static int G_ClanMemberIndex( int clanIdx, const char *name ) {
	int		m;

	for ( m = 0; m < MAX_CLAN_MEMBERS; m++ ) {
		if ( !Q_stricmp( name, g_clans[clanIdx].members[m].alias ) ) {
			return m;
		}
	}
	return -1;
}


/*
===============
G_RotateClanBots

Same-map cycles go through map_restart, which keeps bots — swap the
outgoing clan's bots for the incoming roster so rotation turns the
roster over, not just the team name. Queued ahead of ExitLevel's map
command, so the kicks and adds land before the restart.
===============
*/
static void G_RotateClanBots( team_t team, int oldIdx, int newIdx ) {
	int			i, m;
	float		skill;
	gclient_t	*cl;
	char		clean[MAX_NETNAME];
	char		userinfo[MAX_INFO_STRING];
	char		*skillstr;
	const char	*replacement;

	for ( i = 0; i < level.maxclients; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !( g_entities[i].r.svFlags & SVF_BOT ) ) {
			continue;
		}
		if ( cl->sess.sessionTeam != team ) {
			continue;
		}

		Q_strncpyz( clean, cl->pers.netname, sizeof( clean ) );
		Q_CleanStr( clean );

		// already fits the incoming clan
		if ( G_ClanMemberIndex( newIdx, clean ) >= 0 ) {
			continue;
		}
		// wildcards stay; only the old roster and stray base characters go
		if ( G_ClanMemberIndex( oldIdx, clean ) < 0 && !G_IsClanBaseCharacter( clean ) ) {
			continue;
		}

		replacement = NULL;
		for ( m = 0; m < MAX_CLAN_MEMBERS; m++ ) {
			const char *alias = g_clans[newIdx].members[m].alias;
			if ( !G_IsBotInUse( alias ) && G_GetBotInfoByName( alias ) ) {
				replacement = alias;
				break;
			}
		}

		trap_GetUserinfo( i, userinfo, sizeof( userinfo ) );
		skillstr = Info_ValueForKey( userinfo, "skill" );
		skill = skillstr[0] ? atof( skillstr ) : trap_Cvar_VariableValue( "g_spSkill" );

		trap_SendConsoleCommand( EXEC_APPEND, va( "clientkick %i\n", i ) );
		if ( replacement ) {
			G_MarkBotPending( replacement );
			trap_SendConsoleCommand( EXEC_APPEND, va( "addbot %s %1.2f %s 0\n",
				replacement, skill, team == TEAM_RED ? "red" : "blue" ) );
		}
	}
}


/*
===============
G_RotateLosingClan

Winner stays: the loser's clan advances to the next in teaminfo order,
skipping both current clans. ExitLevel-only, so aborted matches never rotate.
===============
*/
void G_RotateLosingClan( void ) {
	team_t		loserTeam;
	int			loserIdx, winnerIdx, next, tries;
	const char	*loserCvar;

	if ( !g_clanRotation.integer ) {
		return;
	}
	// the SP ladder owns its team names; rotation is inert there
	if ( level.singlePlayer ) {
		return;
	}
	if ( g_gametype.integer < GT_TEAM ) {
		return;
	}
	// need a candidate that is neither the loser nor the winner
	if ( g_numClans < 3 ) {
		return;
	}

	if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
		return;
	}
	loserTeam = ( level.teamScores[TEAM_RED] < level.teamScores[TEAM_BLUE] ) ? TEAM_RED : TEAM_BLUE;

	loserCvar = ( loserTeam == TEAM_RED ) ? "g_redteam" : "g_blueteam";
	loserIdx = G_FindClanIndexByName( loserTeam == TEAM_RED ? g_redteam.string : g_blueteam.string );
	if ( loserIdx < 0 ) {
		return;
	}
	winnerIdx = G_FindClanIndexByName( loserTeam == TEAM_RED ? g_blueteam.string : g_redteam.string );

	next = loserIdx;
	for ( tries = 0; tries < g_numClans; tries++ ) {
		next = ( next + 1 ) % g_numClans;
		if ( next != loserIdx && next != winnerIdx ) {
			break;
		}
	}

	G_LogPrintf( "ClanRotation: %s %s -> %s\n",
		loserTeam == TEAM_RED ? "red" : "blue",
		g_clans[loserIdx].name, g_clans[next].name );
	trap_Cvar_Set( loserCvar, g_clans[next].name );
	G_RotateClanBots( loserTeam, loserIdx, next );
}

#endif	// MISSIONPACK


/*
===============
G_AddRandomBot
===============
*/
void G_AddRandomBot( team_t team ) {
	int		n, num;
	float	skill;
	char	*botname, netname[36], displayname[MAX_NETNAME], *teamstr;
	char	skillstr[8];
	char	*info;
	int		available[MAX_BOTS];
#ifdef MISSIONPACK
	int		clanIdx = -1;

	if ( g_gametype.integer >= GT_TEAM && ( team == TEAM_RED || team == TEAM_BLUE ) ) {
		clanIdx = G_FindClanIndexByName( team == TEAM_RED ? g_redteam.string : g_blueteam.string );
	}
#endif

	info = NULL;

#ifdef MISSIONPACK
	// Clanned team fill: 75% of adds draw from the team's own roster
	if ( clanIdx >= 0 && random() < 0.75f ) {
		const clan_t	*clan = &g_clans[clanIdx];
		const char		*names[MAX_CLAN_MEMBERS];

		num = 0;
		for ( n = 0; n < MAX_CLAN_MEMBERS; n++ ) {
			if ( !G_IsBotInUse( clan->members[n].alias ) &&
					G_GetBotInfoByName( clan->members[n].alias ) ) {
				names[num++] = clan->members[n].alias;
			}
		}
		if ( num ) {
			info = G_GetBotInfoByName( names[ G_RandomIndex( num ) ] );
		}
		// roster exhausted: fall through to the wildcard draw
	}
#endif

	if ( info == NULL ) {
		// First pass: collect indices of bots not currently in use
		num = 0;
		for ( n = 0; n < g_numBots; n++ ) {
#ifdef MISSIONPACK
			// clan base characters appear only via their own clan's roster
			if ( clanIdx >= 0 && G_IsClanBaseCharacter( Info_ValueForKey( g_botInfos[n], "name" ) ) ) {
				continue;
			}
#endif
			// Use the display name (funname or name) since that's what becomes netname
			botname = Info_ValueForKey( g_botInfos[n], "funname" );
			if ( !botname[0] ) {
				botname = Info_ValueForKey( g_botInfos[n], "name" );
			}
			if ( !G_IsBotInUse( botname ) ) {
				available[num++] = n;
			}
		}

		// Fallback: if no unique bots available, use all bots
		if ( num == 0 ) {
			for ( n = 0; n < g_numBots; n++ ) {
				available[num++] = n;
			}
		}

		// No bots loaded at all
		if ( num == 0 ) {
			return;
		}

		// Pick a random bot from available list
		info = g_botInfos[ available[ G_RandomIndex( num ) ] ];
	}

	// copy out: Info_ValueForKey rotates only two static buffers
	Q_strncpyz( netname, Info_ValueForKey( info, "name" ), sizeof(netname) );
	Q_strncpyz( displayname, Info_ValueForKey( info, "funname" ), sizeof(displayname) );
	if ( !displayname[0] ) {
		Q_strncpyz( displayname, netname, sizeof(displayname) );
	}
	Q_strncpyz( skillstr, Info_ValueForKey( info, "skill" ), sizeof(skillstr) );
	if ( skillstr[0] ) {
		skill = atof( skillstr );
	} else {
		skill = trap_Cvar_VariableValue( "g_spSkill" );
	}

	if ( team == TEAM_RED ) {
		teamstr = "red";
	} else if ( team == TEAM_BLUE ) {
		teamstr = "blue";
	} else {
		teamstr = "";
	}

	Q_CleanStr( netname );

	// Mark this bot as pending so subsequent calls to G_AddRandomBot
	// in the same frame won't select the same bot
	G_MarkBotPending( displayname );

	trap_SendConsoleCommand( EXEC_INSERT, va( "addbot %s %1.2f %s 0\n", netname, skill, teamstr ) );
}


/*
===============
G_RemoveRandomBot
===============
*/
int G_RemoveRandomBot( int team ) {
	int i;
	char netname[36];
	gclient_t	*cl;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		strcpy(netname, cl->pers.netname);
		Q_CleanStr(netname);
		trap_SendConsoleCommand( EXEC_INSERT, va("kick %s\n", netname) );
		return qtrue;
	}
	return qfalse;
}


/*
===============
G_CountHumanPlayers
===============
*/
static int G_CountHumanPlayers( team_t team ) {
	int i, num;
	gclient_t	*cl;

	num = 0;
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		num++;
	}
	return num;
}


/*
===============
G_CountBotPlayers
===============
*/
static int G_CountBotPlayers( team_t team ) {
	int i, n, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< level.maxclients ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		num++;
	}
	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		num++;
	}
	return num;
}


/*
===============
G_CheckMinimumPlayers
===============
*/
void G_CheckMinimumPlayers( void ) {
	int minplayers;
	int humanplayers, botplayers;
	static int checkminimumplayers_time;

	if ( level.intermissiontime )
		return;

	//only check once each 10 seconds
	if ( checkminimumplayers_time > level.time - 10000 )
		return;

	if ( level.time - level.startTime < 2000 )
		return;

	checkminimumplayers_time = level.time;
	trap_Cvar_Update(&bot_minplayers);
	minplayers = bot_minplayers.integer;
	if (minplayers <= 0) return;

	if (g_gametype.integer >= GT_TEAM) {
		if (minplayers >= level.maxclients / 2) {
			minplayers = (level.maxclients / 2) -1;
		}

		humanplayers = G_CountHumanPlayers( TEAM_RED );
		botplayers = G_CountBotPlayers(	TEAM_RED );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_RED );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_RED );
		}
		//
		humanplayers = G_CountHumanPlayers( TEAM_BLUE );
		botplayers = G_CountBotPlayers( TEAM_BLUE );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_BLUE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_BLUE );
		}
	}
	else if (g_gametype.integer == GT_TOURNAMENT ) {
		if (minplayers >= level.maxclients) {
			minplayers = level.maxclients-1;
		}
		humanplayers = G_CountHumanPlayers( -1 );
		botplayers = G_CountBotPlayers( -1 );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			// try to remove spectators first
			if (!G_RemoveRandomBot( TEAM_SPECTATOR )) {
				// just remove the bot that is playing
				G_RemoveRandomBot( -1 );
			}
		}
	}
	else if (g_gametype.integer == GT_FFA) {
		if (minplayers >= level.maxclients) {
			minplayers = level.maxclients-1;
		}
		humanplayers = G_CountHumanPlayers( TEAM_FREE );
		botplayers = G_CountBotPlayers( TEAM_FREE );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_FREE );
		}
	}
}


/*
===============
G_CheckBotSpawn
===============
*/
void G_CheckBotSpawn( void ) {
	int		n;

	G_CheckMinimumPlayers();

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		ClientBegin( botSpawnQueue[n].clientNum );
		botSpawnQueue[n].spawnTime = 0;
	}
}


/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue( int clientNum, int delay, const char *botName ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			botSpawnQueue[n].spawnTime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			Q_strncpyz( botSpawnQueue[n].botName, botName, sizeof(botSpawnQueue[n].botName) );
			G_ClearBotPending( botName );
			return;
		}
	}

	G_Printf( S_COLOR_YELLOW "Unable to delay bot spawn\n" );

	G_ClearBotPending( botName );
	ClientBegin( clientNum );
}


/*
===============
G_RemoveQueuedBotBegin

Called on client disconnect to make sure the delayed spawn
doesn't happen on a freed index
===============
*/
void G_RemoveQueuedBotBegin( int clientNum ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( botSpawnQueue[n].clientNum == clientNum ) {
			botSpawnQueue[n].spawnTime = 0;
			return;
		}
	}
}


/*
===============
G_BotConnect
===============
*/
qboolean G_BotConnect( int clientNum, qboolean restart ) {
	bot_settings_t	settings;
	char			userinfo[MAX_INFO_STRING];

	trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );

	Q_strncpyz( settings.characterfile, Info_ValueForKey( userinfo, "characterfile" ), sizeof(settings.characterfile) );
	settings.skill = atof( Info_ValueForKey( userinfo, "skill" ) );
	Q_strncpyz( settings.team, Info_ValueForKey( userinfo, "team" ), sizeof(settings.team) );

	if (!BotAISetupClient( clientNum, &settings, restart )) {
		trap_DropClient( clientNum, "BotAISetupClient failed" );
		return qfalse;
	}

	return qtrue;
}


/*
===============
G_AddBot
===============
*/
static void G_AddBot( const char *name, float skill, const char *team, int delay, const char *altname ) {
	int				clientNum;
	char			*botinfo;
	gentity_t		*bot;
	char			*key;
	char			*s;
	const char		*botname;
	const char		*model;
	const char		*headmodel;
	char			userinfo[MAX_INFO_STRING];
	char			nm[MAX_CVAR_VALUE_STRING];

	// get the botinfo from bots.txt
	botinfo = G_GetBotInfoByName( name );
	if ( !botinfo ) {
		G_Printf( S_COLOR_RED "Error: Bot '%s' not defined\n", name );
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey( botinfo, "funname" );
	if( !botname[0] ) {
		botname = Info_ValueForKey( botinfo, "name" );
	}
	// check for an alternative name
	if (altname && altname[0]) {
		botname = altname;
	}

	BG_CleanName( botname, nm, sizeof( nm ), "unnamed bot" );
	Info_SetValueForKey( userinfo, "name", nm );

	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", va( "%i", sv_fps.integer ) );
	Info_SetValueForKey( userinfo, "skill", va("%1.2f", skill) );

	if ( skill >= 1 && skill < 2 ) {
		Info_SetValueForKey( userinfo, "handicap", "50" );
	}
	else if ( skill >= 2 && skill < 3 ) {
		Info_SetValueForKey( userinfo, "handicap", "70" );
	}
	else if ( skill >= 3 && skill < 4 ) {
		Info_SetValueForKey( userinfo, "handicap", "90" );
	}

	key = "model";
	model = Info_ValueForKey( botinfo, key );
	if ( !*model ) {
		model = "visor/default";
	}
	Info_SetValueForKey( userinfo, key, model );
	key = "team_model";
	Info_SetValueForKey( userinfo, key, model );

	key = "headmodel";
	headmodel = Info_ValueForKey( botinfo, key );
	if ( !*headmodel ) {
		headmodel = model;
	}
	Info_SetValueForKey( userinfo, key, headmodel );
	key = "team_headmodel";
	Info_SetValueForKey( userinfo, key, headmodel );

	key = "gender";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "male";
	}
	Info_SetValueForKey( userinfo, "sex", s );

	key = "color1";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "4";
	}
	Info_SetValueForKey( userinfo, key, s );

	key = "color2";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "5";
	}
	Info_SetValueForKey( userinfo, key, s );

	s = Info_ValueForKey(botinfo, "aifile");
	if (!*s ) {
		trap_Print( S_COLOR_RED "Error: bot has no aifile specified\n" );
		return;
	}

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if ( clientNum == -1 ) {
		G_Printf( S_COLOR_RED "Unable to add bot.  All player slots are in use.\n" );
		G_Printf( S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n" );
		return;
	}

	// cleanup previous data manually
	// because client may silently (re)connect without ClientDisconnect in case of crash for example
	if ( level.clients[ clientNum ].pers.connected != CON_DISCONNECTED ) {
		ClientDisconnect( clientNum );
	}

	Info_SetValueForKey( userinfo, "characterfile", Info_ValueForKey( botinfo, "aifile" ) );
	Info_SetValueForKey( userinfo, "skill", va( "%1.2f", skill ) );
	Info_SetValueForKey( userinfo, "team", team );

	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;
	bot->inuse = qtrue;

	// register the userinfo
	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if ( ClientConnect( clientNum, qtrue, qtrue ) ) {
		return;
	}

	if ( delay == 0 ) {
		G_ClearBotPending( botname );
		ClientBegin( clientNum );
		return;
	}

	AddBotToSpawnQueue( clientNum, delay, botname );
}


/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f( void ) {
	float			skill;
	int				delay;
	char			name[MAX_TOKEN_CHARS];
	char			altname[MAX_TOKEN_CHARS];
	char			string[MAX_TOKEN_CHARS];
	char			team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	// name
	trap_Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		trap_Print( "Usage: Addbot <botname> [skill 1-5] [team] [msec delay] [altname]\n" );
		return;
	}

	// skill
	trap_Argv( 2, string, sizeof( string ) );
	if ( !string[0] ) {
		skill = 4;
	}
	else {
		skill = atof( string );
		if ( skill < 1 )
			skill = 1;
		else if ( skill > 5 )
			skill = 5;
	}

	// team
	trap_Argv( 3, team, sizeof( team ) );

	// delay
	trap_Argv( 4, string, sizeof( string ) );
	if ( !string[0] ) {
		delay = 0;
	}
	else {
		delay = atoi( string );
	}

	// alternative name
	trap_Argv( 5, altname, sizeof( altname ) );

	G_AddBot( name, skill, team, delay, altname );

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if ( level.time - level.startTime > 1000 &&
		trap_Cvar_VariableIntegerValue( "cl_running" ) ) {
		trap_SendServerCommand( -1, "loaddeferred\n" );	// FIXME: spelled wrong, but not changing for demo
	}
}

/*
===============
G_PrintBotInfo
===============
*/
static void G_PrintBotInfo( const char *info ) {
	char name[MAX_NETNAME];
	char funname[MAX_NETNAME];
	char model[MAX_QPATH];
	char aifile[MAX_QPATH];

	Q_strncpyz( name, Info_ValueForKey( info, "name" ), sizeof( name ) );
	if ( !*name ) {
		strcpy(name, "UnnamedPlayer");
	}
	Q_strncpyz( funname, Info_ValueForKey( info, "funname" ), sizeof( funname ) );
	if ( !*funname ) {
		strcpy( funname, "" );
	}
	Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
	if ( !*model ) {
		strcpy( model, "visor/default" );
	}
	Q_strncpyz( aifile, Info_ValueForKey( info, "aifile" ), sizeof( aifile ) );
	if ( !*aifile ) {
		strcpy( aifile, "bots/default_c.c" );
	}
	trap_Print( va( "%-16s %-16s %-20s %-20s\n", name, model, aifile, funname ) );
}


/*
===============
Svcmd_BotList_f
===============
*/
void Svcmd_BotList_f( void ) {
	int i;

	trap_Print( S_COLOR_RED "name             model            aifile              funname\n" );
	for ( i = 0; i < g_numBots; i++ ) {
		G_PrintBotInfo( g_botInfos[i] );
	}
#ifdef MISSIONPACK
	for ( i = 0; i < g_numAliasBots; i++ ) {
		G_PrintBotInfo( g_aliasBotInfos[i] );
	}
#endif
}


/*
===============
G_SpawnBots
===============
*/
static void G_SpawnBots( const char *botList, int baseDelay ) {
	char		*bot;
	char		*p;
	float		skill;
	int			delay;
	char		bots[MAX_INFO_VALUE];

	podium1 = NULL;
	podium2 = NULL;
	podium3 = NULL;

	skill = trap_Cvar_VariableValue( "g_spSkill" );
	if( skill < 1 ) {
		trap_Cvar_Set( "g_spSkill", "1" );
		skill = 1;
	}
	else if ( skill > 5 ) {
		trap_Cvar_Set( "g_spSkill", "5" );
		skill = 5;
	}

	Q_strncpyz( bots, botList, sizeof( bots ) );
	p = &bots[0];
	delay = baseDelay;
	while( *p ) {
		//skip spaces
		while( *p == ' ' ) {
			p++;
		}
		if( !*p ) {
			break;
		}

		// mark start of bot name
		bot = p;

		// skip until space of null
		while( *p && *p != ' ' ) {
			p++;
		}
		if( *p ) {
			*p++ = '\0';
		}

		// we must add the bot this way, calling G_AddBot directly at this stage
		// does "Bad Things"
		trap_SendConsoleCommand( EXEC_INSERT, va("addbot %s %f free %i\n", bot, skill, delay) );

		delay += BOT_BEGIN_DELAY_INCREMENT;
	}
}


/*
===============
G_LoadBotsFromFile
===============
*/
static void G_LoadBotsFromFile( const char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_BOTS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( f == FS_INVALID_HANDLE ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	trap_FS_FCloseFile( f );
	buf[ len ] = '\0';

	g_numBots += G_ParseInfos( buf, MAX_BOTS - g_numBots, &g_botInfos[g_numBots] );
}


/*
===============
G_LoadBots
===============
*/
static void G_LoadBots( void ) {
	vmCvar_t	botsFile;
	vmCvar_t	skipBotFiles;
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	g_numBots = 0;

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_ARCHIVE | CVAR_LATCH );
	trap_Cvar_Register( &skipBotFiles, "g_skipBotFiles", "0", CVAR_ARCHIVE | CVAR_LATCH );

	if ( *botsFile.string && g_gametype.integer != GT_SINGLE_PLAYER ) {
		G_LoadBotsFromFile( botsFile.string );
	} else {
		G_LoadBotsFromFile( "scripts/bots.txt" );
	}

	// get all bots from .bot files (unless g_skipBotFiles is set)
	if ( !skipBotFiles.integer ) {
		numdirs = trap_FS_GetFileList( "scripts", ".bot", dirlist, sizeof( dirlist ) );
		dirptr  = dirlist;
		for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
			dirlen = (int)strlen(dirptr);
			strcpy(filename, "scripts/");
			strcat(filename, dirptr);
			G_LoadBotsFromFile(filename);
		}
	}
	trap_Print( va( "%i bots parsed\n", g_numBots ) );
}



/*
===============
G_GetBotInfoByNumber
===============
*/
char *G_GetBotInfoByNumber( int num ) {
	if( num < 0 || num >= g_numBots ) {
		trap_Print( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}
	return g_botInfos[num];
}


/*
===============
G_GetBotInfoByName
===============
*/
char *G_GetBotInfoByName( const char *name ) {
	int		n;
	char	*value;

	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return g_botInfos[n];
		}
	}

#ifdef MISSIONPACK
	for ( n = 0; n < g_numAliasBots ; n++ ) {
		value = Info_ValueForKey( g_aliasBotInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return g_aliasBotInfos[n];
		}
	}
#endif

	return NULL;
}


/*
===============
G_InitBots
===============
*/
void G_InitBots( qboolean restart ) {
	int			fragLimit;
	int			timeLimit;
	const char	*arenainfo;
	char		*strValue;
	int			basedelay;

	G_LoadBots();
#ifdef MISSIONPACK
	if ( g_numBots ) {
		G_SynthesizeClanBots();
	}
#endif
	G_LoadArenas();

	trap_Cvar_Register( &bot_minplayers, "bot_minplayers", "0", CVAR_SERVERINFO );

	if( g_gametype.integer == GT_SINGLE_PLAYER ) {
		arenainfo = G_GetArenaInfoByMap( mapname );
		if ( !arenainfo ) {
			return;
		}

		strValue = Info_ValueForKey( arenainfo, "fraglimit" );
		fragLimit = atoi( strValue );
		if ( fragLimit ) {
			trap_Cvar_Set( "fraglimit", strValue );
		}
		else {
			trap_Cvar_Set( "fraglimit", "0" );
		}

		strValue = Info_ValueForKey( arenainfo, "timelimit" );
		timeLimit = atoi( strValue );
		if ( timeLimit ) {
			trap_Cvar_Set( "timelimit", strValue );
		}
		else {
			trap_Cvar_Set( "timelimit", "0" );
		}

		if ( !fragLimit && !timeLimit ) {
			trap_Cvar_Set( "fraglimit", "10" );
			trap_Cvar_Set( "timelimit", "0" );
		}

		basedelay = BOT_BEGIN_DELAY_BASE;
		strValue = Info_ValueForKey( arenainfo, "special" );
		if( Q_stricmp( strValue, "training" ) == 0 ) {
			basedelay += 10000;
		}

		if( !restart ) {
			G_SpawnBots( Info_ValueForKey( arenainfo, "bots" ), basedelay );
		}
	}
}
