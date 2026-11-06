/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define NEAR(a,b) do { double actual = (a), expected = (b); if (fabs(actual-expected) >= .002 || !isfinite(actual)) { fprintf(stderr,"line %d: %.6f != %.6f\n",__LINE__,actual,expected); exit(1); } } while (0)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_fov, cg_zoomFov, cg_specFov, cg_zoomScaling, cg_zoomSensitivity;
vmCvar_t cg_zoomToggle, cg_zoomOutOnDeath, cg_waterWarp, cg_fovAdjust;
static snapshot_t snapshot;
static const char *preset = "0";
static int contents;
int CG_PointContents(const vec3_t point, int skip) { CHECK(point == cg.refdef.vieworg && skip == -1); return contents; }
void trap_Cvar_VariableStringBuffer(const char *name, char *buffer, int size) {
    CHECK(!strcmp(name,"r_aspectRatio") && size >= 16);
    strcpy(buffer,preset);
}
static void View(float x, float y, int command) {
    CHECK(CG_CalcFov() == !!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)));
    NEAR(cg.refdef.fov_x,x); NEAR(cg.refdef.fov_y,y); CHECK(cg.userCmdFov == command);
}
static void Reset(void) {
    memset(&cg,0,sizeof(cg)); memset(&snapshot,0,sizeof(snapshot));
    cg.snap = &snapshot; cg.time = 1000; cg.refdef.width = 640; cg.refdef.height = 480;
    cg_fov.value = 90; cg_zoomFov.value = 30; cg_zoomSensitivity.value = 1;
    cg_specFov.integer = cg_zoomScaling.integer = cg_zoomOutOnDeath.integer = cg_waterWarp.integer = 1;
    cg_zoomToggle.integer = cg_fovAdjust.integer = contents = 0; preset = "0";
}
int main(void) {
    float warpedX, warpedY;
    int types[] = {PM_NORMAL, PM_NOCLIP, PM_SPECTATOR, PM_DEAD, PM_FREEZE, PM_INTERMISSION};
    int i;
    Reset(); View(90,73.739795,90); NEAR(cg.zoomSensitivity,1);
    // Presets are independent of viewsize and actual drawable dimensions.
    cg.refdef.width = 960; cg.refdef.height = 600;
    View(90,73.739795,90);
    preset = "1"; View(106.260205,73.739795,90);
    preset = "2"; View(100.388858,73.739795,90);
    preset = "3"; View(86.304779,73.739795,90);
    preset = "-1"; View(86.304779,73.739795,90);
    preset = "999999999999999999999999"; View(86.304779,73.739795,90);
    preset = "garbage"; View(90,73.739795,90);
    preset = "2"; cg.refdef.width = 480; cg.refdef.height = 300; View(100.388858,73.739795,90);
    cg_fovAdjust.integer = 1; preset = "0"; View(100.388858,73.739795,90);
    cg.refdef.width = 2560; cg.refdef.height = 1080; View(121.284493,73.739795,90);
    cg.refdef.width = cg.refdef.height = 0; View(90,73.739795,90);

    Reset(); CG_ZoomDown_f(); CHECK(cg.zoomed && cg.zoomTime == 1000);
    View(90,73.739795,30); NEAR(cg.zoomSensitivity,1);
    cg.time = 1075; CG_ZoomDown_f(); CHECK(cg.zoomTime == 1000);
    View(60,46.826449,30); NEAR(cg.zoomSensitivity,.635021);
    cg.time = 1150; View(30,22.725830,30);
    cg.time = 1151; View(30,22.725830,30);
    CG_ZoomUp_f(); CHECK(!cg.zoomed && cg.zoomTime == 1151);
    View(30,22.725830,30); NEAR(cg.zoomSensitivity,1);
    cg.time = 1226; CG_ZoomUp_f(); CHECK(cg.zoomTime == 1151);
    View(60,46.826449,30);
    cg.time = 1301; View(90,73.739795,30);
    cg.time = 1302; View(90,73.739795,90);
    cg_zoomScaling.integer = 0; CG_ZoomDown_f(); View(30,22.725830,30);
    CG_ZoomUp_f(); View(90,73.739795,90);
    cg_zoomToggle.integer = 1; CG_ZoomDown_f(); CHECK(cg.zoomed);
    CG_ZoomUp_f(); CHECK(cg.zoomed); CG_ZoomDown_f(); CHECK(!cg.zoomed);
    // Rewinding/overflowing timestamps settle safely to the current state.
    cg_zoomScaling.integer = 1; cg.zoomTime = INT_MAX; cg.time = INT_MIN;
    View(90,73.739795,90); cg.zoomed = qtrue; View(30,22.725830,30);
    cg.zoomTime = INT_MIN; cg.time = INT_MAX; View(30,22.725830,30);

    Reset(); snapshot.ps.fov = 110; snapshot.ps.pm_flags = PMF_FOLLOW;
    View(110,93.932930,110);
    CG_ZoomDown_f(); View(110,93.932930,110); // recorded camera wins over local zoom
    cg_specFov.integer = 0; View(90,73.739795,30);
    cg_specFov.integer = 1; snapshot.ps.pm_flags = 0; View(90,73.739795,30);
    cg.demoPlayback = qtrue; View(110,93.932930,110);
    snapshot.ps.fov = 0; View(90,73.739795,30);
    snapshot.ps.fov = 110; cg.predictedPlayerState.pm_type = PM_INTERMISSION;
    View(90,73.739795,90);
    // Recorded FOVs need not be within the current local cvar range.
    cg.predictedPlayerState.pm_type = PM_NORMAL; snapshot.ps.fov = 160;
    CG_CalcFov(); CHECK(cg.userCmdFov == 160 && cg.refdef.fov_y < 180);
    snapshot.ps.fov = INT_MAX; CG_CalcFov(); CHECK(cg.userCmdFov == 179);
    snapshot.ps.fov = -1; CG_CalcFov(); CHECK(cg.userCmdFov == 1);

    Reset(); cg.time = 625; contents = CONTENTS_WATER;
    View(91,72.739795,90); warpedX = cg.refdef.fov_x; warpedY = cg.refdef.fov_y;
    cg.time += 2500; View(warpedX,warpedY,90);
    contents = CONTENTS_SLIME; View(warpedX,warpedY,90);
    contents = CONTENTS_LAVA; View(warpedX,warpedY,90);
    preset = "2"; View(99.359964,72.739795,90); // warp vertical before aspect projection
    cg_fovAdjust.integer = 1; cg.refdef.width = 960; cg.refdef.height = 600;
    View(101.388858,72.739795,90); // retain the explicit option's post-projection warp
    cg_fovAdjust.integer = 0;
    cg_waterWarp.integer = 0; View(100.388858,73.739795,90);
    contents = CONTENTS_SOLID; View(100.388858,73.739795,90);

    // Malformed recordings remain finite and renderable even with water warp.
    contents = CONTENTS_WATER; cg_waterWarp.integer = cg.demoPlayback = 1;
    snapshot.ps.fov = INT_MIN; CG_CalcFov();
    CHECK(cg.refdef.fov_x > 0 && cg.refdef.fov_y > 0);
    snapshot.ps.fov = INT_MAX; preset = "0"; CG_CalcFov();
    CHECK(cg.refdef.fov_x < 180 && cg.refdef.fov_y < 180);

    Reset(); cg.zoomed = qtrue; View(30,22.725830,30);
    NEAR(cg.zoomSensitivity,.308189);
    cg_zoomSensitivity.value = 2; CG_CalcFov(); NEAR(cg.zoomSensitivity,.616379);
    cg_zoomSensitivity.value = 0; CG_CalcFov(); NEAR(cg.zoomSensitivity,.303011);
    cg_zoomSensitivity.value = 1; cg.refdef.width = 960; cg.refdef.height = 600;
    preset = "2"; CG_CalcFov(); NEAR(cg.zoomSensitivity,.355031);
    cg_fov.value = NAN; cg_zoomFov.value = INFINITY; cg_zoomSensitivity.value = NAN;
    CG_CalcFov(); CHECK(cgs.fov == 1 && cgs.zoomFov == 130 && isfinite(cg.zoomSensitivity));
    cg.zoomed = qfalse; cg_fov.value = INFINITY; CG_CalcFov(); CHECK(cg.userCmdFov == 130);

    for (i = 0; i < ARRAY_LEN(types); ++i) {
        Reset(); cg.predictedPlayerState.pm_type = types[i];
        CG_ZoomDown_f(); CHECK(cg.zoomed == (types[i] != PM_DEAD && types[i] != PM_FREEZE));
        cg.zoomed = qtrue; cg.zoomTime = 456; CG_UpdateZoomState();
        CHECK(cg.zoomed == (types[i] != PM_DEAD && types[i] != PM_FREEZE && types[i] != PM_INTERMISSION));
        CHECK(cg.zoomTime == 456);
        cg_zoomOutOnDeath.integer = 0; cg.zoomed = qtrue; CG_UpdateZoomState();
        CHECK(cg.zoomed == (types[i] != PM_INTERMISSION));
        cg_zoomToggle.integer = 0; CG_ZoomUp_f(); CHECK(!cg.zoomed);
    }
    puts("cgame FOV, projection, command values and zoom state passed");
    return 0;
}
