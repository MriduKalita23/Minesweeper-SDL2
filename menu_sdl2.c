#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    STATE_MAIN,
    STATE_GAME
} GameState;

typedef struct {
    SDL_Rect rect;
    const char* text;
    bool hovered;
    int mode;      // 0 for 2D, 1 for 3D
} Button;

typedef struct {
    GameState state;
    bool running;
    int selectedMode;      // 0 for 2D, 1 for 3D
    int selectedDifficulty; // 0 for Easy, 1 for Medium, 2 for Hard
} App;

void launchGame(int mode) {
    const char* modes[] = {"B24CI1014_B24BB1019_B24EE1040_B24ME1008\\<2D\\>"};
    char command[256];

    snprintf(command, sizeof(command), "./%s &", modes[mode]);
    system(command);
}

void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Rect rect, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    
    int textW, textH;
    TTF_SizeText(font, text, &textW, &textH);
    SDL_Rect dstRect = {rect.x + (rect.w - textW) / 2, rect.y + (rect.h - textH) / 2, textW, textH};
    
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

bool isMouseOverButton(Button* button, int mouseX, int mouseY) {
    return (mouseX >= button->rect.x && mouseX <= button->rect.x + button->rect.w &&
            mouseY >= button->rect.y && mouseY <= button->rect.y + button->rect.h);
}

void animateButton(Button* button, float* scale, float targetScale, float speed) {
    if (*scale < targetScale) {
        *scale += speed;
        if (*scale > targetScale) *scale = targetScale;
    }
    else if (*scale > targetScale) {
        *scale -= speed;
        if (*scale < targetScale) *scale = targetScale;
    }
}

void runGame(App* app, SDL_Window* window, SDL_Renderer* renderer) {
    launchGame(app->selectedMode);
    app->running = false; // End the app after game execution
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);
    int SCREEN_WIDTH = DM.w;
    int SCREEN_HEIGHT = DM.h;

    SDL_Window* window = SDL_CreateWindow("Game Menu",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SCREEN_WIDTH,
                                        SCREEN_HEIGHT,
                                        SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("Minecraft.ttf", 32);
    if (!font) {
        printf("Font loading failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return 1;
    }

    int buttonWidth = SCREEN_WIDTH / 4;
    int buttonHeight = SCREEN_HEIGHT / 8;
    int centerX = SCREEN_WIDTH / 2 - buttonWidth / 2;
    int spacing = SCREEN_HEIGHT / 10;

    // Define buttons for 2D and 3D with default difficulty (Easy)
    Button mainButtons[1] = {
        {{centerX, SCREEN_HEIGHT/2, buttonWidth, buttonHeight}, "PLAY", false, 0},
    };

    float buttonScales[2] = {1.0f, 1.0f};

    App app = {STATE_MAIN, true, -1, -1};

    SDL_Event event;
    while (app.running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                app.running = false;
            }
            else if (event.type == SDL_MOUSEMOTION && app.state == STATE_MAIN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                for (int i = 0; i < 2; i++) {
                    mainButtons[i].hovered = isMouseOverButton(&mainButtons[i], mouseX, mouseY);
                }
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && app.state == STATE_MAIN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                for (int i = 0; i < 2; i++) {
                    if (isMouseOverButton(&mainButtons[i], mouseX, mouseY)) {
                        app.state = STATE_GAME;
                        app.selectedMode = mainButtons[i].mode;
                        break;
                    }
                }
            }
        }
        TTF_Font* t = TTF_OpenFont("Minecraft.ttf", 100);

        SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
        SDL_RenderClear(renderer);

        if (app.state == STATE_MAIN) {
            // Render "MINESWEEPER" title at the top
            SDL_Rect titleRect = {0, SCREEN_HEIGHT/10, SCREEN_WIDTH, SCREEN_HEIGHT/10};
            renderText(renderer, t, "MINESWEEPER", titleRect, (SDL_Color){255, 255, 255, 255});

            for (int i = 0; i < 2; i++) {
                Button* btn = &mainButtons[i];
                animateButton(btn, &buttonScales[i], btn->hovered ? 1.1f : 1.0f, 0.05f);

                SDL_Rect scaledRect = {
                    btn->rect.x - (int)((btn->rect.w * buttonScales[i] - btn->rect.w) / 2),
                    btn->rect.y - (int)((btn->rect.h * buttonScales[i] - btn->rect.h) / 2),
                    (int)(btn->rect.w * buttonScales[i]),
                    (int)(btn->rect.h * buttonScales[i])
                };

                SDL_Color btnColor = btn->hovered ? (SDL_Color){100, 150, 200, 255} : 
                                                  (SDL_Color){80, 120, 160, 255};
                SDL_SetRenderDrawColor(renderer, btnColor.r, btnColor.g, btnColor.b, btnColor.a);
                SDL_RenderFillRect(renderer, &scaledRect);
                
                SDL_Color borderColor = {255, 255, 255, 255};
                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                SDL_RenderDrawRect(renderer, &scaledRect);

                renderText(renderer, font, btn->text, scaledRect, (SDL_Color){255, 255, 255, 255});
            }
        }
        else if (app.state == STATE_GAME) {
            runGame(&app, window, renderer);
            app.state = STATE_MAIN; // Return to main menu after game ends
        }

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
