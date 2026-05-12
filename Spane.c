// =============================================================================
// SPANE GAME ENGINE - Multi-Instance Sync + Web Mode
// =============================================================================
// Compile: gcc -O3 -o spane Spane.c -lX11 -lm -ldl -lpthread -lrt
// Run: ./spane [--web] [--clear]
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
#include <sys/file.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

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
#define STATE_DIR "~/.spane"
#define SHM_NAME "/spane_shared_state"

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
// SDK TYPES
// =============================================================================

typedef struct Framebuffer Framebuffer;
typedef struct Game Game;
typedef struct GameManager GameManager;

struct Framebuffer {
    unsigned char pixels[MAIN_WINDOW_WIDTH * MAIN_WINDOW_HEIGHT * 4];
};

struct Game {
    char name[64];
    char path[MAX_PATH];
    void* data;
    void* handle;
    int active;
    
    void (*init)(Game* game);
    void (*handle_key)(Game* game, int key_code, int pressed);
    void (*handle_click)(Game* game, int x, int y);
    void (*update)(Game* game);
    void (*render)(Game* game, Framebuffer* fb);
    void (*cleanup)(Game* game);
};

typedef struct {
    int game_count;
    int current_game;
    int instance_count;
    int version;
    char game_paths[MAX_GAMES][MAX_PATH];
    int lock;
} SharedState;

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
    
    SharedState* shared;
    int shm_fd;
    int instance_id;
    int local_version;
    
    char state_dir[512];
};

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

static void sync_from_shared(GameManager* gm);
static void sync_to_shared(GameManager* gm);
static void scan_games_directory(GameManager* gm, const char* dir_path);
static void save_disk_backup(GameManager* gm);
static int load_disk_backup(GameManager* gm);
static int load_game_from_so(GameManager* gm, const char* path);
static void gm_render(GameManager* gm);
static void gm_switch(GameManager* gm, int i);
static void gm_click(GameManager* gm, int x, int y);

// =============================================================================
// SPINLOCK
// =============================================================================

static inline void shm_lock(SharedState* s) {
    while (__sync_lock_test_and_set(&s->lock, 1)) {
        usleep(10);
    }
}

static inline void shm_unlock(SharedState* s) {
    __sync_lock_release(&s->lock);
}

// =============================================================================
// DRAWING FUNCTIONS
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
// UTILITY
// =============================================================================

static void expand_path(const char* path, char* expanded, int max_len) {
    if (path[0] == '~') {
        const char* home = getenv("HOME");
        if (!home) home = "/tmp";
        snprintf(expanded, max_len, "%s%s", home, path + 1);
    } else {
        strncpy(expanded, path, max_len - 1);
    }
}

// =============================================================================
// SHARED MEMORY
// =============================================================================

static int ensure_state_directory(GameManager* gm) {
    char expanded[512];
    expand_path(STATE_DIR, expanded, sizeof(expanded));
    strncpy(gm->state_dir, expanded, sizeof(gm->state_dir) - 1);
    mkdir(expanded, 0755);
    return 1;
}

static int init_shared_state(GameManager* gm) {
    shm_unlink(SHM_NAME);
    
    gm->shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (gm->shm_fd < 0) {
        perror("shm_open");
        return 0;
    }
    
    if (ftruncate(gm->shm_fd, sizeof(SharedState)) < 0) {
        perror("ftruncate");
        close(gm->shm_fd);
        shm_unlink(SHM_NAME);
        return 0;
    }
    
    gm->shared = mmap(NULL, sizeof(SharedState), PROT_READ | PROT_WRITE, 
                     MAP_SHARED, gm->shm_fd, 0);
    if (gm->shared == MAP_FAILED) {
        perror("mmap");
        close(gm->shm_fd);
        shm_unlink(SHM_NAME);
        return 0;
    }
    
    memset(gm->shared, 0, sizeof(SharedState));
    gm->shared->current_game = -1;
    gm->shared->instance_count = 1;
    gm->shared->version = 1;
    gm->instance_id = 0;
    gm->local_version = 1;
    
    printf("Shared memory ready (instance %d)\n", gm->instance_id + 1);
    return 1;
}

static void sync_from_shared(GameManager* gm) {
    if (!gm->shared) return;
    
    shm_lock(gm->shared);
    
    if (gm->shared->version == gm->local_version) {
        shm_unlock(gm->shared);
        return;
    }
    
    gm->local_version = gm->shared->version;
    int new_count = gm->shared->game_count;
    int new_current = gm->shared->current_game;
    char paths_to_load[MAX_GAMES][MAX_PATH];
    
    for (int i = 0; i < new_count && i < MAX_GAMES; i++) {
        strncpy(paths_to_load[i], gm->shared->game_paths[i], MAX_PATH-1);
    }
    
    shm_unlock(gm->shared);
    
    if (new_count != gm->game_count || new_current != gm->current_game) {
        for (int i = 0; i < gm->game_count; i++) {
            if (gm->games[i]->active && gm->games[i]->cleanup)
                gm->games[i]->cleanup(gm->games[i]);
            if (gm->games[i]->handle) dlclose(gm->games[i]->handle);
            gm->games[i] = NULL;
        }
        gm->game_count = 0;
        
        for (int i = 0; i < new_count && i < MAX_GAMES; i++) {
            if (load_game_from_so(gm, paths_to_load[i])) {
                Game* game = gm->games[gm->game_count - 1];
                if (game->init) game->init(game);
            }
        }
        
        gm->current_game = new_current;
        if (gm->current_game >= gm->game_count) gm->current_game = -1;
    }
}

static void sync_to_shared(GameManager* gm) {
    if (!gm->shared) return;
    
    shm_lock(gm->shared);
    
    int changed = 0;
    
    if (gm->shared->game_count != gm->game_count) {
        gm->shared->game_count = gm->game_count;
        for (int i = 0; i < gm->game_count && i < MAX_GAMES; i++) {
            strncpy(gm->shared->game_paths[i], gm->games[i]->path, MAX_PATH-1);
        }
        changed = 1;
    }
    
    if (gm->shared->current_game != gm->current_game) {
        gm->shared->current_game = gm->current_game;
        changed = 1;
    }
    
    if (changed) {
        gm->shared->version++;
        gm->local_version = gm->shared->version;
    }
    
    shm_unlock(gm->shared);
}

static void cleanup_shared_state(GameManager* gm) {
    if (!gm->shared) return;
    
    shm_lock(gm->shared);
    gm->shared->instance_count--;
    int is_last = (gm->shared->instance_count <= 0);
    shm_unlock(gm->shared);
    
    munmap(gm->shared, sizeof(SharedState));
    gm->shared = NULL;
    
    if (gm->shm_fd >= 0) {
        close(gm->shm_fd);
        gm->shm_fd = -1;
    }
    
    if (is_last) {
        shm_unlink(SHM_NAME);
    }
}

// =============================================================================
// DISK BACKUP
// =============================================================================

static void save_disk_backup(GameManager* gm) {
    if (!gm->shared || gm->game_count == 0) return;
    
    char backup_path[1024];
    snprintf(backup_path, sizeof(backup_path), "%s/backup.dat", gm->state_dir);
    
    shm_lock(gm->shared);
    FILE* f = fopen(backup_path, "wb");
    if (f) {
        fwrite(&gm->shared->game_count, sizeof(int), 1, f);
        fwrite(&gm->shared->current_game, sizeof(int), 1, f);
        for (int i = 0; i < gm->shared->game_count && i < MAX_GAMES; i++) {
            fwrite(gm->shared->game_paths[i], MAX_PATH, 1, f);
        }
        fclose(f);
    }
    shm_unlock(gm->shared);
}

static int load_disk_backup(GameManager* gm) {
    char backup_path[1024];
    snprintf(backup_path, sizeof(backup_path), "%s/backup.dat", gm->state_dir);
    
    FILE* f = fopen(backup_path, "rb");
    if (!f) return 0;
    
    int count, current;
    if (fread(&count, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&current, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    
    if (gm->shared) {
        shm_lock(gm->shared);
        gm->shared->game_count = count;
        gm->shared->current_game = current;
        for (int i = 0; i < count && i < MAX_GAMES; i++) {
            if (fread(gm->shared->game_paths[i], MAX_PATH, 1, f) != 1) break;
        }
        gm->shared->version++;
        shm_unlock(gm->shared);
    }
    
    fclose(f);
    return 1;
}

static void clear_all_state(GameManager* gm) {
    if (gm->shared) {
        shm_lock(gm->shared);
        gm->shared->game_count = 0;
        gm->shared->current_game = -1;
        gm->shared->version++;
        shm_unlock(gm->shared);
    }
    
    char backup_path[1024];
    snprintf(backup_path, sizeof(backup_path), "%s/backup.dat", gm->state_dir);
    unlink(backup_path);
}

// =============================================================================
// GAME LOADER
// =============================================================================

typedef Game* (*create_game_fn)();

static int load_game_from_so(GameManager* gm, const char* path) {
    void* handle = dlopen(path, RTLD_NOW);
    if (!handle) return 0;
    
    create_game_fn create = (create_game_fn)dlsym(handle, "create_game");
    if (!create) { dlclose(handle); return 0; }
    
    Game* game = create();
    if (!game) { dlclose(handle); return 0; }
    
    game->handle = handle;
    strncpy(game->path, path, MAX_PATH-1);
    
    if (!game->init || !game->render || !game->update || !game->cleanup) {
        dlclose(handle);
        return 0;
    }
    
    if (gm->game_count < MAX_GAMES) {
        gm->games[gm->game_count++] = game;
        printf("Loaded: %s\n", game->name);
        return 1;
    }
    
    dlclose(handle);
    return 0;
}

static void scan_games_directory(GameManager* gm, const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent* entry;
    char full_path[MAX_PATH];
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        int len = strlen(entry->d_name);
        if (len < 3 || strcmp(entry->d_name + len - 3, ".so") != 0) continue;
        snprintf(full_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode))
            load_game_from_so(gm, full_path);
    }
    closedir(dir);
}

static void remove_game_instance(GameManager* gm, int index) {
    if (index < 0 || index >= gm->game_count) return;
    
    printf("Removing: %s\n", gm->games[index]->name);
    
    if (gm->games[index]->active && gm->games[index]->cleanup)
        gm->games[index]->cleanup(gm->games[index]);
    if (gm->games[index]->handle) dlclose(gm->games[index]->handle);
    
    for (int i = index; i < gm->game_count - 1; i++)
        gm->games[i] = gm->games[i+1];
    gm->game_count--;
    
    if (gm->current_game >= gm->game_count)
        gm->current_game = gm->game_count > 0 ? gm->game_count - 1 : -1;
    
    sync_to_shared(gm);
}

// =============================================================================
// RENDERING
// =============================================================================

static void gm_render(GameManager* gm) {
    pthread_mutex_lock(&gm->fb_mutex);
    Framebuffer* fb = &gm->framebuffer;
    
    fb_fill(fb, 0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, 0x1A, 0x1A, 0x1A);
    
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
        
        fb_fill(fb, 10, by, SIDEBAR_WIDTH-35, 35, r, g, b);
        fb_rect(fb, 10, by, SIDEBAR_WIDTH-35, 35, act ? 0x00:0x44, act ? 0xCC:0x44, act ? 0xFF:0x44);
        fb_text_center(fb, 10, by+12, SIDEBAR_WIDTH-35, gm->games[i]->name, 0xFF, 0xFF, 0xFF);
        
        int cx = SIDEBAR_WIDTH - 22;
        fb_fill(fb, cx, by, 18, 18, 0x88, 0x22, 0x22);
        fb_text_center(fb, cx, by+4, 18, "X", 0xFF, 0xFF, 0xFF);
        
        by += 45;
    }
    
    int lby = MAIN_WINDOW_HEIGHT - 110;
    
    fb_fill(fb, 10, lby, SIDEBAR_WIDTH-20, 28, 0x00, 0x66, 0x00);
    fb_rect(fb, 10, lby, SIDEBAR_WIDTH-20, 28, 0x00, 0x88, 0x00);
    fb_text_center(fb, 10, lby+8, SIDEBAR_WIDTH-20, "[ Load Game ]", 0xFF, 0xFF, 0xFF);
    
    fb_fill(fb, 10, lby+33, SIDEBAR_WIDTH-20, 28, 0x66, 0x00, 0x00);
    fb_rect(fb, 10, lby+33, SIDEBAR_WIDTH-20, 28, 0x88, 0x00, 0x00);
    fb_text_center(fb, 10, lby+41, SIDEBAR_WIDTH-20, "[ Clear All ]", 0xFF, 0xFF, 0xFF);
    
    char info[64];
    snprintf(info, sizeof(info), "FPS:%.1f Inst:%d %s", gm->fps, gm->instance_id + 1,
             gm->web_mode ? "WEB" : "X11");
    fb_text(fb, 10, MAIN_WINDOW_HEIGHT - 25, info, 0x88, 0x88, 0x88);
    
    for (int t = 0; t < 2; t++)
        fb_rect(fb, GAME_AREA_X-t-1, GAME_AREA_Y-t-1, GAME_AREA_WIDTH+2*t+2, GAME_AREA_HEIGHT+2*t+2, 0x44, 0x44, 0x55);
    
    for (int i = 0; i < gm->game_count; i++) {
        if (gm->games[i]->active && gm->games[i]->render && i == gm->current_game) {
            char lb[128];
            snprintf(lb, sizeof(lb), "Game: %s", gm->games[i]->name);
            fb_text(fb, GAME_AREA_X, GAME_AREA_Y-15, lb, 0xCC, 0xCC, 0xCC);
            gm->games[i]->render(gm->games[i], fb);
        }
    }
    
    if (gm->game_count == 0) {
        fb_text_center(fb, GAME_AREA_X, GAME_AREA_Y + GAME_AREA_HEIGHT/2, GAME_AREA_WIDTH,
                       "Click [Load Game] to add a game", 0x66, 0x66, 0x66);
    }
    
    pthread_mutex_unlock(&gm->fb_mutex);
}

static void gm_switch(GameManager* gm, int i) {
    if (i >= 0 && i < gm->game_count) {
        gm->current_game = i;
        sync_to_shared(gm);
    }
}

static void gm_click(GameManager* gm, int x, int y) {
    int lby = MAIN_WINDOW_HEIGHT - 110;
    
    if (x >= 10 && x < SIDEBAR_WIDTH-10) {
        if (y >= lby && y < lby+28) {
            int ret __attribute__((unused));
            ret = system("zenity --file-selection --title=\"Select Game .so file\" "
                         "--file-filter=\"Shared Objects (*.so) | *.so\" "
                         "> /tmp/spane_load.txt 2>/dev/null");
            FILE* f = fopen("/tmp/spane_load.txt", "r");
            if (f) {
                char path[MAX_PATH];
                if (fgets(path, sizeof(path), f)) {
                    path[strcspn(path, "\n")] = 0;
                    if (strlen(path) > 0 && load_game_from_so(gm, path)) {
                        Game* g = gm->games[gm->game_count - 1];
                        if (g->init) g->init(g);
                        gm->current_game = gm->game_count - 1;
                        sync_to_shared(gm);
                        save_disk_backup(gm);
                    }
                }
                fclose(f);
            }
            remove("/tmp/spane_load.txt");
            return;
        }
        
        if (y >= lby+33 && y < lby+61) {
            for (int i = 0; i < gm->game_count; i++) {
                if (gm->games[i]->active && gm->games[i]->cleanup)
                    gm->games[i]->cleanup(gm->games[i]);
                if (gm->games[i]->handle) dlclose(gm->games[i]->handle);
            }
            gm->game_count = 0;
            gm->current_game = -1;
            clear_all_state(gm);
            return;
        }
    }
    
    if (x < SIDEBAR_WIDTH) {
        int by = 55;
        for (int i = 0; i < gm->game_count; i++) {
            int cx = SIDEBAR_WIDTH - 22;
            if (x >= cx && x < cx+18 && y >= by && y < by+18) {
                remove_game_instance(gm, i);
                return;
            }
            if (y >= by && y < by+35 && x >= 10 && x < cx-2) {
                gm_switch(gm, i);
                return;
            }
            by += 45;
        }
    } else if (gm->game_count > 0 && gm->current_game < gm->game_count 
               && gm->games[gm->current_game]->active
               && gm->games[gm->current_game]->handle_click) {
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
// X11
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
    printf("X11 ready\n");
    return 1;
}

static int x11_to_keycode(KeySym ks) {
    // Function keys
    if (ks == XK_Escape) return 27;
    if (ks >= XK_F1 && ks <= XK_F12) return 111 + (ks - XK_F1);
    
    // Cursor keys
    if (ks == XK_Up || ks == XK_w || ks == XK_W) return 38;
    if (ks == XK_Down || ks == XK_s || ks == XK_S) return 40;
    if (ks == XK_Left || ks == XK_a || ks == XK_A) return 37;
    if (ks == XK_Right || ks == XK_d || ks == XK_D) return 39;
    
    // Modifier/action keys
    if (ks == XK_Return || ks == XK_KP_Enter) return 13;
    if (ks == XK_space || ks == XK_KP_Space) return 32;
    if (ks == XK_BackSpace) return 8;
    if (ks == XK_Delete || ks == XK_KP_Delete) return 127;
    if (ks == XK_Tab) return 9;
    if (ks == XK_Shift_L || ks == XK_Shift_R) return 16;
    if (ks == XK_Control_L || ks == XK_Control_R) return 17;
    
    // Alphanumeric keys (A-Z)
    if (ks >= XK_A && ks <= XK_Z) return 'A' + (ks - XK_A);
    if (ks >= XK_a && ks <= XK_z) return 'A' + (ks - XK_a);
    
    // Main keyboard numbers (0-9)
    if (ks >= XK_0 && ks <= XK_9) return '0' + (ks - XK_0);
    
    // Keypad numbers
    if (ks >= XK_KP_0 && ks <= XK_KP_9) return '0' + (ks - XK_KP_0);
    
    // Keypad operators
    if (ks == XK_KP_Add) return '+';
    if (ks == XK_KP_Subtract) return '-';
    if (ks == XK_KP_Multiply) return '*';
    if (ks == XK_KP_Divide) return '/';
    if (ks == XK_KP_Decimal) return '.';
    
    return 0;  // Unknown key
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
                     && gm->games[gm->current_game]->active
                     && gm->games[gm->current_game]->handle_key)
                gm->games[gm->current_game]->handle_key(gm->games[gm->current_game], k, 1);
        }
        else if (e.type == KeyRelease) {
            int k = x11_to_keycode(XLookupKeysym(&e.xkey, 0));
            if (k && gm->game_count > 0 && gm->current_game < gm->game_count 
                && gm->games[gm->current_game]->active
                && gm->games[gm->current_game]->handle_key)
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
                        && e.xmotion.x >= 10 && e.xmotion.x < SIDEBAR_WIDTH-25) {
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
    if (gm->ximage) { free(gm->ximage->data); gm->ximage->data = NULL; XDestroyImage(gm->ximage); }
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
        }
        else if (strcmp(t, "c") == 0) {
            gm_click(w->gm, x, y);
        }
        else if (strcmp(t, "kd") == 0 || strcmp(t, "ku") == 0) {
            int pr = (t[1] == 'd');
            if (k == 27) w->run = 0;
            else if (k >= 112 && k <= 115 && pr) gm_switch(w->gm, k - 112);
            else if (w->gm->game_count > 0 && w->gm->current_game < w->gm->game_count 
                     && w->gm->games[w->gm->current_game]->active
                     && w->gm->games[w->gm->current_game]->handle_key)
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

int main(int argc, char** argv) {
    GameManager gm;
    memset(&gm, 0, sizeof(gm));
    pthread_mutex_init(&gm.fb_mutex, NULL);
    gm.current_game = -1;
    gm.shm_fd = -1;
    
    int web = 0, clear_state = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--web") == 0) web = 1;
        else if (strcmp(argv[i], "--clear") == 0) clear_state = 1;
    }
    
    ensure_state_directory(&gm);
    
    if (!init_shared_state(&gm)) {
        printf("Running without shared memory sync\n");
    }
    
    if (clear_state) {
        clear_all_state(&gm);
        printf("State cleared.\n");
        cleanup_shared_state(&gm);
        return 0;
    }
    
    // Try X11 first, fallback to web
    int has_x11 = 0;
    if (!web) {
        has_x11 = x11_init(&gm);
        if (!has_x11) {
            printf("No display available - using web mode\n");
            web = 1;
        }
    }
    
    gm.web_mode = web;
    
    // Start web server if in web mode
    if (web) {
        ws.gm = &gm;
        pthread_create(&ws.th, NULL, wthread, &ws);
        usleep(500000); // Wait for server to start
    }
    
    printf("SPANE Engine - Instance #%d (%s)\n", gm.instance_id + 1, web ? "Web" : "X11");
    
    srand(time(NULL));
    gm.last_fps_update = time(NULL);
    
    // Load state
    if (!load_disk_backup(&gm)) {
        scan_games_directory(&gm, "./games");
    }
    if (gm.shared) sync_from_shared(&gm);
    
    if (gm.game_count > 0) {
        gm.current_game = 0;
        for (int i = 0; i < gm.game_count; i++) {
            if (gm.games[i]->init) gm.games[i]->init(gm.games[i]);
        }
        sync_to_shared(&gm);
    }
    
    int running = 1;
    int counter = 0;
    
    while (running) {
        if (has_x11) x11_process(&gm, &running);
        if (web && !ws.run) running = 0;
        
        // Update ALL games
        for (int i = 0; i < gm.game_count; i++) {
            if (gm.games[i]->active && gm.games[i]->update)
                gm.games[i]->update(gm.games[i]);
        }
        
        gm_render(&gm);
        if (has_x11) x11_mirror_frame(&gm);
        gm_fps(&gm);
        
        counter++;
        if (counter >= 10) {
            if (gm.shared) sync_from_shared(&gm);
            if (counter >= 300) {
                save_disk_backup(&gm);
                counter = 0;
            }
        }
        
        usleep(16667);
    }
    
    printf("Shutting down...\n");
    save_disk_backup(&gm);
    
    for (int i = 0; i < gm.game_count; i++) {
        if (gm.games[i]->active && gm.games[i]->cleanup)
            gm.games[i]->cleanup(gm.games[i]);
        if (gm.games[i]->handle) dlclose(gm.games[i]->handle);
    }
    if (has_x11) x11_cleanup(&gm);
    if (web) {
        ws.run = 0;
        pthread_join(ws.th, NULL);
    }
    cleanup_shared_state(&gm);
    pthread_mutex_destroy(&gm.fb_mutex);
    
    return 0;
}