#ifndef GRAVITY_H
#define GRAVITY_H

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

void menu(void);

#endif