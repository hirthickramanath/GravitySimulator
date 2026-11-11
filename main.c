/* main.c - opens the window and switches between the screens
   Gravity Simulator - P L Hirthick Ramanath (26PT20), Svithin S (26PT35) */

#include <stdio.h>
#include "gravity.h"

int main(void) {
    SETTINGS s = {3600, 8766, 0, 1};
    int screen = SCREEN_MAIN;
    int previous = SCREEN_MAIN;
    float fade = 1.0f;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1100, 700, "Gravity Simulation");
    SetWindowMinSize(800, 560);
    SetExitKey(KEY_NULL);             /* ESC means Back here, not quit */
    SetTargetFPS(FPS);

    while (screen != SCREEN_EXIT && !WindowShouldClose()) {
        if (screen != previous) {     /* a new screen starts at its first box and fades in */
            focusedField = 0;
            previous = screen;
            fade = 1.0f;
        }

        BeginDrawing();
        drawBackground();

        switch (screen) {
            case SCREEN_MAIN:
                screen = drawMainMenu();
                break;
            case SCREEN_LIST:
                screen = drawBodyList(&s);
                break;
            case SCREEN_NEW:
                screen = drawNewSimulation(&s);
                break;
            case SCREEN_SAVE:
                screen = drawSaveScreen();
                break;
            case SCREEN_SETTINGS:
                screen = drawSettings(&s);
                break;
            case SCREEN_RUN:
                screen = drawPlayback();
                break;
        }

        if (fade > 0) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(COL_BG_TOP, fade));
            fade -= GetFrameTime() * 4.0f;
        }
        drawOverlay();
        EndDrawing();
    }

    loadPlayback(NULL, 0);
    CloseWindow();
    return 0;
}
