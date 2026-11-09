#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gravity.h"

/* ---------- everything the playback needs, kept between frames ---------- */
static FRAME f[FRAMES];
static BODY b[2];
static Model model[2];
static Texture tex[2];
static Color col[2];
static float radius[2];
static double scale;                /* metres -> 3D units */
static double vmax = 1;             /* fastest speed in the run, for arrow lengths */
static int loaded = 0;

static double timeDiv = 86400;      /* time readout unit: days, hours or minutes */
static const char *timeUnit = "days";

/* playback controls */
static float pos = 0;               /* current frame (a float, so 0.5x speed works) */
static int paused = 0;
static int speedIndex = 2;
static const float speeds[5] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
static int showVectors = 0;
static int dragging = 0, scrubbing = 0;
static float spin = 0.0f;
static float yaw = 0.6f, pitch = 0.54f, zoom = 1.0f;   /* camera */

/* reads data.csv into 1800 frames and makes the two spheres.
   returns 1 if it worked, 0 if not */
int loadPlayback(const char *filename) {
    FILE *fp;
    char line[512], name[20], symbol;
    int rows = 0, lastStep = 0, frame = 0, step;
    double t, mass, x, y, vx, vy, r, v, force, KE, PE, TE;
    double x0 = 0, y0 = 0, vx0 = 0, vy0 = 0;
    double farthest = 1.0;

    unloadPlayback();

    fp = fopen(filename, "r");
    if (fp == NULL)
        return 0;

    /* first pass: count the rows */
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp) != NULL)
        rows++;
    lastStep = rows / 2 - 1;
    if (lastStep < 0) {
        fclose(fp);
        return 0;
    }

    /* second pass: read every row */
    rewind(fp);
    fgets(line, sizeof(line), fp);

    for (int row=0; row < rows; row++) {
        if (fscanf(fp, " %d,%lf,%19[^,],%c,%le,%le,%le,%le,%le,%le,%le,%le,%le,%le,%le", &step, &t, name, &symbol,
                   &mass, &x, &y, &vx, &vy, &r, &v, &force, &KE, &PE, &TE) != 15)
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

        if (i == 0) {                                  /* body 1: keep it until body 2 arrives */
            x0 = x;
            y0 = y;
            vx0 = vx;
            vy0 = vy;
            continue;
        }

        /* the step is complete: is it one of the 1800 frames? */
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

    /* follow the pair: measure every position and velocity from their centre of mass,
       so a pair that drifts (like Earth + Moon) stays in the middle of the view */
    double M = b[0].mass + b[1].mass;
    vmax = 1;
    for (int j=0; j<FRAMES; j++) {
        double cx = (b[0].mass * f[j].x[0] + b[1].mass * f[j].x[1]) / M;
        double cy = (b[0].mass * f[j].y[0] + b[1].mass * f[j].y[1]) / M;
        double cvx = (b[0].mass * f[j].vx[0] + b[1].mass * f[j].vx[1]) / M;
        double cvy = (b[0].mass * f[j].vy[0] + b[1].mass * f[j].vy[1]) / M;
        for (int i=0; i<2; i++) {
            f[j].x[i] -= cx;
            f[j].y[i] -= cy;
            f[j].vx[i] -= cvx;
            f[j].vy[i] -= cvy;
            double sp = sqrt(f[j].vx[i] * f[j].vx[i] + f[j].vy[i] * f[j].vy[i]);
            if (sp > vmax) vmax = sp;
            if (fabs(f[j].x[i]) > farthest) farthest = fabs(f[j].x[i]);
            if (fabs(f[j].y[i]) > farthest) farthest = fabs(f[j].y[i]);
        }
    }
    scale = 9.0 / farthest;

    /* pick a time unit that suits the length of this run */
    if (f[FRAMES - 1].t < 7200) {
        timeDiv = 60;
        timeUnit = "min";
    }
    else if (f[FRAMES - 1].t < 172800) {
        timeDiv = 3600;
        timeUnit = "hours";
    }
    else {
        timeDiv = 86400;
        timeUnit = "days";
    }

    /* the two spheres: colour, size, checker texture so the spin shows */
    for (int i=0; i<2; i++) {
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
    dragging = scrubbing = 0;
    yaw = 0.6f;
    pitch = 0.54f;
    zoom = 1.0f;
    loaded = 1;
    return 1;
}

/* a 3D point for body i in frame k */
static Vector3 toWorld(int k, int i) {
    return (Vector3){ (float)(f[k].x[i] * scale), 0.0f, (float)(-f[k].y[i] * scale) };
}

/* handles the keys and mouse, draws the current frame sized to the window,
   then moves forward. returns 1 once the last frame has been reached */
int drawPlayback(void) {
    if (!loaded)
        return 1;

    int W = GetScreenWidth();
    int H = GetScreenHeight();
    float u = getScale();
    Vector2 mouse = GetMousePosition();
    Rectangle bar = { 20 * u, H - 30 * u, W - 40 * u, 20 * u };      /* progress bar, with room to grab */

    /* ---------- keys ---------- */
    if (IsKeyPressed(KEY_SPACE))
        paused = !paused;
    if (IsKeyPressed(KEY_UP) && speedIndex < 4)
        speedIndex++;
    if (IsKeyPressed(KEY_DOWN) && speedIndex > 0)
        speedIndex--;
    if (IsKeyPressed(KEY_V))
        showVectors = !showVectors;

    /* ---------- mouse: drag the bar to jump, drag elsewhere to rotate, wheel to zoom ---------- */
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, bar))
            scrubbing = 1;
        else if (mouse.y > 80 * u && mouse.y < H - 60 * u)
            dragging = 1;
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        dragging = scrubbing = 0;

    if (scrubbing) {
        float frac = (mouse.x - bar.x) / bar.width;
        if (frac < 0) frac = 0;
        if (frac > 1) frac = 1;
        pos = frac * (FRAMES - 1);
        cursorKind = CURSOR_HAND;
    }
    else if (dragging) {
        Vector2 d = GetMouseDelta();
        yaw -= d.x * 0.005f;
        pitch += d.y * 0.005f;
        if (pitch < 0.1f)  pitch = 0.1f;
        if (pitch > 1.45f) pitch = 1.45f;
        cursorKind = CURSOR_DRAG;
    }
    else if (CheckCollisionPointRec(mouse, bar))
        cursorKind = CURSOR_HAND;

    zoom *= 1.0f - GetMouseWheelMove() * 0.1f;
    if (zoom < 0.4f) zoom = 0.4f;
    if (zoom > 2.5f) zoom = 2.5f;

    /* ---------- camera: pull back in narrow windows so the orbit still fits ---------- */
    float aspect = (float)W / H;
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

    /* ---------- 3D scene ---------- */
    BeginMode3D(camera);
        for (int i=0; i<2; i++) {
            /* trail: older parts fade out */
            for (int j=1; j<=k; j++)
                DrawLine3D(toWorld(j - 1, i), toWorld(j, i), Fade(col[i], 0.08f + 0.6f * j / (k + 1)));

            DrawModelEx(model[i], toWorld(k, i), (Vector3){ 0.0f, 1.0f, 0.0f },
                        spin * (i == 0 ? 0.6f : 1.0f), (Vector3){ radius[i], radius[i], radius[i] }, WHITE);

            if (showVectors) {
                /* green = velocity (length shows speed), red = gravity's pull towards the other body */
                Vector3 p3 = toWorld(k, i);
                float len = 2.5f * (float)(sqrt(f[k].vx[i] * f[k].vx[i] + f[k].vy[i] * f[k].vy[i]) / vmax);
                double sp = sqrt(f[k].vx[i] * f[k].vx[i] + f[k].vy[i] * f[k].vy[i]);
                if (sp > 0) {
                    Vector3 tip = { p3.x + len * (float)(f[k].vx[i] / sp), 0, p3.z - len * (float)(f[k].vy[i] / sp) };
                    DrawLine3D(p3, tip, COL_GREEN);
                    DrawSphere(tip, 0.08f, COL_GREEN);
                }
                Vector3 other = toWorld(k, 1 - i);
                float ox = other.x - p3.x, oz = other.z - p3.z, od = sqrtf(ox * ox + oz * oz);
                if (od > 0) {
                    Vector3 tip = { p3.x + 1.5f * ox / od, 0, p3.z + 1.5f * oz / od };
                    DrawLine3D(p3, tip, COL_RED);
                    DrawSphere(tip, 0.08f, COL_RED);
                }
            }
        }
    EndMode3D();

    int fs = (int)(20 * u);
    int small = (int)fmaxf(13, 16 * u);

    /* ---------- names floating above the spheres ---------- */
    for (int i=0; i<2; i++) {
        Vector3 above = toWorld(k, i);
        above.y = radius[i] + 0.5f;
        Vector2 sp = GetWorldToScreen(above, camera);
        drawTextCentered(b[i].name, sp.x, sp.y - small, small, col[i]);
    }

    /* ---------- readout panel ---------- */
    float px = 20 * u, py = 96 * u;
    DrawRectangleRounded((Rectangle){ px, py, 340 * u, 225 * u }, 0.08f, 8, Fade(COL_PANEL, 0.85f));

    DrawText(b[0].name, (int)(px + 16 * u), (int)(py + 14 * u), fs, col[0]);
    DrawText(b[1].name, (int)(px + 16 * u) + MeasureText(b[0].name, fs) + (int)(24 * u), (int)(py + 14 * u), fs, col[1]);

    char value[6][32];
    const char *unit[6];
    snprintf(value[0], 32, "%.1f", f[k].t / timeDiv);
    unit[0] = timeUnit;
    unit[1] = formatDistance(f[k].r, value[1]);
    snprintf(value[2], 32, "%.3e", f[k].f);
    unit[2] = "N";
    unit[3] = formatSpeed(f[k].v, value[3]);
    snprintf(value[4], 32, "%.3e", f[k].TE);
    unit[4] = "J";
    snprintf(value[5], 32, "%+.4f", (f[k].TE - f[0].TE) / fabs(f[0].TE) * 100.0);
    unit[5] = "%";
    const char *label[6] = { "Time", "Distance", "Force", "Speed", "Energy", "Energy drift" };

    for (int i=0; i<6; i++) {
        int y = (int)(py + 50 * u + 27 * u * i);
        Color vc = (i == 5) ? COL_GOLD : COL_TEXT;
        DrawText(label[i], (int)(px + 16 * u), y, fs, COL_MUTED);
        DrawText(value[i], (int)(px + 250 * u) - MeasureText(value[i], fs), y, fs, vc);
        DrawText(unit[i], (int)(px + 258 * u), y, fs, COL_MUTED);
    }

    /* ---------- status, help line, progress bar ---------- */
    const char *status = paused ? "PAUSED" : TextFormat("%gx speed", speeds[speedIndex]);
    DrawText(status, W - (int)(20 * u) - MeasureText(status, fs), (int)(96 * u), fs, paused ? COL_RED : COL_GOLD);
    DrawText("Space: pause   Up/Down: speed   V: arrows   Drag: rotate   Wheel: zoom   Drag bar: jump",
             (int)(20 * u), H - (int)(54 * u), small, COL_MUTED);
    if (showVectors)
        DrawText("green = velocity   red = pull of gravity", W - (int)(20 * u) - MeasureText("green = velocity   red = pull of gravity", small),
                 (int)(126 * u), small, COL_MUTED);

    DrawRectangleRounded((Rectangle){ bar.x, bar.y + 6 * u, bar.width, 8 * u }, 1.0f, 6, COL_PANEL);
    DrawRectangleRounded((Rectangle){ bar.x, bar.y + 6 * u, bar.width * (k + 1) / FRAMES, 8 * u }, 1.0f, 6, COL_GOLD);
    DrawCircleV((Vector2){ bar.x + bar.width * (k + 1) / FRAMES, bar.y + 10 * u }, 7 * u, COL_GOLD);

    int finished = (k >= FRAMES - 1);

    /* ---------- move forward ---------- */
    if (!paused && !scrubbing)
        pos += speeds[speedIndex];
    if (pos > FRAMES - 1)
        pos = FRAMES - 1;

    return finished;
}

/* frees the spheres (safe to call even if nothing is loaded) */
void unloadPlayback(void) {
    if (!loaded)
        return;
    for (int i=0; i<2; i++) {
        UnloadModel(model[i]);
        UnloadTexture(tex[i]);
    }
    loaded = 0;
}
