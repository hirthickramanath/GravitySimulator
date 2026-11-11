/* loadData.c - reading, saving, editing and deleting bodies in bodies.txt
   Gravity Simulator - P L Hirthick Ramanath (26PT20), Svithin S (26PT35) */

#include <stdio.h>
#include <string.h>
#include "gravity.h"

char bodyFileError[100] = "";

/* reads every saved body into list[] and returns how many.
   blank lines and # lines are skipped; the first broken line is noted in bodyFileError */
int listBodies(BODY *list, int max) {
    FILE *fp;
    char line[256];
    char first[2];
    int count = 0;
    int lineNo = 0;

    bodyFileError[0] = '\0';
    fp = fopen(BODY_FILE, "r");
    if (fp == NULL)
        return 0;

    while (count < max && fgets(line, sizeof line, fp) != NULL) {
        lineNo++;
        if (sscanf(line, " %1s", first) != 1 || first[0] == '#')
            continue;

        BODY *b = &list[count];
        int ok = (sscanf(line, "%19s %c %lf %lf %lf %lf %lf", b->name, &b->symbol, &b->mass, &b->x, &b->y, &b->vx, &b->vy) == 7);
        if (ok && b->mass > 0 && strchr(b->name, ',') == NULL)
            count++;
        else if (bodyFileError[0] == '\0')
            snprintf(bodyFileError, sizeof bodyFileError, "bodies.txt line %d skipped: expected name symbol mass x y vx vy", lineNo);
    }

    fclose(fp);
    return count;
}

/* searches bodies.txt by name and copies that body into *b. returns 1 if found */
int loadBody(const char *name, BODY *b) {
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);

    for(int i=0; i<n; i++) {
        if (strcmp(list[i].name, name) == 0) {
            *b = list[i];
            return 1;
        }
    }
    return 0;
}

/* adds a new body to the end of bodies.txt.
   returns 1 if saved, 0 if the name is taken, -1 if the file won't open, -2 if it is full */
int saveBody(const BODY *b) {
    FILE *fp;
    BODY list[MAX_BODIES];

    if (loadBody(b->name, &list[0]))
        return 0;
    if (listBodies(list, MAX_BODIES) >= MAX_BODIES)
        return -2;

    fp = fopen(BODY_FILE, "a");
    if (fp == NULL)
        return -1;

    fprintf(fp, "%s %c %.10g %.10g %.10g %.10g %.10g\n", b->name, b->symbol, b->mass, b->x, b->y, b->vx, b->vy);
    fclose(fp);
    return 1;
}

/* replaces the body called oldName with *b, or deletes it if b is NULL, and rewrites the file.
   returns 1 if done, 0 if oldName isn't there, -1 if the file won't open, -3 if the new name is taken */
int changeBody(const char *oldName, const BODY *b) {
    FILE *fp;
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);
    int kept = 0;
    int found = 0;

    for(int i=0; i<n; i++) {
        if (strcmp(list[i].name, oldName) == 0) {
            found = 1;
            if (b != NULL) {
                list[kept] = *b;
                kept++;
            }
        }
        else if (b != NULL && strcmp(list[i].name, b->name) == 0) {
            return -3;
        }
        else {
            list[kept] = list[i];
            kept++;
        }
    }
    if (!found)
        return 0;

    fp = fopen(BODY_FILE, "w");
    if (fp == NULL)
        return -1;

    fprintf(fp, "# name symbol mass(kg) x(m) y(m) vx(m/s) vy(m/s)\n");
    for(int i=0; i<kept; i++) {
        fprintf(fp, "%s %c %.10g %.10g %.10g %.10g %.10g\n", list[i].name, list[i].symbol, list[i].mass, list[i].x, list[i].y, list[i].vx, list[i].vy);
    }
    fclose(fp);
    return 1;
}
