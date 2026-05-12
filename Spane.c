// =============================================================================
// SPANE GAME ENGINE - Complete Single File
// =============================================================================
// Compile: gcc -O3 -o spane Spane.c -lX11 -lm -ldl -lpthread
// Run: ./spane [--web]
// 
// Games are loaded from ./games/*.so at startup
// Place compiled game .so files in the games/ directory
// =============================================================================

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>

// =============================================================================
// CONFIGURATION
// =============================================================================

#define MAIN_WINDOW_WIDTH  1000
#define MAIN_WINDOW_HEIGHT 700
#define GAME_AREA_WIDTH    800
#define GAME_AREA_HEIGHT   600
#define GAME_AREA_X        180
#define GAME_AREA_Y        50
#define SIDEBAR_WIDTH      160
#define MAX_GAMES          20
#define MAX_PATH           512

#define WEB_PORT 3000

// =============================================================================
// 5x7 BITMAP FONT (ASCII 32-126)
// =============================================================================

static const unsigned char font_5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5f,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7f,0x14,0x7f,0x14},{0x24,0x2a,0x7f,0x2a,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1c,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1c,0x00},{0x08,0x2a,0x1c,0x2a,0x08},{0x08,0x08,0x3e,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02},{0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},
    {0x27,0x45,0x45,0x45,0x39},{0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e},{0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00},{0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},
    {0x41,0x22,0x14,0x08,0x00},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3e},
    {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
    {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x01,0x01},
    {0x3e,0x41,0x41,0x51,0x32},{0x7f,0x08,0x08,0x08,0x7f},{0x00,0x41,0x7f,0x41,0x00},
    {0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
    {0x7f,0x02,0x04,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
    {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},
    {0x1f,0x20,0x40,0x20,0x1f},{0x7f,0x20,0x18,0x20,0x7f},{0x63,0x14,0x08,0x14,0x63},
    {0x03,0x04,0x78,0x04,0x03},{0x61,0x51,0x49,0x45,0x43},{0x00,0x00,0x7f,0x41,0x41},
    {0x02,0x04,0x08,0x10,0x20},{0x41,0x41,0x7f,0x00,0x00},{0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
    {0x7f,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7f},
    {0x38,0x54,0x54,0x54,0x18},{0x08,0x7e,0x09,0x01,0x02},{0x08,0x14,0x54,0x54,0x3c},
    {0x7f,0x08,0x04,0x04,0x78},{0x00,0x44,0x7d,0x40,0x00},{0x20,0x40,0x44,0x3d,0x00},
    {0x00,0x7f,0x10,0x28,0x44},{0x00,0x41,0x7f,0x40,0x00},{0x7c,0x04,0x18,0x04,0x78},
    {0x7c,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7c,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7c},{0x7c,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3f,0x44,0x40,0x20},{0x3c,0x40,0x40,0x20,0x7c},{0x1c,0x20,0x40,0x20,0x1c},
    {0x3c,0x40,0x30,0x40,0x3c},{0x44,0x28,0x10,0x28,0x44},{0x0c,0x50,0x50,0x50,0x3c},
    {0x44,0x64,0x54,0x4c,0x44},{0x00,0x08,0x36,0x41,0x00},{0x00,0x00,0x7f,0x00,0x00},
    {0x00,0x41,0x36,0x08,0x00},{0x08,0x08,0x2a,0x1c,0x08}
};

// =============================================================================
// SDK TYPES - Game developers use these
// =============================================================================

typedef struct Framebuffer Framebuffer;
typedef struct Game Game;
typedef struct GameManager GameManager;

struct Framebuffer {
    unsigned char pixels[MAIN_WINDOW_WIDTH * MAIN_WINDOW_HEIGHT * 4];
};

// Game interface - define your game by implementing these functions
struct Game {
    char name[64];
    char path[MAX_PATH];
    void* data;
    void* handle;        // dlopen handle
    int active;
    
    // SDK function pointers - implement these in your game
    void (*init)(Game* game);
    void (*handle_key)(Game* game, int key_code, int pressed);
    void (*handle_click)(Game* game, int x, int y);
    void (*update)(Game* game);
    void (*render)(Game* game, Framebuffer* fb);
    void (*cleanup)(Game* game);
};

struct GameManager {
    Framebuffer framebuffer;
    Game* games[MAX_GAMES];
    int game_count;
    int current_game;
    int hover_button;
    int mouse_x, mouse_y;
    int frame_count;
    time_t last_fps_update;
    double fps;
    pthread_mutex_t fb_mutex;
    
    Display* display;
    Window window;
    GC gc;
    XImage* ximage;
    int x11_running;
    int web_mode;
};

// =============================================================================
// SDK DRAWING FUNCTIONS
// =============================================================================

static inline void fb_pixel(Framebuffer* fb, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= MAIN_WINDOW_WIDTH || y < 0 || y >= MAIN_WINDOW_HEIGHT) return;
    int i = (y * MAIN_WINDOW_WIDTH + x) * 4;
    fb->pixels[i]=r; fb->pixels[i+1]=g; fb->pixels[i+2]=b; fb->pixels[i+3]=255;
}

static void fb_fill(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int dy=0; dy<h; dy++)
        for (int dx=0; dx<w; dx++)
            fb_pixel(fb, x+dx, y+dy, r, g, b);
}

static void fb_rect(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int dx=0; dx<w; dx++) { fb_pixel(fb,x+dx,y,r,g,b); fb_pixel(fb,x+dx,y+h-1,r,g,b); }
    for (int dy=1; dy<h-1; dy++) { fb_pixel(fb,x,y+dy,r,g,b); fb_pixel(fb,x+w-1,y+dy,r,g,b); }
}

static void fb_circle(Framebuffer* fb, int cx, int cy, int radius, unsigned char R, unsigned char G, unsigned char B) {
    for (int dy=-radius; dy<=radius; dy++)
        for (int dx=-radius; dx<=radius; dx++)
            if (dx*dx+dy*dy <= radius*radius) fb_pixel(fb, cx+dx, cy+dy, R, G, B);
}

static void fb_char(Framebuffer* fb, int x, int y, char c, unsigned char r, unsigned char g, unsigned char b) {
    if (c<32||c>126) return;
    const unsigned char* glyph = font_5x7[c-32];
    for (int row=0; row<7; row++)
        for (int col=0; col<5; col++)
            if (glyph[col] & (1<<row)) fb_pixel(fb, x+col, y+row, r, g, b);
}

static void fb_text(Framebuffer* fb, int x, int y, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    for (int i=0; s[i]; i++) fb_char(fb, x+i*6, y, s[i], r, g, b);
}

static void fb_text_center(Framebuffer* fb, int x, int y, int w, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    fb_text(fb, x+(w-(int)strlen(s)*6)/2, y, s, r, g, b);
}

// =============================================================================
// GAME LOADER - Dynamically loads .so files
// =============================================================================

typedef Game* (*create_game_fn)();

static int load_game_from_so(GameManager* gm, const char* path) {
    void* handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "Failed to load %s: %s\n", path, dlerror());
        return 0;
    }
    
    create_game_fn create = (create_game_fn)dlsym(handle, "create_game");
    if (!create) {
        fprintf(stderr, "No create_game symbol in %s\n", path);
        dlclose(handle);
        return 0;
    }
    
    Game* game = create();
    if (!game) {
        fprintf(stderr, "create_game returned NULL for %s\n", path);
        dlclose(handle);
        return 0;
    }
    
    game->handle = handle;
    strncpy(game->path, path, MAX_PATH-1);
    
    if (gm->game_count < MAX_GAMES) {
        gm->games[gm->game_count++] = game;
        printf("Loaded game: %s from %s\n", game->name, path);
        return 1;
    }
    
    dlclose(handle);
    return 0;
}

static void scan_games_directory(GameManager* gm, const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        printf("No games directory found at: %s\n", dir_path);
        return;
    }
    
    struct dirent* entry;
    char full_path[MAX_PATH];
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (entry->d_name[0] == '.') continue;
        
        // Check if it's a .so file
        int len = strlen(entry->d_name);
        if (len < 3 || strcmp(entry->d_name + len - 3, ".so") != 0) continue;
        
        snprintf(full_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);
        
        // Check if it's a regular file
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            load_game_from_so(gm, full_path);
        }
    }
    
    closedir(dir);
    
    if (gm->game_count == 0) {
        printf("No valid game .so files found in %s\n", dir_path);
    }
}

// =============================================================================
// ENGINE RENDERING
// =============================================================================

static void gm_render(GameManager* gm) {
    pthread_mutex_lock(&gm->fb_mutex);
    Framebuffer* fb = &gm->framebuffer;
    
    // Clear background
    fb_fill(fb, 0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, 0x1A, 0x1A, 0x1A);
    
    // Sidebar
    fb_fill(fb, 0, 0, SIDEBAR_WIDTH, MAIN_WINDOW_HEIGHT, 0x15, 0x15, 0x18);
    fb_text_center(fb, 0, 15, SIDEBAR_WIDTH, "SPANE ENGINE", 0x00, 0xCC, 0xFF);
    fb_fill(fb, 10, 35, SIDEBAR_WIDTH-20, 2, 0x00, 0x88, 0xCC);
    
    int by = 55;
    for (int i = 0; i < gm->game_count; i++) {
        int act = (i == gm->current_game);
        int hov = (i == gm->hover_button);
        unsigned char r, g, b;
        
        if (act) { r = 0x00; g = 0x88; b = 0xCC; }
        else if (hov) { r = 0xCC; g = 0x88; b = 0x00; }
        else { r = 0x25; g = 0x25; b = 0x2A; }
        
        fb_fill(fb, 10, by, SIDEBAR_WIDTH-20, 35, r, g, b);
        fb_rect(fb, 10, by, SIDEBAR_WIDTH-20, 35, act ? 0x00:0x44, act ? 0xCC:0x44, act ? 0xFF:0x44);
        fb_text_center(fb, 10, by+12, SIDEBAR_WIDTH-20, gm->games[i]->name, 0xFF, 0xFF, 0xFF);
        by += 45;
    }
    
    // Load button
    int lby = MAIN_WINDOW_HEIGHT - 140;
    fb_fill(fb, 10, lby, SIDEBAR_WIDTH-20, 30, 0x00, 0x66, 0x00);
    fb_rect(fb, 10, lby, SIDEBAR_WIDTH-20, 30, 0x00, 0x88, 0x00);
    fb_text_center(fb, 10, lby+9, SIDEBAR_WIDTH-20, "[ Load Game ]", 0xFF, 0xFF, 0xFF);
    
    // FPS and info
    char fs[64];
    snprintf(fs, sizeof(fs), "FPS: %.1f", gm->fps);
    fb_text(fb, 10, lby+45, fs, 0x88, 0x88, 0x88);
    
    char game_count_str[32];
    snprintf(game_count_str, sizeof(game_count_str), "Games: %d", gm->game_count);
    fb_text(fb, 10, lby+60, game_count_str, 0x88, 0x88, 0x88);
    
    // Game area border
    for (int t = 0; t < 2; t++)
        fb_rect(fb, GAME_AREA_X-t-1, GAME_AREA_Y-t-1, GAME_AREA_WIDTH+2*t+2, GAME_AREA_HEIGHT+2*t+2, 0x44, 0x44, 0x55);
    
    // Game title
    if (gm->game_count > 0 && gm->current_game < gm->game_count) {
        char lb[128];
        snprintf(lb, sizeof(lb), "Game: %s", gm->games[gm->current_game]->name);
        fb_text(fb, GAME_AREA_X, GAME_AREA_Y-15, lb, 0xCC, 0xCC, 0xCC);
        
        // Render current game
        if (gm->games[gm->current_game]->active)
            gm->games[gm->current_game]->render(gm->games[gm->current_game], fb);
    } else {
        // No games loaded message
        fb_text_center(fb, GAME_AREA_X, GAME_AREA_Y + GAME_AREA_HEIGHT/2 - 10, GAME_AREA_WIDTH,
                       "No games loaded. Place .so files in ./games/", 0x88, 0x88, 0x88);
        fb_text_center(fb, GAME_AREA_X, GAME_AREA_Y + GAME_AREA_HEIGHT/2 + 10, GAME_AREA_WIDTH,
                       "Click [Load Game] to select a game file", 0x66, 0x66, 0x66);
    }
    
    pthread_mutex_unlock(&gm->fb_mutex);
}

static void gm_switch(GameManager* gm, int i) {
    if (i >= 0 && i < gm->game_count && i != gm->current_game) {
        gm->current_game = i;
        printf("Switched to: %s\n", gm->games[i]->name);
    }
}

static void gm_click(GameManager* gm, int x, int y) {
    // Check load button
    int lby = MAIN_WINDOW_HEIGHT - 140;
    if (x >= 10 && x < SIDEBAR_WIDTH-10 && y >= lby && y < lby+30) {
        // Open file browser dialog using zenity
        printf("Load Game button clicked - opening file browser...\n");
        int ret = system("zenity --file-selection --title=\"Select Game .so file\" "
                         "--file-filter=\"Shared Objects (*.so) | *.so\" "
                         "> /tmp/spane_load_game.txt 2>/dev/null");
        if (ret == 0) {
            FILE* f = fopen("/tmp/spane_load_game.txt", "r");
            if (f) {
                char path[MAX_PATH];
                if (fgets(path, sizeof(path), f)) {
                    // Remove newline
                    path[strcspn(path, "\n")] = 0;
                    if (strlen(path) > 0) {
                        printf("Loading game from: %s\n", path);
                        if (load_game_from_so(gm, path)) {
                            // Initialize the newly loaded game
                            Game* new_game = gm->games[gm->game_count - 1];
                            if (new_game->init) {
                                new_game->init(new_game);
                            }
                            gm->current_game = gm->game_count - 1;
                            printf("Game loaded and activated!\n");
                        } else {
                            printf("Failed to load game from: %s\n", path);
                        }
                    }
                }
                fclose(f);
            }
            remove("/tmp/spane_load_game.txt");
        }
        return;
    }
    
    // Check sidebar game buttons
    if (x < SIDEBAR_WIDTH) {
        int by = 55;
        for (int i = 0; i < gm->game_count; i++) {
            if (y >= by && y < by+35 && x >= 10 && x < SIDEBAR_WIDTH-10) {
                gm_switch(gm, i);
                return;
            }
            by += 45;
        }
    } else if (gm->game_count > 0 && gm->current_game < gm->game_count 
               && gm->games[gm->current_game]->active) {
        gm->games[gm->current_game]->handle_click(gm->games[gm->current_game], x, y);
    }
}

static void gm_fps(GameManager* gm) {
    gm->frame_count++;
    time_t n = time(NULL);
    if (n - gm->last_fps_update >= 1) {
        gm->fps = gm->frame_count / (double)(n - gm->last_fps_update);
        gm->frame_count = 0;
        gm->last_fps_update = n;
    }
}

// =============================================================================
// X11 MIRROR
// =============================================================================

static void x11_mirror_frame(GameManager* gm) {
    if (!gm->display || !gm->ximage) return;
    pthread_mutex_lock(&gm->fb_mutex);
    unsigned char* src = gm->framebuffer.pixels;
    char* dst = gm->ximage->data;
    for (int i = 0; i < MAIN_WINDOW_WIDTH * MAIN_WINDOW_HEIGHT; i++) {
        dst[i*4+0] = src[i*4+2];
        dst[i*4+1] = src[i*4+1];
        dst[i*4+2] = src[i*4+0];
        dst[i*4+3] = 0;
    }
    XPutImage(gm->display, gm->window, gm->gc, gm->ximage, 0, 0, 0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
    XFlush(gm->display);
    pthread_mutex_unlock(&gm->fb_mutex);
}

static int x11_init(GameManager* gm) {
    gm->display = XOpenDisplay(NULL);
    if (!gm->display) return 0;
    
    int s = DefaultScreen(gm->display);
    gm->window = XCreateSimpleWindow(gm->display, RootWindow(gm->display, s),
                                     50, 50, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, 1,
                                     BlackPixel(gm->display, s), 0x1A1A1A);
    XSelectInput(gm->display, gm->window,
                 ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | 
                 ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);
    XStoreName(gm->display, gm->window, "SPANE Game Engine");
    XMapWindow(gm->display, gm->window);
    gm->gc = XCreateGC(gm->display, gm->window, 0, NULL);
    
    char* imgdata = malloc(MAIN_WINDOW_WIDTH * MAIN_WINDOW_HEIGHT * 4);
    gm->ximage = XCreateImage(gm->display, DefaultVisual(gm->display, s), 24, ZPixmap, 0,
                              imgdata, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, 32, 0);
    
    XEvent e;
    do { XNextEvent(gm->display, &e); } while (e.type != MapNotify);
    printf("X11 display ready\n");
    return 1;
}

static int x11_to_keycode(KeySym ks) {
    if (ks == XK_Escape) return 27;
    if (ks == XK_F1) return 112;
    if (ks == XK_F2) return 113;
    if (ks == XK_F3) return 114;
    if (ks == XK_F4) return 115;
    if (ks == XK_Up || ks == XK_w || ks == XK_W) return 38;
    if (ks == XK_Down || ks == XK_s || ks == XK_S) return 40;
    if (ks == XK_Left || ks == XK_a || ks == XK_A) return 37;
    if (ks == XK_Right || ks == XK_d || ks == XK_D) return 39;
    if (ks == XK_r || ks == XK_R) return 82;
    if (ks == XK_n || ks == XK_N) return 78;
    if (ks == XK_q || ks == XK_Q) return 81;
    if (ks == XK_Return || ks == XK_KP_Enter) return 13;
    if (ks == XK_space || ks == XK_KP_Space) return 32;
    if (ks == XK_BackSpace) return 8;
    if (ks == XK_Delete || ks == XK_KP_Delete) return 127;
    
    // NOVO: Mapeamento de dígitos do teclado principal
    if (ks >= XK_0 && ks <= XK_9) return '0' + (ks - XK_0);
    
    // NOVO: Mapeamento de dígitos do keypad numérico
    if (ks >= XK_KP_0 && ks <= XK_KP_9) return '0' + (ks - XK_KP_0);
    
    return 0;
}

static void x11_process(GameManager* gm, int* running) {
    while (XPending(gm->display)) {
        XEvent e;
        XNextEvent(gm->display, &e);
        
        if (e.type == KeyPress) {
            int k = x11_to_keycode(XLookupKeysym(&e.xkey, 0));
            if (k == 27) *running = 0;
            else if (k >= 112 && k <= 115) gm_switch(gm, k - 112);
            else if (gm->game_count > 0 && gm->current_game < gm->game_count 
                     && gm->games[gm->current_game]->active)
                gm->games[gm->current_game]->handle_key(gm->games[gm->current_game], k, 1);
        }
        else if (e.type == KeyRelease) {
            int k = x11_to_keycode(XLookupKeysym(&e.xkey, 0));
            if (k && gm->game_count > 0 && gm->current_game < gm->game_count 
                && gm->games[gm->current_game]->active)
                gm->games[gm->current_game]->handle_key(gm->games[gm->current_game], k, 0);
        }
        else if (e.type == ButtonPress) {
            gm_click(gm, e.xbutton.x, e.xbutton.y);
        }
        else if (e.type == MotionNotify) {
            gm->mouse_x = e.xmotion.x;
            gm->mouse_y = e.xmotion.y;
            gm->hover_button = -1;
            if (e.xmotion.x < SIDEBAR_WIDTH) {
                int by = 55;
                for (int i = 0; i < gm->game_count; i++) {
                    if (e.xmotion.y >= by && e.xmotion.y < by+35 
                        && e.xmotion.x >= 10 && e.xmotion.x < SIDEBAR_WIDTH-10) {
                        gm->hover_button = i;
                        break;
                    }
                    by += 45;
                }
            }
        }
    }
}

static void x11_cleanup(GameManager* gm) {
    if (gm->ximage) {
        free(gm->ximage->data);
        gm->ximage->data = NULL;
        XDestroyImage(gm->ximage);
    }
    if (gm->gc) XFreeGC(gm->display, gm->gc);
    if (gm->window) XDestroyWindow(gm->display, gm->window);
    if (gm->display) XCloseDisplay(gm->display);
}

// =============================================================================
// WEB SERVER
// =============================================================================

typedef struct {
    int fd;
    GameManager* gm;
    pthread_t th;
    int run;
} WebServer;

static WebServer ws;

static const char* html =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n"
    "<!DOCTYPE html><html><head><title>SPANE Engine</title><style>"
    "body{margin:0;background:#000;display:flex;justify-content:center;align-items:center;height:100vh;overflow:hidden}"
    "canvas{image-rendering:pixelated;cursor:crosshair}"
    "#s{position:fixed;top:10px;left:10px;color:#0f0;font-size:14px;background:rgba(0,0,0,.7);padding:8px;border-radius:4px}"
    "</style></head><body><div id=s>Connecting...</div><canvas id=g></canvas><script>"
    "const s=document.getElementById('s'),c=document.getElementById('g'),x=c.getContext('2d'),W=1000,H=700;c.width=W;c.height=H;"
    "let img=x.createImageData(W,H),fc=0,lt=Date.now(),fps=0,con=0;"
    "function rs(){let sc=Math.min((innerWidth-20)/W,(innerHeight-20)/H,2);c.style.width=Math.floor(W*sc)+'px';c.style.height=Math.floor(H*sc)+'px';}"
    "addEventListener('resize',rs);rs();"
    "function ff(){fetch('/f?'+Date.now()).then(r=>r.arrayBuffer()).then(b=>{"
    "let p=new Uint8Array(b);if(p.length===W*H*4){img.data.set(p);x.putImageData(img,0,0);"
    "if(!con){con=1}s.textContent='Connected | FPS: '+fps;fc++;let n=Date.now();"
    "if(n-lt>=1000){fps=fc;fc=0;lt=n;}}setTimeout(ff,16);}).catch(e=>{con=0;s.textContent='Reconnecting...';setTimeout(ff,500);});}"
    "function si(p){fetch('/i?'+p,{method:'POST',cache:'no-cache'}).catch(()=>{});}"
    "c.addEventListener('mousemove',e=>{let r=c.getBoundingClientRect();"
    "si('t=m&x='+Math.round((e.clientX-r.left)*W/r.width)+'&y='+Math.round((e.clientY-r.top)*H/r.height));});"
    "c.addEventListener('mousedown',e=>{let r=c.getBoundingClientRect();"
    "si('t=c&x='+Math.round((e.clientX-r.left)*W/r.width)+'&y='+Math.round((e.clientY-r.top)*H/r.height)+'&b='+e.button);e.preventDefault();});"
    "c.addEventListener('contextmenu',e=>e.preventDefault());"
    "document.addEventListener('keydown',e=>{si('t=kd&k='+e.keyCode);e.preventDefault();});"
    "document.addEventListener('keyup',e=>{si('t=ku&k='+e.keyCode);e.preventDefault();});"
    "ff();</script></body></html>\n";

static void whandle(WebServer* w, int cf) {
    char b[16384];
    int n = recv(cf, b, sizeof(b)-1, 0);
    if (n <= 0) return;
    b[n] = 0;
    
    char m[16], p[512];
    sscanf(b, "%15s %511s", m, p);
    
    if (strcmp(p, "/") == 0 || strcmp(p, "/index.html") == 0) {
        send(cf, html, strlen(html), 0);
    }
    else if (strncmp(p, "/f", 2) == 0) {
        pthread_mutex_lock(&w->gm->fb_mutex);
        int sz = MAIN_WINDOW_WIDTH * MAIN_WINDOW_HEIGHT * 4;
        char h[256];
        int hl = snprintf(h, sizeof(h),
            "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"
            "Content-Length: %d\r\nCache-Control: no-cache,no-store\r\nConnection: close\r\n\r\n", sz);
        send(cf, h, hl, 0);
        int sent = 0;
        while (sent < sz) {
            int c = send(cf, w->gm->framebuffer.pixels + sent, sz - sent, 0);
            if (c <= 0) break;
            sent += c;
        }
        pthread_mutex_unlock(&w->gm->fb_mutex);
    }
    else if (strncmp(p, "/i?", 3) == 0) {
        char* q = strchr(p, '?') + 1;
        char t[8] = "";
        int x = 0, y = 0, k = 0;
        char* tk = strtok(q, "&");
        while (tk) {
            if (strncmp(tk, "t=", 2) == 0) sscanf(tk+2, "%7s", t);
            else if (strncmp(tk, "x=", 2) == 0) sscanf(tk+2, "%d", &x);
            else if (strncmp(tk, "y=", 2) == 0) sscanf(tk+2, "%d", &y);
            else if (strncmp(tk, "k=", 2) == 0) sscanf(tk+2, "%d", &k);
            tk = strtok(NULL, "&");
        }
        pthread_mutex_lock(&w->gm->fb_mutex);
        if (strcmp(t, "m") == 0) {
            w->gm->mouse_x = x;
            w->gm->mouse_y = y;
            w->gm->hover_button = -1;
            if (x < SIDEBAR_WIDTH) {
                int by = 55;
                for (int i = 0; i < w->gm->game_count; i++) {
                    if (y >= by && y < by+35 && x >= 10 && x < SIDEBAR_WIDTH-10) {
                        w->gm->hover_button = i;
                        break;
                    }
                    by += 45;
                }
            }
        }
        else if (strcmp(t, "c") == 0) {
            gm_click(w->gm, x, y);
        }
        else if (strcmp(t, "kd") == 0 || strcmp(t, "ku") == 0) {
            int pr = (t[1] == 'd');
            if (k == 27) w->run = 0;
            else if (k >= 112 && k <= 115 && pr) gm_switch(w->gm, k - 112);
            else if (w->gm->game_count > 0 && w->gm->current_game < w->gm->game_count 
                     && w->gm->games[w->gm->current_game]->active)
                w->gm->games[w->gm->current_game]->handle_key(w->gm->games[w->gm->current_game], k, pr);
        }
        pthread_mutex_unlock(&w->gm->fb_mutex);
        send(cf, "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK", 40, 0);
    }
    else {
        send(cf, "HTTP/1.1 404\r\nContent-Length: 0\r\n\r\n", 38, 0);
    }
}

static void* wthread(void* a) {
    WebServer* w = (WebServer*)a;
    w->fd = socket(AF_INET, SOCK_STREAM, 0);
    int o = 1;
    setsockopt(w->fd, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));
    
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(ad));
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(WEB_PORT);
    
    bind(w->fd, (struct sockaddr*)&ad, sizeof(ad));
    listen(w->fd, 10);
    
    printf("\n========================================\n");
    printf("  Web: http://localhost:%d\n", WEB_PORT);
    printf("========================================\n\n");
    
    w->run = 1;
    while (w->run) {
        fd_set f;
        FD_ZERO(&f);
        FD_SET(w->fd, &f);
        struct timeval tv = {0, 50000};
        
        if (select(w->fd+1, &f, 0, 0, &tv) > 0) {
            struct sockaddr_in ca;
            socklen_t cl = sizeof(ca);
            int cf = accept(w->fd, (struct sockaddr*)&ca, &cl);
            if (cf >= 0) {
                struct timeval to = {5, 0};
                setsockopt(cf, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));
                whandle(w, cf);
                close(cf);
            }
        }
    }
    close(w->fd);
    return NULL;
}

// =============================================================================
// MAIN
// =============================================================================

// In Spane.c, modify the main function to start with 0 games
// and scan both current directory AND games subdirectory

int main(int argc, char** argv) {
    GameManager gm;
    memset(&gm, 0, sizeof(gm));
    pthread_mutex_init(&gm.fb_mutex, NULL);
    
    int web = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--web") == 0) web = 1;
    }
    
    // Try X11
    int has_x11 = x11_init(&gm);
    if (!has_x11) {
        printf("No display available - using web mode\n");
        web = 1;
    }
    if (web && has_x11) {
        x11_cleanup(&gm);
        memset(&gm.display, 0, sizeof(Display*) + sizeof(Window) + sizeof(GC) + sizeof(XImage*));
        has_x11 = 0;
    }
    
    gm.web_mode = web;
    gm.game_count = 0;  // Start with 0 games
    gm.current_game = -1;  // No game selected
    
    // Start web server if needed
    if (web) {
        ws.gm = &gm;
        pthread_create(&ws.th, NULL, wthread, &ws);
        usleep(500000);
    }
    
    printf("SPANE Engine running in %s mode\n", web ? "Web" : "X11");
    printf("Click [Load Game] to load game .so files\n");
    printf("Place .so files in ./games/ and restart to auto-load\n\n");
    
    srand(time(NULL));
    gm.last_fps_update = time(NULL);
    
    int running = 1;
    while (running) {
        if (has_x11) x11_process(&gm, &running);
        if (web && !ws.run) running = 0;
        
        // Update current game
        if (gm.game_count > 0 && gm.current_game >= 0 && gm.current_game < gm.game_count 
            && gm.games[gm.current_game]->active 
            && gm.games[gm.current_game]->update)
            gm.games[gm.current_game]->update(gm.games[gm.current_game]);
        
        gm_render(&gm);
        if (has_x11) x11_mirror_frame(&gm);
        gm_fps(&gm);
        usleep(16667);
    }
    
    printf("Shutting down...\n");
    for (int i = 0; i < gm.game_count; i++) {
        if (gm.games[i]->active && gm.games[i]->cleanup)
            gm.games[i]->cleanup(gm.games[i]);
        if (gm.games[i]->handle) dlclose(gm.games[i]->handle);
    }
    if (has_x11) x11_cleanup(&gm);
    if (web) pthread_join(ws.th, NULL);
    pthread_mutex_destroy(&gm.fb_mutex);
    
    return 0;
}