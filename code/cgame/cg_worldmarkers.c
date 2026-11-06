/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "cg_local.h"

/* Retail cvar limits 10077990 (cg_poiMaxWidth 16..32) and 100779a8
 * (cg_poiMinWidth 2..16). Ordered tests reject NaN. Retail EV_POI scales
 * both values by 0.75 for neutral flag carriers (0x1001c381). */
void CG_WorldMarkerWidths(float scale, float *worldWidth, float *minimum) {
    float world = cg_poiMaxWidth.value, least = cg_poiMinWidth.value;
    if (!(world >= 16)) world = 16;
    if (world > 32) world = 32;
    if (!(least >= 2)) least = 2;
    if (least > 16) least = 16;
    *worldWidth = world * scale;
    *minimum = least * scale;
}

/* Retail 0x10011680 projects the marker point and a camera-up probe half a
 * world width above it; twice the projected separation is the icon size,
 * with the minimum applied in 640x480 HUD units. Icons are horizontally
 * centered and bottom-anchored at the point (0x100119e3 / 0x10011a42).
 * No occlusion trace is used. */
qboolean CG_ProjectWorldMarker(const vec3_t point, float worldWidth, float minimum,
                               float *x, float *y, float *size) {
    vec3_t edge;
    vec4_t projected, projectedEdge;
    float pixels;
    if (cg.refdef.height <= 0) return qfalse;
    VectorMA(point, worldWidth * .5f, cg.refdef.viewaxis[2], edge);
    if (!trap_R_ProjectPoint(point, projected) || !trap_R_ProjectPoint(edge, projectedEdge))
        return qfalse;
    pixels = 2 * fabsf(projectedEdge[1] - projected[1]);
    if (pixels < minimum * cgs.screenYScale) pixels = minimum * cgs.screenYScale;
    *size = pixels;
    *x = cg.refdef.x + projected[0] - pixels * .5f;
    *y = cg.refdef.y + cg.refdef.height - projected[1] - pixels;
    return (pixels > 0 && pixels < 65536) && (*x > -65536 && *x < 65536) &&
           (*y > -65536 && *y < 65536);
}
