/* physics.c - the orbit check, the time step, and the simulation itself
   Gravity Simulator - P L Hirthick Ramanath (26PT20), Svithin S (26PT35) */

#include <stdio.h>
#include <math.h>
#include "gravity.h"

/* looks at the starting state and says what kind of orbit it is. *closest gets the
   closest the bodies will ever get. If s isn't NULL it also picks dt and steps:
   one full orbit (Kepler's third law), or 6 times the starting distance if they escape */
int checkOrbit(const BODY *b, SETTINGS *s, double *closest) {
    double dx = b[1].x - b[0].x;
    double dy = b[1].y - b[0].y;
    double dvx = b[1].vx - b[0].vx;
    double dvy = b[1].vy - b[0].vy;
    double r = sqrt(dx * dx + dy * dy);
    double v2 = dvx * dvx + dvy * dvy;
    double mu = G * (b[0].mass + b[1].mass);

    if (r == 0) {
        *closest = 0;
        return ORBIT_COLLIDE;
    }

    double energy = v2 / 2 - mu / r;                      /* per kg */
    double h = dx * dvy - dy * dvx;                       /* angular momentum per kg */
    double e2 = 1 + 2 * energy * h * h / (mu * mu);       /* eccentricity squared */
    double e = sqrt(e2 > 0 ? e2 : 0);

    if (s != NULL) {
        double duration;
        if (energy < 0) {
            double a = -mu / (2 * energy);                /* size of the orbit, from vis-viva */
            duration = 2 * PI * sqrt(a * a * a / mu);
        }
        else {
            duration = 6 * r / sqrt(v2);
        }
        s->steps = AUTO_STEPS;
        s->dt = duration / AUTO_STEPS;
    }

    *closest = (h * h / mu) / (1 + e);
    if (energy >= 0 && dx * dvx + dy * dvy >= 0)
        *closest = r;                                     /* moving apart and never coming back: this is as close as they get */
    if (*closest < COLLIDE_DIST)
        return ORBIT_COLLIDE;
    if (energy >= 0)
        return ORBIT_ESCAPE;
    return ORBIT_CLOSED;
}

/* runs the simulation and writes every step to data.csv; *drift gets the % change in energy.
   returns the number of steps written, 0 if both start at the same place, -1 if the file won't open */
int runSimulation(BODY *b, PAIR *p, SETTINGS *s, double *drift) {
    FILE *fp;
    double firstTE = 0;
    double M = b[0].mass + b[1].mass;

    p->dx = b[1].x - b[0].x;
    p->dy = b[1].y - b[0].y;
    p->eps = sqrt(p->dx * p->dx + p->dy * p->dy) / 1000.0;
    if (p->eps == 0)
        return 0;

    fp = fopen(DATA_FILE, "w");
    if (fp == NULL)
        return -1;

    fprintf(fp, "step,t,name,symbol,mass,x,y,vx,vy,r,v,f,KE,PE,TE\n");

    for(int step=0; step<=s->steps; step++) {
        s->t = step * s->dt;

        b[0].v = sqrt(b[0].vx * b[0].vx + b[0].vy * b[0].vy);
        b[0].ax = 0;
        b[0].ay = 0;
        b[1].v = sqrt(b[1].vx * b[1].vx + b[1].vy * b[1].vy);
        b[1].ax = 0;
        b[1].ay = 0;
        p->dx = b[1].x - b[0].x;
        p->dy = b[1].y - b[0].y;
        p->r = sqrt(p->dx * p->dx + p->dy * p->dy + p->eps * p->eps);
        p->f = (G * b[0].mass * b[1].mass) / (p->r * p->r);
        p->fx = p->f * (p->dx / p->r);
        p->fy = p->f * (p->dy / p->r);
        b[0].ax += p->fx / b[0].mass;
        b[0].ay += p->fy / b[0].mass;
        b[1].ax -= p->fx / b[1].mass;
        b[1].ay -= p->fy / b[1].mass;

        /* kinetic energy around the centre of mass, so Earth + Moon only counts their motion around each other */
        double cvx = (b[0].mass * b[0].vx + b[1].mass * b[1].vx) / M;
        double cvy = (b[0].mass * b[0].vy + b[1].mass * b[1].vy) / M;
        double ux0 = b[0].vx - cvx;
        double uy0 = b[0].vy - cvy;
        double ux1 = b[1].vx - cvx;
        double uy1 = b[1].vy - cvy;
        p->KE = 0.5 * b[0].mass * (ux0 * ux0 + uy0 * uy0) + 0.5 * b[1].mass * (ux1 * ux1 + uy1 * uy1);
        p->PE = -(G * b[0].mass * b[1].mass) / p->r;
        p->TE = p->KE + p->PE;
        if (step == 0)
            firstTE = p->TE;

        for(int i=0; i<2; i++) {
            fprintf(fp, "%d,%.3f,%s,%c,%e,%e,%e,%.4f,%.4f,%e,%.4f,%e,%e,%e,%e\n", step, s->t, b[i].name, b[i].symbol, b[i].mass, b[i].x, b[i].y, b[i].vx, b[i].vy, p->r, b[i].v, p->f, p->KE, p->PE, p->TE);
        }

        for(int i=0; i<2; i++) {
            b[i].vx += b[i].ax * s->dt;
            b[i].vy += b[i].ay * s->dt;
            b[i].x += b[i].vx * s->dt;
            b[i].y += b[i].vy * s->dt;
        }
    }

    fclose(fp);
    *drift = 0;
    if (firstTE != 0)
        *drift = (p->TE - firstTE) / fabs(firstTE) * 100.0;
    return s->steps + 1;
}
