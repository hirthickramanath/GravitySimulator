#include <stdio.h>
#include "gravity.h"

void file(BODY *b, PAIR *p, SETTINGS *s) {
    FILE *fp;
    fp = fopen("data.csv", "w");
    if (fp == NULL) {
        perror("\n Error opening file!\n");
        return;
    }

    for(int i=0; i<2; i++) {
        b[i].vx += b[i].ax * s->dt;
        b[i].vy += b[i].ay * s->dt;
        b[i].x += b[i].vx * s->dt;
        b[i].y += b[i].vy * s->dt;
        fprintf(fp, "%s,%c,%lf,%lf,%lf,%lf,%lf,%lf\n", b[i].name, b[i].symbol, b[i].mass, b[i].x, b[i].y, b[i].vx, b[i].vy, s->dt);
    }

    fclose(fp);
}