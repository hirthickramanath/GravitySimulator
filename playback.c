/* playback.c - reads data.csv and plays the run back in 3D
   Gravity Simulator - P L Hirthick Ramanath (26PT20), Svithin S (26PT35) */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gravity.h"

static FRAME f[FRAMES];
static BODY b[2];
static Model model[2];
static Texture tex[2];
static Color col[2];
static float radius[2];
static double worldScale;             /* metres -> 3D units */
static double vmax;                   /* fastest speed in the run, for the arrow lengths */
static double timeDiv;
static const char *timeUnit;
static int orbitType;
static int loaded = 0;
static int backTo = SCREEN_MAIN;

static float pos = 0;                 /* the current frame; a float so 0.5x speed works */
static int paused = 0;
static int speedIndex = 2;
static const float speeds[5] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
static int showVectors = 0;
static int dragging = 0;
static int scrubbing = 0;
static float spin = 0;
static float yaw = 0.6f, pitch = 0.54f, zoom = 1.0f;

/* body i in frame k as a 3D point; the simulation's x-y plane is laid flat */
static Vector3 toWorld(int k, int i) {
    return (Vector3){ (float)(f[k].x[i] * worldScale), 0.0f, (float)(-f[k].y[i] * worldScale) };
}

/* reads data.csv into 1800 frames and builds the two spheres. back is where the Back button goes.
   loadPlayback(NULL, 0) just frees the spheres. returns 1 if it worked */
int loadPlayback(const char *filename, int back) {
    FILE *fp;
    char line[512], name[20], symbol;
    int rows = 0, frame = 0, step;
    double t, mass, x, y, vx, vy, r, v, force, KE, PE, TE, closest;
    double x0 = 0, y0 = 0, vx0 = 0, vy0 = 0;
    double farthest = 1.0;

    if (loaded) {
        for(int i=0; i<2; i++) {
            UnloadModel(model[i]);
            UnloadTexture(tex[i]);
        }
        loaded = 0;
    }
    if (filename == NULL)
        return 0;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return 0;

    /* count the rows first (not the header), then go back and read them */
    while (fgets(line, sizeof(line), fp) != NULL)
        rows++;
    rows--;
    int lastStep = rows / 2 - 1;
    rewind(fp);
    if (lastStep < 0 || fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return 0;
    }

    for(int row=0; row < rows; row++) {
        if (fscanf(fp, " %d,%lf,%19[^,],%c,%le,%le,%le,%le,%le,%le,%le,%le,%le,%le,%le", &step, &t, name, &symbol, &mass, &x, &y, &vx, &vy, &r, &v, &force, &KE, &PE, &TE) != 15)
            break;

        int i = row % 2;
        if (step == 0) {
            strcpy(b[i].name, name);
            b[i].symbol = symbol;
            b[i].mass = mass;
            b[i].x = x;
            b[i].y = y;
            b[i].vx = vx;
            b[i].vy = vy;
        }

        if (i == 0) {                 /* body 1: hold on to it until body 2's row arrives */
            x0 = x;
            y0 = y;
            vx0 = vx;
            vy0 = vy;
            continue;
        }

        /* we only keep the steps that land on one of the 1800 frames */
        while (frame < FRAMES && (long long)frame * lastStep / (FRAMES - 1) == step) {
            f[frame].t = t;
            f[frame].x[0] = x0;
            f[frame].y[0] = y0;
            f[frame].x[1] = x;
            f[frame].y[1] = y;
            f[frame].vx[0] = vx0;
            f[frame].vy[0] = vy0;
            f[frame].vx[1] = vx;
            f[frame].vy[1] = vy;
            f[frame].r = r;
            f[frame].f = force;
            f[frame].v = v;
            f[frame].TE = TE;
            frame++;
        }
    }
    fclose(fp);
    if (frame < FRAMES)
        return 0;

    /* measure everything from the centre of mass, so a pair like Earth + Moon stays in the middle */
    double M = b[0].mass + b[1].mass;
    vmax = 1;
    for(int j=0; j<FRAMES; j++) {
        double cx = (b[0].mass * f[j].x[0] + b[1].mass * f[j].x[1]) / M;
        double cy = (b[0].mass * f[j].y[0] + b[1].mass * f[j].y[1]) / M;
        double cvx = (b[0].mass * f[j].vx[0] + b[1].mass * f[j].vx[1]) / M;
        double cvy = (b[0].mass * f[j].vy[0] + b[1].mass * f[j].vy[1]) / M;
        for(int i=0; i<2; i++) {
            f[j].x[i] -= cx;
            f[j].y[i] -= cy;
            f[j].vx[i] -= cvx;
            f[j].vy[i] -= cvy;
            double speed = sqrt(f[j].vx[i] * f[j].vx[i] + f[j].vy[i] * f[j].vy[i]);
            if (speed > vmax)
                vmax = speed;
            if (fabs(f[j].x[i]) > farthest)
                farthest = fabs(f[j].x[i]);
            if (fabs(f[j].y[i]) > farthest)
                farthest = fabs(f[j].y[i]);
        }
    }
    worldScale = 9.0 / farthest;

    /* pick a time unit that suits how long the run is */
    double end = f[FRAMES - 1].t;
    if (end < 7200) {
        timeDiv = 60;
        timeUnit = "min";
    }
    else if (end < 172800) {
        timeDiv = 3600;
        timeUnit = "hours";
    }
    else {
        timeDiv = 86400;
        timeUnit = "days";
    }
    orbitType = checkOrbit(b, NULL, &closest);

    /* the checker pattern is only there so you can see the spheres spin */
    for(int i=0; i<2; i++) {
        col[i] = getBodyColor(b[i].name, i);
        radius[i] = 0.3f + 0.08f * (float)log10(b[i].mass / 1e14);
        if (radius[i] < 0.3f)
            radius[i] = 0.3f;

        Color dark = { (unsigned char)(col[i].r * 0.55f), (unsigned char)(col[i].g * 0.55f), (unsigned char)(col[i].b * 0.55f), 255 };
        Image img = GenImageChecked(256, 128, 8, 4, col[i], dark);
        tex[i] = LoadTextureFromImage(img);
        UnloadImage(img);

        model[i] = LoadModelFromMesh(GenMeshSphere(1.0f, 32, 32));
        model[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex[i];
    }

    pos = 0;
    paused = 0;
    speedIndex = 2;
    showVectors = 0;
    dragging = 0;
    scrubbing = 0;
    yaw = 0.6f;
    pitch = 0.54f;
    zoom = 1.0f;
    backTo = back;
    loaded = 1;
    return 1;
}

/* the run screen: handles keys and mouse, draws the current frame, then moves forward */
int drawPlayback(void) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int next = SCREEN_RUN;
    float scale = getScale();
    Vector2 mouse = GetMousePosition();
    Rectangle bar = { 20 * scale, screenH - 30 * scale, screenW - 40 * scale, 20 * scale };

    if (!loaded)
        return backTo;

    if (IsKeyPressed(KEY_SPACE))
        paused = !paused;
    if (IsKeyPressed(KEY_UP) && speedIndex < 4)
        speedIndex++;
    if (IsKeyPressed(KEY_DOWN) && speedIndex > 0)
        speedIndex--;
    if (IsKeyPressed(KEY_V))
        showVectors = !showVectors;

    /* drag the bar to jump in time, drag anywhere else to rotate, wheel to zoom */
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, bar))
            scrubbing = 1;
        else if (mouse.y > 80 * scale && mouse.y < screenH - 60 * scale)
            dragging = 1;
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        dragging = 0;
        scrubbing = 0;
    }

    if (scrubbing) {
        float part = (mouse.x - bar.x) / bar.width;
        if (part < 0)
            part = 0;
        if (part > 1)
            part = 1;
        pos = part * (FRAMES - 1);
        cursorKind = MOUSE_CURSOR_POINTING_HAND;
    }
    else if (dragging) {
        Vector2 moved = GetMouseDelta();
        yaw -= moved.x * 0.005f;
        pitch += moved.y * 0.005f;
        if (pitch < 0.1f)
            pitch = 0.1f;
        if (pitch > 1.45f)
            pitch = 1.45f;
        cursorKind = MOUSE_CURSOR_RESIZE_ALL;
    }
    else if (CheckCollisionPointRec(mouse, bar)) {
        cursorKind = MOUSE_CURSOR_POINTING_HAND;
    }

    zoom *= 1.0f - GetMouseWheelMove() * 0.1f;
    if (zoom < 0.4f)
        zoom = 0.4f;
    if (zoom > 2.5f)
        zoom = 2.5f;

    /* move the camera back in narrow windows so the whole orbit still fits */
    float aspect = (float)screenW / screenH;
    float dist = 25.6f * zoom;
    if (aspect < 1100.0f / 700.0f)
        dist *= (1100.0f / 700.0f) / aspect;

    spin += 1.5f;
    yaw += 0.0015f;

    Camera3D camera = { 0 };
    camera.position = (Vector3){ dist * cosf(pitch) * sinf(yaw), dist * sinf(pitch), dist * cosf(pitch) * cosf(yaw) };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    int k = (int)pos;

    BeginMode3D(camera);
    for(int i=0; i<2; i++) {
        /* the trail fades out the further back it goes */
        for(int j=1; j<=k; j++)
            DrawLine3D(toWorld(j - 1, i), toWorld(j, i), Fade(col[i], 0.08f + 0.6f * j / (k + 1)));

        float turn = spin;
        if (i == 0)
            turn = spin * 0.6f;
        DrawModelEx(model[i], toWorld(k, i), (Vector3){ 0.0f, 1.0f, 0.0f }, turn, (Vector3){ radius[i], radius[i], radius[i] }, WHITE);

        if (showVectors) {
            /* green is the velocity, red points along gravity's pull */
            Vector3 here = toWorld(k, i);
            Vector3 other = toWorld(k, 1 - i);
            double speed = sqrt(f[k].vx[i] * f[k].vx[i] + f[k].vy[i] * f[k].vy[i]);
            if (speed > 0) {
                float len = 2.5f * (float)(speed / vmax);
                Vector3 tip = { here.x + len * (float)(f[k].vx[i] / speed), 0, here.z - len * (float)(f[k].vy[i] / speed) };
                DrawLine3D(here, tip, COL_GREEN);
                DrawSphere(tip, 0.08f, COL_GREEN);
            }
            float gapX = other.x - here.x;
            float gapZ = other.z - here.z;
            float gap = sqrtf(gapX * gapX + gapZ * gapZ);
            if (gap > 0) {
                Vector3 tip = { here.x + 1.5f * gapX / gap, 0, here.z + 1.5f * gapZ / gap };
                DrawLine3D(here, tip, COL_RED);
                DrawSphere(tip, 0.08f, COL_RED);
            }
        }
    }
    EndMode3D();

    int fontSize = (int)(20 * scale);
    int small = (int)fmaxf(13, 16 * scale);

    /* names floating above the spheres */
    for(int i=0; i<2; i++) {
        Vector3 above = toWorld(k, i);
        above.y = radius[i] + 0.5f;
        Vector2 onScreen = GetWorldToScreen(above, camera);
        drawTextCentered(b[i].name, onScreen.x, onScreen.y - small, small, col[i]);
    }

    /* the readout panel on the left */
    char value[7][32];
    const char *label[7] = { "Time", "Distance", "Force", "Speed", "Energy", "Energy drift", "Orbit" };
    const char *unit[7] = { timeUnit, "km", "N", "km/s", "J", "%", "" };
    snprintf(value[0], 32, "%.1f", f[k].t / timeDiv);
    snprintf(value[1], 32, "%.0f", f[k].r / 1000);
    snprintf(value[2], 32, "%.3e", f[k].f);
    snprintf(value[3], 32, "%.2f", f[k].v / 1000);
    snprintf(value[4], 32, "%.3e", f[k].TE);
    if (f[0].TE != 0)
        snprintf(value[5], 32, "%+.4f", (f[k].TE - f[0].TE) / fabs(f[0].TE) * 100.0);
    else
        snprintf(value[5], 32, "-");
    if (orbitType == ORBIT_CLOSED)
        snprintf(value[6], 32, "closed");
    else if (orbitType == ORBIT_ESCAPE)
        snprintf(value[6], 32, "escaping");
    else
        snprintf(value[6], 32, "collision");

    float panelX = 20 * scale;
    float panelY = 96 * scale;
    DrawRectangleRounded((Rectangle){ panelX, panelY, 340 * scale, 252 * scale }, 0.08f, 8, Fade(COL_PANEL, 0.85f));
    DrawText(b[0].name, (int)(panelX + 16 * scale), (int)(panelY + 14 * scale), fontSize, col[0]);
    DrawText(b[1].name, (int)(panelX + 40 * scale) + MeasureText(b[0].name, fontSize), (int)(panelY + 14 * scale), fontSize, col[1]);

    for(int i=0; i<7; i++) {
        int y = (int)(panelY + 50 * scale + 27 * scale * i);
        Color valueCol = COL_TEXT;
        if (i == 5)
            valueCol = COL_GOLD;
        if (i == 6 && orbitType != ORBIT_CLOSED)
            valueCol = COL_RED;
        DrawText(label[i], (int)(panelX + 16 * scale), y, fontSize, COL_MUTED);
        DrawText(value[i], (int)(panelX + 250 * scale) - MeasureText(value[i], fontSize), y, fontSize, valueCol);
        DrawText(unit[i], (int)(panelX + 258 * scale), y, fontSize, COL_MUTED);
    }

    /* speed or PAUSED at the top right, the help line and the progress bar at the bottom */
    if (paused) {
        DrawText("PAUSED", screenW - (int)(20 * scale) - MeasureText("PAUSED", fontSize), (int)(96 * scale), fontSize, COL_RED);
    }
    else {
        const char *status = TextFormat("%gx speed", speeds[speedIndex]);
        DrawText(status, screenW - (int)(20 * scale) - MeasureText(status, fontSize), (int)(96 * scale), fontSize, COL_GOLD);
    }
    if (showVectors)
        DrawText("green = velocity   red = pull of gravity", screenW - (int)(20 * scale) - MeasureText("green = velocity   red = pull of gravity", small), (int)(126 * scale), small, COL_MUTED);
    DrawText("Space: pause   Up/Down: speed   V: arrows   Drag: rotate   Wheel: zoom   Drag bar: jump", (int)(20 * scale), screenH - (int)(54 * scale), small, COL_MUTED);

    DrawRectangleRounded((Rectangle){ bar.x, bar.y + 6 * scale, bar.width, 8 * scale }, 1.0f, 6, COL_PANEL);
    DrawRectangleRounded((Rectangle){ bar.x, bar.y + 6 * scale, bar.width * (k + 1) / FRAMES, 8 * scale }, 1.0f, 6, COL_GOLD);
    DrawCircleV((Vector2){ bar.x + bar.width * (k + 1) / FRAMES, bar.y + 10 * scale }, 7 * scale, COL_GOLD);

    if (k >= FRAMES - 1) {
        if (drawButton((Rectangle){ screenW / 2 - 110 * scale, screenH - 120 * scale, 220 * scale, 48 * scale }, "Replay", COL_GOLD, ICON_REPLAY))
            loadPlayback(DATA_FILE, backTo);
    }
    if (drawHeader("SIMULATION")) {
        next = backTo;
        loadPlayback(NULL, 0);
    }

    if (!paused && !scrubbing)
        pos += speeds[speedIndex];
    if (pos > FRAMES - 1)
        pos = FRAMES - 1;
    return next;
}
