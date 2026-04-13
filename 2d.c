#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_SCORES 5
#define FRAGMENT_COUNT 8
#define FLASH_INTERVAL 500
#define ANIMATION_DURATION 500

// Tile structure representing each cell in the Minesweeper grid
typedef struct {
    int is_bomb;        
    int revealed;       
    int flagged;        
    int nearby_bombs;   
} Tile;

// Fragment structure for explosion animation particles
typedef struct {
    float x, y;         
    float vx, vy;       
    int active;         
} Fragment;

// GameState structure to hold the game's state
typedef struct {
    Tile grid[20][20];  
    int grid_width, grid_height; 
    int bomb_count;  
    int tiles_revealed; 
    int first_click;    
    int game_over;      
    int game_won;       
    int animating;      
    int anim_x, anim_y; 
    Uint32 anim_start_time; 
    Fragment fragments[FRAGMENT_COUNT]; 
    Uint32 start_time; 
    int flagged_bombs;  
    int difficulty;     
} GameState;

// HighScore structure for storing high scores
typedef struct {
    int time;          
    char difficulty[10]; 
} HighScore;

// RenderState structure for rendering-related data
typedef struct {
    SDL_Window* window; 
    SDL_Renderer* renderer; 
    TTF_Font* font;     
    SDL_Texture* flag_texture; 
    SDL_Texture* bomb_texture; 
    SDL_Texture* game_over_texture; 
    SDL_Texture* victory_texture; 
    SDL_Texture* restart_texture; 
    SDL_Texture* number_textures[9];
    Mix_Chunk* click_sound; 
    Mix_Chunk* explosion_sound;
    Mix_Chunk* game_over_sound;
    Mix_Chunk* restart_sound; 
    Mix_Chunk* flag_sound; 
    Mix_Chunk* win_sound; 
    int window_width, window_height; 
    int tile_size;      
    int offset_x, offset_y; 
    int highlighted_x, highlighted_y; 
    Uint32 last_flash_time; 
    int show_game_over; 
} RenderState;

// Frees all SDL resources and quits SDL subsystems
void cleanup(RenderState* rs) {
    if (rs->click_sound) Mix_FreeChunk(rs->click_sound);
    if (rs->explosion_sound) Mix_FreeChunk(rs->explosion_sound);
    if (rs->game_over_sound) Mix_FreeChunk(rs->game_over_sound);
    if (rs->restart_sound) Mix_FreeChunk(rs->restart_sound);
    if (rs->flag_sound) Mix_FreeChunk(rs->flag_sound);
    if (rs->win_sound) Mix_FreeChunk(rs->win_sound);
    for (int i = 1; i <= 8; i++) {
        if (rs->number_textures[i]) SDL_DestroyTexture(rs->number_textures[i]);
    }
    if (rs->flag_texture) SDL_DestroyTexture(rs->flag_texture);
    if (rs->bomb_texture) SDL_DestroyTexture(rs->bomb_texture);
    if (rs->game_over_texture) SDL_DestroyTexture(rs->game_over_texture);
    if (rs->victory_texture) SDL_DestroyTexture(rs->victory_texture);
    if (rs->restart_texture) SDL_DestroyTexture(rs->restart_texture);
    if (rs->font) TTF_CloseFont(rs->font);
    if (rs->renderer) SDL_DestroyRenderer(rs->renderer);
    if (rs->window) SDL_DestroyWindow(rs->window);
    Mix_Quit();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

// Initializes textures for numbers 1-8 with different colors
void init_number_textures(TTF_Font* font, SDL_Renderer* renderer, SDL_Texture* number_textures[]) {
    for (int i = 1; i <= 8; i++) {
        char num_str[2] = {i + '0', '\0'};
        SDL_Color color;
        switch (i) {
            case 1: color = (SDL_Color){0, 0, 255, 255}; break; // Blue
            case 2: color = (SDL_Color){0, 128, 0, 255}; break; // Green
            case 3: color = (SDL_Color){255, 0, 0, 255}; break; // Red
            case 4: color = (SDL_Color){0, 0, 128, 255}; break; // Dark Blue
            case 5: color = (SDL_Color){128, 0, 0, 255}; break; // Dark Red
            case 6: color = (SDL_Color){0, 128, 128, 255}; break; // Teal
            case 7: color = (SDL_Color){0, 0, 0, 255}; break; // Black
            case 8: color = (SDL_Color){128, 128, 128, 255}; break; // Gray
            default: color = (SDL_Color){255, 255, 255, 255}; break; // White
        }
        SDL_Surface* surface = TTF_RenderText_Solid(font, num_str, color);
        number_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
}

// Initializes the game grid with bombs and calculates nearby bomb counts
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

    // Seed random number generator
    srand(time(NULL));
    int bombs_placed = 0;
    int min_distance = gs->grid_width / 4; // Minimum distance between bombs
    int attempts = 0;
    int max_attempts = gs->bomb_count * 10;

    // Place bombs randomly, ensuring minimum distance
    while (bombs_placed < gs->bomb_count && attempts < max_attempts) {
        int x = rand() % gs->grid_width;
        int y = rand() % gs->grid_height;
        int too_close = 0;

        // Check distance to existing bombs
        for (int by = 0; by < gs->grid_height; by++) {
            for (int bx = 0; bx < gs->grid_width; bx++) {
                if (gs->grid[by][bx].is_bomb) {
                    int dx = bx - x;
                    int dy = by - y;
                    if (dx * dx + dy * dy < min_distance * min_distance) {
                        too_close = 1;
                        break;
                    }
                }
            }
        }

        if (!too_close && !gs->grid[y][x].is_bomb) {
            gs->grid[y][x].is_bomb = 1;
            bombs_placed++;
        }
        attempts++;
    }

    // Fallback: Place remaining bombs randomly if needed
    while (bombs_placed < gs->bomb_count) {
        int x = rand() % gs->grid_width;
        int y = rand() % gs->grid_height;
        if (!gs->grid[y][x].is_bomb) {
            gs->grid[y][x].is_bomb = 1;
            bombs_placed++;
        }
    }

    // Calculate number of nearby bombs for each non-bomb tile
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

// Resets the game to a new state based on difficulty
void reset_game(GameState* gs, RenderState* rs, int difficulty) {
    gs->difficulty = difficulty;
    // Set grid size and bomb count based on difficulty
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
    gs->animating = 0;
    gs->anim_x = 0;
    gs->anim_y = 0;
    gs->anim_start_time = 0;
    gs->start_time = SDL_GetTicks();
    gs->flagged_bombs = 0;
    // Deactivate all fragments
    for (int i = 0; i < FRAGMENT_COUNT; i++) {
        gs->fragments[i].active = 0;
    }
    rs->last_flash_time = 0;
    rs->show_game_over = 0;
    // Calculate tile size and grid offsets for centering
    rs->tile_size = (rs->window_width < rs->window_height ? rs->window_width : rs->window_height) / gs->grid_width;
    rs->offset_x = (rs->window_width - (gs->grid_width * rs->tile_size)) / 2;
    rs->offset_y = (rs->window_height - (gs->grid_height * rs->tile_size)) / 2;
    Mix_PlayChannel(-1, rs->restart_sound, 0);
}

// Initializes fragments for explosion animation
void init_fragments(GameState* gs, RenderState* rs, int tile_x, int tile_y) {
    float center_x = rs->offset_x + tile_x * rs->tile_size + rs->tile_size / 2.0f;
    float center_y = rs->offset_y + tile_y * rs->tile_size + rs->tile_size / 2.0f;
    // Create fragments with random directions and speeds
    for (int i = 0; i < FRAGMENT_COUNT; i++) {
        gs->fragments[i].x = center_x;
        gs->fragments[i].y = center_y;
        float angle = (float)(rand() % 360) / 180.0f;
        float speed = 2.0f + (rand() % 100) / 20.0f;
        gs->fragments[i].vx = cos(angle) * speed;
        gs->fragments[i].vy = sin(angle) * speed;
        gs->fragments[i].active = 1;
    }
}

// Updates fragment positions for explosion animation
void update_fragments(GameState* gs, RenderState* rs) {
    for (int i = 0; i < FRAGMENT_COUNT; i++) {
        if (gs->fragments[i].active) {
            gs->fragments[i].x += gs->fragments[i].vx;
            gs->fragments[i].y += gs->fragments[i].vy;
            gs->fragments[i].vy += 0.1f; // Apply gravity
            // Deactivate fragments that move off-screen
            if (gs->fragments[i].x < 0 || gs->fragments[i].x > rs->window_width ||
                gs->fragments[i].y < 0 || gs->fragments[i].y > rs->window_height) {
                gs->fragments[i].active = 0;
            }
        }
    }
}

// Reveals all bombs when the game ends
void reveal_all_bombs(GameState* gs) {
    for (int y = 0; y < gs->grid_height; y++) {
        for (int x = 0; x < gs->grid_width; x++) {
            if (gs->grid[y][x].is_bomb) {
                gs->grid[y][x].revealed = 1;
            }
        }
    }
}

// Recursively reveals tiles using flood-fill for empty areas
void flood_fill(GameState* gs, RenderState* rs, int x, int y) {
    if (x < 0 || x >= gs->grid_width || y < 0 || y >= gs->grid_height ||
        gs->grid[y][x].revealed || gs->grid[y][x].flagged) return;

    gs->grid[y][x].revealed = 1;
    gs->tiles_revealed++;
    Mix_PlayChannel(-1, rs->click_sound, 0);

    // Continue flood-fill for adjacent tiles if no nearby bombs
    if (gs->grid[y][x].nearby_bombs == 0) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx != 0 || dy != 0) {
                    flood_fill(gs, rs, x + dx, y + dy);
                }
            }
        }
    }
}

// Reveals a tile and handles game logic (first click, bombs, flood-fill)
void reveal_tile(GameState* gs, RenderState* rs, int x, int y) {
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
            // Recalculate nearby bomb counts
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
        flood_fill(gs, rs, x, y);
    } else {
        gs->grid[y][x].revealed = 1;
        gs->tiles_revealed++;
        if (gs->grid[y][x].is_bomb) {
            // Trigger explosion animation and game over
            gs->animating = 1;
            gs->anim_x = x;
            gs->anim_y = y;
            gs->anim_start_time = SDL_GetTicks();
            init_fragments(gs, rs, x, y);
            gs->game_over = 1;
            gs->game_won = 0;
            Mix_PlayChannel(-1, rs->explosion_sound, 0);
        } else {
            Mix_PlayChannel(-1, rs->click_sound, 0);
            if (gs->grid[y][x].nearby_bombs == 0) {
                flood_fill(gs, rs, x, y);
            }
        }
    }
}

// Toggles a flag on a tile
void toggle_flag(GameState* gs, RenderState* rs, int x, int y) {
    if (x < 0 || x >= gs->grid_width || y < 0 || y >= gs->grid_height || gs->grid[y][x].revealed) return;
    gs->grid[y][x].flagged = !gs->grid[y][x].flagged;
    gs->flagged_bombs += gs->grid[y][x].flagged ? 1 : -1;
    Mix_PlayChannel(-1, rs->flag_sound, 0);
}

// Saves the player's score if it qualifies for the high score list
void save_high_score(GameState* gs) {
    HighScore scores[MAX_SCORES];
    FILE* file = fopen("highscores.dat", "rb");
    int count = 0;
    if (file) {
        count = fread(scores, sizeof(HighScore), MAX_SCORES, file);
        fclose(file);
    }

    int time = (SDL_GetTicks() - gs->start_time) / 1000;
    // Add score if there's space or if it's better than the worst score
    if (count < MAX_SCORES || time < scores[count-1].time) {
        scores[count].time = time;
        strcpy(scores[count].difficulty, gs->difficulty == 0 ? "Easy" : gs->difficulty == 1 ? "Medium" : "Hard");
        if (count < MAX_SCORES) count++;

        // Sort scores in ascending order
        for (int i = 0; i < count - 1; i++) {
            for (int j = 0; j < count - i - 1; j++) {
                if (scores[j].time > scores[j + 1].time) { 
                    HighScore temp = scores[j];
                    scores[j] = scores[j + 1];
                    scores[j + 1] = temp;
                }
            }
        }

        file = fopen("highscores.dat", "wb");
        if (file) {
            fwrite(scores, sizeof(HighScore), count, file);
            fclose(file);
        }
    }
}

// Renders the game grid, HUD, and animations
void render_game(GameState* gs, RenderState* rs) {
    SDL_SetRenderDrawColor(rs->renderer, 50, 50, 50, 255);
    SDL_RenderClear(rs->renderer);

    // Render each tile
    for (int y = 0; y < gs->grid_height; y++) {
        for (int x = 0; x < gs->grid_width; x++) {
            SDL_Rect tile_rect = {rs->offset_x + x * rs->tile_size, rs->offset_y + y * rs->tile_size, rs->tile_size - 1, rs->tile_size - 1};
            int is_highlighted = (x == rs->highlighted_x && y == rs->highlighted_y && !gs->grid[y][x].revealed && !gs->game_over);

            // Set tile color based on state
            if (!gs->grid[y][x].revealed) {
                SDL_SetRenderDrawColor(rs->renderer, is_highlighted ? 180 : 150, 150, 150, 255);
            } else if (gs->grid[y][x].is_bomb) {
                if (gs->animating && x == gs->anim_x && y == gs->anim_y && SDL_GetTicks() - gs->anim_start_time < ANIMATION_DURATION) {
                    continue; // Skip rendering during animation
                }
                SDL_SetRenderDrawColor(rs->renderer, 200, 200, 200, 255);
            } else {
                SDL_SetRenderDrawColor(rs->renderer, 200, 200, 200, 255);
            }
            SDL_RenderFillRect(rs->renderer, &tile_rect);

            // Render flag if present
            if (gs->grid[y][x].flagged && !gs->grid[y][x].revealed) {
                SDL_Rect flag_rect = {rs->offset_x + x * rs->tile_size, rs->offset_y + y * rs->tile_size, rs->tile_size, rs->tile_size};
                SDL_RenderCopy(rs->renderer, rs->flag_texture, NULL, &flag_rect);
            }

            // Mark incorrect flags when game is over
            if (gs->game_over && !gs->animating && gs->grid[y][x].flagged && !gs->grid[y][x].is_bomb) {
                SDL_SetRenderDrawColor(rs->renderer, 255, 0, 0, 255);
                SDL_RenderDrawLine(rs->renderer,
                    rs->offset_x + x * rs->tile_size + rs->tile_size / 4, rs->offset_y + y * rs->tile_size + rs->tile_size / 4,
                    rs->offset_x + x * rs->tile_size + 3 * rs->tile_size / 4, rs->offset_y + y * rs->tile_size + 3 * rs->tile_size / 4);
                SDL_RenderDrawLine(rs->renderer,
                    rs->offset_x + x * rs->tile_size + 3 * rs->tile_size / 4, rs->offset_y + y * rs->tile_size + rs->tile_size / 4,
                    rs->offset_x + x * rs->tile_size + rs->tile_size / 4, rs->offset_y + y * rs->tile_size + 3 * rs->tile_size / 4);
            }

            // Render bomb or number for revealed tiles
            if (gs->grid[y][x].revealed) {
                if (gs->grid[y][x].is_bomb) {
                    SDL_Rect bomb_rect = {rs->offset_x + x * rs->tile_size, rs->offset_y + y * rs->tile_size, rs->tile_size, rs->tile_size};
                    SDL_RenderCopy(rs->renderer, rs->bomb_texture, NULL, &bomb_rect);
                } else if (gs->grid[y][x].nearby_bombs > 0) {
                    SDL_Rect num_rect = {rs->offset_x + x * rs->tile_size + rs->tile_size/4, rs->offset_y + y * rs->tile_size + rs->tile_size/4, rs->tile_size/2, rs->tile_size/2};
                    SDL_RenderCopy(rs->renderer, rs->number_textures[gs->grid[y][x].nearby_bombs], NULL, &num_rect);
                }
            }
        }
    }

    // Draw grid lines
    SDL_SetRenderDrawColor(rs->renderer, 100, 100, 100, 255);
    for (int i = 0; i <= gs->grid_width; i++) {
        SDL_RenderDrawLine(rs->renderer, rs->offset_x + i * rs->tile_size, rs->offset_y,
                           rs->offset_x + i * rs->tile_size, rs->offset_y + gs->grid_height * rs->tile_size);
    }
    for (int i = 0; i <= gs->grid_height; i++) {
        SDL_RenderDrawLine(rs->renderer, rs->offset_x, rs->offset_y + i * rs->tile_size,
                           rs->offset_x + gs->grid_width * rs->tile_size, rs->offset_y + i * rs->tile_size);
    }

    // Render explosion fragments
    if (gs->animating) {
        Uint32 elapsed = SDL_GetTicks() - gs->anim_start_time;
        if (elapsed < ANIMATION_DURATION) {
            update_fragments(gs, rs);
            SDL_SetRenderDrawColor(rs->renderer, 255, 0, 0, 255);
            for (int i = 0; i < FRAGMENT_COUNT; i++) {
                if (gs->fragments[i].active) {
                    SDL_Rect frag_rect = {(int)gs->fragments[i].x, (int)gs->fragments[i].y, rs->tile_size / 4, rs->tile_size / 4};
                    SDL_RenderFillRect(rs->renderer, &frag_rect);
                }
            }
        } else {
            gs->animating = 0;
            reveal_all_bombs(gs);
            Mix_PlayChannel(-1, rs->game_over_sound, 0);
        }
    }

    // Render HUD (bombs flagged and time)
    char hud_text[100];
    int time = (SDL_GetTicks() - gs->start_time) / 1000;
    snprintf(hud_text, sizeof(hud_text), "Bombs: %d/%d  Time: %d", gs->flagged_bombs, gs->bomb_count, time);
    SDL_Surface* hud_surface = TTF_RenderText_Solid(rs->font, hud_text, (SDL_Color){255, 255, 255, 255});
    SDL_Texture* hud_texture = SDL_CreateTextureFromSurface(rs->renderer, hud_surface);
    SDL_Rect hud_rect = {rs->offset_x, rs->offset_y - rs->tile_size, hud_surface->w, hud_surface->h};
    SDL_RenderCopy(rs->renderer, hud_texture, NULL, &hud_rect);
    SDL_FreeSurface(hud_surface);
    SDL_DestroyTexture(hud_texture);

    // Render restart button
    SDL_Rect restart_rect = {rs->offset_x + (gs->grid_width * rs->tile_size) - rs->tile_size, rs->offset_y - rs->tile_size, rs->tile_size, rs->tile_size};
    SDL_RenderCopy(rs->renderer, rs->restart_texture, NULL, &restart_rect);

    // Render game over or victory message with flashing effect
    if (gs->game_over && !gs->animating) {
        Uint32 current_time = SDL_GetTicks();
        if (current_time - rs->last_flash_time >= FLASH_INTERVAL) {
            rs->show_game_over = !rs->show_game_over;
            rs->last_flash_time = current_time;
        }
        if (rs->show_game_over) {
            int text_w, text_h;
            SDL_Texture* text_texture = gs->game_won ? rs->victory_texture : rs->game_over_texture;
            SDL_QueryTexture(text_texture, NULL, NULL, &text_w, &text_h);
            SDL_Rect text_rect = {
                (rs->window_width - text_w) / 2,
                (rs->window_height - text_h) / 2,
                text_w,
                text_h
            };
            SDL_RenderCopy(rs->renderer, text_texture, NULL, &text_rect);
        }
    }

    SDL_RenderPresent(rs->renderer);
}

// Renders the difficulty selection menu
void render_menu(RenderState* rs, int selected) {
    SDL_SetRenderDrawColor(rs->renderer, 50, 50, 50, 255);
    SDL_RenderClear(rs->renderer);

    const char* options[] = {"Easy", "Medium", "Hard"};
    // Render each difficulty option highlighting the selected one
    for (int i = 0; i < 3; i++) {
        SDL_Color color = (i == selected) ? (SDL_Color){255, 255, 0, 255} : (SDL_Color){255, 255, 255, 255};
        SDL_Surface* surface = TTF_RenderText_Solid(rs->font, options[i], color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(rs->renderer, surface);
        SDL_Rect rect = {rs->window_width / 2 - surface->w / 2, rs->window_height / 2 - surface->h + i * surface->h * 2, surface->w, surface->h};
        SDL_RenderCopy(rs->renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    SDL_RenderPresent(rs->renderer);
}

// Main function: initializes SDL, handles game loop, and cleans up
int main() {
    // Initialize SDL subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! Mix_Error: %s\n", Mix_GetError());
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    RenderState rs = {0};
    // Get display resolution
    SDL_DisplayMode display_mode;
    if (SDL_GetCurrentDisplayMode(0, &display_mode) < 0) {
        printf("Could not get display mode! SDL_Error: %s\n", SDL_GetError());
        cleanup(&rs);
        return 1;
    }
    rs.window_width = display_mode.w;
    rs.window_height = display_mode.h;

    // Create window
    rs.window = SDL_CreateWindow("2D Minesweeper",
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 rs.window_width, rs.window_height,
                                 SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!rs.window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        cleanup(&rs);
        return 1;
    }

    // Create renderer
    rs.renderer = SDL_CreateRenderer(rs.window, -1, SDL_RENDERER_ACCELERATED);
    if (!rs.renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        cleanup(&rs);
        return 1;
    }

    // Load font
    rs.font = TTF_OpenFont("Minecraft.ttf", 24);
    if (!rs.font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        cleanup(&rs);
        return 1;
    }

    // Load larger font for game over/victory messages
    TTF_Font* large_font = TTF_OpenFont("Minecraft.ttf", 250);
    if (!large_font) {
        printf("Failed to load large font! TTF_Error: %s\n", TTF_GetError());
        cleanup(&rs);
        return 1;
    }

    // Create game over texture
    SDL_Surface* game_over_surface = TTF_RenderText_Solid(large_font, "Game Over", (SDL_Color){0, 0, 255, 255});
    rs.game_over_texture = SDL_CreateTextureFromSurface(rs.renderer, game_over_surface);
    SDL_FreeSurface(game_over_surface);
    if (!rs.game_over_texture) {
        printf("Failed to create game over texture! SDL_Error: %s\n", SDL_GetError());
        cleanup(&rs);
        return 1;
    }

    // Create victory texture
    SDL_Surface* victory_surface = TTF_RenderText_Solid(large_font, "Victory", (SDL_Color){0, 255, 0, 255});
    rs.victory_texture = SDL_CreateTextureFromSurface(rs.renderer, victory_surface);
    SDL_FreeSurface(victory_surface);
    if (!rs.victory_texture) {
        printf("Failed to create victory texture! SDL_Error: %s\n", SDL_GetError());
        cleanup(&rs);
        return 1;
    }

    TTF_CloseFont(large_font);

    // Load textures
    SDL_Surface* flag_surface = IMG_Load("flag.png");
    rs.flag_texture = SDL_CreateTextureFromSurface(rs.renderer, flag_surface);
    SDL_FreeSurface(flag_surface);
    if (!rs.flag_texture) {
        printf("Failed to create flag texture! SDL_Error: %s\n", SDL_GetError());
        cleanup(&rs);
        return 1;
    }

    SDL_Surface* bomb_surface = IMG_Load("bomb.png");
    rs.bomb_texture = SDL_CreateTextureFromSurface(rs.renderer, bomb_surface);
    SDL_FreeSurface(bomb_surface);
    if (!rs.bomb_texture) {
        printf("Failed to create bomb texture! SDL_Error: %s\n", SDL_GetError());
        cleanup(&rs);
        return 1;
    }

    SDL_Surface* restart_surface = IMG_Load("restart.png");
    rs.restart_texture = SDL_CreateTextureFromSurface(rs.renderer, restart_surface);
    SDL_FreeSurface(restart_surface);
    if (!rs.restart_texture) {
        printf("Failed to create restart texture! SDL_Error: %s\n", SDL_GetError());
        cleanup(&rs);
        return 1;
    }

    // Initialize number textures
    init_number_textures(rs.font, rs.renderer, rs.number_textures);

    // Load sound effects
    rs.click_sound = Mix_LoadWAV("click.wav");
    rs.explosion_sound = Mix_LoadWAV("explosion.wav");
    rs.game_over_sound = Mix_LoadWAV("gameover.wav");
    rs.restart_sound = Mix_LoadWAV("restart.wav");
    rs.flag_sound = Mix_LoadWAV("flag.wav");
    rs.win_sound = Mix_LoadWAV("win.wav");

    GameState gs = {0};
    int running = 1;
    int in_menu = 1;
    int selected_difficulty = 0;
    SDL_Event event;

    // Main game loop
    while (running) {
        // Process events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (in_menu) {
                // Handle menu navigation
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_UP) {
                        selected_difficulty = (selected_difficulty - 1 + 3) % 3;
                    } else if (event.key.keysym.sym == SDLK_DOWN) {
                        selected_difficulty = (selected_difficulty + 1) % 3;
                    } else if (event.key.keysym.sym == SDLK_RETURN) {
                        in_menu = 0;
                        reset_game(&gs, &rs, selected_difficulty);
                    }
                }
            } else {
                // Handle game input
                if (event.type == SDL_MOUSEBUTTONDOWN && !gs.game_over) {
                    int x = (event.button.x - rs.offset_x) / rs.tile_size;
                    int y = (event.button.y - rs.offset_y) / rs.tile_size;
                    if (x >= 0 && x < gs.grid_width && y >= 0 && y < gs.grid_height) {
                        if (event.button.button == SDL_BUTTON_LEFT) {
                            reveal_tile(&gs, &rs, x, y);
                        } else if (event.button.button == SDL_BUTTON_RIGHT) {
                            toggle_flag(&gs, &rs, x, y);
                        }
                    }
                    // Check for restart button click
                    SDL_Rect restart_rect = {rs.offset_x + (gs.grid_width * rs.tile_size) - rs.tile_size, rs.offset_y - rs.tile_size, rs.tile_size, rs.tile_size};
                    if (event.button.x >= restart_rect.x && event.button.x < restart_rect.x + restart_rect.w &&
                        event.button.y >= restart_rect.y && event.button.y < restart_rect.y + restart_rect.h) {
                        reset_game(&gs, &rs, gs.difficulty);
                    }
                } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r && gs.game_over) {
                    reset_game(&gs, &rs, gs.difficulty);
                } else if (event.type == SDL_MOUSEMOTION) {
                    // Update highlighted tile
                    int x = (event.motion.x - rs.offset_x) / rs.tile_size;
                    int y = (event.motion.y - rs.offset_y) / rs.tile_size;
                    rs.highlighted_x = (x >= 0 && x < gs.grid_width && y >= 0 && y < gs.grid_height) ? x : -1;
                    rs.highlighted_y = y;
                }
            }
        }

        if (in_menu) {
            render_menu(&rs, selected_difficulty);
        } else {
            // For victory condition
            if (gs.tiles_revealed == (gs.grid_width * gs.grid_height - gs.bomb_count) && !gs.game_over) {
                gs.game_over = 1;
                gs.game_won = 1;
                save_high_score(&gs);
                Mix_PlayChannel(-1, rs.win_sound, 0); // Tried loop count but didin't work for some reasons unknown to me
            }
            render_game(&gs, &rs);
        }
    }

    cleanup(&rs);
    return 0;
}