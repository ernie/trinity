// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_hostlabels.c -- friendly browser labels for master-server hostnames

#include "q_shared.h"
#include "bg_hostlabels.h"

typedef struct {
	const char	*host;
	const char	*label;
} hostlabel_t;

static const hostlabel_t hostLabels[] = {
	{ "mp.quakevr.com",			"QuakeVR" },
	{ "directory.trinity.run",	"Trinity" },
	{ "master.quake3arena.com",	"Q3A" },
	{ "master.ioquake3.org",	"ioquake3" },
	{ "master.maverickservers.com",	"Maverick" },
};

static const int numHostLabels = ARRAY_LEN( hostLabels );

/*
=================
BG_StripPort

Copies host into out without any :port suffix. A bracketed IPv6 literal
([::1]:27950) keeps the address inside the brackets; anything else is cut at
the first colon.
=================
*/
static void BG_StripPort( const char *host, char *out, int outSize ) {
	int		i;
	int		len;

	if ( outSize < 1 ) {
		return;
	}

	if ( host[0] == '[' ) {
		host++;
		len = 0;
		while ( host[len] != '\0' && host[len] != ']' ) {
			len++;
		}
	} else {
		len = 0;
		while ( host[len] != '\0' && host[len] != ':' ) {
			len++;
		}
	}

	if ( len > outSize - 1 ) {
		len = outSize - 1;
	}
	for ( i = 0; i < len; i++ ) {
		out[i] = host[i];
	}
	out[len] = '\0';
}

/*
=================
BG_ServerSourceLabel
=================
*/
const char *BG_ServerSourceLabel( const char *host ) {
	char	stripped[256];
	int		i;

	if ( !host || !host[0] ) {
		return NULL;
	}

	BG_StripPort( host, stripped, sizeof( stripped ) );

	for ( i = 0; i < numHostLabels; i++ ) {
		if ( !Q_stricmp( stripped, hostLabels[i].host ) ) {
			return hostLabels[i].label;
		}
	}

	return NULL;
}

/*
=================
BG_ServerSourceName
=================
*/
void BG_ServerSourceName( const char *host, char *out, int outSize, int maxChars ) {
	const char	*label;

	if ( outSize < 1 ) {
		return;
	}
	out[0] = '\0';
	if ( !host || !host[0] ) {
		return;
	}

	label = BG_ServerSourceLabel( host );
	if ( label ) {
		Q_strncpyz( out, label, outSize );
	} else {
		BG_StripPort( host, out, outSize );
	}

	if ( maxChars > 0 && maxChars < outSize - 1 && (int)strlen( out ) > maxChars ) {
		out[maxChars] = '\0';
	}
}
