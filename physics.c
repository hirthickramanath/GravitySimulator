#include <stdio.h>
#include <math.h>
#include "gravity.h"

void physics(BODY *b, PAIR *p, SETTINGS *s, int option) {
    FILE *fp;

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
        double presetDt[6] = {3600, 600, 86400, 43200, 3600, 3600};
        int presetSteps[6] = {8766, 3934, 4332, 3800, 6200, 8766};

        loadData(b, option);
        s->dt = presetDt[option - 1];
        s->steps = presetSteps[option - 1];
    }

    p->dx = b[1].x - b[0].x;
    p->dy = b[1].y - b[0].y;
    p->eps = sqrt(p->dx * p->dx + p->dy * p->dy) / 1000.0;
    if (p->eps == 0) {
        printf("\n The two bodies cannot start at the same position.\n");
        return;
    }

    fp = fopen("data.csv", "w");
    if (fp == NULL) {
        perror("\n Error opening file!\n");
        return;
    }

    fprintf(fp, "step,t,name,symbol,mass,x,y,r,v,f,TE\n");

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
        p->KE = 0.5 * b[0].mass * (b[0].v * b[0].v) + 0.5 * b[1].mass * (b[1].v * b[1].v);
        p->PE = -(G * b[0].mass * b[1].mass) / p->r;
        p->TE = p->KE + p->PE;

        for(int i=0; i<2; i++) {
            fprintf(fp, "%d,%.0f,%s,%c,%e,%e,%e,%e,%.4f,%e,%e\n", step, s->t, b[i].name, b[i].symbol, b[i].mass, b[i].x, b[i].y, p->r, b[i].v, p->f, p->TE);
        }

        for(int i=0; i<2; i++) {
            b[i].vx += b[i].ax * s->dt;
            b[i].vy += b[i].ay * s->dt;
            b[i].x += b[i].vx * s->dt;
            b[i].y += b[i].vy * s->dt;
        }
    }

    fclose(fp);
    printf("\n Done: %d steps saved to CSV file.\n Starting the animation...\n", s->steps + 1);

    animate("data.csv");
}