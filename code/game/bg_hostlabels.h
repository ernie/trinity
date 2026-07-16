// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_hostlabels.h -- friendly browser labels for master-server hostnames
//
// Shared by both UI families (q3_ui and ui) so the server browser shows the
// same source names everywhere. Lives in code/game like bg_mode so both ui
// module builds compile the one copy (Makefile.build's $(QADIR) ui pattern
// rule and trinity-vr's native module manifest resolve it identically).

// Returns the friendly label for a known master-server hostname (the host is
// compared case-insensitively with any :port suffix ignored), or NULL when
// the host is not in the table.
const char *BG_ServerSourceLabel( const char *host );

// Writes the display name for a master-server host into out: the friendly
// label when the host is known, otherwise the hostname itself, port-stripped.
// The result is truncated to maxChars characters (and always to outSize
// bytes including the terminator) so callers pass the budget their widget
// actually fits.
void BG_ServerSourceName( const char *host, char *out, int outSize, int maxChars );
