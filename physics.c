#include <stdio.h>
#include <math.h>
#include "gravity.h"

void physics(BODY b[2], PAIR *p, SETTINGS *s, int option) {
    if (option == 0){
        for(int i=0; i<2; i++) {
            printf("\n Enter details for body %d:\n", i+1);
            printf(" Name: ");
            scanf("%19s", b[i].name);
            printf(" Symbol: ");
            scanf(" %c", &b[i].symbol);
            printf(" Mass (kg): ");
            scanf("%lf", &b[i].mass);
            printf(" Initial X position (m): ");
            scanf("%lf", &b[i].x);
            printf(" Initial Y position (m): ");
            scanf("%lf", &b[i].y);
            printf(" Initial X velocity (m/s): ");
            scanf("%lf", &b[i].vx);
            printf(" Initial Y velocity (m/s): ");
            scanf("%lf", &b[i].vy);
        }
    }
    else {
        loadData(b, option);
    }

    b[0].v = sqrt(b[0].vx * b[0].vx + b[0].vy * b[0].vy);
    b[0].ax = 0;
    b[0].ay = 0;
    b[1].v = sqrt(b[1].vx * b[1].vx + b[1].vy * b[1].vy);
    b[1].ax = 0;
    b[1].ay = 0;
    p->dx = b[1].x - b[0].x;
    p->dy = b[1].y - b[0].y;
    p->eps = sqrt(p->dx * p->dx + p->dy * p->dy) / 1000.0;
    p->r = sqrt(p->dx * p->dx + p->dy * p->dy + p->eps * p->eps);
    p->f = (G * b[0].mass * b[1].mass) / (p->r * p->r);
    p->fx = p->f * (p->dx / p->r);
    p->fy = p->f * (p->dy / p->r);
    b[0].ax += p->fx / b[0].mass;
    b[0].ay += p->fy / b[0].mass;
    b[1].ax -= p->fx / b[1].mass;
    b[1].ay -= p->fy / b[1].mass;
    p->KE = 0.5 * b[0].mass * (b[0].v * b[0].v) + 0.5 * b[1].mass * (b[1].v * b[1].v);
    p->PE = -(G * b[0].mass * b[1].mass) / p->r;
    p->TE = p->KE + p->PE;

    file(b, p, s);
}