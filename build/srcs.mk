# Game sources (shared between baseq3 and missionpack)
QA_SRC = \
 g_main $(QADIR)/g_syscalls.asm \
 ai_chat ai_cmd ai_dmnet ai_dmq3 ai_main ai_team ai_vcmd \
 bg_lib bg_misc bg_pmove bg_slidemove \
 g_active g_arenas g_bot g_client g_cmds g_combat g_items g_mem g_misc \
 g_missile g_mover g_rotation g_session g_spawn g_svcmds g_target g_team \
 g_trigger g_unlagged g_utils g_weapon \
 q_math q_shared \

# CGgame sources differ between baseq3 and missionpack
ifeq ($(CONFIG),missionpack)
# Missionpack cgame adds cg_newdraw and ui_shared
CG_SRC = \
 cg_main $(CGDIR)/cg_syscalls.asm \
 cg_consolecmds cg_draw cg_drawtools cg_effects cg_ents cg_event cg_info \
 cg_localents cg_marks cg_newdraw cg_players cg_playerstate \
 cg_predict cg_scoreboard cg_servercmds cg_snapshot cg_view cg_weapons \
 ui_shared \
 bg_slidemove bg_pmove bg_lib bg_misc \
 q_math q_shared \

else
# Baseq3 cgame
CG_SRC = \
 cg_main $(CGDIR)/cg_syscalls.asm \
 cg_consolecmds cg_draw cg_drawtools cg_effects cg_ents cg_event cg_info \
 cg_localents cg_marks cg_players cg_playerstate cg_predict cg_scoreboard \
 cg_servercmds cg_snapshot cg_view cg_weapons \
 bg_slidemove bg_pmove bg_lib bg_misc \
 q_math q_shared \

endif

# UI sources are completely different between baseq3 and missionpack
ifeq ($(CONFIG),missionpack)
# Missionpack uses code/ui
UI_SRC = \
 ui_main $(UIDIR)/ui_syscalls.asm \
 ui_atoms ui_gameinfo ui_players ui_shared ui_util \
 bg_misc bg_lib \
 q_math q_shared \

else
# Baseq3 uses code/q3_ui
UI_SRC = \
 ui_main $(UIDIR)/ui_syscalls.asm \
 ui_addbots ui_atoms ui_cdkey ui_cinematics ui_confirm ui_connect \
 ui_controls2 ui_credits ui_demo2 ui_display ui_gameinfo ui_ingame \
 ui_loadconfig ui_menu ui_mfield ui_mods ui_network ui_options \
 ui_playermodel ui_players ui_playersettings ui_preferences ui_qmenu \
 ui_removebots ui_saveconfig ui_serverinfo ui_servers2 ui_setup ui_sound \
 ui_sparena ui_specifyserver ui_splevel ui_sppostgame ui_spskill \
 ui_startserver ui_team ui_teamorders ui_video \
 bg_misc bg_lib \
 q_math q_shared \

endif
