#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum {
    STATE_MAIN,
    STATE_GAME
} GameState;

typedef struct {
    GameState state;
    bool running;
    int selectedMode;      // 0 for 2D, 1 for 3D
    int selectedDifficulty; // 0 for Easy, 1 for Medium, 2 for Hard
} App;

void launchGame(int mode) {
    const char* modes[] = {"2d"};
    char command[256];

    snprintf(command, sizeof(command), "./%s &", modes[mode]);
    system(command);
}

void clearScreen() {
    // Clear terminal screen (works on Unix-like systems)
    printf("\033[H\033[J");
}

void displayMenu() {
    clearScreen();
    printf("=== MINESWEEPER ===\n");
    printf("1. Play 2D Game\n");
    printf("2. Play 3D Game (Not Implemented)\n");
    printf("3. Exit\n");
    printf("Enter your choice (1-3): ");
}

void runGame(App* app) {
    if (app->selectedMode == 0) {
        launchGame(app->selectedMode);
        app->running = false; // Exit after launching game
    } else {
        printf("Selected mode is not implemented.\n");
        printf("Press Enter to return to menu...");
        getchar(); // Clear input buffer
        getchar(); // Wait for Enter
        app->state = STATE_MAIN;
    }
}

int main() {
    App app = {STATE_MAIN, true, -1, -1};
    char input[10];

    while (app.running) {
        if (app.state == STATE_MAIN) {
            displayMenu();
            
            // Get user input
            if (fgets(input, sizeof(input), stdin) != NULL) {
                // Remove trailing newline
                input[strcspn(input, "\n")] = '\0';             //strcspn is search 1st character of 2nd string in 1st string
                
                // Process input
                if (strcmp(input, "1") == 0) {
                    app.state = STATE_GAME;
                    app.selectedMode = 0; // 2D mode
                } else if (strcmp(input, "2") == 0) {
                    app.state = STATE_GAME;
                    app.selectedMode = 1; // 3D mode (not implemented)
                } else if (strcmp(input, "3") == 0) {
                    app.running = false;
                } else {
                    printf("Invalid choice. Press Enter to try again...");
                    getchar(); // Clear input buffer
                    getchar(); // Wait for Enter
                }
            }
        } else if (app.state == STATE_GAME) {
            runGame(&app);
        }
    }

    clearScreen();
    printf("Thank you for playing!\n");
    return 0;
}
