#include <stdio.h>
#include "gravity.h"

int main(void) {
    SETTINGS s = {3600, 8766, 0, 1};
    int screen = SCREEN_MAIN;
    int previous = SCREEN_MAIN;
    float fade = 1.0f;                /* dark overlay that fades out after a screen change */

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1100, 700, "Gravity Simulation");
    SetWindowMinSize(800, 560);
    SetExitKey(KEY_NULL);             /* ESC means Back, not quit */
    SetTargetFPS(FPS);
    HideCursor();                     /* we draw our own space cursor */
    loadSounds();

    while (screen != SCREEN_EXIT && !WindowShouldClose()) {
        if (screen != previous) {     /* new screen: start typing in its first box, fade in */
            focusedField = 0;
            previous = screen;
            fade = 1.0f;
        }

        BeginDrawing();
        drawBackground();

        switch (screen) {
            case SCREEN_MAIN:     screen = drawMainMenu(); break;
            case SCREEN_LIST:     screen = drawBodyList(&s); break;
            case SCREEN_NEW:      screen = drawNewMenu(); break;
            case SCREEN_FORM:     screen = drawNewForm(&s); break;
            case SCREEN_SAVE:     screen = drawSaveScreen(); break;
            case SCREEN_SETTINGS: screen = drawSettings(&s); break;
            case SCREEN_RUN:      screen = drawRunScreen(); break;
        }

        if (fade > 0) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(COL_BG_TOP, fade));
            fade -= GetFrameTime() * 4.0f;            /* gone in a quarter of a second */
        }
        drawToast();
        drawCursor();
        EndDrawing();
    }

    unloadPlayback();
    unloadSounds();
    CloseWindow();
    return 0;
}
