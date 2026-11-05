#include <stdio.h>
#include <string.h>
#include "gravity.h"

void loadData(BODY b[2], int option) {
    switch(option) {
        case 1:
            strcpy(b[0].name, "Sun");
            b[0].symbol = 'S';
            b[0].mass = 1.989e30;
            b[0].x = 0;
            b[0].y = 0;
            b[0].vx = 0;
            b[0].vy = 0;
            strcpy(b[1].name, "Earth");
            b[1].symbol = 'E';
            b[1].mass = 5.972e24;
            b[1].x = 1.496e11;
            b[1].y = 0;
            b[1].vx = 0;
            b[1].vy = 29780;
            break;
        case 2:
            strcpy(b[0].name, "Earth");
            b[0].symbol = 'E';
            b[0].mass = 5.972e24;
            b[0].x = 0;
            b[0].y = 0;
            b[0].vx = 0;
            b[0].vy = -12.4;
            strcpy(b[1].name, "Moon");
            b[1].symbol = 'M';
            b[1].mass = 7.348e22;
            b[1].x = 3.844e8;
            b[1].y = 0;
            b[1].vx = 0;
            b[1].vy = 1012;
            break;
        case 3:
            strcpy(b[0].name, "Sun");
            b[0].symbol = 'S';
            b[0].mass = 1.989e30;
            b[0].x = 0;
            b[0].y = 0;
            b[0].vx = 0;
            b[0].vy = 0;
            strcpy(b[1].name, "Jupiter");
            b[1].symbol = 'J';
            b[1].mass = 1.898e27;
            b[1].x = 7.785e11;
            b[1].y = 0;
            b[1].vx = 0;
            b[1].vy = 13070;
            break;
        case 4:
            strcpy(b[0].name, "Sun");
            b[0].symbol = 'S';
            b[0].mass = 1.989e30;
            b[0].x = 0;
            b[0].y = 0;
            b[0].vx = 0;
            b[0].vy = 0;
            strcpy(b[1].name, "Comet");
            b[1].symbol = 'C';
            b[1].mass = 2.2e14;
            b[1].x = 1.496e11;
            b[1].y = 0;
            b[1].vx = 0;
            b[1].vy = 38460;
            break;
        case 5:
            strcpy(b[0].name, "Alpha");
            b[0].symbol = 'A';
            b[0].mass = 1.989e30;
            b[0].x = -7.48e10;
            b[0].y = 0;
            b[0].vx = 0;
            b[0].vy = -21060;
            strcpy(b[1].name, "Beta");
            b[1].symbol = 'B';
            b[1].mass = 1.989e30;
            b[1].x = 7.48e10;
            b[1].y = 0;
            b[1].vx = 0;
            b[1].vy = 21060;
            break;
        case 6:
            strcpy(b[0].name, "Sun");
            b[0].symbol = 'S';
            b[0].mass = 1.989e30;
            b[0].x = 0;
            b[0].y = 0;
            b[0].vx = 0;
            b[0].vy = 0;
            strcpy(b[1].name, "Earth");
            b[1].symbol = 'E';
            b[1].mass = 5.972e24;
            b[1].x = 1.496e11;
            b[1].y = 0;
            b[1].vx = 0;
            b[1].vy = 45000;
            break;
    }
}