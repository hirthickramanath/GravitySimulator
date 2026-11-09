#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gravity.h"

int focusedField = 0;
int cursorKind = CURSOR_ARROW;       /* set by whatever the mouse is over, drawn by drawCursor */
int soundOn = 1;

static char toastText[128];
static int toastGood = 1;
static float toastTime = 0;

/* how much to enlarge or shrink everything: 1.0 at the original 1100 x 700 window */
float getScale(void) {
    float sx = GetScreenWidth() / 1100.0f;
    float sy = GetScreenHeight() / 700.0f;
    return (sx < sy) ? sx : sy;
}

/* colour of a body, looked up by its name (used by the list and the 3D view) */
Color getBodyColor(const char *name, int index) {
    const char *names[13] = { "Sun", "Mercury", "Venus", "Earth", "Moon", "Mars", "Jupiter",
                              "Saturn", "Uranus", "Neptune", "Comet", "Alpha", "Beta" };
    const Color colors[13] = { { 255, 203, 0, 255 },  { 170, 160, 150, 255 }, { 230, 200, 140, 255 },
                               { 80, 160, 255, 255 }, { 200, 200, 200, 255 }, { 220, 100, 70, 255 },
                               { 215, 175, 130, 255 }, { 230, 210, 150, 255 }, { 150, 220, 230, 255 },
                               { 90, 120, 255, 255 }, { 120, 220, 225, 255 }, { 255, 161, 0, 255 },
                               { 255, 120, 90, 255 } };
    for (int i=0; i<13; i++)
        if (strcmp(name, names[i]) == 0)
            return colors[i];
    return (index == 0) ? COL_GOLD : COL_CYAN;
}

/* writes a distance into value[] and returns its unit: km, or m for tiny distances */
const char *formatDistance(double metres, char *value) {
    if (metres >= 1000) {
        snprintf(value, 32, "%.0f", metres / 1000);
        return "km";
    }
    snprintf(value, 32, "%.0f", metres);
    return "m";
}

/* writes a speed into value[] in km/s and returns the unit */
const char *formatSpeed(double ms, char *value) {
    snprintf(value, 32, "%.2f", ms / 1000);
    return "km/s";
}

/* soft round glow: many see-through circles stacked, so the centre is brightest.
   (works on every raylib version, unlike DrawCircleGradient whose arguments changed) */
void drawGlow(Vector2 c, float radius, Color col, float strength) {
    for (int i=0; i<16; i++)
        DrawCircleV(c, radius * (1.0f - i / 16.0f), Fade(col, strength / 16.0f));
}

/* deep-space background: gradient sky, two nebula glows, twinkling stars,
   and a shooting star every 7 seconds. called once at the start of every frame */
void drawBackground(void) {
    static float starX[STARS], starY[STARS], starPhase[STARS];
    static int made = 0;
    int W = GetScreenWidth(), H = GetScreenHeight();
    float u = getScale();
    float t = (float)GetTime();

    if (!made) {
        for (int i=0; i<STARS; i++) {
            starX[i] = GetRandomValue(0, 10000) / 10000.0f;     /* 0..1 of the width  */
            starY[i] = GetRandomValue(0, 10000) / 10000.0f;     /* 0..1 of the height */
            starPhase[i] = GetRandomValue(0, 628) / 100.0f;
        }
        made = 1;
    }

    ClearBackground(COL_BG_TOP);          /* also clears the depth buffer the 3D view needs */
    DrawRectangleGradientV(0, 0, W, H, COL_BG_TOP, COL_BG_BOTTOM);
    drawGlow((Vector2){ W * 0.82f, H * 0.2f }, H * 0.6f, COL_VIOLET, 0.12f);
    drawGlow((Vector2){ W * 0.12f, H * 0.85f }, H * 0.5f, COL_CYAN, 0.08f);

    for (int i=0; i<STARS; i++) {
        float glow = 0.35f + 0.3f * sinf(t * 1.5f + starPhase[i]);
        DrawCircleV((Vector2){ starX[i] * W, starY[i] * H }, (i % 7 == 0) ? 1.6f : 1.0f, Fade(COL_TEXT, glow));
    }

    float phase = fmodf(t, 7.0f);
    if (phase < 0.9f) {
        int cycle = (int)(t / 7.0f);
        float p = phase / 0.9f;
        Vector2 start = { W * ((cycle * 37) % 70) / 100.0f, H * ((cycle * 53) % 35) / 100.0f };
        Vector2 head = { start.x + p * W * 0.35f, start.y + p * W * 0.16f };
        Vector2 tail = { head.x - 110 * u, head.y - 50 * u };
        DrawLineEx(tail, head, 2 * u, Fade(COL_TEXT, 0.8f * (1 - p)));
    }

    cursorKind = CURSOR_ARROW;            /* widgets drawn after this change it if hovered */
}

void drawTextCentered(const char *text, float cx, float y, int size, Color c) {
    DrawText(text, (int)(cx - MeasureText(text, size) / 2.0f), (int)y, size, c);
}

/* small pictures made from circles and lines; c = centre, s = size */
void drawIcon(int icon, Vector2 c, float s, Color col) {
    float t = (float)GetTime();
    switch (icon) {
        case ICON_ORBIT:
            DrawCircleV(c, s * 0.32f, col);
            DrawRing(c, s * 0.78f, s * 0.86f, 0, 360, 48, Fade(col, 0.6f));
            DrawCircleV((Vector2){ c.x + s * 0.82f * cosf(t), c.y + s * 0.82f * sinf(t) }, s * 0.16f, col);
            break;
        case ICON_NEW:
            DrawLineEx((Vector2){ c.x - s * 0.7f, c.y }, (Vector2){ c.x + s * 0.7f, c.y }, s * 0.24f, col);
            DrawLineEx((Vector2){ c.x, c.y - s * 0.7f }, (Vector2){ c.x, c.y + s * 0.7f }, s * 0.24f, col);
            break;
        case ICON_GEAR:
            for (int i=0; i<8; i++) {
                float a = i * 45.0f * DEG2RAD;
                DrawLineEx((Vector2){ c.x + cosf(a) * s * 0.6f, c.y + sinf(a) * s * 0.6f },
                           (Vector2){ c.x + cosf(a) * s * 0.95f, c.y + sinf(a) * s * 0.95f }, s * 0.28f, col);
            }
            DrawRing(c, s * 0.35f, s * 0.7f, 0, 360, 32, col);
            break;
        case ICON_EXIT:
            DrawRing(c, s * 0.82f, s * 0.98f, 0, 360, 32, col);
            DrawLineEx((Vector2){ c.x - s * 0.4f, c.y - s * 0.4f }, (Vector2){ c.x + s * 0.4f, c.y + s * 0.4f }, s * 0.16f, col);
            DrawLineEx((Vector2){ c.x + s * 0.4f, c.y - s * 0.4f }, (Vector2){ c.x - s * 0.4f, c.y + s * 0.4f }, s * 0.16f, col);
            break;
        case ICON_BACK:
            DrawLineEx((Vector2){ c.x + s * 0.3f, c.y - s * 0.7f }, (Vector2){ c.x - s * 0.4f, c.y }, s * 0.22f, col);
            DrawLineEx((Vector2){ c.x - s * 0.4f, c.y }, (Vector2){ c.x + s * 0.3f, c.y + s * 0.7f }, s * 0.22f, col);
            break;
        case ICON_PLAY: {
            Vector2 a = { c.x - s * 0.5f, c.y - s * 0.7f };
            Vector2 b = { c.x - s * 0.5f, c.y + s * 0.7f };
            Vector2 d = { c.x + s * 0.7f, c.y };
            DrawTriangle(a, b, d, col);      /* one of these two orders is the visible side */
            DrawTriangle(a, d, b, col);
        } break;
        case ICON_SAVE:
            DrawRectangleLinesEx((Rectangle){ c.x - s * 0.7f, c.y - s * 0.7f, s * 1.4f, s * 1.4f }, s * 0.16f, col);
            DrawRectangleRec((Rectangle){ c.x - s * 0.4f, c.y - s * 0.7f, s * 0.8f, s * 0.45f }, col);
            DrawRectangleRec((Rectangle){ c.x - s * 0.45f, c.y + s * 0.15f, s * 0.9f, s * 0.12f }, col);
            break;
        case ICON_DELETE:
            DrawRectangleRec((Rectangle){ c.x - s * 0.75f, c.y - s * 0.62f, s * 1.5f, s * 0.16f }, col);
            DrawRectangleRec((Rectangle){ c.x - s * 0.2f, c.y - s * 0.82f, s * 0.4f, s * 0.2f }, col);
            DrawRectangleLinesEx((Rectangle){ c.x - s * 0.55f, c.y - s * 0.4f, s * 1.1f, s * 1.2f }, s * 0.14f, col);
            break;
        case ICON_EDIT:                                  /* a pencil */
            DrawLineEx((Vector2){ c.x - s * 0.6f, c.y + s * 0.6f }, (Vector2){ c.x + s * 0.5f, c.y - s * 0.5f }, s * 0.32f, col);
            DrawTriangle((Vector2){ c.x - s * 0.82f, c.y + s * 0.82f }, (Vector2){ c.x - s * 0.42f, c.y + s * 0.72f },
                         (Vector2){ c.x - s * 0.72f, c.y + s * 0.42f }, col);
            DrawTriangle((Vector2){ c.x - s * 0.82f, c.y + s * 0.82f }, (Vector2){ c.x - s * 0.72f, c.y + s * 0.42f },
                         (Vector2){ c.x - s * 0.42f, c.y + s * 0.72f }, col);
            break;
        case ICON_REPLAY:
            DrawRing(c, s * 0.55f, s * 0.75f, 40, 330, 32, col);
            DrawCircleV((Vector2){ c.x + s * 0.65f * cosf(40 * DEG2RAD), c.y + s * 0.65f * sinf(40 * DEG2RAD) }, s * 0.22f, col);
            break;
        default:
            break;
    }
}

/* a glassy rounded button: tinted with 'accent', outline glows on hover.
   returns 1 on the frame it is clicked */
int drawButton(Rectangle r, const char *label, Color accent, int icon) {
    float u = getScale();
    int over = CheckCollisionPointRec(GetMousePosition(), r);
    if (over)
        cursorKind = CURSOR_HAND;

    DrawRectangleRounded(r, 0.35f, 12, Fade(accent, over ? 0.32f : 0.14f));
    DrawRectangleRoundedLinesEx(r, 0.35f, 12, over ? 2.0f * u : 1.0f, Fade(accent, over ? 1.0f : 0.55f));

    int fs = (int)fminf(22 * u, r.height * 0.42f);
    if (label[0] == '\0') {                                  /* icon only */
        drawIcon(icon, (Vector2){ r.x + r.width / 2, r.y + r.height / 2 }, r.height * 0.28f, accent);
    }
    else if (icon == ICON_NONE) {                            /* text only, centred */
        drawTextCentered(label, r.x + r.width / 2, r.y + (r.height - fs) / 2, fs, COL_TEXT);
    }
    else {                                                   /* icon on the left, text after it */
        drawIcon(icon, (Vector2){ r.x + r.height * 0.55f, r.y + r.height / 2 }, r.height * 0.22f, accent);
        DrawText(label, (int)(r.x + r.height * 1.05f), (int)(r.y + (r.height - fs) / 2), fs, COL_TEXT);
    }

    int clicked = over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (clicked)
        playSound(SOUND_CLICK);
    return clicked;
}

/* top bar: Back button, title, and a gold line fading out to the right.
   returns 1 if Back or ESC was pressed */
int drawHeader(const char *title) {
    int W = GetScreenWidth();
    float u = getScale();

    DrawRectangle(0, 0, W, (int)(76 * u), Fade(COL_BG_TOP, 0.85f));
    DrawRectangleGradientH(0, (int)(76 * u), W, (int)(2 * u) + 1, COL_GOLD, Fade(COL_CYAN, 0.0f));
    DrawText(title, (int)(144 * u), (int)(24 * u), (int)(30 * u), COL_TEXT);

    int back = drawButton((Rectangle){ 20 * u, 18 * u, 104 * u, 40 * u }, "Back", COL_CYAN, ICON_BACK);
    return back || IsKeyPressed(KEY_ESCAPE);
}

/* one labelled text box in a row: label on the left, box on the right.
   number boxes show their allowed range under the label.
   clicking the box focuses it; the focused box receives typing */
void drawField(FIELD *f, Rectangle row, int index) {
    float u = getScale();
    int fs = (int)(18 * u);
    Rectangle box = { row.x + row.width * 0.42f, row.y, row.width * 0.58f, row.height };

    if (f->numeric) {
        DrawText(f->label, (int)row.x, (int)(row.y + row.height / 2 - fs), fs, COL_MUTED);
        DrawText(TextFormat("range %g to %g", f->min, f->max), (int)row.x, (int)(row.y + row.height / 2 + 1),
                 (int)fmaxf(11, 13 * u), Fade(COL_GOLD, 0.8f));
    }
    else
        DrawText(f->label, (int)row.x, (int)(row.y + (row.height - fs) / 2), fs, COL_MUTED);

    if (CheckCollisionPointRec(GetMousePosition(), box)) {
        cursorKind = CURSOR_TEXT;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            focusedField = index;
    }
    int focused = (focusedField == index);

    if (focused) {
        int ch, len;
        while ((ch = GetCharPressed()) > 0) {
            len = (int)strlen(f->text);
            if (ch == ' ')
                ch = '_';                                   /* the file is space separated */
            if (ch < 33 || ch > 126)
                continue;
            if (f->numeric && strchr("0123456789.-+eE", ch) == NULL)
                continue;
            if (len < f->maxLen) {
                f->text[len] = (char)ch;
                f->text[len + 1] = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            len = (int)strlen(f->text);
            if (len > 0)
                f->text[len - 1] = '\0';
        }
    }

    DrawRectangleRounded(box, 0.3f, 8, COL_FIELD);
    DrawRectangleRoundedLinesEx(box, 0.3f, 8, focused ? 2.0f : 1.0f, focused ? COL_GOLD : Fade(COL_MUTED, 0.5f));
    DrawText(f->text, (int)(box.x + 10 * u), (int)(box.y + (box.height - fs) / 2), fs, COL_TEXT);
    if (focused && ((int)(GetTime() * 2) % 2) == 0)
        DrawRectangle((int)(box.x + 12 * u + MeasureText(f->text, fs)), (int)(box.y + box.height * 0.2f), 2, (int)(box.height * 0.6f), COL_GOLD);
}

/* turns a numeric field's text into a number and checks its range.
   returns 1 if valid; otherwise shows a red message and returns 0 */
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

/* green (good = 1) or red (good = 0) message near the bottom for 4 seconds */
void showToast(const char *message, int good) {
    snprintf(toastText, sizeof toastText, "%s", message);
    toastGood = good;
    toastTime = 4.0f;
    if (!good)
        playSound(SOUND_ERROR);
}

void drawToast(void) {
    if (toastTime <= 0)
        return;

    float u = getScale();
    int fs = (int)(20 * u);
    toastTime -= GetFrameTime();
    float a = (toastTime < 0.5f) ? toastTime / 0.5f : 1.0f;            /* fade out at the end */
    float tw = MeasureText(toastText, fs) + 50 * u;
    Rectangle r = { (GetScreenWidth() - tw) / 2, GetScreenHeight() - 56 * u, tw, 40 * u };
    Color c = toastGood ? (Color){ 40, 160, 110, 255 } : (Color){ 200, 60, 80, 255 };

    DrawRectangleRounded(r, 0.5f, 8, Fade(c, 0.9f * a));
    DrawText(toastText, (int)(r.x + 25 * u), (int)(r.y + (r.height - fs) / 2), fs, Fade(WHITE, a));
}

/* the space cursor: a glowing dot with a comet tail. Over something clickable it gets
   a gold ring, over a text box it becomes an I-beam, while dragging it shows four arcs.
   A click sends out a ring that grows and fades. Drawn last, on top of everything */
void drawCursor(void) {
    static Vector2 trail[12];
    static float pulse = 0;
    float u = getScale();
    Vector2 m = GetMousePosition();

    for (int i=11; i>0; i--)                        /* move the tail along */
        trail[i] = trail[i - 1];
    trail[0] = m;

    if (!IsCursorOnScreen())
        return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        pulse = 1.0f;
    pulse -= GetFrameTime() * 3.0f;

    for (int i=1; i<12; i++)
        DrawCircleV(trail[i], (3.0f - i * 0.2f) * u, Fade(COL_GOLD, 0.25f * (1.0f - i / 12.0f)));

    switch (cursorKind) {
        case CURSOR_HAND:
            DrawRing(m, 9 * u, 11 * u, 0, 360, 32, COL_GOLD);
            DrawCircleV(m, 3 * u, COL_GOLD);
            break;
        case CURSOR_TEXT:
            DrawLineEx((Vector2){ m.x, m.y - 10 * u }, (Vector2){ m.x, m.y + 10 * u }, 2 * u, COL_TEXT);
            DrawLineEx((Vector2){ m.x - 4 * u, m.y - 10 * u }, (Vector2){ m.x + 4 * u, m.y - 10 * u }, 2 * u, COL_TEXT);
            DrawLineEx((Vector2){ m.x - 4 * u, m.y + 10 * u }, (Vector2){ m.x + 4 * u, m.y + 10 * u }, 2 * u, COL_TEXT);
            break;
        case CURSOR_DRAG:
            for (int i=0; i<4; i++)
                DrawRing(m, 10 * u, 12 * u, i * 90.0f + 15, i * 90.0f + 75, 8, COL_CYAN);
            DrawCircleV(m, 3 * u, COL_CYAN);
            break;
        default:
            drawGlow(m, 10 * u, COL_GOLD, 0.5f);
            DrawCircleV(m, 3.5f * u, COL_TEXT);
            break;
    }

    if (pulse > 0)
        DrawRing(m, (12 + (1 - pulse) * 20) * u, (14 + (1 - pulse) * 20) * u, 0, 360, 32, Fade(COL_GOLD, pulse));
}

/* ---------- sound effects: short tones made in code, so no sound files are needed ---------- */

static Sound sounds[3];
static int soundsReady = 0;

/* builds a tone that slides from f0 to f1 Hz over 'seconds' and fades out */
static Sound makeTone(float f0, float f1, float seconds, float volume) {
    int rate = 44100;
    int count = (int)(rate * seconds);
    short *data = (short *)malloc(count * sizeof(short));
    float phase = 0;

    for (int i=0; i<count; i++) {
        float t = (float)i / count;                     /* 0 at the start, 1 at the end */
        float freq = f0 + (f1 - f0) * t;
        float envelope = (1.0f - t) * fminf(1.0f, i / 200.0f);    /* quick fade-in, slow fade-out */
        phase += 2 * PI * freq / rate;
        data[i] = (short)(sinf(phase) * envelope * volume * 32767);
    }

    Wave wave = { .frameCount = (unsigned int)count, .sampleRate = (unsigned int)rate,
                  .sampleSize = 16, .channels = 1, .data = data };
    Sound sound = LoadSoundFromWave(wave);              /* copies the samples */
    free(data);
    return sound;
}

/* opens the speakers and makes the three sounds (once, at the start) */
void loadSounds(void) {
    InitAudioDevice();
    if (!IsAudioDeviceReady())
        return;                                         /* no speakers: stay silent */
    sounds[SOUND_CLICK] = makeTone(1400, 900, 0.06f, 0.25f);
    sounds[SOUND_RUN]   = makeTone(220, 880, 0.45f, 0.30f);
    sounds[SOUND_ERROR] = makeTone(300, 180, 0.18f, 0.30f);
    soundsReady = 1;
}

void playSound(int which) {
    if (soundsReady && soundOn)
        PlaySound(sounds[which]);
}

/* frees the sounds and closes the speakers (once, at the end) */
void unloadSounds(void) {
    if (soundsReady) {
        for (int i=0; i<3; i++)
            UnloadSound(sounds[i]);
        soundsReady = 0;
    }
    if (IsAudioDeviceReady())
        CloseAudioDevice();
}
