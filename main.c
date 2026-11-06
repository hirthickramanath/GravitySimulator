#include <stdio.h>
#include "gravity.h"

int main() {
    BODY b[2];
    PAIR p;
    SETTINGS s = {3600, 8766, 0};
    int choice = 0;
    int option = 0;

    printf("\n Welcome to the Gravity Simulation!\n");
    do {
        printf("\n Choose an option:\n 1. Load and Run Simulation\n 2. New Simulation\n 3. Simulation Settings\n 4. Exit\n Your choice: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = 0;
        }

        switch(choice) {
            case 1:
                printf("\n Available Predefined Simulations:\n 1. Sun-Earth System\n 2. Earth-Moon System\n 3. Sun-Jupiter System\n 4. Sun-Comet System\n 5. Binary Star System\n 6. Escape Velocity (Sun-Earth)\n 7. Back to Main Menu\n Your choice: ");
                if (scanf("%d", &option) != 1) {
                    while (getchar() != '\n');
                    option = 0;
                }
                if (option >=1 && option <= 6) {
                    physics(b, &p, &s, option);
                }
                else if (option == 7) {
                    break;
                }
                else {
                    printf("\n Invalid option. Please choose 1-7.\n");
                }
                break;
            case 2:
                option = 0;
                physics(b, &p, &s, option);
                break;
            case 3:
                settings(&s);
                break;
            case 4:
                printf("\n Exiting...\n");
                break;
            default:
                printf("\n Invalid option. Please try again.\n");
                break;
        }
    } while (choice != 4);

    return 0;
}