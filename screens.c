/* screens.c - the main menu, Load & Run, New Simulation and the save question
   Gravity Simulator - P L Hirthick Ramanath (26PT20), Svithin S (26PT35) */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "gravity.h"

#define MENU_ITEMS 5

static BODY b[2];
static PAIR p;

static int menuTurn = 0;              /* how far the menu orbit has been turned */
static float menuAngle = 0;

static int sel[2] = { -1, -1 };       /* the two rows chosen in Load & Run */
static float scroll = 0;
static int armed = -1;                /* the row waiting for a second Delete click */
static double armedTime = 0;
static FIELD filter = { "Filter", "", 19, 0, 0, 0 };

static int newMode = 0;               /* 0 = choose, 1 = load one + type one, 2 = type both, 3 = edit a saved body */
static char pickedName[20] = "";     /* the saved body chosen as Body 1 in "load one + type one" */
static float pickScroll = 0;
static char editName[20];
/* form[0] and form[1] are Body 1 and Body 2 of a new simulation, form[2] is the edit screen */
static FIELD form[3][7] = {
    {
        { "Name", "", 19, 0, 0, 0 },
        { "Symbol", "", 1, 0, 0, 0 },
        { "Mass (kg)", "", 30, 1, 1, 1e32 },
        { "X position (m)", "", 30, 1, -1e13, 1e13 },
        { "Y position (m)", "", 30, 1, -1e13, 1e13 },
        { "X velocity (m/s)", "", 30, 1, -1e6, 1e6 },
        { "Y velocity (m/s)", "", 30, 1, -1e6, 1e6 }
    },
    {
        { "Name", "", 19, 0, 0, 0 },
        { "Symbol", "", 1, 0, 0, 0 },
        { "Mass (kg)", "", 30, 1, 1, 1e32 },
        { "X position (m)", "", 30, 1, -1e13, 1e13 },
        { "Y position (m)", "", 30, 1, -1e13, 1e13 },
        { "X velocity (m/s)", "", 30, 1, -1e6, 1e6 },
        { "Y velocity (m/s)", "", 30, 1, -1e6, 1e6 }
    },
    {
        { "Name", "", 19, 0, 0, 0 },
        { "Symbol", "", 1, 0, 0, 0 },
        { "Mass (kg)", "", 30, 1, 1, 1e32 },
        { "X position (m)", "", 30, 1, -1e13, 1e13 },
        { "Y position (m)", "", 30, 1, -1e13, 1e13 },
        { "X velocity (m/s)", "", 30, 1, -1e6, 1e6 },
        { "Y velocity (m/s)", "", 30, 1, -1e6, 1e6 }
    }
};

static BODY typedBody[2];             /* what the user typed, kept for the save question after the run */
static int typed[2] = { 0, 0 };
static int saveStep = 0;              /* 0 = ask, 1 = which ones, 2 = confirm */
static int saveWhich[2] = { 0, 0 };

/* works out dt and steps (if Auto), runs the physics and opens the playback.
   from = where to stay if it fails, after = where Back goes once you've watched it */
static int startRun(SETTINGS *s, int from, int after) {
    SETTINGS run = *s;                /* a copy, so Auto never overwrites the manual values */
    double closest, drift;
    int orbit;

    if (run.autoStep)
        orbit = checkOrbit(b, &run, &closest);
    else
        orbit = checkOrbit(b, NULL, &closest);

    int result = runSimulation(b, &p, &run, &drift);
    if (result == 0) {
        showToast("The two bodies cannot start at the same position", 0);
        return from;
    }
    if (result < 0) {
        showToast("Could not write data.csv", 0);
        return from;
    }
    if (!loadPlayback(DATA_FILE, after)) {
        showToast("Could not read data.csv", 0);
        return from;
    }

    double total = run.dt * run.steps;
    if (orbit == ORBIT_COLLIDE)
        showToast(TextFormat("Warning: they pass within %.0f km - a real collision", closest / 1000), 0);
    else if (total < 7200)
        showToast(TextFormat("dt = %.4g s  x  %d steps  =  %.1f minutes, saved to data.csv", run.dt, run.steps, total / 60), 1);
    else if (total < 172800)
        showToast(TextFormat("dt = %.4g s  x  %d steps  =  %.1f hours, saved to data.csv", run.dt, run.steps, total / 3600), 1);
    else
        showToast(TextFormat("dt = %.4g s  x  %d steps  =  %.1f days, saved to data.csv", run.dt, run.steps, total / 86400), 1);
    return SCREEN_RUN;
}

/* the main menu is a little solar system: the Sun in the middle, one planet per option */
int drawMainMenu(void) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int next = SCREEN_MAIN;
    int open = 0;
    float scale = getScale();
    float t = (float)GetTime();
    float step = 2 * PI / MENU_ITEMS;
    float wheel = GetMouseWheelMove();
    BODY list[MAX_BODIES];
    char loadInfo[80];
    snprintf(loadInfo, sizeof loadInfo, "Pick two of the %d saved bodies and watch them orbit.", listBodies(list, MAX_BODIES));

    const char *title[MENU_ITEMS] = { "Load & Run", "New Simulation", "Replay Last Run", "Settings", "Exit" };
    const char *info[MENU_ITEMS] = {
        loadInfo,
        "Load one saved body and type the other, or type both.",
        "Play data.csv again without recalculating.",
        "Auto time step from Kepler's third law, or your own.",
        "Close the simulator."
    };
    const Color planetCol[MENU_ITEMS] = { { 80, 160, 255, 255 }, { 220, 100, 70, 255 }, COL_VIOLET, { 230, 210, 150, 255 }, { 170, 170, 180, 255 } };

    if (wheel < 0 || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN))
        menuTurn++;
    if (wheel > 0 || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP))
        menuTurn--;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
        open = 1;

    /* the orbit eases towards where it should be instead of jumping */
    float ease = 8.0f * GetFrameTime();
    if (ease > 1.0f)
        ease = 1.0f;
    menuAngle += (menuTurn * step - menuAngle) * ease;
    int selected = ((menuTurn % MENU_ITEMS) + MENU_ITEMS) % MENU_ITEMS;

    Vector2 centre = { screenW * 0.35f, screenH * 0.5f };
    float rx = fminf(screenW * 0.27f, screenH * 0.62f);
    float ry = rx * 0.34f;
    float sunR = fminf(105 * scale, ry * 0.95f);
    Vector2 pos[MENU_ITEMS];
    float depth[MENU_ITEMS];

    /* depth is 1 at the front of the orbit and -1 at the back; the selected planet sits at the front */
    int hovered = -1;
    Vector2 mouse = GetMousePosition();
    for(int i=0; i<MENU_ITEMS; i++) {
        float a = PI / 2 + i * step - menuAngle;
        pos[i] = (Vector2){ centre.x + rx * cosf(a), centre.y + ry * sinf(a) };
        depth[i] = sinf(a);

        /* if two planets overlap under the mouse, the one in front wins */
        float r = 34 * scale * (0.8f + 0.2f * depth[i]);
        if (fabsf(mouse.x - pos[i].x) < r && fabsf(mouse.y - pos[i].y) < r) {
            if (hovered < 0 || depth[i] > depth[hovered])
                hovered = i;
        }
    }
    if (hovered >= 0) {
        cursorKind = MOUSE_CURSOR_POINTING_HAND;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (hovered == selected) {
                open = 1;
            }
            else {
                int diff = ((hovered - selected) % MENU_ITEMS + MENU_ITEMS) % MENU_ITEMS;
                if (diff <= MENU_ITEMS / 2)             /* turn the shortest way round */
                    menuTurn += diff;
                else
                    menuTurn += diff - MENU_ITEMS;
            }
        }
    }

    /* back planets first, then the Sun, then the front planets so they overlap it */
    DrawEllipseLines((int)centre.x, (int)centre.y, rx, ry, Fade(COL_MUTED, 0.35f));
    for(int pass=0; pass<2; pass++) {
        if (pass == 1) {
            drawGlow(centre, sunR * 1.9f, COL_GOLD, 0.45f);
            DrawCircleV(centre, sunR, COL_GOLD);
            DrawCircleV((Vector2){ centre.x - sunR * 0.25f, centre.y - sunR * 0.25f }, sunR * 0.55f, Fade(WHITE, 0.12f));
            drawTextCentered("GRAVITY", centre.x, centre.y - sunR * 0.3f, (int)(sunR * 0.38f), COL_BG_TOP);
            drawTextCentered("Orbital Simulator", centre.x, centre.y + sunR * 0.17f, (int)fmaxf(11, sunR * 0.15f), Fade(COL_BG_TOP, 0.75f));
        }
        for(int i=0; i<MENU_ITEMS; i++) {
            if (pass == 0 && depth[i] >= 0)
                continue;
            if (pass == 1 && depth[i] < 0)
                continue;

            float k = 0.8f + 0.2f * depth[i];
            float r = 34 * scale * k;
            if (i == selected)
                drawGlow(pos[i], r * 2.0f, planetCol[i], 0.5f + 0.15f * sinf(t * 3));
            DrawCircleV(pos[i], r, Fade(planetCol[i], 0.72f + 0.28f * depth[i]));
            DrawCircleV((Vector2){ pos[i].x - r * 0.3f, pos[i].y - r * 0.3f }, r * 0.45f, Fade(WHITE, 0.15f));
            if (i == 3)                                 /* the Settings planet has a ring */
                DrawEllipseLines((int)pos[i].x, (int)pos[i].y, r * 1.8f, r * 0.5f, Fade(planetCol[i], 0.8f));

            /* labels only on the front half, and kept inside the window */
            if (depth[i] > -0.3f) {
                int fontSize = (int)fmaxf(13, 18 * scale * k);
                float labelW = MeasureText(title[i], fontSize);
                float labelX = pos[i].x - labelW / 2;
                if (labelX < 8 * scale)
                    labelX = 8 * scale;
                if (labelX + labelW > screenW * 0.68f)
                    labelX = screenW * 0.68f - labelW;
                Color labelCol = Fade(COL_MUTED, 0.8f);
                if (i == selected)
                    labelCol = COL_TEXT;
                DrawText(title[i], (int)labelX, (int)(pos[i].y + r + 8 * scale), fontSize, labelCol);
            }
        }
    }

    /* the selected option explained on the right, word-wrapped to fit */
    float infoX = screenW * 0.7f;
    float infoW = screenW * 0.27f;
    float lineY = screenH * 0.3f + 96 * scale;
    int fontSize = (int)fmaxf(14, 20 * scale);
    int hintSize = (int)fmaxf(12, 15 * scale);
    char words[200], line[200] = "", test[200];

    DrawText(TextFormat("%d / %d", selected + 1, MENU_ITEMS), (int)infoX, (int)(screenH * 0.3f), (int)fmaxf(13, 18 * scale), COL_MUTED);
    DrawText(title[selected], (int)infoX, (int)(screenH * 0.3f + 30 * scale), (int)(38 * scale), planetCol[selected]);
    DrawRectangleGradientH((int)infoX, (int)(screenH * 0.3f + 78 * scale), (int)infoW, (int)(2 * scale) + 1, planetCol[selected], Fade(planetCol[selected], 0.0f));

    strcpy(words, info[selected]);
    for(char *w = strtok(words, " "); w != NULL; w = strtok(NULL, " ")) {
        strcpy(test, line);
        if (line[0] != '\0')
            strcat(test, " ");
        strcat(test, w);

        if (MeasureText(test, fontSize) > infoW && line[0] != '\0') {
            DrawText(line, (int)infoX, (int)lineY, fontSize, COL_TEXT);
            lineY += fontSize * 1.4f;
            strcpy(line, w);
        }
        else {
            strcpy(line, test);
        }
    }
    DrawText(line, (int)infoX, (int)lineY, fontSize, COL_TEXT);

    for(int i=0; i<MENU_ITEMS; i++) {
        if (i == selected)
            DrawCircleV((Vector2){ infoX + 6 * scale + i * 22 * scale, lineY + 60 * scale }, 6 * scale, planetCol[i]);
        else
            DrawCircleV((Vector2){ infoX + 6 * scale + i * 22 * scale, lineY + 60 * scale }, 4 * scale, Fade(COL_MUTED, 0.5f));
    }
    DrawText("Scroll or use the arrow keys to turn", (int)infoX, (int)(lineY + 90 * scale), hintSize, COL_MUTED);
    DrawText("Click the front planet or press Enter", (int)infoX, (int)(lineY + 90 * scale + hintSize * 1.5f), hintSize, COL_MUTED);

    if (open) {
        switch (selected) {
            case 0:
                next = SCREEN_LIST;
                break;
            case 1:
                newMode = 0;
                next = SCREEN_NEW;
                break;
            case 2:
                if (loadPlayback(DATA_FILE, SCREEN_MAIN))
                    next = SCREEN_RUN;
                else
                    showToast("No saved run yet - run a simulation first", 0);
                break;
            case 3:
                next = SCREEN_SETTINGS;
                break;
            case 4:
                next = SCREEN_EXIT;
                break;
        }
    }
    return next;
}

/* checks one panel of the form and builds a body from it; returns 1 if every value is fine */
static int readPanel(int k, BODY *out, const char *who) {
    if (form[k][0].text[0] == '\0') {
        showToast(TextFormat("%s: please enter a name", who), 0);
        return 0;
    }
    if (form[k][1].text[0] == '\0') {
        showToast(TextFormat("%s: please enter a symbol", who), 0);
        return 0;
    }
    strcpy(out->name, form[k][0].text);
    out->symbol = form[k][1].text[0];

    if (!readField(&form[k][2], &out->mass))
        return 0;
    if (!readField(&form[k][3], &out->x))
        return 0;
    if (!readField(&form[k][4], &out->y))
        return 0;
    if (!readField(&form[k][5], &out->vx))
        return 0;
    if (!readField(&form[k][6], &out->vy))
        return 0;
    return 1;
}

/* Load & Run: every saved body in a scrolling list. pick two and run, or edit / delete one */
int drawBodyList(SETTINGS *s) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int next = SCREEN_LIST;
    float scale = getScale();
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);
    int visible[MAX_BODIES];
    int shown = 0;

    for(int k=0; k<2; k++) {
        if (sel[k] >= n)
            sel[k] = -1;
    }

    float listW = fminf(screenW - 80 * scale, 940 * scale);
    float listX = (screenW - listW) / 2;
    DrawText(TextFormat("%d bodies in bodies.txt  -  click two to choose Body 1 and Body 2", n), (int)listX, (int)(96 * scale), (int)fmaxf(13, 18 * scale), COL_MUTED);
    focusedField = 0;
    drawField(&filter, (Rectangle){ listX + listW - 300 * scale, 88 * scale, 300 * scale, 34 * scale }, 0);
    if (bodyFileError[0] != '\0')
        DrawText(bodyFileError, (int)listX, (int)(124 * scale), (int)fmaxf(12, 15 * scale), COL_RED);

    /* the filter ignores capitals, so "ear" finds Earth */
    for(int i=0; i<n; i++) {
        char lowName[20], lowFilter[20];
        for(int j=0; j<20; j++) {
            lowName[j] = (char)tolower((unsigned char)list[i].name[j]);
            lowFilter[j] = (char)tolower((unsigned char)filter.text[j]);
        }
        if (strstr(lowName, lowFilter) != NULL) {
            visible[shown] = i;
            shown++;
        }
    }

    float top = 146 * scale;
    float bottom = screenH - 120 * scale;
    float rowH = 58 * scale;
    float rowGap = 8 * scale;
    Rectangle area = { listX, top, listW, bottom - top };
    int mouseIn = CheckCollisionPointRec(GetMousePosition(), area);

    if (mouseIn)
        scroll -= GetMouseWheelMove() * 60 * scale;
    if (scroll > shown * (rowH + rowGap) - area.height)
        scroll = shown * (rowH + rowGap) - area.height;
    if (scroll < 0)
        scroll = 0;
    if (shown == 0)
        drawTextCentered("No bodies match the filter", screenW / 2.0f, top + 40 * scale, (int)(20 * scale), COL_MUTED);

    BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
    for(int r=0; r<shown; r++) {
        int i = visible[r];
        Rectangle row = { listX, top + r * (rowH + rowGap) - scroll, listW, rowH };
        if (row.y + rowH < top || row.y > bottom)
            continue;

        int over = mouseIn && CheckCollisionPointRec(GetMousePosition(), row);
        int which = -1;
        if (sel[0] == i)
            which = 0;
        else if (sel[1] == i)
            which = 1;
        Color edge = COL_CYAN;
        if (which == 0)
            edge = COL_GOLD;
        if (over)
            cursorKind = MOUSE_CURSOR_POINTING_HAND;

        if (over)
            DrawRectangleRounded(row, 0.3f, 8, Fade(COL_PANEL, 0.95f));
        else
            DrawRectangleRounded(row, 0.3f, 8, Fade(COL_PANEL, 0.7f));
        if (which >= 0)
            DrawRectangleRoundedLinesEx(row, 0.3f, 8, 2 * scale, edge);

        int fontSize = (int)fmaxf(13, 20 * scale);
        float textY = row.y + (rowH - fontSize) / 2;
        float dot = fminf(8 * scale + 1.2f * scale * (float)log10(list[i].mass / 1e14), 22 * scale);
        double distance = sqrt(list[i].x * list[i].x + list[i].y * list[i].y);
        double speed = sqrt(list[i].vx * list[i].vx + list[i].vy * list[i].vy);

        DrawCircleV((Vector2){ row.x + 36 * scale, row.y + rowH / 2 }, dot, getBodyColor(list[i].name, i));
        DrawText(list[i].name, (int)(row.x + 70 * scale), (int)textY, (int)fmaxf(14, 22 * scale), COL_TEXT);
        DrawText(TextFormat("%.3g kg", list[i].mass), (int)(row.x + listW * 0.3f), (int)textY, fontSize, COL_MUTED);
        DrawText(TextFormat("%.0f km from centre", distance / 1000), (int)(row.x + listW * 0.47f), (int)textY, fontSize, COL_MUTED);
        DrawText(TextFormat("%.2f km/s", speed / 1000), (int)(row.x + listW * 0.72f), (int)textY, fontSize, COL_MUTED);
        if (which == 0)
            DrawText("BODY 1", (int)(row.x + listW - 20 * scale - MeasureText("BODY 1", fontSize)), (int)textY, fontSize, edge);
        if (which == 1)
            DrawText("BODY 2", (int)(row.x + listW - 20 * scale - MeasureText("BODY 2", fontSize)), (int)textY, fontSize, edge);

        /* click a chosen row again to unchoose it; with both chosen, a new click replaces Body 2 */
        if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (which >= 0)
                sel[which] = -1;
            else if (sel[0] < 0)
                sel[0] = i;
            else
                sel[1] = i;
            armed = -1;
        }
    }
    EndScissorMode();

    float barY = screenH - 100 * scale;
    int one = (sel[0] >= 0) != (sel[1] >= 0);          /* exactly one body chosen */
    int idx = sel[0];
    if (idx < 0)
        idx = sel[1];

    if (sel[0] >= 0)
        DrawText(TextFormat("Body 1: %s", list[sel[0]].name), (int)listX, (int)(barY + 6 * scale), (int)(20 * scale), COL_GOLD);
    else
        DrawText("Body 1: -", (int)listX, (int)(barY + 6 * scale), (int)(20 * scale), COL_GOLD);
    if (sel[1] >= 0)
        DrawText(TextFormat("Body 2: %s", list[sel[1]].name), (int)listX, (int)(barY + 32 * scale), (int)(20 * scale), COL_CYAN);
    else
        DrawText("Body 2: -", (int)listX, (int)(barY + 32 * scale), (int)(20 * scale), COL_CYAN);

    if (drawButton((Rectangle){ listX + listW - 560 * scale, barY, 150 * scale, 52 * scale }, "Edit", COL_VIOLET, ICON_EDIT)) {
        if (!one) {
            showToast("Choose just one body to edit", 0);
        }
        else {
            snprintf(editName, sizeof editName, "%s", list[idx].name);
            snprintf(form[2][0].text, 32, "%s", list[idx].name);
            snprintf(form[2][1].text, 32, "%c", list[idx].symbol);
            snprintf(form[2][2].text, 32, "%.10g", list[idx].mass);
            snprintf(form[2][3].text, 32, "%.10g", list[idx].x);
            snprintf(form[2][4].text, 32, "%.10g", list[idx].y);
            snprintf(form[2][5].text, 32, "%.10g", list[idx].vx);
            snprintf(form[2][6].text, 32, "%.10g", list[idx].vy);
            newMode = 3;
            next = SCREEN_NEW;
        }
    }

    /* delete needs two clicks within 3 seconds, so nothing goes by accident */
    if (drawButton((Rectangle){ listX + listW - 396 * scale, barY, 180 * scale, 52 * scale }, "Delete", COL_RED, ICON_DELETE)) {
        if (!one) {
            showToast("Choose just one body to delete", 0);
        }
        else if (armed == idx && GetTime() - armedTime < 3.0) {
            if (changeBody(list[idx].name, NULL) == 1)
                showToast(TextFormat("%s deleted from bodies.txt", list[idx].name), 1);
            else
                showToast("Could not update bodies.txt", 0);
            sel[0] = -1;
            sel[1] = -1;
            armed = -1;
        }
        else {
            armed = idx;
            armedTime = GetTime();
            showToast(TextFormat("Press Delete again to remove %s", list[idx].name), 0);
        }
    }

    if (drawButton((Rectangle){ listX + listW - 200 * scale, barY, 200 * scale, 52 * scale }, "Run", COL_GOLD, ICON_PLAY)) {
        if (sel[0] < 0 || sel[1] < 0)
            showToast("Choose two bodies first", 0);
        else if (!loadBody(list[sel[0]].name, &b[0]) || !loadBody(list[sel[1]].name, &b[1]))
            showToast("A chosen body is no longer in bodies.txt", 0);
        else
            next = startRun(s, SCREEN_LIST, SCREEN_LIST);
    }

    if (drawHeader("LOAD AND RUN"))
        next = SCREEN_MAIN;
    return next;
}

/* New Simulation: first a choice, then the form (newMode 1 or 2). Edit from Load & Run uses it too (newMode 3) */
int drawNewSimulation(SETTINGS *s) {
    int screenW = GetScreenWidth();
    int next = SCREEN_NEW;
    float scale = getScale();
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);
    int small = (int)fmaxf(13, 16 * scale);

    if (newMode == 0) {
        float buttonW = 560 * scale;
        float buttonH = 76 * scale;
        float x = (screenW - buttonW) / 2;
        float y = GetScreenHeight() * 0.36f;

        drawTextCentered("How do you want to set up the two bodies?", screenW / 2.0f, y - 60 * scale, (int)(22 * scale), COL_TEXT);
        if (drawButton((Rectangle){ x, y, buttonW, buttonH }, "Load one saved body + type one", COL_GOLD, ICON_ORBIT))
            newMode = 1;
        drawTextCentered("Pick Body 1 from bodies.txt, enter Body 2 yourself", screenW / 2.0f, y + buttonH + 10 * scale, small, COL_MUTED);
        if (drawButton((Rectangle){ x, y + buttonH + 56 * scale, buttonW, buttonH }, "Type both bodies", COL_CYAN, ICON_NEW))
            newMode = 2;
        drawTextCentered("Enter every value for both bodies", screenW / 2.0f, y + 2 * buttonH + 66 * scale, small, COL_MUTED);

        if (drawHeader("NEW SIMULATION"))
            next = SCREEN_MAIN;
        return next;
    }

    /* the boxes are numbered set * 7 + i, so Tab only moves through the boxes on screen */
    int first = 7, last = 13;
    if (newMode == 2)
        first = 0;
    if (newMode == 3) {
        first = 14;
        last = 20;
    }
    if (focusedField < first || focusedField > last)
        focusedField = first;
    if (IsKeyPressed(KEY_TAB)) {
        focusedField++;
        if (focusedField > last)
            focusedField = first;
    }

    /* the picked body is remembered by name, so deleting another body can't change it */
    int picked = -1;
    for(int i=0; i<n; i++) {
        if (strcmp(list[i].name, pickedName) == 0)
            picked = i;
    }

    if (newMode == 1)
        drawTextCentered("Choose Body 1 from the saved list, then type Body 2", screenW / 2.0f, 92 * scale, (int)fmaxf(13, 18 * scale), COL_MUTED);
    else if (newMode == 2)
        drawTextCentered("Type the starting values for both bodies", screenW / 2.0f, 92 * scale, (int)fmaxf(13, 18 * scale), COL_MUTED);
    else
        drawTextCentered("Change any value, then press Save Changes", screenW / 2.0f, 92 * scale, (int)fmaxf(13, 18 * scale), COL_MUTED);

    float panelW = 480 * scale;
    float panelH = 400 * scale;
    float gap = 30 * scale;
    float top = 124 * scale;
    float buttonY = top + panelH + 20 * scale;
    float startX = (screenW - (2 * panelW + gap)) / 2;
    if (newMode == 3)
        startX = (screenW - panelW) / 2 - (panelW + gap);      /* only the second panel is drawn, centred */

    for(int k=0; k<2; k++) {
        if (newMode == 3 && k == 0)
            continue;

        float panelX = startX + k * (panelW + gap);
        float boxX = panelX + 20 * scale;
        float boxW = panelW - 40 * scale;
        Color tint = COL_CYAN;
        if (k == 0)
            tint = COL_GOLD;

        DrawRectangleRounded((Rectangle){ panelX, top, panelW, panelH }, 0.05f, 8, Fade(COL_PANEL, 0.85f));
        DrawRectangleRoundedLinesEx((Rectangle){ panelX, top, panelW, panelH }, 0.05f, 8, 1, Fade(tint, 0.5f));
        if (newMode == 3)
            DrawText(TextFormat("EDIT %s", editName), (int)boxX, (int)(top + 14 * scale), (int)(22 * scale), tint);
        else
            DrawText(TextFormat("BODY %d", k + 1), (int)boxX, (int)(top + 14 * scale), (int)(22 * scale), tint);

        if (k == 1 || newMode == 2) {
            int set = k;
            if (newMode == 3)
                set = 2;
            for(int i=0; i<7; i++)
                drawField(&form[set][i], (Rectangle){ boxX, top + 60 * scale + i * 44 * scale, boxW, 36 * scale }, set * 7 + i);
            continue;
        }

        /* newMode 1: the first panel is a scrolling list of saved bodies to pick from */
        Rectangle area = { boxX, top + 54 * scale, boxW, panelH - 70 * scale };
        float itemH = 38 * scale;
        int mouseIn = CheckCollisionPointRec(GetMousePosition(), area);

        DrawText("from bodies.txt", (int)(boxX + 100 * scale), (int)(top + 18 * scale), (int)fmaxf(12, 16 * scale), COL_MUTED);
        if (mouseIn)
            pickScroll -= GetMouseWheelMove() * 50 * scale;
        if (pickScroll > n * itemH - area.height)
            pickScroll = n * itemH - area.height;
        if (pickScroll < 0)
            pickScroll = 0;

        BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
        for(int i=0; i<n; i++) {
            Rectangle row = { area.x, area.y + i * itemH - pickScroll, area.width, itemH - 4 * scale };
            int over = mouseIn && CheckCollisionPointRec(GetMousePosition(), row);
            if (over)
                cursorKind = MOUSE_CURSOR_POINTING_HAND;

            if (i == picked)
                DrawRectangleRounded(row, 0.3f, 6, Fade(COL_GOLD, 0.3f));
            else if (over)
                DrawRectangleRounded(row, 0.3f, 6, Fade(COL_PANEL, 0.9f));
            else
                DrawRectangleRounded(row, 0.3f, 6, Fade(COL_PANEL, 0.5f));
            DrawCircleV((Vector2){ row.x + 18 * scale, row.y + row.height / 2 }, 8 * scale, getBodyColor(list[i].name, 0));
            DrawText(list[i].name, (int)(row.x + 36 * scale), (int)(row.y + (row.height - 18 * scale) / 2), (int)fmaxf(13, 18 * scale), COL_TEXT);
            DrawText(TextFormat("%.3g kg", list[i].mass), (int)(row.x + boxW * 0.6f), (int)(row.y + (row.height - 16 * scale) / 2), (int)fmaxf(12, 16 * scale), COL_MUTED);

            if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                strcpy(pickedName, list[i].name);
        }
        EndScissorMode();
    }

    if (newMode == 3) {
        if (drawButton((Rectangle){ screenW / 2 - 150 * scale, buttonY, 300 * scale, 50 * scale }, "Save Changes", COL_VIOLET, ICON_SAVE)) {
            BODY changed;
            if (readPanel(2, &changed, "This body")) {
                int result = changeBody(editName, &changed);
                if (result == 1) {
                    showToast(TextFormat("%s updated in bodies.txt", changed.name), 1);
                    next = SCREEN_LIST;
                }
                else if (result == -3) {
                    showToast(TextFormat("Another body is already called %s", changed.name), 0);
                }
                else {
                    showToast("Could not update bodies.txt", 0);
                }
            }
        }
        if (drawHeader("EDIT A SAVED BODY"))
            next = SCREEN_LIST;
        return next;
    }

    if (s->autoStep)
        drawTextCentered("Time step: Auto (one full orbit, from Kepler's third law)", screenW / 2.0f, buttonY + 60 * scale, small, COL_MUTED);
    else
        drawTextCentered(TextFormat("Time step: Manual, dt = %g s x %d steps", s->dt, s->steps), screenW / 2.0f, buttonY + 60 * scale, small, COL_MUTED);

    if (drawButton((Rectangle){ screenW / 2 - 150 * scale, buttonY, 300 * scale, 50 * scale }, "Run Simulation", COL_GOLD, ICON_PLAY)) {
        int ok = 0;
        if (newMode == 1 && picked < 0)
            showToast("Choose Body 1 from the list first", 0);
        else if (newMode == 1)
            ok = loadBody(list[picked].name, &b[0]) && readPanel(1, &b[1], "Body 2");
        else
            ok = readPanel(0, &b[0], "Body 1") && readPanel(1, &b[1], "Body 2");

        if (ok) {
            typed[0] = (newMode == 2);
            typed[1] = 1;
            typedBody[0] = b[0];            /* copies, because the simulation moves b[] */
            typedBody[1] = b[1];
            next = startRun(s, SCREEN_NEW, SCREEN_SAVE);
        }
    }

    if (newMode == 1 && drawHeader("LOAD ONE + TYPE ONE"))
        newMode = 0;
    if (newMode == 2 && drawHeader("TYPE BOTH BODIES"))
        newMode = 0;
    return next;
}

/* after a new run: ask whether to save the typed bodies, which ones, then confirm */
int drawSaveScreen(void) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int next = SCREEN_SAVE;
    int count = typed[0] + typed[1];
    float scale = getScale();
    float panelW = 640 * scale;
    float panelH = 300 * scale;
    float panelX = (screenW - panelW) / 2;
    float panelY = (screenH - panelH) / 2;
    float buttonY = panelY + panelH - 90 * scale;
    float buttonW = 180 * scale;
    float buttonH = 50 * scale;
    int small = (int)fmaxf(13, 16 * scale);
    char names[60];

    DrawRectangleRounded((Rectangle){ panelX, panelY, panelW, panelH }, 0.06f, 8, Fade(COL_PANEL, 0.9f));
    if (saveWhich[0] && saveWhich[1])
        snprintf(names, sizeof names, "%s and %s", typedBody[0].name, typedBody[1].name);
    else if (saveWhich[0])
        snprintf(names, sizeof names, "%s", typedBody[0].name);
    else
        snprintf(names, sizeof names, "%s", typedBody[1].name);

    if (saveStep == 0) {
        if (count == 2)
            drawTextCentered("Save the two bodies you typed to bodies.txt?", screenW / 2.0f, panelY + 100 * scale, (int)(24 * scale), COL_TEXT);
        else
            drawTextCentered(TextFormat("Save %s to bodies.txt?", typedBody[1].name), screenW / 2.0f, panelY + 100 * scale, (int)(24 * scale), COL_TEXT);
        drawTextCentered("Saved bodies appear in Load & Run next time.", screenW / 2.0f, panelY + 136 * scale, small, COL_MUTED);

        if (drawButton((Rectangle){ screenW / 2 - buttonW - 10 * scale, buttonY, buttonW, buttonH }, "Yes", COL_GOLD, ICON_NONE)) {
            if (count == 2) {
                saveStep = 1;
            }
            else {
                saveWhich[0] = 0;
                saveWhich[1] = 1;
                saveStep = 2;
            }
        }
        if (drawButton((Rectangle){ screenW / 2 + 10 * scale, buttonY, buttonW, buttonH }, "No", COL_MUTED, ICON_NONE)) {
            showToast("Nothing saved", 1);
            next = SCREEN_MAIN;
        }
    }
    else if (saveStep == 1) {
        float choiceW = 190 * scale;
        float choiceX = (screenW - (3 * choiceW + 28 * scale)) / 2;
        drawTextCentered("Which bodies?", screenW / 2.0f, panelY + 100 * scale, (int)(24 * scale), COL_TEXT);

        if (drawButton((Rectangle){ choiceX, buttonY, choiceW, buttonH }, TextFormat("1: %s", typedBody[0].name), COL_GOLD, ICON_NONE)) {
            saveWhich[0] = 1;
            saveWhich[1] = 0;
            saveStep = 2;
        }
        if (drawButton((Rectangle){ choiceX + choiceW + 14 * scale, buttonY, choiceW, buttonH }, TextFormat("2: %s", typedBody[1].name), COL_CYAN, ICON_NONE)) {
            saveWhich[0] = 0;
            saveWhich[1] = 1;
            saveStep = 2;
        }
        if (drawButton((Rectangle){ choiceX + 2 * (choiceW + 14 * scale), buttonY, choiceW, buttonH }, "Both", COL_VIOLET, ICON_NONE)) {
            saveWhich[0] = 1;
            saveWhich[1] = 1;
            saveStep = 2;
        }
    }
    else {
        drawTextCentered(TextFormat("Save %s to bodies.txt?", names), screenW / 2.0f, panelY + 100 * scale, (int)(24 * scale), COL_TEXT);
        drawTextCentered("Please confirm.", screenW / 2.0f, panelY + 136 * scale, small, COL_MUTED);

        if (drawButton((Rectangle){ screenW / 2 - buttonW - 10 * scale, buttonY, buttonW, buttonH }, "Confirm", COL_GOLD, ICON_SAVE)) {
            int saved = 0;
            int failed = 0;
            for(int k=0; k<2; k++) {
                if (!saveWhich[k])
                    continue;
                if (saveBody(&typedBody[k]) == 1)
                    saved++;
                else
                    failed++;
            }
            if (failed == 0)
                showToast(TextFormat("Saved %s to bodies.txt", names), 1);
            else if (saved > 0)
                showToast("Saved one - the other name is already in bodies.txt", 0);
            else
                showToast("Not saved - that name is already in bodies.txt (or the file is full)", 0);
            next = SCREEN_MAIN;
        }
        if (drawButton((Rectangle){ screenW / 2 + 10 * scale, buttonY, buttonW, buttonH }, "Cancel", COL_MUTED, ICON_NONE))
            saveStep = 0;
    }

    if (drawHeader("SAVE NEW BODIES?")) {
        showToast("Nothing saved", 1);
        next = SCREEN_MAIN;
    }
    if (next == SCREEN_MAIN)
        saveStep = 0;
    return next;
}
