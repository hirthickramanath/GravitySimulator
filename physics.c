#include <stdio.h>
#include <math.h>
#include "gravity.h"

/* picks dt and steps from the two bodies: one full orbit, using the vis-viva
   equation for the orbit's size and Kepler's third law for its period.
   If the bodies are escaping, runs long enough to travel 6 times the starting distance */
void chooseTimeStep(BODY *b, SETTINGS *s) {
    double dx = b[1].x - b[0].x;
    double dy = b[1].y - b[0].y;
    double dvx = b[1].vx - b[0].vx;
    double dvy = b[1].vy - b[0].vy;
    double r = sqrt(dx * dx + dy * dy);
    double v2 = dvx * dvx + dvy * dvy;
    double mu = G * (b[0].mass + b[1].mass);
    double duration;

    if (r == 0)
        return;                                   /* runSimulation reports this case */

    double inv_a = 2.0 / r - v2 / mu;
    if (inv_a > 0) {
        double a = 1.0 / inv_a;                   /* semi-major axis */
        duration = 2.0 * PI * sqrt(a * a * a / mu);
    }
    else {
        duration = 6.0 * r / sqrt(v2);
    }

    s->steps = AUTO_STEPS;
    s->dt = duration / AUTO_STEPS;
}

/* works out what kind of orbit the two bodies are on, before running anything.
   *closest gets the closest distance they will reach (periapsis), in metres.
   returns ORBIT_CLOSED, ORBIT_ESCAPE or ORBIT_COLLIDE */
int classifyOrbit(const BODY *b, double *closest) {
    double dx = b[1].x - b[0].x, dy = b[1].y - b[0].y;
    double dvx = b[1].vx - b[0].vx, dvy = b[1].vy - b[0].vy;
    double r = sqrt(dx * dx + dy * dy);
    double mu = G * (b[0].mass + b[1].mass);
    double energy = 0.5 * (dvx * dvx + dvy * dvy) - mu / r;   /* per unit mass */
    double h = dx * dvy - dy * dvx;                             /* angular momentum per unit mass */
    double e2 = 1.0 + 2.0 * energy * h * h / (mu * mu);         /* eccentricity squared */
    double e = sqrt(e2 > 0 ? e2 : 0);

    *closest = (h * h / mu) / (1.0 + e);
    if (*closest < COLLIDE_DIST)
        return ORBIT_COLLIDE;
    if (energy >= 0)
        return ORBIT_ESCAPE;
    return ORBIT_CLOSED;
}

/* runs the whole simulation and writes every step to data.csv.
   *drift gets the percentage change in total energy from the first step to the last.
   returns the number of steps written, 0 if the bodies start at the same
   position, -1 if the file can't be opened */
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

        /* kinetic energy measured from the centre of mass, so a pair moving
           together (like Earth + Moon) only counts its motion around each other */
        double cvx = (b[0].mass * b[0].vx + b[1].mass * b[1].vx) / M;
        double cvy = (b[0].mass * b[0].vy + b[1].mass * b[1].vy) / M;
        p->KE = 0.5 * b[0].mass * ((b[0].vx - cvx) * (b[0].vx - cvx) + (b[0].vy - cvy) * (b[0].vy - cvy))
              + 0.5 * b[1].mass * ((b[1].vx - cvx) * (b[1].vx - cvx) + (b[1].vy - cvy) * (b[1].vy - cvy));
        p->PE = -(G * b[0].mass * b[1].mass) / p->r;
        p->TE = p->KE + p->PE;
        if (step == 0)
            firstTE = p->TE;

        for(int i=0; i<2; i++) {
            fprintf(fp, "%d,%.0f,%s,%c,%e,%e,%e,%.4f,%.4f,%e,%.4f,%e,%e,%e,%e\n", step, s->t, b[i].name, b[i].symbol,
                    b[i].mass, b[i].x, b[i].y, b[i].vx, b[i].vy, p->r, b[i].v, p->f, p->KE, p->PE, p->TE);
        }

        for(int i=0; i<2; i++) {
            b[i].vx += b[i].ax * s->dt;
            b[i].vy += b[i].ay * s->dt;
            b[i].x += b[i].vx * s->dt;
            b[i].y += b[i].vy * s->dt;
        }
    }

    fclose(fp);
    *drift = (p->TE - firstTE) / fabs(firstTE) * 100.0;
    return s->steps + 1;
}
