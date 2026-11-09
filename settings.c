#include <stdio.h>
#include <stdlib.h>
#include "gravity.h"

static FIELD setField[2] = {
    { "Time step (s)",   "", 12, 1, 1, 1e7 },
    { "Number of steps", "", 6,  1, 1, 100000 }
};

/* the Settings screen: Auto/Manual time step, the manual values, and sound on/off */
int drawSettings(SETTINGS *s) {
    int W = GetScreenWidth(), H = GetScreenHeight();
    float u = getScale();
    int next = SCREEN_SETTINGS;

    if (setField[0].text[0] == '\0' && setField[1].text[0] == '\0') {
        snprintf(setField[0].text, 32, "%g", s->dt);
        snprintf(setField[1].text, 32, "%d", s->steps);
    }
    if (focusedField > 1)
        focusedField = 0;
    if (IsKeyPressed(KEY_TAB))
        focusedField = (focusedField + 1) % 2;

    float pw = 600 * u, ph = 500 * u;
    float px = (W - pw) / 2, py = (H - ph) / 2 + 36 * u;
    DrawRectangleRounded((Rectangle){ px, py, pw, ph }, 0.06f, 8, Fade(COL_PANEL, 0.85f));

    /* ---------- Auto / Manual switch ---------- */
    DrawText("Time step", (int)(px + 40 * u), (int)(py + 34 * u), (int)(22 * u), COL_TEXT);
    if (drawButton((Rectangle){ px + 220 * u, py + 24 * u, 160 * u, 44 * u }, "Auto", s->autoStep ? COL_GOLD : COL_MUTED, ICON_NONE))
        s->autoStep = 1;
    if (drawButton((Rectangle){ px + 396 * u, py + 24 * u, 160 * u, 44 * u }, "Manual", s->autoStep ? COL_MUTED : COL_GOLD, ICON_NONE))
        s->autoStep = 0;

    int fs = (int)(18 * u);
    if (s->autoStep) {
        drawTextCentered("Auto: works out the orbit's period from Kepler's third law,", W / 2.0f, py + 100 * u, fs, COL_MUTED);
        drawTextCentered(TextFormat("then runs one full orbit in %d steps.", AUTO_STEPS), W / 2.0f, py + 126 * u, fs, COL_MUTED);
    }
    else {
        drawTextCentered("Manual: every run uses the values below.", W / 2.0f, py + 100 * u, fs, COL_MUTED);
    }

    /* ---------- manual values ---------- */
    for (int i=0; i<2; i++)
        drawField(&setField[i], (Rectangle){ px + 40 * u, py + 170 * u + i * 54 * u, pw - 80 * u, 40 * u }, i);

    double dt = atof(setField[0].text), steps = atof(setField[1].text);
    drawTextCentered(TextFormat("Manual run covers %.1f days", dt * steps / 86400.0), W / 2.0f, py + 290 * u, fs, COL_TEXT);

    int save = drawButton((Rectangle){ W / 2 - 130 * u, py + 326 * u, 260 * u, 48 * u }, "Save Manual Values", COL_CYAN, ICON_SAVE);
    if (save || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        double newDt, newSteps;
        if (readField(&setField[0], &newDt) && readField(&setField[1], &newSteps)) {
            s->dt = newDt;
            s->steps = (int)newSteps;
            showToast(TextFormat("Saved: dt = %g s, %d steps", s->dt, s->steps), 1);
        }
    }

    /* ---------- sound on / off ---------- */
    DrawRectangle((int)(px + 40 * u), (int)(py + 400 * u), (int)(pw - 80 * u), 1, Fade(COL_MUTED, 0.4f));
    DrawText("Sound", (int)(px + 40 * u), (int)(py + 432 * u), (int)(22 * u), COL_TEXT);
    if (drawButton((Rectangle){ px + 220 * u, py + 422 * u, 160 * u, 44 * u }, "On", soundOn ? COL_GOLD : COL_MUTED, ICON_NONE))
        soundOn = 1;
    if (drawButton((Rectangle){ px + 396 * u, py + 422 * u, 160 * u, 44 * u }, "Off", soundOn ? COL_MUTED : COL_GOLD, ICON_NONE))
        soundOn = 0;

    if (drawHeader("SETTINGS"))
        next = SCREEN_MAIN;
    return next;
}
