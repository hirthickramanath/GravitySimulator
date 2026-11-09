#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "gravity.h"

static BODY b[2];
static PAIR p;
static int runReturn = SCREEN_MAIN;        /* where Back goes from the run screen */

/* checks the orbit, works out dt and steps, runs the physics, then opens
   the playback; returns the next screen */
static int startRun(SETTINGS *s, int returnTo) {
    SETTINGS run = *s;                     /* a copy, so Auto doesn't overwrite the manual values */
    double closest, drift;
    int orbit = classifyOrbit(b, &closest);

    if (run.autoStep)
        chooseTimeStep(b, &run);

    int result = runSimulation(b, &p, &run, &drift);
    if (result == 0) {
        showToast("The two bodies cannot start at the same position", 0);
        return returnTo;
    }
    if (result < 0) {
        showToast("Could not write data.csv", 0);
        return returnTo;
    }
    if (!loadPlayback(DATA_FILE)) {
        showToast("Could not read data.csv", 0);
        return returnTo;
    }

    runReturn = returnTo;
    playSound(SOUND_RUN);

    char near[32];
    const char *unit = formatDistance(closest, near);
    double total = run.dt * run.steps;
    if (orbit == ORBIT_COLLIDE)
        showToast(TextFormat("Warning: they pass within %s %s - a real collision", near, unit), 0);
    else if (total < 7200)
        showToast(TextFormat("dt = %.3g s  x  %d steps  =  %.1f minutes, saved to data.csv", run.dt, run.steps, total / 60), 1);
    else if (total < 172800)
        showToast(TextFormat("dt = %.3g s  x  %d steps  =  %.1f hours, saved to data.csv", run.dt, run.steps, total / 3600), 1);
    else
        showToast(TextFormat("dt = %.0f s  x  %d steps  =  %.1f days, saved to data.csv", run.dt, run.steps, total / 86400), 1);
    return SCREEN_RUN;
}

/* ------------------------------------------- main menu: the solar system */

/* an ellipse drawn as 64 short lines (used for the orbit and Saturn's ring) */
static void drawEllipse(Vector2 c, float rx, float ry, float thick, Color col) {
    for (int i=0; i<64; i++) {
        float a1 = i * 2 * PI / 64, a2 = (i + 1) * 2 * PI / 64;
        DrawLineEx((Vector2){ c.x + rx * cosf(a1), c.y + ry * sinf(a1) },
                   (Vector2){ c.x + rx * cosf(a2), c.y + ry * sinf(a2) }, thick, col);
    }
}

#define MENU_ITEMS 5
static int menuTurn = 0;          /* how many steps the orbit has turned (can go below 0 or past 4) */
static float menuAngle = 0;       /* the orbit's current angle, eases towards menuTurn */

int drawMainMenu(void) {
    int W = GetScreenWidth(), H = GetScreenHeight();
    float u = getScale();
    float t = (float)GetTime();
    int next = SCREEN_MAIN;
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);

    const char *title[MENU_ITEMS] = { "Load & Run", "New Simulation", "Replay Last Run", "Settings", "Exit" };
    char loadInfo[80];
    snprintf(loadInfo, sizeof loadInfo, "Pick two of the %d saved bodies and watch them orbit.", n);
    const char *info[MENU_ITEMS] = {
        loadInfo,
        "Load one saved body and type the other, or type both.",
        "Play data.csv again without recalculating.",
        "Auto time step from Kepler's third law, or your own. Sound on or off.",
        "Close the simulator."
    };
    const Color planetCol[MENU_ITEMS] = { { 80, 160, 255, 255 }, { 220, 100, 70, 255 }, COL_VIOLET,
                                          { 230, 210, 150, 255 }, { 170, 170, 180, 255 } };
    float step = 2 * PI / MENU_ITEMS;

    /* ---------- input: wheel or arrows turn the orbit, Enter opens ---------- */
    int open = 0;
    float wheel = GetMouseWheelMove();
    if (wheel < 0 || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN))
        menuTurn++;
    if (wheel > 0 || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP))
        menuTurn--;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
        open = 1;

    float ease = 8.0f * GetFrameTime();
    if (ease > 1.0f)
        ease = 1.0f;
    menuAngle += (menuTurn * step - menuAngle) * ease;
    int selected = ((menuTurn % MENU_ITEMS) + MENU_ITEMS) % MENU_ITEMS;

    /* ---------- layout: the orbit stays left of the text on the right ---------- */
    Vector2 c = { W * 0.35f, H * 0.5f };
    float rx = fminf(W * 0.27f, H * 0.62f), ry = rx * 0.34f;
    float sunR = fminf(105 * u, ry * 0.95f);

    /* each planet's place on the tilted orbit; the selected one comes to the front (bottom) */
    Vector2 pos[MENU_ITEMS];
    float depth[MENU_ITEMS];
    for (int i=0; i<MENU_ITEMS; i++) {
        float a = PI / 2 + i * step - menuAngle;
        pos[i] = (Vector2){ c.x + rx * cosf(a), c.y + ry * sinf(a) };
        depth[i] = sinf(a);                    /* 1 = front, -1 = back */
    }

    /* the mouse over a planet: hand cursor; click selects it, or opens it if already selected */
    for (int i=0; i<MENU_ITEMS; i++) {
        float r = 34 * u * (0.6f + 0.4f * (depth[i] + 1) / 2);
        Vector2 m = GetMousePosition();
        if (fabsf(m.x - pos[i].x) < r && fabsf(m.y - pos[i].y) < r) {
            cursorKind = CURSOR_HAND;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (i == selected)
                    open = 1;
                else {
                    int diff = ((i - selected) % MENU_ITEMS + MENU_ITEMS) % MENU_ITEMS;
                    menuTurn += (diff <= MENU_ITEMS / 2) ? diff : diff - MENU_ITEMS;   /* shortest way round */
                    playSound(SOUND_CLICK);
                }
            }
        }
    }

    /* ---------- draw: orbit, back planets, Sun, front planets ---------- */
    drawEllipse(c, rx, ry, 1.5f * u, Fade(COL_MUTED, 0.35f));

    for (int pass=0; pass<2; pass++) {
        if (pass == 1) {
            drawGlow(c, sunR * 1.9f, COL_GOLD, 0.45f);
            DrawCircleV(c, sunR, COL_GOLD);
            DrawCircleV((Vector2){ c.x - sunR * 0.25f, c.y - sunR * 0.25f }, sunR * 0.55f, Fade(WHITE, 0.12f));
            drawTextCentered("GRAVITY", c.x, c.y - sunR * 0.3f, (int)(sunR * 0.38f), COL_BG_TOP);
            drawTextCentered("Orbital Simulator", c.x, c.y + sunR * 0.17f, (int)fmaxf(11, sunR * 0.15f), Fade(COL_BG_TOP, 0.75f));
        }
        for (int i=0; i<MENU_ITEMS; i++) {
            if ((pass == 0) != (depth[i] < 0))
                continue;                       /* back planets on pass 0, front ones on pass 1 */
            float k = 0.6f + 0.4f * (depth[i] + 1) / 2;
            float r = 34 * u * k;
            Color pc = Fade(planetCol[i], 0.45f + 0.55f * (depth[i] + 1) / 2);
            if (i == selected)
                drawGlow(pos[i], r * 2.0f, planetCol[i], 0.5f + 0.15f * sinf(t * 3));
            DrawCircleV(pos[i], r, pc);
            DrawCircleV((Vector2){ pos[i].x - r * 0.3f, pos[i].y - r * 0.3f }, r * 0.45f, Fade(WHITE, 0.15f));
            if (i == 3)                                             /* Settings planet has rings */
                drawEllipse(pos[i], r * 1.8f, r * 0.5f, 2 * u, Fade(planetCol[i], 0.8f));

            /* label: only the front half, never smaller than 13 px, always inside the window */
            if (depth[i] > -0.3f) {
                int fs = (int)fmaxf(13, 18 * u * k);
                float lw = MeasureText(title[i], fs);
                float lx = pos[i].x - lw / 2;
                if (lx < 8 * u) lx = 8 * u;
                if (lx + lw > W * 0.68f) lx = W * 0.68f - lw;
                DrawText(title[i], (int)lx, (int)(pos[i].y + r + 8 * u), fs, i == selected ? COL_TEXT : Fade(COL_MUTED, 0.8f));
            }
        }
    }

    /* ---------- the selected option, explained on the right ---------- */
    float ix = W * 0.7f, iw = W * 0.27f;
    DrawText(TextFormat("%d / %d", selected + 1, MENU_ITEMS), (int)ix, (int)(H * 0.3f), (int)fmaxf(13, 18 * u), COL_MUTED);
    DrawText(title[selected], (int)ix, (int)(H * 0.3f + 30 * u), (int)(38 * u), planetCol[selected]);
    DrawRectangleGradientH((int)ix, (int)(H * 0.3f + 78 * u), (int)iw, (int)(2 * u) + 1, planetCol[selected], Fade(planetCol[selected], 0.0f));

    /* wrap the description onto lines that fit iw */
    char words[200], line[200] = "";
    float ly = H * 0.3f + 96 * u;
    int fs = (int)fmaxf(14, 20 * u);
    snprintf(words, sizeof words, "%s", info[selected]);
    for (char *w = strtok(words, " "); w != NULL; w = strtok(NULL, " ")) {
        char test[200];
        snprintf(test, sizeof test, "%s%s%s", line, line[0] ? " " : "", w);
        if (MeasureText(test, fs) > iw && line[0]) {
            DrawText(line, (int)ix, (int)ly, fs, COL_TEXT);
            ly += fs * 1.4f;
            snprintf(line, sizeof line, "%s", w);
        }
        else
            snprintf(line, sizeof line, "%s", test);
    }
    DrawText(line, (int)ix, (int)ly, fs, COL_TEXT);

    for (int i=0; i<MENU_ITEMS; i++)                    /* position dots */
        DrawCircleV((Vector2){ ix + 6 * u + i * 22 * u, ly + 60 * u }, (i == selected ? 6 : 4) * u,
                    i == selected ? planetCol[i] : Fade(COL_MUTED, 0.5f));
    int hs = (int)fmaxf(12, 15 * u);
    DrawText("Scroll or use the arrow keys to turn", (int)ix, (int)(ly + 90 * u), hs, COL_MUTED);
    DrawText("Click the front planet or press Enter", (int)ix, (int)(ly + 90 * u + hs * 1.5f), hs, COL_MUTED);

    /* ---------- open the selected option ---------- */
    if (open) {
        switch (selected) {
            case 0: next = SCREEN_LIST; break;
            case 1: next = SCREEN_NEW; break;
            case 2:
                if (loadPlayback(DATA_FILE)) {
                    runReturn = SCREEN_MAIN;
                    next = SCREEN_RUN;
                }
                else
                    showToast("No saved run yet - run a simulation first", 0);
                break;
            case 3: next = SCREEN_SETTINGS; break;
            case 4: next = SCREEN_EXIT; break;
        }
    }
    return next;
}

/* ------------------------------------------------ new simulation: form data */

/* newMode: 1 = load one + type one, 2 = type both, 3 = edit a saved body */
static int newMode = 2;

/* 7 fields per body: [0] name, [1] symbol, [2..6] numbers */
static FIELD form[2][7];
static int formReady = 0;
static int picked = -1;                    /* row chosen in the saved-body list (mode 1) */
static float pickScroll = 0;
static char editName[20];                  /* the body being edited (mode 3) */

/* bodies typed in the last run from this form, offered for saving afterwards */
static BODY typedBody[2];
static int typed[2] = { 0, 0 };

static void setupForm(void) {
    for (int k=0; k<2; k++) {
        form[k][0] = (FIELD){ "Name",             "", 19, 0, 0, 0 };
        form[k][1] = (FIELD){ "Symbol",           "", 1,  0, 0, 0 };
        form[k][2] = (FIELD){ "Mass (kg)",        "", 30, 1, 1, 1e32 };
        form[k][3] = (FIELD){ "X position (m)",   "", 30, 1, -1e13, 1e13 };
        form[k][4] = (FIELD){ "Y position (m)",   "", 30, 1, -1e13, 1e13 };
        form[k][5] = (FIELD){ "X velocity (m/s)", "", 30, 1, -1e6, 1e6 };
        form[k][6] = (FIELD){ "Y velocity (m/s)", "", 30, 1, -1e6, 1e6 };
    }
    formReady = 1;
}

/* checks panel k's fields and builds a body from them; returns 1 if all valid */
static int readPanel(int k, BODY *out) {
    if (form[k][0].text[0] == '\0') {
        showToast(TextFormat("Body %d: please enter a name", k + 1), 0);
        return 0;
    }
    if (form[k][1].text[0] == '\0') {
        showToast(TextFormat("Body %d: please enter a symbol", k + 1), 0);
        return 0;
    }
    strcpy(out->name, form[k][0].text);
    out->symbol = form[k][1].text[0];
    return readField(&form[k][2], &out->mass) && readField(&form[k][3], &out->x)
        && readField(&form[k][4], &out->y) && readField(&form[k][5], &out->vx)
        && readField(&form[k][6], &out->vy);
}

/* ------------------------------------------- load & run: list of bodies */

static int sel[2] = { -1, -1 };            /* the two chosen rows, -1 = none */
static float scroll = 0;
static int armed = -1;                     /* row waiting for a second Delete click */
static double armedTime = 0;
static FIELD filter = { "Filter", "", 19, 0, 0, 0 };

/* 1 if name contains the filter text, ignoring capitals ("ear" matches "Earth") */
static int matchesFilter(const char *name, const char *text) {
    int n = (int)strlen(name), m = (int)strlen(text);
    for (int i=0; i + m <= n; i++) {
        int j = 0;
        while (j < m && tolower((unsigned char)name[i + j]) == tolower((unsigned char)text[j]))
            j++;
        if (j == m)
            return 1;
    }
    return 0;
}

int drawBodyList(SETTINGS *s) {
    int W = GetScreenWidth(), H = GetScreenHeight();
    float u = getScale();
    int next = SCREEN_LIST;
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);

    for (int k=0; k<2; k++)
        if (sel[k] >= n)
            sel[k] = -1;

    /* ---------- top line: count + filter box (+ any problem in the file) ---------- */
    float lw = fminf(W - 80 * u, 940 * u), lx = (W - lw) / 2;
    DrawText(TextFormat("%d bodies in bodies.txt  -  click two to choose Body 1 and Body 2", n),
             (int)lx, (int)(96 * u), (int)fmaxf(13, 18 * u), COL_MUTED);
    focusedField = 0;
    drawField(&filter, (Rectangle){ lx + lw - 300 * u, 88 * u, 300 * u, 34 * u }, 0);
    if (bodyFileError[0] != '\0')
        DrawText(bodyFileError, (int)lx, (int)(124 * u), (int)fmaxf(12, 15 * u), COL_RED);

    /* rows that pass the filter */
    int vis[MAX_BODIES], nv = 0;
    for (int i=0; i<n; i++)
        if (matchesFilter(list[i].name, filter.text))
            vis[nv++] = i;

    /* ---------- the scrolling list ---------- */
    float top = 146 * u, bottom = H - 120 * u;
    float rowH = 58 * u, rowGap = 8 * u;
    float content = nv * (rowH + rowGap);
    Rectangle area = { lx, top, lw, bottom - top };
    int mouseIn = CheckCollisionPointRec(GetMousePosition(), area);

    if (mouseIn)
        scroll -= GetMouseWheelMove() * 60 * u;
    if (scroll > content - area.height)
        scroll = content - area.height;
    if (scroll < 0)
        scroll = 0;

    if (nv == 0)
        drawTextCentered("No bodies match the filter", W / 2.0f, top + 40 * u, (int)(20 * u), COL_MUTED);

    BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
    for (int r=0; r<nv; r++) {
        int i = vis[r];
        Rectangle row = { lx, top + r * (rowH + rowGap) - scroll, lw, rowH };
        if (row.y + rowH < top || row.y > bottom)
            continue;

        int over = mouseIn && CheckCollisionPointRec(GetMousePosition(), row);
        int which = (sel[0] == i) ? 0 : (sel[1] == i) ? 1 : -1;
        Color edge = (which == 0) ? COL_GOLD : COL_CYAN;
        if (over)
            cursorKind = CURSOR_HAND;

        DrawRectangleRounded(row, 0.3f, 8, Fade(COL_PANEL, over ? 0.95f : 0.7f));
        if (which >= 0)
            DrawRectangleRoundedLinesEx(row, 0.3f, 8, 2 * u, edge);

        Color pc = getBodyColor(list[i].name, i);
        float pr = 8 * u + 1.2f * u * (float)log10(list[i].mass / 1e14);
        DrawCircleV((Vector2){ row.x + 36 * u, row.y + rowH / 2 }, fminf(pr, 22 * u), pc);

        int fs = (int)fmaxf(13, 20 * u);
        char dist[32], speed[32];
        const char *du = formatDistance(sqrt(list[i].x * list[i].x + list[i].y * list[i].y), dist);
        const char *su = formatSpeed(sqrt(list[i].vx * list[i].vx + list[i].vy * list[i].vy), speed);
        DrawText(list[i].name, (int)(row.x + 70 * u), (int)(row.y + (rowH - fs) / 2), (int)fmaxf(14, 22 * u), COL_TEXT);
        DrawText(TextFormat("%.3g kg", list[i].mass), (int)(row.x + lw * 0.3f), (int)(row.y + (rowH - fs) / 2), fs, COL_MUTED);
        DrawText(TextFormat("%s %s from centre", dist, du), (int)(row.x + lw * 0.47f), (int)(row.y + (rowH - fs) / 2), fs, COL_MUTED);
        DrawText(TextFormat("%s %s", speed, su), (int)(row.x + lw * 0.72f), (int)(row.y + (rowH - fs) / 2), fs, COL_MUTED);

        if (which >= 0) {
            const char *tag = (which == 0) ? "BODY 1" : "BODY 2";
            DrawText(tag, (int)(row.x + lw - 20 * u - MeasureText(tag, fs)), (int)(row.y + (rowH - fs) / 2), fs, edge);
        }

        if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (which >= 0)
                sel[which] = -1;                  /* click again to unselect */
            else if (sel[0] < 0)
                sel[0] = i;
            else if (sel[1] < 0)
                sel[1] = i;
            else
                sel[1] = i;                       /* both chosen: replace Body 2 */
            armed = -1;
            playSound(SOUND_CLICK);
        }
    }
    EndScissorMode();

    /* ---------- bottom bar: chosen bodies, Edit, Delete, Run ---------- */
    float by = H - 100 * u;
    int one = (sel[0] >= 0) != (sel[1] >= 0);                /* exactly one chosen */
    int idx = (sel[0] >= 0) ? sel[0] : sel[1];
    DrawText(TextFormat("Body 1: %s", sel[0] >= 0 ? list[sel[0]].name : "-"), (int)lx, (int)(by + 6 * u), (int)(20 * u), COL_GOLD);
    DrawText(TextFormat("Body 2: %s", sel[1] >= 0 ? list[sel[1]].name : "-"), (int)lx, (int)(by + 32 * u), (int)(20 * u), COL_CYAN);

    if (drawButton((Rectangle){ lx + lw - 560 * u, by, 150 * u, 52 * u }, "Edit", COL_VIOLET, ICON_EDIT)) {
        if (!one)
            showToast("Choose just one body to edit", 0);
        else {
            if (!formReady)
                setupForm();
            snprintf(editName, sizeof editName, "%s", list[idx].name);
            snprintf(form[1][0].text, 32, "%s", list[idx].name);
            snprintf(form[1][1].text, 32, "%c", list[idx].symbol);
            snprintf(form[1][2].text, 32, "%g", list[idx].mass);
            snprintf(form[1][3].text, 32, "%g", list[idx].x);
            snprintf(form[1][4].text, 32, "%g", list[idx].y);
            snprintf(form[1][5].text, 32, "%g", list[idx].vx);
            snprintf(form[1][6].text, 32, "%g", list[idx].vy);
            newMode = 3;
            next = SCREEN_FORM;
        }
    }

    if (drawButton((Rectangle){ lx + lw - 396 * u, by, 180 * u, 52 * u }, "Delete", COL_RED, ICON_DELETE)) {
        if (!one)
            showToast("Choose just one body to delete", 0);
        else if (armed == idx && GetTime() - armedTime < 3.0) {
            char name[20];
            strcpy(name, list[idx].name);
            if (deleteBody(name) == 1)
                showToast(TextFormat("%s deleted from bodies.txt", name), 1);
            else
                showToast("Could not update bodies.txt", 0);
            sel[0] = sel[1] = armed = -1;
        }
        else {
            armed = idx;
            armedTime = GetTime();
            showToast(TextFormat("Press Delete again to remove %s", list[idx].name), 0);
        }
    }

    if (drawButton((Rectangle){ lx + lw - 200 * u, by, 200 * u, 52 * u }, "Run", COL_GOLD, ICON_PLAY)) {
        if (sel[0] < 0 || sel[1] < 0)
            showToast("Choose two bodies first", 0);
        else if (!loadBody(list[sel[0]].name, &b[0]) || !loadBody(list[sel[1]].name, &b[1]))
            showToast("A chosen body is no longer in bodies.txt", 0);
        else
            next = startRun(s, SCREEN_LIST);
    }

    if (drawHeader("LOAD AND RUN"))
        next = SCREEN_MAIN;
    return next;
}

/* ------------------------------------------------ new simulation: submenu */

int drawNewMenu(void) {
    int W = GetScreenWidth(), H = GetScreenHeight();
    float u = getScale();
    int next = SCREEN_NEW;

    float bw = 560 * u, bh = 76 * u, x = (W - bw) / 2, y = H * 0.36f;
    drawTextCentered("How do you want to set up the two bodies?", W / 2.0f, y - 60 * u, (int)(22 * u), COL_TEXT);

    if (drawButton((Rectangle){ x, y, bw, bh }, "Load one saved body + type one", COL_GOLD, ICON_ORBIT)) {
        newMode = 1;
        next = SCREEN_FORM;
    }
    drawTextCentered("Pick Body 1 from bodies.txt, enter Body 2 yourself", W / 2.0f, y + bh + 10 * u, (int)fmaxf(13, 16 * u), COL_MUTED);

    if (drawButton((Rectangle){ x, y + bh + 56 * u, bw, bh }, "Type both bodies", COL_CYAN, ICON_NEW)) {
        newMode = 2;
        next = SCREEN_FORM;
    }
    drawTextCentered("Enter every value for both bodies", W / 2.0f, y + 2 * bh + 66 * u, (int)fmaxf(13, 16 * u), COL_MUTED);

    if (drawHeader("NEW SIMULATION"))
        next = SCREEN_MAIN;
    return next;
}

/* --------------------------------------------------- the form (modes 1, 2, 3) */

int drawNewForm(SETTINGS *s) {
    int W = GetScreenWidth();
    float u = getScale();
    int next = SCREEN_FORM;
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);

    if (!formReady)
        setupForm();
    if (picked >= n)
        picked = -1;

    /* typing goes to fields 0..13 (panel k, field i = k * 7 + i); modes 1 and 3 only use panel 2 */
    int first = (newMode == 2) ? 0 : 7;
    if (focusedField < first || focusedField > 13)
        focusedField = first;
    if (IsKeyPressed(KEY_TAB))
        focusedField = (focusedField + 1 > 13) ? first : focusedField + 1;

    const char *hint = (newMode == 1) ? "Choose Body 1 from the saved list, then type Body 2"
                     : (newMode == 2) ? "Type the starting values for both bodies"
                     : "Change any value, then press Save Changes";
    drawTextCentered(hint, W / 2.0f, 92 * u, (int)fmaxf(13, 18 * u), COL_MUTED);

    float pw = 480 * u, gap = 30 * u, ph = 400 * u, rowH = 36 * u, rowStep = 44 * u;
    float x0 = (W - (2 * pw + gap)) / 2, top = 124 * u;
    if (newMode == 3)
        x0 = (W - pw) / 2 - (pw + gap);           /* only panel 2 is drawn, centred */

    for (int k=0; k<2; k++) {
        if (newMode == 3 && k == 0)
            continue;
        float px = x0 + k * (pw + gap);
        float ix = px + 20 * u, iw = pw - 40 * u;
        Color tint = (k == 0) ? COL_GOLD : COL_CYAN;
        DrawRectangleRounded((Rectangle){ px, top, pw, ph }, 0.05f, 8, Fade(COL_PANEL, 0.85f));
        DrawRectangleRoundedLinesEx((Rectangle){ px, top, pw, ph }, 0.05f, 8, 1, Fade(tint, 0.5f));
        DrawText(newMode == 3 ? TextFormat("EDIT %s", editName) : TextFormat("BODY %d", k + 1),
                 (int)ix, (int)(top + 14 * u), (int)(22 * u), tint);

        if (k == 0 && newMode == 1) {
            /* ---------- saved-body picker ---------- */
            DrawText("from bodies.txt", (int)(ix + 100 * u), (int)(top + 18 * u), (int)fmaxf(12, 16 * u), COL_MUTED);
            Rectangle area = { ix, top + 54 * u, iw, ph - 70 * u };
            float itemH = 38 * u;
            int mouseIn = CheckCollisionPointRec(GetMousePosition(), area);
            if (mouseIn)
                pickScroll -= GetMouseWheelMove() * 50 * u;
            if (pickScroll > n * itemH - area.height)
                pickScroll = n * itemH - area.height;
            if (pickScroll < 0)
                pickScroll = 0;

            BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
            for (int i=0; i<n; i++) {
                Rectangle row = { area.x, area.y + i * itemH - pickScroll, area.width, itemH - 4 * u };
                int over = mouseIn && CheckCollisionPointRec(GetMousePosition(), row);
                if (over)
                    cursorKind = CURSOR_HAND;
                DrawRectangleRounded(row, 0.3f, 6, Fade(i == picked ? COL_GOLD : COL_PANEL, i == picked ? 0.3f : (over ? 0.9f : 0.5f)));
                DrawCircleV((Vector2){ row.x + 18 * u, row.y + row.height / 2 }, 8 * u, getBodyColor(list[i].name, 0));
                DrawText(list[i].name, (int)(row.x + 36 * u), (int)(row.y + (row.height - 18 * u) / 2), (int)fmaxf(13, 18 * u), COL_TEXT);
                DrawText(TextFormat("%.3g kg", list[i].mass), (int)(row.x + iw * 0.6f), (int)(row.y + (row.height - 16 * u) / 2), (int)fmaxf(12, 16 * u), COL_MUTED);
                if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    picked = i;
                    playSound(SOUND_CLICK);
                }
            }
            EndScissorMode();
        }
        else {
            /* ---------- typed fields ---------- */
            float y = top + 60 * u;
            for (int i=0; i<7; i++)
                drawField(&form[k][i], (Rectangle){ ix, y + i * rowStep, iw, rowH }, k * 7 + i);
        }
    }

    float ry = top + ph + 20 * u;

    /* ---------- edit mode: save the changes back to the file ---------- */
    if (newMode == 3) {
        if (drawButton((Rectangle){ W / 2 - 150 * u, ry, 300 * u, 50 * u }, "Save Changes", COL_VIOLET, ICON_SAVE)) {
            BODY nb;
            if (readPanel(1, &nb)) {
                int r = updateBody(editName, &nb);
                if (r == 1) {
                    showToast(TextFormat("%s updated in bodies.txt", nb.name), 1);
                    next = SCREEN_LIST;
                }
                else if (r == -3)
                    showToast(TextFormat("Another body is already called %s", nb.name), 0);
                else
                    showToast("Could not update bodies.txt", 0);
            }
        }
        if (drawHeader("EDIT A SAVED BODY"))
            next = SCREEN_LIST;
        return next;
    }

    /* ---------- run ---------- */
    drawTextCentered(s->autoStep ? "Time step: Auto (one full orbit, from Kepler's third law)"
                                 : TextFormat("Time step: Manual, dt = %g s x %d steps", s->dt, s->steps),
                     W / 2.0f, ry + 60 * u, (int)fmaxf(13, 16 * u), COL_MUTED);
    if (drawButton((Rectangle){ W / 2 - 150 * u, ry, 300 * u, 50 * u }, "Run Simulation", COL_GOLD, ICON_PLAY)) {
        int ok;
        if (newMode == 1) {
            if (picked < 0) {
                showToast("Choose Body 1 from the list first", 0);
                ok = 0;
            }
            else
                ok = loadBody(list[picked].name, &b[0]) && readPanel(1, &b[1]);
        }
        else
            ok = readPanel(0, &b[0]) && readPanel(1, &b[1]);

        if (ok) {
            typed[0] = (newMode == 2);
            typed[1] = 1;
            typedBody[0] = b[0];               /* copies: the simulation changes b[] as it runs */
            typedBody[1] = b[1];
            next = startRun(s, SCREEN_FORM);
            if (next == SCREEN_RUN)
                runReturn = SCREEN_SAVE;       /* after watching, offer to save the typed bodies */
        }
    }

    if (drawHeader(newMode == 1 ? "LOAD ONE + TYPE ONE" : "TYPE BOTH BODIES"))
        next = SCREEN_NEW;
    return next;
}

/* ------------------------------------------------ save after a new run */

static int saveStep = 0;                   /* 0 = ask, 1 = which ones, 2 = confirm */
static int saveWhich[2] = { 0, 0 };

/* leaves the save screen and resets it for next time */
static int finishSave(void) {
    saveStep = 0;
    typed[0] = typed[1] = 0;
    return SCREEN_MAIN;
}

int drawSaveScreen(void) {
    int W = GetScreenWidth(), H = GetScreenHeight();
    float u = getScale();
    int next = SCREEN_SAVE;
    int count = typed[0] + typed[1];

    float pw = 640 * u, ph = 300 * u, px = (W - pw) / 2, py = (H - ph) / 2;
    DrawRectangleRounded((Rectangle){ px, py, pw, ph }, 0.06f, 8, Fade(COL_PANEL, 0.9f));
    drawIcon(ICON_SAVE, (Vector2){ W / 2.0f, py + 50 * u }, 22 * u, COL_GOLD);

    float by = py + ph - 90 * u, bw = 180 * u, bh = 50 * u;

    if (saveStep == 0) {
        drawTextCentered(count == 2 ? "Save the two bodies you typed to bodies.txt?"
                                    : TextFormat("Save %s to bodies.txt?", typedBody[1].name),
                         W / 2.0f, py + 100 * u, (int)(24 * u), COL_TEXT);
        drawTextCentered("Saved bodies appear in Load & Run next time.", W / 2.0f, py + 136 * u, (int)fmaxf(13, 16 * u), COL_MUTED);
        if (drawButton((Rectangle){ W / 2 - bw - 10 * u, by, bw, bh }, "Yes", COL_GOLD, ICON_NONE)) {
            if (count == 2)
                saveStep = 1;
            else {
                saveWhich[0] = 0;
                saveWhich[1] = 1;
                saveStep = 2;
            }
        }
        if (drawButton((Rectangle){ W / 2 + 10 * u, by, bw, bh }, "No", COL_MUTED, ICON_NONE)) {
            showToast("Nothing saved", 1);
            next = finishSave();
        }
    }
    else if (saveStep == 1) {
        drawTextCentered("Which bodies?", W / 2.0f, py + 100 * u, (int)(24 * u), COL_TEXT);
        float w3 = 190 * u, x3 = (W - (3 * w3 + 2 * 14 * u)) / 2;
        if (drawButton((Rectangle){ x3, by, w3, bh }, TextFormat("1: %s", typedBody[0].name), COL_GOLD, ICON_NONE)) {
            saveWhich[0] = 1; saveWhich[1] = 0; saveStep = 2;
        }
        if (drawButton((Rectangle){ x3 + w3 + 14 * u, by, w3, bh }, TextFormat("2: %s", typedBody[1].name), COL_CYAN, ICON_NONE)) {
            saveWhich[0] = 0; saveWhich[1] = 1; saveStep = 2;
        }
        if (drawButton((Rectangle){ x3 + 2 * (w3 + 14 * u), by, w3, bh }, "Both", COL_VIOLET, ICON_NONE)) {
            saveWhich[0] = 1; saveWhich[1] = 1; saveStep = 2;
        }
    }
    else {
        char names[60];
        if (saveWhich[0] && saveWhich[1])
            snprintf(names, sizeof names, "%s and %s", typedBody[0].name, typedBody[1].name);
        else
            snprintf(names, sizeof names, "%s", saveWhich[0] ? typedBody[0].name : typedBody[1].name);

        drawTextCentered(TextFormat("Save %s to bodies.txt?", names), W / 2.0f, py + 100 * u, (int)(24 * u), COL_TEXT);
        drawTextCentered("Please confirm.", W / 2.0f, py + 136 * u, (int)fmaxf(13, 16 * u), COL_MUTED);

        if (drawButton((Rectangle){ W / 2 - bw - 10 * u, by, bw, bh }, "Confirm", COL_GOLD, ICON_SAVE)) {
            int saved = 0, taken = 0;
            for (int k=0; k<2; k++) {
                if (!saveWhich[k])
                    continue;
                int r = saveBody(&typedBody[k]);
                if (r == 1)
                    saved++;
                else
                    taken++;
            }
            if (taken == 0)
                showToast(TextFormat("Saved %s to bodies.txt", names), 1);
            else if (saved > 0)
                showToast("Saved one - the other name is already in bodies.txt", 0);
            else
                showToast("Not saved - that name is already in bodies.txt (or the file is full)", 0);
            next = finishSave();
        }
        if (drawButton((Rectangle){ W / 2 + 10 * u, by, bw, bh }, "Cancel", COL_MUTED, ICON_NONE))
            saveStep = 0;
    }

    if (drawHeader("SAVE NEW BODIES?")) {
        showToast("Nothing saved", 1);
        next = finishSave();
    }
    return next;
}

/* ---------------------------------------------------------- run screen */

int drawRunScreen(void) {
    int W = GetScreenWidth(), H = GetScreenHeight();
    float u = getScale();
    int next = SCREEN_RUN;

    int finished = drawPlayback();

    if (finished) {
        if (drawButton((Rectangle){ W / 2 - 110 * u, H - 120 * u, 220 * u, 48 * u }, "Replay", COL_GOLD, ICON_REPLAY))
            loadPlayback(DATA_FILE);
    }

    if (drawHeader("SIMULATION")) {
        unloadPlayback();
        next = runReturn;
    }
    return next;
}
