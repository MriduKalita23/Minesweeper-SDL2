#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Tile structure
typedef struct {
    int is_bomb;
    int revealed;
    int flagged;
    int nearby_bombs;
} Tile;

// GameState structure
typedef struct {
    Tile grid[20][20];
    int grid_width, grid_height;
    int bomb_count;
    int tiles_revealed;
    int first_click;
    int game_over;
    int game_won;
    int flagged_bombs;
    int difficulty;
} GameState;

// Initializes the grid with bombs and calculates nearby bomb counts
void init_grid(GameState* gs) {
    // Reset all tiles
    for (int y = 0; y < gs->grid_height; y++) {
        for (int x = 0; x < gs->grid_width; x++) {
            gs->grid[y][x].is_bomb = 0;
            gs->grid[y][x].revealed = 0;
            gs->grid[y][x].flagged = 0;
            gs->grid[y][x].nearby_bombs = 0;
        }
    }

    // Place bombs randomly
    srand(time(NULL));
    int bombs_placed = 0;
    while (bombs_placed < gs->bomb_count) {
        int x = rand() % gs->grid_width;
        int y = rand() % gs->grid_height;
        if (!gs->grid[y][x].is_bomb) {
            gs->grid[y][x].is_bomb = 1;
            bombs_placed++;
        }
    }

    // Calculate nearby bombs
    for (int y = 0; y < gs->grid_height; y++) {
        for (int x = 0; x < gs->grid_width; x++) {
            if (!gs->grid[y][x].is_bomb) {
                int count = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int ny = y + dy;
                        int nx = x + dx;
                        if (nx >= 0 && nx < gs->grid_width && ny >= 0 && ny < gs->grid_height && gs->grid[ny][nx].is_bomb) {
                            count++;
                        }
                    }
                }
                gs->grid[y][x].nearby_bombs = count;
            }
        }
    }
}

// Resets the game based on difficulty
void reset_game(GameState* gs, int difficulty) {
    gs->difficulty = difficulty;
    switch (difficulty) {
        case 0: // Easy
            gs->grid_width = 9;
            gs->grid_height = 9;
            gs->bomb_count = 10;
            break;
        case 1: // Medium
            gs->grid_width = 16;
            gs->grid_height = 16;
            gs->bomb_count = 40;
            break;
        case 2: // Hard
            gs->grid_width = 20;
            gs->grid_height = 20;
            gs->bomb_count = 99;
            break;
    }
    init_grid(gs);
    gs->game_over = 0;
    gs->game_won = 0;
    gs->tiles_revealed = 0;
    gs->first_click = 1;
    gs->flagged_bombs = 0;
}

// Recursively reveals tiles using flood-fill
void flood_fill(GameState* gs, int x, int y) {
    if (x < 0 || x >= gs->grid_width || y < 0 || y >= gs->grid_height ||
        gs->grid[y][x].revealed || gs->grid[y][x].flagged) return;

    gs->grid[y][x].revealed = 1;
    gs->tiles_revealed++;

    if (gs->grid[y][x].nearby_bombs == 0) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx != 0 || dy != 0) {
                    flood_fill(gs, x + dx, y + dy);
                }
            }
        }
    }
}

// Reveals a tile
void reveal_tile(GameState* gs, int x, int y) {
    if (x < 0 || x >= gs->grid_width || y < 0 || y >= gs->grid_height ||
        gs->grid[y][x].revealed || gs->grid[y][x].flagged) return;

    if (gs->first_click) {
        gs->first_click = 0;
        // Ensure first click is not a bomb
        if (gs->grid[y][x].is_bomb) {
            gs->grid[y][x].is_bomb = 0;
            int placed = 0;
            while (!placed) {
                int new_x = rand() % gs->grid_width;
                int new_y = rand() % gs->grid_height;
                if (!gs->grid[new_y][new_x].is_bomb && !(new_x == x && new_y == y)) {
                    gs->grid[new_y][new_x].is_bomb = 1;
                    placed = 1;
                }
            }
            // Recalculate nearby bombs
            for (int gy = 0; gy < gs->grid_height; gy++) {
                for (int gx = 0; gx < gs->grid_width; gx++) {
                    if (!gs->grid[gy][gx].is_bomb) {
                        int count = 0;
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                int ny = gy + dy;
                                int nx = gx + dx;
                                if (nx >= 0 && nx < gs->grid_width && ny >= 0 && ny < gs->grid_height && gs->grid[ny][nx].is_bomb) {
                                    count++;
                                }
                            }
                        }
                        gs->grid[gy][gx].nearby_bombs = count;
                    }
                }
            }
        }
        flood_fill(gs, x, y);
    } else {
        gs->grid[y][x].revealed = 1;
        gs->tiles_revealed++;
        if (gs->grid[y][x].is_bomb) {
            gs->game_over = 1;
            gs->game_won = 0;
        } else if (gs->grid[y][x].nearby_bombs == 0) {
            flood_fill(gs, x, y);
        }
    }
}

// Toggles a flag
void toggle_flag(GameState* gs, int x, int y) {
    if (x < 0 || x >= gs->grid_width || y < 0 || y >= gs->grid_height || gs->grid[y][x].revealed) return;
    gs->grid[y][x].flagged = !gs->grid[y][x].flagged;
    gs->flagged_bombs += gs->grid[y][x].flagged ? 1 : -1;
}

// Displays the game grid
// Displays the game grid
void display_grid(GameState* gs) {
    printf("\n   ");
    for (int x = 0; x < gs->grid_width; x++) printf("%2d", x);
    printf("\n");
    for (int y = 0; y < gs->grid_height; y++) {
        printf("%2d ", y);
        for (int x = 0; x < gs->grid_width; x++) {
            if (gs->grid[y][x].flagged) printf("F ");
            else if (!gs->grid[y][x].revealed) printf(". ");
            else if (gs->grid[y][x].is_bomb) printf("* ");
            else if (gs->grid[y][x].nearby_bombs > 0) printf("%d ", gs->grid[y][x].nearby_bombs);
            else printf("0 ");
        }
        printf("\n");
    }
    printf("----------------\n");
    printf("Bombs: %d/%d\n", gs->flagged_bombs, gs->bomb_count);
}

// Main function
int main() {
    GameState gs = {0};
    int running = 1;
    int in_menu = 1;
    int difficulty = 0;

    while (running) {
        if (in_menu) {
            printf("Select difficulty (0: Easy, 1: Medium, 2: Hard): ");
            scanf("%d", &difficulty);
            if (difficulty >= 0 && difficulty <= 2) {
                in_menu = 0;
                reset_game(&gs, difficulty);
            } else {
                printf("Invalid difficulty. Try again.\n");
            }
        } else {
            display_grid(&gs);
            if (gs.game_over) {
                if (gs.game_won) {
                    printf("Victory! You cleared the board!\n");
                } else {
                    printf("Game Over! You hit a bomb.\n");
                }
                printf("Play again? (1: Yes, 0: No): ");
                int play_again;
                scanf("%d", &play_again);
                if (play_again) {
                    in_menu = 1;
                } else {
                    running = 0;
                }
                continue;
            }

            // Check win condition
            if (gs.tiles_revealed == (gs.grid_width * gs.grid_height - gs.bomb_count)) {
                gs.game_over = 1;
                gs.game_won = 1;
                continue;
            }

            printf("Enter action (r: reveal, f: flag), x, y: ");
            char action;
            int x, y;
            scanf(" %c %d %d", &action, &x, &y);

            if (x >= 0 && x < gs.grid_width && y >= 0 && y < gs.grid_height) {
                if (action == 'r') {
                    reveal_tile(&gs, x, y);
                } else if (action == 'f') {
                    toggle_flag(&gs, x, y);
                } else {
                    printf("Invalid action. Use 'r' or 'f'.\n");
                }
            } else {
                printf("Invalid coordinates.\n");
            }
        }
    }

    return 0;
}
