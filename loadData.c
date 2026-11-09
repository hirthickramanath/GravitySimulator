#include <stdio.h>
#include <string.h>
#include "gravity.h"

/* the first problem found the last time bodies.txt was read ("" if none) */
char bodyFileError[100] = "";

/* writes the whole list back to bodies.txt (used by edit and delete).
   returns 1 if it worked, -1 if the file can't be opened */
static int writeBodies(const BODY *list, int n) {
    FILE *fp = fopen(BODY_FILE, "w");
    if (fp == NULL)
        return -1;

    fprintf(fp, "# name symbol mass(kg) x(m) y(m) vx(m/s) vy(m/s)\n");
    for (int i=0; i<n; i++)
        fprintf(fp, "%s %c %g %g %g %g %g\n", list[i].name, list[i].symbol, list[i].mass,
                list[i].x, list[i].y, list[i].vx, list[i].vy);
    fclose(fp);
    return 1;
}

/* reads every saved body into list[]; returns how many.
   blank lines and lines starting with # are skipped; a broken line is skipped
   too, and the first one is reported in bodyFileError with its line number */
int listBodies(BODY *list, int max) {
    FILE *fp;
    char line[256];
    int count = 0, lineNo = 0;

    bodyFileError[0] = '\0';
    fp = fopen(BODY_FILE, "r");
    if (fp == NULL)
        return 0;

    while (count < max && fgets(line, sizeof line, fp) != NULL) {
        lineNo++;
        char first[2];
        if (sscanf(line, " %1s", first) != 1 || first[0] == '#')
            continue;                                         /* blank or comment */

        BODY *b = &list[count];
        if (sscanf(line, "%19s %c %lf %lf %lf %lf %lf", b->name, &b->symbol, &b->mass,
                   &b->x, &b->y, &b->vx, &b->vy) == 7 && b->mass > 0)
            count++;
        else if (bodyFileError[0] == '\0')
            snprintf(bodyFileError, sizeof bodyFileError,
                     "bodies.txt line %d skipped: expected name symbol mass x y vx vy", lineNo);
    }

    fclose(fp);
    return count;
}

/* searches bodies.txt for a body by name and copies its data into *b.
   returns 1 if found, 0 if not */
int loadBody(const char *name, BODY *b) {
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);

    for (int i=0; i<n; i++) {
        if (strcmp(list[i].name, name) == 0) {
            *b = list[i];
            return 1;
        }
    }
    return 0;
}

/* adds a body to the end of bodies.txt.
   returns 1 if saved, 0 if the name is taken, -1 if the file can't be opened, -2 if the file is full */
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

    fprintf(fp, "%s %c %g %g %g %g %g\n", b->name, b->symbol, b->mass, b->x, b->y, b->vx, b->vy);
    fclose(fp);
    return 1;
}

/* replaces the body called oldName with *b (the name may change too).
   returns 1 if saved, 0 if oldName isn't there, -1 if the file can't be opened,
   -3 if the new name already belongs to another body */
int updateBody(const char *oldName, const BODY *b) {
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);
    int found = -1;

    for (int i=0; i<n; i++) {
        if (strcmp(list[i].name, oldName) == 0)
            found = i;
        else if (strcmp(list[i].name, b->name) == 0)
            return -3;
    }
    if (found < 0)
        return 0;

    list[found] = *b;
    return writeBodies(list, n);
}

/* removes a body from bodies.txt by rewriting the file without it.
   returns 1 if deleted, 0 if not found, -1 if the file can't be opened */
int deleteBody(const char *name) {
    BODY list[MAX_BODIES];
    int n = listBodies(list, MAX_BODIES);
    int kept = 0, found = 0;

    for (int i=0; i<n; i++) {
        if (strcmp(list[i].name, name) == 0)
            found = 1;
        else
            list[kept++] = list[i];
    }
    if (!found)
        return 0;
    return writeBodies(list, kept);
}
