/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
#undef EXTERN_CG_CVAR
#define DECLARE_CG_CVAR
#include "../code/cgame/cg_cvar.h"
static snapshot_t snapshot, nextSnapshot;
static usercmd_t commands[512];
static playerState_t serverStates[512];
static int currentCommand, missingCommand = -999, transitions, misses, traces, commandReads;

void QDECL Com_Printf(const char *format, ...) { (void)format; }
void QDECL Com_Error(int level, const char *format, ...) { (void)level; fprintf(stderr,"%s\n",format); exit(1); }
void QDECL CG_Error(const char *format, ...) { fprintf(stderr,"%s\n",format); exit(1); }
void QDECL CG_Printf(const char *format, ...) { if (strstr(format,"Prediction miss")) ++misses; }
int trap_GetCurrentCmdNumber(void) { return currentCommand; }
qboolean trap_GetUserCmd(int number, usercmd_t *cmd) {
	++commandReads;
    if (number < 0) { memset(cmd,0,sizeof(*cmd)); return qtrue; }
    CHECK(number <= currentCommand && number < ARRAY_LEN(commands));
    if (number == missingCommand || number <= currentCommand-CMD_BACKUP) return qfalse;
    *cmd = commands[number]; return qtrue;
}
void CG_TransitionPlayerState(playerState_t *ps, playerState_t *old) { (void)ps; (void)old; ++transitions; }
void CG_EntityEvent(centity_t *cent, vec3_t position, int client) { (void)cent; (void)position; (void)client; }
void CG_AdjustPositionForMover(const vec3_t in, int mover, int from, int to, vec3_t out,
                              const vec3_t anglesIn, vec3_t anglesOut) {
    (void)mover; (void)from; (void)to; VectorCopy(in,out); VectorCopy(anglesIn,anglesOut);
}
clipHandle_t trap_CM_InlineModel(int model) { (void)model; CHECK(0); return 0; }
clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs) { (void)mins; (void)maxs; CHECK(0); return 0; }
int trap_CM_PointContents(const vec3_t point, clipHandle_t model) { (void)model; return point[2] < 0 ? CONTENTS_SOLID : 0; }
int trap_CM_TransformedPointContents(const vec3_t point, clipHandle_t model, const vec3_t origin, const vec3_t angles) {
    (void)point; (void)model; (void)origin; (void)angles; CHECK(0); return 0;
}
/* A deterministic infinite floor: all real movement/prediction code is
 * linked unchanged; only the engine collision and presentation imports are
 * replaced. Synthetic commands remain inside this process. */
void trap_CM_BoxTrace(trace_t *trace, const vec3_t start, const vec3_t end,
                      const vec3_t mins, const vec3_t maxs, clipHandle_t model, int mask) {
    float bottom = mins ? mins[2] : 0;
    (void)maxs; (void)model; (void)mask; ++traces;
    memset(trace,0,sizeof(*trace)); trace->fraction = 1; VectorCopy(end,trace->endpos);
    if (start[2]+bottom < 0) {
        trace->startsolid = qtrue;
        if (end[2]+bottom < 0) trace->allsolid = qtrue;
    }
    if (end[2]+bottom < 0 && start[2]+bottom >= 0) {
        trace->fraction = (start[2]+bottom)/(start[2]-end[2]);
        trace->plane.normal[2] = 1; trace->contents = CONTENTS_SOLID;
        for (int i = 0; i < 3; ++i) trace->endpos[i] = start[i]+trace->fraction*(end[i]-start[i]);
    }
}
void trap_CM_TransformedBoxTrace(trace_t *trace, const vec3_t start, const vec3_t end,
                                const vec3_t mins, const vec3_t maxs, clipHandle_t model, int mask,
                                const vec3_t origin, const vec3_t angles) {
    (void)trace; (void)start; (void)end; (void)mins; (void)maxs; (void)model; (void)mask;
    (void)origin; (void)angles; CHECK(0);
}
static void Reset(void) {
    memset(&cg,0,sizeof(cg)); memset(&cgs,0,sizeof(cgs));
    memset(&snapshot,0,sizeof(snapshot)); memset(commands,0,sizeof(commands));
    memset(serverStates,0,sizeof(serverStates));
    cg.snap = &snapshot; cg.time = 1000; cgs.levelStartTime = 1000;
    snapshot.serverTime = snapshot.ps.commandTime = 1000;
    snapshot.ps.pm_type = PM_NORMAL; snapshot.ps.origin[2] = 24;
    snapshot.ps.gravity = 800; snapshot.ps.speed = 320;
    snapshot.ps.stats[STAT_HEALTH] = snapshot.ps.stats[STAT_MAX_HEALTH] = 100;
    snapshot.ps.stats[STAT_WEAPONS] = 1 << WP_MACHINEGUN;
    snapshot.ps.weapon = WP_MACHINEGUN; snapshot.ps.ammo[WP_MACHINEGUN] = 100;
    snapshot.ps.groundEntityNum = ENTITYNUM_WORLD;
    BG_DefaultPmoveSettings(&cgs.pmoveSettings);
    serverStates[0] = snapshot.ps; commands[0].serverTime = 1000;
    currentCommand = 0; missingCommand = -999; transitions = misses = traces = commandReads = 0;
    cg_showmiss.integer = 1; cg_nopredict.integer = 0; cg_errorDecay.value = 100;
    CG_BuildSolidList();
}
static void Course(int movementFlags) {
    int i, acknowledgements = 0;
    pmove_t serverMove = {0};
    Reset();
    snapshot.ps.pm_flags = movementFlags; serverStates[0] = snapshot.ps;
    serverMove.settings = &cgs.pmoveSettings; serverMove.trace = CG_Trace;
    serverMove.pointcontents = CG_PointContents; serverMove.tracemask = MASK_PLAYERSOLID;
    for (i = 1; i < 400; ++i) {
        int beforeMove, moveTraces, beforePredict;
        usercmd_t *cmd = &commands[i];
        currentCommand = i; cmd->serverTime = 1000+i*8; cmd->weapon = WP_MACHINEGUN;
        cmd->forwardmove = i < 100 || i > 300 ? 127 : 0;
        cmd->rightmove = i >= 100 && i < 200 ? 127 : 0;
        cmd->upmove = i == 80 || i == 220 ? 127 : i >= 250 && i < 300 ? -127 : 0;
        cmd->angles[YAW] = i > 300 ? 16384 : 0;
        serverStates[i] = serverStates[i-1]; serverMove.ps = &serverStates[i]; serverMove.cmd = *cmd;
        beforeMove = traces; Pmove(&serverMove); moveTraces = traces-beforeMove;
        if (i > 6 && i % 3 == 0) {
            snapshot.ps = serverStates[i-6]; snapshot.serverTime = snapshot.ps.commandTime; ++acknowledgements;
        }
        cg.oldTime = cg.time; cg.time = cmd->serverTime;
        beforePredict = traces; CG_PredictPlayerState();
        CHECK(traces-beforePredict <= moveTraces*2); /* Keep incremental replay effective. */
        if (memcmp(&cg.predictedPlayerState,&serverStates[i],sizeof(playerState_t))) {
            fprintf(stderr,"prediction differs at command %d: time %d/%d origin %.6f %.6f %.6f / %.6f %.6f %.6f\n",
                    i,cg.predictedPlayerState.commandTime,serverStates[i].commandTime,
                    cg.predictedPlayerState.origin[0],cg.predictedPlayerState.origin[1],cg.predictedPlayerState.origin[2],
                    serverStates[i].origin[0],serverStates[i].origin[1],serverStates[i].origin[2]); exit(1);
        }
    }
    CHECK(acknowledgements > 100 && transitions == 399 && !misses);
}
static void CorrectedSnapshot(void) {
    const size_t fields[] = {offsetof(playerState_t,jumpTime), offsetof(playerState_t,doubleJumped),
        offsetof(playerState_t,crouchTime), offsetof(playerState_t,crouchSlideTime),
        offsetof(playerState_t,weaponPrimary), offsetof(playerState_t,fov),
        offsetof(playerState_t,location), offsetof(playerState_t,armorTier)};
    int field, i;
    for (field = 0; field < ARRAY_LEN(fields); ++field) {
        pmove_t move = {0};
        playerState_t expected;
        Reset(); snapshot.ps.pm_type = PM_SPECTATOR; serverStates[0] = snapshot.ps;
        move.settings = &cgs.pmoveSettings; move.trace = CG_Trace; move.pointcontents = CG_PointContents;
        move.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
        for (i = 1; i <= 16; ++i) {
            commands[i].serverTime = 1000+i*8; commands[i].forwardmove = 127;
            serverStates[i] = serverStates[i-1]; move.ps = &serverStates[i]; move.cmd = commands[i]; Pmove(&move);
        }
        currentCommand = 15; cg.time = commands[15].serverTime; CG_PredictPlayerState();
        snapshot.ps = serverStates[8]; snapshot.serverTime = snapshot.ps.commandTime;
        *(int *)((char *)&snapshot.ps+fields[field]) = 77;
        expected = snapshot.ps; move.ps = &expected;
        for (i = 9; i <= 16; ++i) { move.cmd = commands[i]; Pmove(&move); }
        currentCommand = 16; cg.oldTime = cg.time; cg.time = commands[16].serverTime; CG_PredictPlayerState();
        if (memcmp(&cg.predictedPlayerState,&expected,sizeof(expected))) {
            fprintf(stderr,"server correction lost for playerState offset %u\n",(unsigned)fields[field]); exit(1);
        }
    }
}
static void MissingHistory(void) {
    int i, beforeTransitions;
    playerState_t before;
    Reset();
    for (i = 1; i <= 4; ++i) {
        commands[i].serverTime = 1000+i*8; commands[i].forwardmove = 127;
        commands[i].weapon = WP_MACHINEGUN;
    }
    currentCommand = 1; cg.time = commands[1].serverTime; CG_PredictPlayerState();
    before = cg.predictedPlayerState; beforeTransitions = transitions;
    currentCommand = 4; cg.time = commands[4].serverTime;
    missingCommand = 4; CG_PredictPlayerState();
    CHECK(!memcmp(&before,&cg.predictedPlayerState,sizeof(before)) && transitions == beforeTransitions);
    CHECK(cg.stateHead == cg.stateTail && cg.lastPredictedCommand == 0);
    missingCommand = 2; CG_PredictPlayerState();
    CHECK(!memcmp(&before,&cg.predictedPlayerState,sizeof(before)) && transitions == beforeTransitions);
    missingCommand = -999; CG_PredictPlayerState();
    CHECK(cg.predictedPlayerState.commandTime == commands[4].serverTime && transitions == beforeTransitions+1);
    before = cg.predictedPlayerState; beforeTransitions = transitions;
    currentCommand = 64; missingCommand = 1; CG_PredictPlayerState();
    CHECK(!memcmp(&before,&cg.predictedPlayerState,sizeof(before)) && transitions == beforeTransitions);

    Reset(); currentCommand = missingCommand = 0;
    CG_PredictPlayerState(); /* Initial read failure still leaves a valid state. */
    CHECK(cg.validPPS && !memcmp(&snapshot.ps,&cg.predictedPlayerState,sizeof(snapshot.ps)) && !transitions);
}
static void Interpolation(void) {
    Reset(); nextSnapshot = snapshot; cg.nextSnap = &nextSnapshot;
    nextSnapshot.serverTime += 100; nextSnapshot.ps.origin[0] = 100;
    snapshot.ps.viewangles[YAW] = 350; nextSnapshot.ps.viewangles[YAW] = 10;
    cg.time += 50; cg.demoPlayback = qtrue; CG_PredictPlayerState();
    CHECK(cg.predictedPlayerState.origin[0] == 50 && !commandReads && !transitions);
    CHECK(cg.predictedPlayerState.viewangles[YAW] == 360);
    cg.demoPlayback = qfalse; snapshot.ps.pm_flags = PMF_FOLLOW; CG_PredictPlayerState();
    CHECK(cg.predictedPlayerState.origin[0] == 50 && !commandReads);
    snapshot.ps.pm_flags = 0; cg_nopredict.integer = 1; missingCommand = 0;
    CG_PredictPlayerState(); /* Unavailable local angles use snapshot interpolation. */
    CHECK(cg.predictedPlayerState.viewangles[YAW] == 360 && cg.predictedPlayerState.origin[0] == 50);
    cg.nextFrameTeleport = qtrue; CG_PredictPlayerState();
    CHECK(cg.predictedPlayerState.origin[0] == 0 && cg.predictedPlayerState.viewangles[YAW] == 350);
}
static void Disconnect(void) {
    Reset(); currentCommand = 64; commands[1].serverTime = 1500;
    cg.time = 3999; CHECK(!CG_IsConnectionInterrupted());
    cg.time = 4000; CHECK(CG_IsConnectionInterrupted());
    snapshot.ps.commandTime = 1500; CHECK(!CG_IsConnectionInterrupted());
    snapshot.ps.commandTime = 1000; commands[1].serverTime = 4001; CHECK(!CG_IsConnectionInterrupted());
    commands[1].serverTime = 1500; missingCommand = 1; CHECK(!CG_IsConnectionInterrupted());
    missingCommand = -999; cgs.levelStartTime = 5000; CHECK(!CG_IsConnectionInterrupted());
    cg.snap = NULL; CHECK(!CG_IsConnectionInterrupted());
}
int main(void) {
    Course(0);
    Course(PMF_AIR_CONTROL | PMF_DOUBLE_JUMP | PMF_CROUCH_SLIDE);
    CorrectedSnapshot();
    MissingHistory();
    Interpolation();
    Disconnect();
    puts("Cgame prediction: movement/cache replay, QL corrections, missing history, interpolation and disconnect rules OK");
    return 0;
}
