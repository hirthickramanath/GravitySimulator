/* settings.c - choosing Auto or Manual time step, and the manual values
   Gravity Simulator - P L Hirthick Ramanath (26PT20), Svithin S (26PT35) */

#include <stdio.h>
#include <stdlib.h>
#include "gravity.h"

static FIELD setField[2] = {
    { "Time step (s)", "", 12, 1, 1, 1e7 },
    { "Number of steps", "", 6, 1, 1, 100000 }
};
static int filled = 0;

int drawSettings(SETTINGS *s) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int next = SCREEN_SETTINGS;
    float scale = getScale();
    float panelW = 600 * scale;
    float panelH = 400 * scale;
    float panelX = (screenW - panelW) / 2;
    float panelY = (screenH - panelH) / 2 + 36 * scale;
    int fontSize = (int)(18 * scale);

    if (!filled) {                    /* the boxes start with the current values, once */
        snprintf(setField[0].text, 32, "%g", s->dt);
        snprintf(setField[1].text, 32, "%d", s->steps);
        filled = 1;
    }
    if (focusedField > 1)
        focusedField = 0;
    if (IsKeyPressed(KEY_TAB))
        focusedField = (focusedField + 1) % 2;

    DrawRectangleRounded((Rectangle){ panelX, panelY, panelW, panelH }, 0.06f, 8, Fade(COL_PANEL, 0.85f));
    DrawText("Time step", (int)(panelX + 40 * scale), (int)(panelY + 34 * scale), (int)(22 * scale), COL_TEXT);

    Color autoCol = COL_MUTED;
    Color manualCol = COL_GOLD;
    if (s->autoStep) {
        autoCol = COL_GOLD;
        manualCol = COL_MUTED;
    }
    if (drawButton((Rectangle){ panelX + 220 * scale, panelY + 24 * scale, 160 * scale, 44 * scale }, "Auto", autoCol, ICON_NONE))
        s->autoStep = 1;
    if (drawButton((Rectangle){ panelX + 396 * scale, panelY + 24 * scale, 160 * scale, 44 * scale }, "Manual", manualCol, ICON_NONE))
        s->autoStep = 0;

    if (s->autoStep) {
        drawTextCentered("Auto: works out the orbit's period from Kepler's third law,", screenW / 2.0f, panelY + 100 * scale, fontSize, COL_MUTED);
        drawTextCentered(TextFormat("then runs one full orbit in %d steps.", AUTO_STEPS), screenW / 2.0f, panelY + 126 * scale, fontSize, COL_MUTED);
    }
    else {
        drawTextCentered("Manual: every run uses the values below.", screenW / 2.0f, panelY + 100 * scale, fontSize, COL_MUTED);
    }

    for(int i=0; i<2; i++)
        drawField(&setField[i], (Rectangle){ panelX + 40 * scale, panelY + 170 * scale + i * 54 * scale, panelW - 80 * scale, 40 * scale }, i);

    double days = atof(setField[0].text) * atof(setField[1].text) / 86400.0;
    drawTextCentered(TextFormat("Manual run covers %.1f days", days), screenW / 2.0f, panelY + 290 * scale, fontSize, COL_TEXT);

    int save = drawButton((Rectangle){ screenW / 2 - 130 * scale, panelY + 326 * scale, 260 * scale, 48 * scale }, "Save Manual Values", COL_CYAN, ICON_SAVE);
    if (save || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        double newDt, newSteps;
        if (readField(&setField[0], &newDt) && readField(&setField[1], &newSteps)) {
            s->dt = newDt;
            s->steps = (int)newSteps;
            showToast(TextFormat("Saved: dt = %g s, %d steps", s->dt, s->steps), 1);
        }
    }

    if (drawHeader("SETTINGS"))
        next = SCREEN_MAIN;
    return next;
}
