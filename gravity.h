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
#define COLLIDE_DIST 1.0e7           /* passing closer than this (10,000 km) counts as a collision */
#define BODY_FILE "bodies.txt"
#define DATA_FILE "data.csv"

/* space colour palette */
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

/* screens */
enum { SCREEN_MAIN, SCREEN_LIST, SCREEN_NEW, SCREEN_FORM, SCREEN_SETTINGS,
       SCREEN_RUN, SCREEN_SAVE, SCREEN_EXIT };

/* icons */
enum { ICON_NONE, ICON_ORBIT, ICON_NEW, ICON_GEAR, ICON_EXIT, ICON_BACK, ICON_PLAY,
       ICON_SAVE, ICON_DELETE, ICON_REPLAY, ICON_EDIT };

/* how the custom cursor looks */
enum { CURSOR_ARROW, CURSOR_HAND, CURSOR_TEXT, CURSOR_DRAG };

/* sound effects */
enum { SOUND_CLICK, SOUND_RUN, SOUND_ERROR };

/* what kind of orbit two bodies are on */
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
    int autoStep;      /* 1 = choose dt and steps from the orbit, 0 = use dt and steps */
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

/* a text box the user types into */
struct field {
    char label[28];
    char text[32];
    int maxLen;
    int numeric;
    double min, max;
};

typedef struct field FIELD;

/* loadData.c */
extern char bodyFileError[100];
int loadBody(const char *, BODY *);
int saveBody(const BODY *);
int updateBody(const char *, const BODY *);
int deleteBody(const char *);
int listBodies(BODY *, int);

/* physics.c */
void chooseTimeStep(BODY *, SETTINGS *);
int classifyOrbit(const BODY *, double *);
int runSimulation(BODY *, PAIR *, SETTINGS *, double *);

/* window.c */
int loadPlayback(const char *);
int drawPlayback(void);
void unloadPlayback(void);

/* ui.c */
extern int focusedField;
extern int cursorKind;
extern int soundOn;
float getScale(void);
Color getBodyColor(const char *, int);
const char *formatDistance(double, char *);
const char *formatSpeed(double, char *);
void drawGlow(Vector2, float, Color, float);
void drawBackground(void);
void drawTextCentered(const char *, float, float, int, Color);
void drawIcon(int, Vector2, float, Color);
int drawButton(Rectangle, const char *, Color, int);
int drawHeader(const char *);
void drawField(FIELD *, Rectangle, int);
int readField(FIELD *, double *);
void showToast(const char *, int);
void drawToast(void);
void drawCursor(void);
void loadSounds(void);
void playSound(int);
void unloadSounds(void);

/* screens.c */
int drawMainMenu(void);
int drawBodyList(SETTINGS *);
int drawNewMenu(void);
int drawNewForm(SETTINGS *);
int drawSaveScreen(void);
int drawRunScreen(void);

/* settings.c */
int drawSettings(SETTINGS *);

#endif
