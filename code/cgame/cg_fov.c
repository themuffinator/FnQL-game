/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"

/* Retail evidence: cgame 1004d5b0/1004d610 (commands), 1004d640
 * (view/command FOV), 1004c5f0 (aspect presets), 10011520 (death reset).
 * The projection policy is independent of camera position and prediction. */
static float CG_BoundedFov(float value, float maximum) {
    if (!(value >= 1.0f)) return 1.0f;
    return value > maximum ? maximum : value;
}

static float CG_ProjectionAngle(float value) {
    if (!(value > 0.0f)) return 0.01f;
    return value >= 180.0f ? 179.99f : value;
}

static float CG_VerticalFov(float horizontal, float aspect) {
    return (float)(atan(tan(horizontal * M_PI / 360.0) / aspect) * 360.0 / M_PI);
}

static float CG_HorizontalFov(float vertical, float aspect) {
    return (float)(atan(tan(vertical * M_PI / 360.0) * aspect) * 360.0 / M_PI);
}

void CG_ZoomDown_f(void) {
    if (cg.predictedPlayerState.pm_type == PM_DEAD ||
        cg.predictedPlayerState.pm_type == PM_FREEZE) return;
    if (cg.zoomed && !cg_zoomToggle.integer) return;
    cg.zoomed = !cg.zoomed;
    cg.zoomTime = cg.time;
}

void CG_ZoomUp_f(void) {
    if (!cg.zoomed || cg_zoomToggle.integer) return;
    cg.zoomed = qfalse;
    cg.zoomTime = cg.time;
}

void CG_UpdateZoomState(void) {
    int type = cg.predictedPlayerState.pm_type;
    if (type == PM_INTERMISSION || (cg_zoomOutOnDeath.integer &&
        (type == PM_DEAD || type == PM_FREEZE))) cg.zoomed = qfalse;
}

int CG_CalcFov(void) {
    float horizontal, vertical, commandFov, aspect, sensitivity;
    double elapsed = (double)cg.time - cg.zoomTime;
    int contents;
    long preset;
    qboolean inwater;
    char aspectText[32] = {0};

    cgs.fov = CG_BoundedFov(cg_fov.value, 130.0f);
    cgs.zoomFov = CG_BoundedFov(cg_zoomFov.value, 130.0f);
    aspect = cg.refdef.width > 0 && cg.refdef.height > 0
        ? (float)cg.refdef.width / cg.refdef.height : 4.0f / 3.0f;

    if (cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
        horizontal = commandFov = 90.0f;
    } else if (cg.snap && cg.snap->ps.fov && cg_specFov.integer &&
               ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback)) {
        // Retail trusts this integer; keep malformed recordings away from
        // singular/negative projections while retaining valid recorded FOVs.
        horizontal = commandFov = CG_BoundedFov((float)cg.snap->ps.fov, 179.0f);
    } else {
        horizontal = commandFov = cg.zoomed ? cgs.zoomFov : cgs.fov;
        if (cg_zoomScaling.integer && elapsed >= 0 && elapsed <= ZOOM_TIME) {
            float fraction = (float)(elapsed / ZOOM_TIME);
            float start = cg.zoomed ? cgs.fov : cgs.zoomFov;
            horizontal = start + fraction * (horizontal - start);
            // Retail's command value remains the zoom target throughout
            // either transition, including its final 150 ms frame.
            commandFov = cgs.zoomFov;
        }
    }
    cg.userCmdFov = (int)commandFov;
    vertical = CG_VerticalFov(horizontal, 4.0f / 3.0f);

    if (cg_fovAdjust.integer) {
        // Preserve the inherited explicit viewport-aspect option.
        // Based on LordHavoc's code for Darkplaces:
        // http://www.quakeworld.nu/forum/topic/53/what-does-your-qw-look-like/page/30
        horizontal = atan2(tan(horizontal * M_PI / 360.0f) * 0.75f * aspect, 1) * 360.0f / M_PI;
        vertical = CG_VerticalFov(horizontal, aspect);
    }

    contents = CG_PointContents(cg.refdef.vieworg, -1);
    inwater = (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) != 0;
    if (inwater && cg_waterWarp.integer) {
        // One degree at 0.4 Hz. Reduce time before trig for long sessions.
        float wave = (float)sin((cg.time % 2500) * (2.0 * M_PI / 2500.0));
        horizontal += wave;
        vertical -= wave;
    }
    // Extreme/corrupt recorded FOVs plus warp must not cross a projection
    // singularity. Ordinary range-registered local settings never reach it.
    horizontal = CG_ProjectionAngle(horizontal);
    vertical = CG_ProjectionAngle(vertical);
    if (!cg_fovAdjust.integer) {
        trap_Cvar_VariableStringBuffer("r_aspectRatio", aspectText, sizeof(aspectText));
        aspectText[sizeof(aspectText)-1] = '\0';
        preset = strtol(aspectText, NULL, 10);
        // Preset zero retains the base horizontal FOV, even in wide views.
        // Other nonzero values use retail's 5:4 fallback.
        if (preset) horizontal = CG_HorizontalFov(vertical,
            preset == 1 ? 16.0f / 9.0f : preset == 2 ? 16.0f / 10.0f : 5.0f / 4.0f);
    }
    cg.refdef.fov_x = horizontal;
    cg.refdef.fov_y = vertical;

    sensitivity = cg_zoomSensitivity.value;
    if (!cg.zoomed) {
        cg.zoomSensitivity = 1.0f;
    } else if (!(sensitivity > 0.0f)) {
        cg.zoomSensitivity = vertical / 75.0f;
    } else {
        if (sensitivity > 10.0f) sensitivity = 10.0f;
        cg.zoomSensitivity = vertical / CG_VerticalFov(cgs.fov, aspect) * sensitivity;
    }
    return inwater;
}
