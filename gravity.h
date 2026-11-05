#ifndef GRAVITY_H
#define GRAVITY_H

#define G 6.67430e-11

struct body {
    char name[20];
    char symbol;
    double mass;
    double x;
    double y;
    double vx;
    double vy;
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
    double eps;
};

typedef struct pair PAIR;

struct simulationSettings {
    double dt;
    int steps;
};

typedef struct simulationSettings SETTINGS;

void loadData(BODY *, int);

void file(void);

void physics(BODY *, PAIR *, SETTINGS *, int);

void settings(SETTINGS *);

#endif