/* ui.c - the pieces every screen uses: background, buttons, text boxes and messages
   Gravity Simulator - P L Hirthick Ramanath (26PT20), Svithin S (26PT35) */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gravity.h"

int focusedField = 0;
int cursorKind = MOUSE_CURSOR_DEFAULT;

static char toastText[128];
static int toastGood = 1;
static float toastTime = 0;

/* 1.0 at the original 1100 x 700 window; every size is multiplied by this */
float getScale(void) {
    float sx = GetScreenWidth() / 1100.0f;
    float sy = GetScreenHeight() / 700.0f;
    if (sx < sy)
        return sx;
    return sy;
}

Color getBodyColor(const char *name, int index) {
    const char *names[13] = { "Sun", "Mercury", "Venus", "Earth", "Moon", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune", "Comet", "Alpha", "Beta" };
    const Color colors[13] = {
        { 255, 203, 0, 255 },        /* Sun */
        { 170, 160, 150, 255 },      /* Mercury */
        { 230, 200, 140, 255 },      /* Venus */
        { 80, 160, 255, 255 },       /* Earth */
        { 200, 200, 200, 255 },      /* Moon */
        { 220, 100, 70, 255 },       /* Mars */
        { 215, 175, 130, 255 },      /* Jupiter */
        { 230, 210, 150, 255 },      /* Saturn */
        { 150, 220, 230, 255 },      /* Uranus */
        { 90, 120, 255, 255 },       /* Neptune */
        { 120, 220, 225, 255 },      /* Comet */
        { 255, 161, 0, 255 },        /* Alpha */
        { 255, 120, 90, 255 }        /* Beta */
    };

    for(int i=0; i<13; i++) {
        if (strcmp(name, names[i]) == 0)
            return colors[i];
    }
    if (index == 0)
        return COL_GOLD;
    return COL_CYAN;
}

/* a soft glow made of see-through circles stacked on top of each other */
void drawGlow(Vector2 centre, float radius, Color col, float strength) {
    for(int i=0; i<16; i++)
        DrawCircleV(centre, radius * (1.0f - i / 16.0f), Fade(col, strength / 16.0f));
}

/* the sky behind every screen: gradient, two nebulas, twinkling stars and a shooting star */
void drawBackground(void) {
    static float starX[STARS], starY[STARS], starPhase[STARS];
    static int made = 0;
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    float scale = getScale();
    float t = (float)GetTime();

    if (!made) {
        for(int i=0; i<STARS; i++) {
            starX[i] = GetRandomValue(0, 10000) / 10000.0f;
            starY[i] = GetRandomValue(0, 10000) / 10000.0f;
            starPhase[i] = GetRandomValue(0, 628) / 100.0f;
        }
        made = 1;
    }

    ClearBackground(COL_BG_TOP);        /* this also clears the depth buffer, or the 3D view disappears */
    DrawRectangleGradientV(0, 0, screenW, screenH, COL_BG_TOP, COL_BG_BOTTOM);
    drawGlow((Vector2){ screenW * 0.82f, screenH * 0.2f }, screenH * 0.6f, COL_VIOLET, 0.12f);
    drawGlow((Vector2){ screenW * 0.12f, screenH * 0.85f }, screenH * 0.5f, COL_CYAN, 0.08f);

    for(int i=0; i<STARS; i++) {
        float brightness = 0.35f + 0.3f * sinf(t * 1.5f + starPhase[i]);
        float size = 1.0f;
        if (i % 7 == 0)
            size = 1.6f;
        DrawCircleV((Vector2){ starX[i] * screenW, starY[i] * screenH }, size, Fade(COL_TEXT, brightness));
    }

    /* every 7 seconds a shooting star crosses for just under a second, starting somewhere new each time */
    float phase = fmodf(t, 7.0f);
    if (phase < 0.9f) {
        int cycle = (int)(t / 7.0f);
        float progress = phase / 0.9f;
        float startX = screenW * ((cycle * 37) % 70) / 100.0f;
        float startY = screenH * ((cycle * 53) % 35) / 100.0f;
        Vector2 head = { startX + progress * screenW * 0.35f, startY + progress * screenW * 0.16f };
        Vector2 tail = { head.x - 110 * scale, head.y - 50 * scale };
        DrawLineEx(tail, head, 2 * scale, Fade(COL_TEXT, 0.8f * (1 - progress)));
    }

    cursorKind = MOUSE_CURSOR_DEFAULT;
}

void drawTextCentered(const char *text, float centreX, float y, int size, Color col) {
    DrawText(text, (int)(centreX - MeasureText(text, size) / 2.0f), (int)y, size, col);
}

/* a glassy button with a small picture on the left; returns 1 on the frame it is clicked */
int drawButton(Rectangle r, const char *label, Color col, int icon) {
    float scale = getScale();
    float size = r.height * 0.22f;
    float t = (float)GetTime();
    int over = CheckCollisionPointRec(GetMousePosition(), r);
    int fontSize = (int)fminf(22 * scale, r.height * 0.42f);
    Vector2 mid = { r.x + r.height * 0.55f, r.y + r.height / 2 };

    if (over) {
        cursorKind = MOUSE_CURSOR_POINTING_HAND;
        DrawRectangleRounded(r, 0.35f, 12, Fade(col, 0.32f));
        DrawRectangleRoundedLinesEx(r, 0.35f, 12, 2.0f * scale, col);
    }
    else {
        DrawRectangleRounded(r, 0.35f, 12, Fade(col, 0.14f));
        DrawRectangleRoundedLinesEx(r, 0.35f, 12, 1.0f, Fade(col, 0.55f));
    }

    if (icon == ICON_NONE)
        drawTextCentered(label, r.x + r.width / 2, r.y + (r.height - fontSize) / 2, fontSize, COL_TEXT);
    else
        DrawText(label, (int)(r.x + r.height * 1.05f), (int)(r.y + (r.height - fontSize) / 2), fontSize, COL_TEXT);

    /* DrawTriangle only shows one winding, so the triangles are drawn both ways round */
    switch (icon) {
        case ICON_ORBIT:
            DrawCircleV(mid, size * 0.32f, col);
            DrawRing(mid, size * 0.78f, size * 0.86f, 0, 360, 48, Fade(col, 0.6f));
            DrawCircleV((Vector2){ mid.x + size * 0.82f * cosf(t), mid.y + size * 0.82f * sinf(t) }, size * 0.16f, col);
            break;
        case ICON_NEW:
            DrawLineEx((Vector2){ mid.x - size * 0.7f, mid.y }, (Vector2){ mid.x + size * 0.7f, mid.y }, size * 0.24f, col);
            DrawLineEx((Vector2){ mid.x, mid.y - size * 0.7f }, (Vector2){ mid.x, mid.y + size * 0.7f }, size * 0.24f, col);
            break;
        case ICON_BACK:
            DrawLineEx((Vector2){ mid.x + size * 0.3f, mid.y - size * 0.7f }, (Vector2){ mid.x - size * 0.4f, mid.y }, size * 0.22f, col);
            DrawLineEx((Vector2){ mid.x - size * 0.4f, mid.y }, (Vector2){ mid.x + size * 0.3f, mid.y + size * 0.7f }, size * 0.22f, col);
            break;
        case ICON_PLAY:
            DrawTriangle((Vector2){ mid.x - size * 0.5f, mid.y - size * 0.7f }, (Vector2){ mid.x - size * 0.5f, mid.y + size * 0.7f }, (Vector2){ mid.x + size * 0.7f, mid.y }, col);
            DrawTriangle((Vector2){ mid.x - size * 0.5f, mid.y - size * 0.7f }, (Vector2){ mid.x + size * 0.7f, mid.y }, (Vector2){ mid.x - size * 0.5f, mid.y + size * 0.7f }, col);
            break;
        case ICON_SAVE:
            DrawRectangleLinesEx((Rectangle){ mid.x - size * 0.7f, mid.y - size * 0.7f, size * 1.4f, size * 1.4f }, size * 0.16f, col);
            DrawRectangleRec((Rectangle){ mid.x - size * 0.4f, mid.y - size * 0.7f, size * 0.8f, size * 0.45f }, col);
            break;
        case ICON_DELETE:
            DrawRectangleRec((Rectangle){ mid.x - size * 0.75f, mid.y - size * 0.62f, size * 1.5f, size * 0.16f }, col);
            DrawRectangleLinesEx((Rectangle){ mid.x - size * 0.55f, mid.y - size * 0.4f, size * 1.1f, size * 1.2f }, size * 0.14f, col);
            break;
        case ICON_EDIT:
            DrawLineEx((Vector2){ mid.x - size * 0.6f, mid.y + size * 0.6f }, (Vector2){ mid.x + size * 0.5f, mid.y - size * 0.5f }, size * 0.32f, col);
            DrawTriangle((Vector2){ mid.x - size * 0.82f, mid.y + size * 0.82f }, (Vector2){ mid.x - size * 0.42f, mid.y + size * 0.72f }, (Vector2){ mid.x - size * 0.72f, mid.y + size * 0.42f }, col);
            DrawTriangle((Vector2){ mid.x - size * 0.82f, mid.y + size * 0.82f }, (Vector2){ mid.x - size * 0.72f, mid.y + size * 0.42f }, (Vector2){ mid.x - size * 0.42f, mid.y + size * 0.72f }, col);
            break;
        case ICON_REPLAY:
            DrawRing(mid, size * 0.55f, size * 0.75f, 40, 330, 32, col);
            DrawCircleV((Vector2){ mid.x + size * 0.65f * cosf(40 * DEG2RAD), mid.y + size * 0.65f * sinf(40 * DEG2RAD) }, size * 0.22f, col);
            break;
    }

    if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return 1;
    return 0;
}

/* the top bar with a Back button; returns 1 if Back or ESC was pressed */
int drawHeader(const char *title) {
    int screenW = GetScreenWidth();
    float scale = getScale();

    DrawRectangle(0, 0, screenW, (int)(76 * scale), Fade(COL_BG_TOP, 0.85f));
    DrawRectangleGradientH(0, (int)(76 * scale), screenW, (int)(2 * scale) + 1, COL_GOLD, Fade(COL_CYAN, 0.0f));
    DrawText(title, (int)(144 * scale), (int)(24 * scale), (int)(30 * scale), COL_TEXT);

    if (drawButton((Rectangle){ 20 * scale, 18 * scale, 104 * scale, 40 * scale }, "Back", COL_CYAN, ICON_BACK))
        return 1;
    if (IsKeyPressed(KEY_ESCAPE))
        return 1;
    return 0;
}

/* a label with a text box beside it. number boxes show their allowed range under the label */
void drawField(FIELD *f, Rectangle row, int index) {
    float scale = getScale();
    int fontSize = (int)(18 * scale);
    int len = (int)strlen(f->text);
    Rectangle box = { row.x + row.width * 0.42f, row.y, row.width * 0.58f, row.height };

    if (f->numeric) {
        DrawText(f->label, (int)row.x, (int)(row.y + row.height / 2 - fontSize), fontSize, COL_MUTED);
        DrawText(TextFormat("range %g to %g", f->min, f->max), (int)row.x, (int)(row.y + row.height / 2 + 1), (int)fmaxf(11, 13 * scale), Fade(COL_GOLD, 0.8f));
    }
    else {
        DrawText(f->label, (int)row.x, (int)(row.y + (row.height - fontSize) / 2), fontSize, COL_MUTED);
    }

    if (CheckCollisionPointRec(GetMousePosition(), box)) {
        cursorKind = MOUSE_CURSOR_IBEAM;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            focusedField = index;
    }
    int focused = (focusedField == index);

    if (focused) {
        int ch = GetCharPressed();
        while (ch > 0) {
            if (ch == ' ')
                ch = '_';               /* bodies.txt is split by spaces, so no spaces in names */

            int allowed = (ch >= 33 && ch <= 126 && len < f->maxLen);
            if (f->numeric && strchr("0123456789.-+eE", ch) == NULL)
                allowed = 0;
            if (!f->numeric && ch == ',')
                allowed = 0;            /* a comma would split the name across two columns of data.csv */
            if (!f->numeric && ch == '#' && len == 0)
                allowed = 0;            /* bodies.txt treats lines starting with # as comments */
            if (allowed) {
                f->text[len] = (char)ch;
                len++;
                f->text[len] = '\0';
            }
            ch = GetCharPressed();
        }
        if (len > 0 && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)))
            f->text[len - 1] = '\0';
    }

    DrawRectangleRounded(box, 0.3f, 8, COL_FIELD);
    if (focused)
        DrawRectangleRoundedLinesEx(box, 0.3f, 8, 2.0f, COL_GOLD);
    else
        DrawRectangleRoundedLinesEx(box, 0.3f, 8, 1.0f, Fade(COL_MUTED, 0.5f));
    DrawText(f->text, (int)(box.x + 10 * scale), (int)(box.y + (box.height - fontSize) / 2), fontSize, COL_TEXT);

    if (focused && ((int)(GetTime() * 2) % 2) == 0)
        DrawRectangle((int)(box.x + 12 * scale + MeasureText(f->text, fontSize)), (int)(box.y + box.height * 0.2f), 2, (int)(box.height * 0.6f), COL_GOLD);
}

/* turns a number box into a value and checks the range; shows a red message if it's wrong */
int readField(FIELD *f, double *value) {
    char *end;
    double v = strtod(f->text, &end);

    if (f->text[0] == '\0' || *end != '\0') {
        showToast(TextFormat("%s: please enter a number", f->label), 0);
        return 0;
    }
    if (v < f->min || v > f->max) {
        showToast(TextFormat("%s must be from %g to %g", f->label, f->min, f->max), 0);
        return 0;
    }
    *value = v;
    return 1;
}

/* a green (good) or red message at the bottom for 4 seconds */
void showToast(const char *message, int good) {
    snprintf(toastText, sizeof toastText, "%s", message);
    toastGood = good;
    toastTime = 4.0f;
}

/* drawn last every frame: the message at the bottom, and the right mouse pointer for whatever is under it */
void drawOverlay(void) {
    static int lastCursor = -1;
    float scale = getScale();
    int fontSize = (int)(20 * scale);

    if (toastTime > 0) {
        toastTime -= GetFrameTime();

        float alpha = 1.0f;
        if (toastTime < 0.5f)
            alpha = toastTime / 0.5f;           /* fade out over the last half second */

        Color back = { 200, 60, 80, 255 };
        if (toastGood)
            back = (Color){ 40, 160, 110, 255 };

        float toastW = MeasureText(toastText, fontSize) + 50 * scale;
        Rectangle r = { (GetScreenWidth() - toastW) / 2, GetScreenHeight() - 56 * scale, toastW, 40 * scale };
        DrawRectangleRounded(r, 0.5f, 8, Fade(back, 0.9f * alpha));
        DrawText(toastText, (int)(r.x + 25 * scale), (int)(r.y + (r.height - fontSize) / 2), fontSize, Fade(WHITE, alpha));
    }

    /* only tell raylib when the pointer actually changes */
    if (cursorKind != lastCursor) {
        SetMouseCursor(cursorKind);
        lastCursor = cursorKind;
    }
}
