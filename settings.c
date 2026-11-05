#include <stdio.h>
#include "gravity.h"

void settings(SETTINGS *s) {
    char confirm;
    int steps;
    double dt;

    printf("\n Current Simulation Settings:\n");
    printf("\n Time step (dt): %lf seconds\n", s->dt);
    printf("\n Number of time steps: %d\n", s->steps);
    
    printf("\n Enter new time step (dt) in seconds: ");
    scanf("%lf", &dt);
    printf("\n Enter new number of time steps: ");
    scanf("%d", &steps);
    
    printf("\n Confirm changes? (y/n): ");
    scanf(" %c", &confirm);

    if (confirm == 'y' || confirm == 'Y') {
        s->dt = dt;
        s->steps = steps;
        printf("\n Simulation settings updated.\n");
    }
    else {
        printf("\n Simulation settings not changed.\n");
    }
}