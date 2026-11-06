/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"

/* Retail 100155d0 changes the existing general entity presentation. Keep
 * selection pure and shared by the scene and HUD passes; no renderer or
 * entity pointers outlive the current snapshot. */
typedef struct {
    qhandle_t model;
    int icon; /* start/checkpoint/finish, or -1 */
    qboolean hide;
} racePointView_t;

void CG_RegisterRaceMedia(void) {
    cgRaceMedia_t *m = &cgs.raceMedia;
    memset(m,0,sizeof(*m));
    if (cgs.gametype != GT_RACE) return;
    /* Retail registrations 10023a00..10023ca0, 10023e19, 1002409b. */
    m->start = trap_R_RegisterModel("models/flag3/g_flag3.md3");
    m->checkpoint = trap_R_RegisterModel("models/flag3/b_flag3.md3");
    m->finish = trap_R_RegisterModel("models/flag3/f_flag3.md3");
    m->inactive = trap_R_RegisterModel("models/flag3/d_flag3.md3");
    m->base = trap_R_RegisterModel("models/powerups/domination/dompoint.md3");
    m->redSkin = trap_R_RegisterSkin("models/powerups/domination/domred.skin");
    m->blueSkin = trap_R_RegisterSkin("models/powerups/domination/domblue.skin");
    m->neutralSkin = trap_R_RegisterSkin("models/powerups/domination/domntrl.skin");
    m->icons[0] = trap_R_RegisterShaderNoMip("gfx/2d/race/start");
    m->icons[1] = trap_R_RegisterShaderNoMip("gfx/2d/race/checkpoint");
    m->icons[2] = trap_R_RegisterShaderNoMip("gfx/2d/race/finish");
}

static qboolean RacePointView(const centity_t *cent, racePointView_t *view) {
    const entityState_t *s = &cent->currentState;
    const cgRaceMedia_t *m = &cgs.raceMedia;
    qboolean selected, start, spectator;
    if (!cg.snap || cgs.gametype != GT_RACE || s->eType != ET_GENERAL ||
        s->modelindex <= 0 || s->modelindex >= MAX_MODELS) return qfalse;
    view->model = cgs.gameModels[s->modelindex];
    if (!view->model) return qfalse;
    selected = s->number == cg.race.nextPoint;
    start = view->model == m->start;
    if (!selected && !start && view->model != m->checkpoint &&
        view->model != m->finish && view->model != m->inactive) return qfalse;
    spectator = (cg.snap->ps.eFlags & EF_SPECTATOR) && !(cg.snap->ps.pm_flags & PMF_FOLLOW);
    view->icon = -1;
    view->hide = qfalse;
    if (selected) {
        qhandle_t selectedModel = s->modelindex2 ? m->finish : spectator ? m->inactive : m->checkpoint;
        if (selectedModel) view->model = selectedModel;
        if (!spectator) view->icon = s->modelindex2 ? 2 : 1;
    } else if (start && cg.race.nextPoint == -1) {
        if (!spectator) view->icon = 0;
    } else if (s->number != cg.race.followingPoint &&
               (view->model == m->inactive || view->model == m->finish)) {
        view->hide = cg.race.active;
    }
    return qtrue;
}

qboolean CG_DrawRacePoint(const centity_t *cent) {
    racePointView_t view;
    const cgRaceMedia_t *m = &cgs.raceMedia;
    refEntity_t entity;
    int effects = cg_itemFx.integer < 0 ? 0 : cg_itemFx.integer > 7 ? 7 : cg_itemFx.integer;
    if (!RacePointView(cent,&view)) return qfalse;
    memset(&entity,0,sizeof(entity));
    VectorCopy(cent->lerpOrigin,entity.origin);
    VectorCopy(cent->lerpOrigin,entity.oldorigin);
    /* Floating spawnflag one omits the floor projector. Hidden later flags
     * still have their base, as at 10015a19 before 10015eb2. */
    if (!(cent->currentState.generic1 & 1) && m->base) {
        qboolean spectator = (cg.snap->ps.eFlags & EF_SPECTATOR) && !(cg.snap->ps.pm_flags & PMF_FOLLOW);
        entity.hModel = m->base;
        entity.origin[2] -= 16;
        AxisClear(entity.axis);
        entity.customSkin = view.model == m->finish ? m->redSkin :
            !spectator && view.model == m->checkpoint ? m->blueSkin : m->neutralSkin;
        trap_R_AddRefEntityToScene(&entity);
    }
    if (view.hide || !view.model) return qtrue;
    entity.hModel = view.model;
    entity.customSkin = 0;
    entity.frame = entity.oldframe = cent->currentState.frame;
    VectorCopy(cent->lerpOrigin,entity.origin);
    /* Retail 100158bc: a 1024 ms bob, amplitude 6, offset 4; itemFx bit
     * two selects the faster shared rotation. Other general models retain
     * the baseline orientation and submission path. */
    if (effects & 1)
        entity.origin[2] += 4 + 6*sinf((cg.time & 1023)*(2*M_PI/1024.0f));
    AxisCopy((effects & 2) ? cg.autoAxisFast : cg.autoAxis,entity.axis);
    if (cent->currentState.number == cg.snap->ps.clientNum) entity.renderfx |= RF_THIRD_PERSON;
    trap_R_AddRefEntityToScene(&entity);
    return qtrue;
}

void CG_DrawRaceMarkers(void) {
    static const vec4_t colors[3] = {{0,1,0,1},{.2f,.6f,1,1},{1,.2f,.2f,1}};
    int i, count = 0;
    float worldWidth, minimum;
    if (!cg.snap || cgs.gametype != GT_RACE || cg.refdef.height <= 0) return;
    CG_WorldMarkerWidths(1, &worldWidth, &minimum);
    for (i = 0; i < cg.snap->numEntities && i < MAX_ENTITIES_IN_SNAPSHOT && count < 32; ++i) {
        int number = cg.snap->entities[i].number;
        const centity_t *cent;
        racePointView_t view;
        vec3_t point;
        float size, x, y;
        if (number < 0 || number >= MAX_GENTITIES) continue;
        cent = &cg_entities[number];
        if (!cent->currentValid || !RacePointView(cent,&view) || view.icon < 0 ||
            !cgs.raceMedia.icons[view.icon]) continue;
        ++count;
        /* 100158f9 raises the marker 64 units above the unbobbed origin.
         * CG_ProjectWorldMarker follows 10011680; these are navigational
         * markers through geometry. Matching-position retail captures
         * confirmed the bottom anchor (not vertical centering). */
        VectorCopy(cent->lerpOrigin,point); point[2] += 64;
        if (!CG_ProjectWorldMarker(point,worldWidth,minimum,&x,&y,&size)) continue;
        trap_R_SetColor(colors[view.icon]);
        trap_R_DrawStretchPic(x,y,size,size,0,0,1,1,cgs.raceMedia.icons[view.icon]);
        trap_R_SetColor(NULL);
    }
}
