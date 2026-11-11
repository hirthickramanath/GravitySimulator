/* gravity.h - constants, colours, structs and the functions each file shares
   Gravity Simulator - P L Hirthick Ramanath (26PT20), Svithin S (26PT35) */

#ifndef GRAVITY_H
#define GRAVITY_H

#include <stdio.h>
#include "raylib.h"

#define G 6.67430e-11
#define FPS 60
#define SEC 30
#define FRAMES (FPS * SEC)
#define STARS 250
#define MAX_BODIES 100
#define AUTO_STEPS 4000
#define COLLIDE_DIST 1.0e7           /* closer than 10,000 km counts as a crash */
#define BODY_FILE "bodies.txt"
#define DATA_FILE "data.csv"

#define COL_BG_TOP    (Color){ 6, 8, 22, 255 }
#define COL_BG_BOTTOM (Color){ 16, 8, 34, 255 }
#define COL_PANEL     (Color){ 22, 26, 56, 255 }
#define COL_FIELD     (Color){ 10, 12, 30, 255 }
#define COL_GOLD      (Color){ 255, 196, 84, 255 }
#define COL_CYAN      (Color){ 94, 200, 255, 255 }
#define COL_VIOLET    (Color){ 170, 120, 255, 255 }
#define COL_GREEN     (Color){ 110, 230, 150, 255 }
#define COL_RED       (Color){ 255, 99, 120, 255 }
#define COL_TEXT      (Color){ 236, 240, 255, 255 }
#define COL_MUTED     (Color){ 136, 146, 190, 255 }

enum { SCREEN_MAIN, SCREEN_LIST, SCREEN_NEW, SCREEN_SETTINGS, SCREEN_RUN, SCREEN_SAVE, SCREEN_EXIT };
enum { ICON_NONE, ICON_ORBIT, ICON_NEW, ICON_BACK, ICON_PLAY, ICON_SAVE, ICON_DELETE, ICON_REPLAY, ICON_EDIT };
enum { ORBIT_CLOSED, ORBIT_ESCAPE, ORBIT_COLLIDE };

struct body {
    char name[20];
    char symbol;
    double mass;
    double x;
    double y;
    double vx;
    double vy;
    double v;
    double ax;
    double ay;
};

typedef struct body BODY;

struct pair {
    double dx;
    double dy;
    double r;
    double f;
    double fx;
    double fy;
    double KE;
    double PE;
    double TE;
    double eps;
};

typedef struct pair PAIR;

struct simulationSettings {
    double dt;
    int steps;
    double t;
    int autoStep;      /* 1 = work out dt and steps from the orbit */
};

typedef struct simulationSettings SETTINGS;

struct frame {
    double t;
    double x[2];
    double y[2];
    double vx[2];
    double vy[2];
    double r;
    double f;
    double v;
    double TE;
};

typedef struct frame FRAME;

struct field {
    char label[28];
    char text[32];
    int maxLen;
    int numeric;
    double min;
    double max;
};

typedef struct field FIELD;

/* loadData.c */
extern char bodyFileError[100];
int listBodies(BODY *, int);
int loadBody(const char *, BODY *);
int saveBody(const BODY *);
int changeBody(const char *, const BODY *);

/* physics.c */
int checkOrbit(const BODY *, SETTINGS *, double *);
int runSimulation(BODY *, PAIR *, SETTINGS *, double *);

/* playback.c */
int loadPlayback(const char *, int);
int drawPlayback(void);

/* ui.c */
extern int focusedField;
extern int cursorKind;
float getScale(void);
Color getBodyColor(const char *, int);
void drawGlow(Vector2, float, Color, float);
void drawBackground(void);
void drawTextCentered(const char *, float, float, int, Color);
int drawButton(Rectangle, const char *, Color, int);
int drawHeader(const char *);
void drawField(FIELD *, Rectangle, int);
int readField(FIELD *, double *);
void showToast(const char *, int);
void drawOverlay(void);

/* screens.c */
int drawMainMenu(void);
int drawBodyList(SETTINGS *);
int drawNewSimulation(SETTINGS *);
int drawSaveScreen(void);

/* settings.c */
int drawSettings(SETTINGS *);

#endif
