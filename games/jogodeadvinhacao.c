// =============================================================================
// GUESSING GAME - SPANE Engine SDK (HEAVILY DEBUGGED VERSION - FIXED COMPILATION)
// =============================================================================
// Compile: gcc -shared -fPIC -O3 -o jogodeadivinhacao.so jogodeadivinhacao.c
// =============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// =============================================================================
// SDK Constants
// =============================================================================
#define MAIN_WINDOW_WIDTH  1000
#define MAIN_WINDOW_HEIGHT 700
#define GAME_AREA_WIDTH    800
#define GAME_AREA_HEIGHT   600
#define GAME_AREA_X        180
#define GAME_AREA_Y        50

// =============================================================================
// Game Constants
// =============================================================================
#define MIN_GUESS 1
#define MAX_GUESS 100
#define MAX_GUESSES 20
#define MAX_SCORES 100
#define HISTORY_FILE "guessing_history.txt"

// Keycodes do SPANE
#define KEY_UP      38
#define KEY_DOWN    40
#define KEY_LEFT    37
#define KEY_RIGHT   39
#define KEY_ENTER   13
#define KEY_SPACE   32
#define KEY_N       78
#define KEY_S       83
#define KEY_R       82
#define KEY_ESC     27
#define KEY_BACK    8
#define KEY_DEL     127

// Telas
#define SCREEN_MAIN_MENU    0
#define SCREEN_GAME         1
#define SCREEN_STATS        2
#define SCREEN_HISTORY      3

// Menu
#define MENU_PLAY      0
#define MENU_ANALYZE   1
#define MENU_HISTORY   2
#define MENU_COUNT     3

// =============================================================================
// SAFETY MACROS
// =============================================================================
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// =============================================================================
// Tipos
// =============================================================================
typedef struct Framebuffer {
    unsigned char pixels[MAIN_WINDOW_WIDTH * MAIN_WINDOW_HEIGHT * 4];
} Framebuffer;

typedef struct Game {
    char name[64];
    char path[512];
    void* data;
    void* handle;
    int active;
    void (*init)(struct Game* game);
    void (*handle_key)(struct Game* game, int key_code, int pressed);
    void (*handle_click)(struct Game* game, int x, int y);
    void (*update)(struct Game* game);
    void (*render)(struct Game* game, Framebuffer* fb);
    void (*cleanup)(struct Game* game);
} Game;

typedef struct {
    time_t timestamp;
    int target;
    int num_guesses;
    int guesses[MAX_GUESSES];
} Session;

typedef struct {
    int screen;
    int menu_selection;
    
    int target;
    int guesses[MAX_GUESSES];
    int num_guesses;
    int game_over;
    int game_active;
    
    char guess_str[8];
    int guess_len;
    
    int cursor_visible;
    int frame_count;
    
    char message[256];
    char hint_message[128];
    char status_message[128];
    
    int total_sessions;
    double avg_guesses;
    int best_session;
    int worst_session;
    double std_dev;
    
    int history_scroll;
    int hint_level;
} GuessingData;

static Session sessions[MAX_SCORES];
static int num_sessions = 0;

// =============================================================================
// Fonte 5x7
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
// Desenho - relative to game area (0,0 = top-left of GAME_AREA)
// =============================================================================
static inline int is_valid_game_coord(int x, int y) {
    return (x >= 0 && x < GAME_AREA_WIDTH && y >= 0 && y < GAME_AREA_HEIGHT);
}

static inline void fb_pixel_rel(Framebuffer* fb, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || !is_valid_game_coord(x, y)) return;
    
    int ax = GAME_AREA_X + x;
    int ay = GAME_AREA_Y + y;
    
    // Bounds check (belt and suspenders)
    if (ax < GAME_AREA_X || ax >= GAME_AREA_X + GAME_AREA_WIDTH || 
        ay < GAME_AREA_Y || ay >= GAME_AREA_Y + GAME_AREA_HEIGHT) return;
    if (ax < 0 || ax >= MAIN_WINDOW_WIDTH || ay < 0 || ay >= MAIN_WINDOW_HEIGHT) return;
    
    int i = (ay * MAIN_WINDOW_WIDTH + ax) * 4;
    fb->pixels[i] = r; 
    fb->pixels[i+1] = g; 
    fb->pixels[i+2] = b; 
    fb->pixels[i+3] = 255;
}

static void fb_fill_rel(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb) return;
    
    // Clamp to game area
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > GAME_AREA_WIDTH) w = GAME_AREA_WIDTH - x;
    if (y + h > GAME_AREA_HEIGHT) h = GAME_AREA_HEIGHT - y;
    if (w <= 0 || h <= 0) return;
    
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            fb_pixel_rel(fb, x + dx, y + dy, r, g, b);
        }
    }
}

static void fb_rect_rel(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || w <= 0 || h <= 0) return;
    
    // Top and bottom edges
    for (int dx = 0; dx < w; dx++) { 
        fb_pixel_rel(fb, x + dx, y, r, g, b); 
        if (h > 1) fb_pixel_rel(fb, x + dx, y + h - 1, r, g, b); 
    }
    // Left and right edges (only if h > 2 to avoid double-drawing corners)
    for (int dy = 1; dy < h - 1; dy++) { 
        fb_pixel_rel(fb, x, y + dy, r, g, b); 
        fb_pixel_rel(fb, x + w - 1, y + dy, r, g, b); 
    }
}

static void fb_char_rel(Framebuffer* fb, int x, int y, char c, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || c < 32 || c > 126) return;
    
    const unsigned char* glyph = font_5x7[c - 32];
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (glyph[col] & (1 << row)) {
                fb_pixel_rel(fb, x + col, y + row, r, g, b);
            }
        }
    }
}

static void fb_text_rel(Framebuffer* fb, int x, int y, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || !s) return;
    for (int i = 0; s[i] && i < 200; i++) {  // Safety limit on string length
        fb_char_rel(fb, x + i * 6, y, s[i], r, g, b);
    }
}

// Helper: center text at given x coordinate (x IS CENTER of text)
static void fb_text_center_at(Framebuffer* fb, int cx, int y, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    if (!fb || !s) return;
    int len = (int)strlen(s);
    if (len > 200) len = 200;  // Safety limit
    fb_text_rel(fb, cx - (len * 6) / 2, y, s, r, g, b);
}

// =============================================================================
// sqrt customizada
// =============================================================================
static double my_sqrt(double x) {
    if (x <= 0.0) return 0.0;
    double guess = x / 2.0;
    for (int i = 0; i < 20; i++) {
        double ng = (guess + x / guess) / 2.0;
        if (ng == guess) break;
        guess = ng;
    }
    return guess;
}

// =============================================================================
// Histórico e Arquivo
// =============================================================================
static void save_session(GuessingData* d) {
    if (!d) return;
    
    FILE* f = fopen(HISTORY_FILE, "a");
    if (!f) return;
    
    time_t now = time(NULL);
    fprintf(f, "%ld %d %d", (long)now, d->target, d->num_guesses);
    for (int i = 0; i < d->num_guesses && i < MAX_GUESSES; i++) {
        fprintf(f, " %d", d->guesses[i]);
    }
    fprintf(f, "\n");
    fclose(f);
}

static void load_history(GuessingData* d) {
    num_sessions = 0;
    if (d) { 
        d->history_scroll = 0; 
        d->total_sessions = 0; 
    }
    
    FILE* f = fopen(HISTORY_FILE, "r");
    if (!f) return;
    
    char line[2048];  // Larger buffer for safety
    
    while (fgets(line, sizeof(line), f) && num_sessions < MAX_SCORES) {
        Session* s = &sessions[num_sessions];
        memset(s, 0, sizeof(Session));  // Zero initialize
        
        char* token = strtok(line, " \n");
        if (!token) continue;
        s->timestamp = (time_t)atol(token);
        
        token = strtok(NULL, " \n");
        if (!token) continue;
        s->target = atoi(token);
        
        token = strtok(NULL, " \n");
        if (!token) continue;
        s->num_guesses = atoi(token);
        
        // Clamp num_guesses to valid range
        if (s->num_guesses > MAX_GUESSES) s->num_guesses = MAX_GUESSES;
        if (s->num_guesses < 0) s->num_guesses = 0;
        
        int i = 0;
        while ((token = strtok(NULL, " \n")) != NULL && i < MAX_GUESSES) {
            s->guesses[i] = atoi(token);
            i++;
        }
        
        // Only count sessions with at least 1 guess
        if (s->num_guesses > 0) {
            num_sessions++;
        }
    }
    fclose(f);
}

// =============================================================================
// Recursão
// =============================================================================
static int rec_sum(int* arr, int n) { 
    if (!arr || n <= 0) return 0;
    return arr[0] + rec_sum(arr + 1, n - 1); 
}

static int rec_min(int* arr, int n) { 
    if (!arr || n <= 0) return 999999;
    if (n == 1) return arr[0];
    int m = rec_min(arr + 1, n - 1);
    return arr[0] < m ? arr[0] : m;
}

static int rec_max(int* arr, int n) {
    if (!arr || n <= 0) return -1;
    if (n == 1) return arr[0];
    int m = rec_max(arr + 1, n - 1);
    return arr[0] > m ? arr[0] : m;
}

static double rec_sum_sq(int* arr, int n, double mean) {
    if (!arr || n <= 0) return 0.0;
    double d = arr[0] - mean;
    return d * d + rec_sum_sq(arr + 1, n - 1, mean);
}

// =============================================================================
// Estatísticas
// =============================================================================
static void calc_stats(GuessingData* d) {
    if (!d) return;
    
    if (num_sessions == 0) {
        d->total_sessions = 0;
        d->avg_guesses = 0;
        d->best_session = 0;
        d->worst_session = 0;
        d->std_dev = 0;
        snprintf(d->status_message, sizeof(d->status_message), "Nenhuma partida registrada ainda!");
        return;
    }
    
    int gc[MAX_SCORES];
    int count = (num_sessions > MAX_SCORES) ? MAX_SCORES : num_sessions;
    
    for (int i = 0; i < count; i++) {
        gc[i] = sessions[i].num_guesses;
    }
    
    d->total_sessions = count;
    int sum = rec_sum(gc, count);
    d->avg_guesses = (double)sum / count;
    d->best_session = rec_min(gc, count);
    d->worst_session = rec_max(gc, count);
    double var = rec_sum_sq(gc, count, d->avg_guesses) / count;
    d->std_dev = my_sqrt(var);
    
    snprintf(d->status_message, sizeof(d->status_message), 
             "%d sessoes | Media: %.1f | Melhor: %d | Pior: %d", 
             count, d->avg_guesses, d->best_session, d->worst_session);
}

static const char* get_strategy(GuessingData* d) {
    if (!d || num_sessions == 0) return "Sem dados - use busca binaria!";
    if (d->avg_guesses < 6) return "Excelente! Busca binaria perfeita!";
    if (d->avg_guesses < 10) return "Bom! Tente melhorar a estrategia.";
    return "Use busca binaria: comece em 50";
}

// =============================================================================
// Dica progressiva
// =============================================================================
static void gen_hint(GuessingData* d) {
    if (!d) return;
    
    if (d->num_guesses == 0) {
        snprintf(d->hint_message, sizeof(d->hint_message), "Digite um numero entre %d e %d", MIN_GUESS, MAX_GUESS);
        return;
    }
    
    int lo = MIN_GUESS, hi = MAX_GUESS;
    for (int i = 0; i < d->num_guesses && i < MAX_GUESSES; i++) {
        if (d->guesses[i] < d->target && d->guesses[i] >= lo) lo = d->guesses[i] + 1;
        if (d->guesses[i] > d->target && d->guesses[i] <= hi) hi = d->guesses[i] - 1;
    }
    
    // Sanity check
    if (lo > hi) { lo = MIN_GUESS; hi = MAX_GUESS; }
    
    int mid = (lo + hi) / 2;
    int range = hi - lo + 1;
    
    if (d->num_guesses >= 8)
        snprintf(d->hint_message, sizeof(d->hint_message), "ULTIMA DICA: Esta entre %d e %d! Tente %d!", lo, hi, mid);
    else if (d->num_guesses >= 5)
        snprintf(d->hint_message, sizeof(d->hint_message), "Intervalo: [%d, %d] (%d numeros)", lo, hi, range);
    else if (d->num_guesses >= 3) {
        if (range <= 20)
            snprintf(d->hint_message, sizeof(d->hint_message), "Esta perto! Entre %d e %d", lo, hi);
        else
            snprintf(d->hint_message, sizeof(d->hint_message), "Sugestao: tente %d", mid);
    } else {
        snprintf(d->hint_message, sizeof(d->hint_message), "O numero esta entre %d e %d", lo, hi);
    }
}

// =============================================================================
// Jogo - CORE GAME LOGIC (HEAVILY DEBUGGED)
// =============================================================================
static void new_game(GuessingData* d) {
    if (!d) return;
    
    // Reset ALL game state
    d->game_active = 1;
    d->target = (rand() % MAX_GUESS) + 1;
    d->num_guesses = 0;
    d->game_over = 0;
    d->guess_str[0] = 0;
    d->guess_len = 0;
    
    // Clear guesses array
    memset(d->guesses, 0, sizeof(d->guesses));
    
    snprintf(d->message, sizeof(d->message), "Nova partida! Adivinhe o numero entre %d e %d", MIN_GUESS, MAX_GUESS);
    snprintf(d->status_message, sizeof(d->status_message), "Digite um numero e pressione ENTER (max %d tentativas)", MAX_GUESSES);
    gen_hint(d);
}

static int get_current_guess(GuessingData* d) {
    if (!d || d->guess_len == 0) return 0;
    return atoi(d->guess_str);
}

static void submit_guess(GuessingData* d) {
    if (!d) return;
    
    // CRITICAL FIX: Check if game is over first
    if (d->game_over) return;
    
    // CRITICAL FIX: Check maximum guesses limit BEFORE processing
    if (d->num_guesses >= MAX_GUESSES) {
        d->game_over = 1;
        d->game_active = 0;
        snprintf(d->message, sizeof(d->message), "FIM DE JOGO! Voce atingiu o limite de %d tentativas. O numero era %d.", MAX_GUESSES, d->target);
        save_session(d);
        load_history(d);
        calc_stats(d);
        snprintf(d->status_message, sizeof(d->status_message), "Pressione ENTER para novo jogo");
        return;
    }
    
    int guess = get_current_guess(d);
    if (guess == 0 && d->guess_len == 0) {
        snprintf(d->message, sizeof(d->message), "Digite um numero primeiro!");
        return;
    }
    
    // Validate guess range
    if (guess < MIN_GUESS || guess > MAX_GUESS) {
        snprintf(d->message, sizeof(d->message), "Numero deve estar entre %d e %d!", MIN_GUESS, MAX_GUESS);
        d->guess_str[0] = 0;
        d->guess_len = 0;
        return;
    }
    
    // CRITICAL FIX: Store guess safely with bounds checking
    if (d->num_guesses < MAX_GUESSES) {
        d->guesses[d->num_guesses] = guess;
        d->num_guesses++;
    } else {
        // Safety net - should never reach here due to check above
        d->game_over = 1;
        d->game_active = 0;
        return;
    }
    
    // Process guess result
    if (guess < d->target) {
        int diff = d->target - guess;
        if (diff > 30) 
            snprintf(d->message, sizeof(d->message), "MUITO BAIXO! Tente um numero bem maior.");
        else if (diff > 10) 
            snprintf(d->message, sizeof(d->message), "BAIXO! Tente um numero maior.");
        else 
            snprintf(d->message, sizeof(d->message), "Um pouco baixo... Esta perto!");
        
        snprintf(d->status_message, sizeof(d->status_message), "Tentativa #%d de %d", d->num_guesses, MAX_GUESSES);
        
    } else if (guess > d->target) {
        int diff = guess - d->target;
        if (diff > 30) 
            snprintf(d->message, sizeof(d->message), "MUITO ALTO! Tente um numero bem menor.");
        else if (diff > 10) 
            snprintf(d->message, sizeof(d->message), "ALTO! Tente um numero menor.");
        else 
            snprintf(d->message, sizeof(d->message), "Um pouco alto... Esta perto!");
        
        snprintf(d->status_message, sizeof(d->status_message), "Tentativa #%d de %d", d->num_guesses, MAX_GUESSES);
        
    } else {
        // CORRECT GUESS
        if (d->num_guesses == 1)
            snprintf(d->message, sizeof(d->message), "INCRIVEL! Acertou de primeira! O numero e %d!", d->target);
        else if (d->num_guesses <= 5)
            snprintf(d->message, sizeof(d->message), "PARABENS! %d tentativas! O numero e %d.", d->num_guesses, d->target);
        else
            snprintf(d->message, sizeof(d->message), "ACERTOU! %d tentativas. O numero era %d.", d->num_guesses, d->target);
        
        d->game_over = 1;
        d->game_active = 0;
        save_session(d);
        load_history(d);
        calc_stats(d);
        snprintf(d->status_message, sizeof(d->status_message), "Sessao salva! Pressione ENTER para novo jogo");
        return;
    }
    
    // Check if this was the last attempt
    if (d->num_guesses >= MAX_GUESSES) {
        d->game_over = 1;
        d->game_active = 0;
        snprintf(d->message, sizeof(d->message), "FIM DE JOGO! Voce usou todas as %d tentativas. O numero era %d.", MAX_GUESSES, d->target);
        save_session(d);
        load_history(d);
        calc_stats(d);
        snprintf(d->status_message, sizeof(d->status_message), "Pressione ENTER para novo jogo");
    } else {
        gen_hint(d);
    }
    
    // Clear input
    d->guess_str[0] = 0;
    d->guess_len = 0;
}

// =============================================================================
// Input Handler - COMPLETELY REWRITTEN FOR CORRECTNESS
// =============================================================================
static void guessing_handle_key(Game* game, int keycode, int pressed) {
    if (!game || !game->data || !pressed) return;
    GuessingData* d = (GuessingData*)game->data;
    
    // ===== GLOBAL SHORTCUTS =====
    if (keycode == KEY_S || keycode == 's' || keycode == 'S') {
        load_history(d);
        calc_stats(d);
        d->screen = SCREEN_STATS;
        d->history_scroll = 0;
        return;
    }
    
    if (keycode == KEY_N || keycode == 'n' || keycode == 'N') {
        new_game(d);
        d->screen = SCREEN_GAME;
        return;
    }
    
    // ===== ESC =====
    if (keycode == KEY_ESC) {
        if (d->screen == SCREEN_GAME) {
            d->screen = SCREEN_MAIN_MENU;
            if (d->game_active && !d->game_over) {
                d->game_active = 0;
                snprintf(d->status_message, sizeof(d->status_message), "Jogo pausado. Va em JOGAR para continuar.");
            }
        } else if (d->screen == SCREEN_STATS || d->screen == SCREEN_HISTORY) {
            d->screen = SCREEN_MAIN_MENU;
        }
        return;
    }
    
    // ===== MENU PRINCIPAL =====
    if (d->screen == SCREEN_MAIN_MENU) {
        switch (keycode) {
            case KEY_UP:
                d->menu_selection--;
                if (d->menu_selection < 0) d->menu_selection = MENU_COUNT - 1;
                break;
                
            case KEY_DOWN:
                d->menu_selection++;
                if (d->menu_selection >= MENU_COUNT) d->menu_selection = 0;
                break;
                
            case KEY_ENTER:
            case KEY_SPACE:
                switch (d->menu_selection) {
                    case MENU_PLAY:
                        new_game(d);
                        d->screen = SCREEN_GAME;
                        break;
                    case MENU_ANALYZE:
                        load_history(d);
                        calc_stats(d);
                        d->screen = SCREEN_STATS;
                        d->history_scroll = 0;
                        break;
                    case MENU_HISTORY:
                        load_history(d);
                        calc_stats(d);
                        d->screen = SCREEN_HISTORY;
                        d->history_scroll = 0;
                        break;
                }
                break;
        }
        return;
    }
    
    // ===== TELAS DE ESTATÍSTICA/HISTÓRICO =====
    if (d->screen == SCREEN_STATS || d->screen == SCREEN_HISTORY) {
        if (keycode == KEY_UP || keycode == KEY_LEFT) { 
            if (d->history_scroll > 0) d->history_scroll--; 
        }
        if (keycode == KEY_DOWN || keycode == KEY_RIGHT) { 
            d->history_scroll++; 
        }
        return;
    }
    
    // ===== TELA DO JOGO =====
    if (d->screen == SCREEN_GAME) {
        // Handle game over state
        if (d->game_over) {
            if (keycode == KEY_ENTER || keycode == KEY_SPACE || keycode == KEY_N || keycode == 'n' || keycode == 'N') {
                new_game(d);
            }
            return;
        }
        
        // Only process input if game is active
        if (!d->game_active) return;
        
        // DIGIT INPUT - COMPLETELY FIXED LOGIC
        if (keycode >= '0' && keycode <= '9') {
            // Convert to digit
            int digit = keycode - '0';
            
            // Safety: never exceed buffer size
            if (d->guess_len >= (int)(sizeof(d->guess_str) - 2)) {
                return;  // Buffer full, ignore
            }
            
            // Append digit
            d->guess_str[d->guess_len] = (char)('0' + digit);
            d->guess_len++;
            d->guess_str[d->guess_len] = '\0';
            
            // Check if value is valid
            int val = atoi(d->guess_str);
            
            // If value exceeds MAX_GUESS, we need to handle it properly
            if (val > MAX_GUESS) {
                // Try clipping to last valid digit
                d->guess_str[d->guess_len - 1] = '\0';
                d->guess_len--;
                d->guess_str[d->guess_len] = '\0';
                
                // If we removed all digits, add just the new one
                if (d->guess_len == 0) {
                    d->guess_str[0] = (char)('0' + digit);
                    d->guess_len = 1;
                    d->guess_str[1] = '\0';
                }
            }
            
            // Check leading zero
            if (d->guess_len > 1 && d->guess_str[0] == '0') {
                // Remove leading zero
                d->guess_str[0] = d->guess_str[1];
                d->guess_str[1] = '\0';
                d->guess_len = 1;
            }
        }
        
        // SPECIAL KEYS - FIXED: Removed duplicate case for 'R'
        switch (keycode) {
            case KEY_BACK:
            case KEY_DEL:
                if (d->guess_len > 0) {
                    d->guess_len--;
                    d->guess_str[d->guess_len] = '\0';
                }
                break;
                
            case KEY_UP:
                {
                    int g = get_current_guess(d);
                    if (g == 0 && d->guess_len == 0) g = 50;  // Default to 50
                    g++;
                    if (g > MAX_GUESS) g = MAX_GUESS;
                    snprintf(d->guess_str, sizeof(d->guess_str), "%d", g);
                    d->guess_len = (int)strlen(d->guess_str);
                }
                break;
                
            case KEY_DOWN:
                {
                    int g = get_current_guess(d);
                    if (g == 0 && d->guess_len == 0) g = 50;  // Default to 50
                    g--;
                    if (g < MIN_GUESS) g = MIN_GUESS;
                    snprintf(d->guess_str, sizeof(d->guess_str), "%d", g);
                    d->guess_len = (int)strlen(d->guess_str);
                }
                break;
                
            case KEY_R:  // FIXED: Only one case for reset
                d->guess_str[0] = '\0';
                d->guess_len = 0;
                break;
                
            case KEY_ENTER:
            case KEY_SPACE:
                submit_guess(d);
                break;
        }
    }
}

// =============================================================================
// Click handler
// =============================================================================
static void guessing_handle_click(Game* game, int x, int y) {
    if (!game || !game->data) return;
    GuessingData* d = (GuessingData*)game->data;
    
    // Convert to game-area-relative coordinates
    int gx = x - GAME_AREA_X;
    int gy = y - GAME_AREA_Y;
    
    // Simple screen handling
    if (d->screen == SCREEN_MAIN_MENU) {
        int menu_start_y = 195;
        int menu_w = 440;
        int menu_h = 50;
        int menu_spacing = 75;
        int menu_x = (GAME_AREA_WIDTH - menu_w) / 2;
        
        for (int i = 0; i < MENU_COUNT; i++) {
            int my = menu_start_y + i * menu_spacing;
            if (gx >= menu_x && gx <= menu_x + menu_w && gy >= my && gy <= my + menu_h) {
                d->menu_selection = i;
                if (i == MENU_PLAY) { new_game(d); d->screen = SCREEN_GAME; }
                else if (i == MENU_ANALYZE) { load_history(d); calc_stats(d); d->screen = SCREEN_STATS; }
                else if (i == MENU_HISTORY) { load_history(d); calc_stats(d); d->screen = SCREEN_HISTORY; }
                return;
            }
        }
    } else if (d->screen == SCREEN_GAME && d->game_over) {
        new_game(d);
    } else if (d->screen == SCREEN_STATS || d->screen == SCREEN_HISTORY) {
        d->screen = SCREEN_MAIN_MENU;
    }
}

// =============================================================================
// Update
// =============================================================================
static void guessing_update(Game* game) {
    if (!game || !game->data) return;
    GuessingData* d = (GuessingData*)game->data;
    d->frame_count++;
    if (d->frame_count % 30 == 0) d->cursor_visible = !d->cursor_visible;
}

// =============================================================================
// LAYOUT CONSTANTS
// =============================================================================
#define MARGIN_X 40
#define MARGIN_TOP 15
#define PANEL_SPACING 20

// =============================================================================
// Render - Menu
// =============================================================================
static void render_menu(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    int cx = GAME_AREA_WIDTH / 2;
    int cy = 80;
    
    // Title
    fb_text_center_at(fb, cx, cy, "JOGO DE ADIVINHACAO", 0xFF, 0xCC, 0x00);
    fb_text_center_at(fb, cx, cy + 35, "Use CIMA/BAIXO e ENTER para selecionar", 0x88, 0x88, 0x88);
    
    const char* items[] = { "[ JOGAR ]", "[ ANALISAR ESTATISTICAS ]", "[ VER HISTORICO ]" };
    const char* descs[] = { "Iniciar nova partida", "Ver estatisticas de desempenho", "Visualizar partidas anteriores" };
    
    int menu_w = 440;
    int menu_h = 50;
    int menu_start_y = cy + 85;
    int menu_spacing = 75;
    int menu_x = cx - menu_w / 2;
    
    for (int i = 0; i < MENU_COUNT; i++) {
        int my = menu_start_y + i * menu_spacing;
        int sel = (i == d->menu_selection);
        
        fb_fill_rel(fb, menu_x, my, menu_w, menu_h, sel ? 0x00 : 0x22, sel ? 0x66 : 0x22, sel ? 0xAA : 0x33);
        if (sel) fb_rect_rel(fb, menu_x, my, menu_w, menu_h, 0x00, 0xAA, 0xFF);
        
        fb_text_center_at(fb, cx, my + 10, items[i], sel ? 0xFF : 0xAA, sel ? 0xFF : 0xAA, sel ? 0x00 : 0xAA);
        fb_text_center_at(fb, cx, my + 30, descs[i], sel ? 0xAA : 0x66, sel ? 0xCC : 0x66, sel ? 0xFF : 0x66);
        
        if (sel && d->cursor_visible) fb_text_rel(fb, menu_x - 30, my + 18, ">>", 0xFF, 0xFF, 0x00);
    }
    
    // Bottom info
    fb_text_center_at(fb, cx, GAME_AREA_HEIGHT - 40, "ESC: Sair  |  S: Estatisticas  |  N: Novo Jogo", 0x66, 0x66, 0x66);
    
    if (num_sessions > 0) {
        char info[128];
        snprintf(info, sizeof(info), "%d partidas | Melhor: %d tentativa(s)", num_sessions, d->best_session);
        fb_text_center_at(fb, cx, GAME_AREA_HEIGHT - 20, info, 0x55, 0x55, 0x55);
    }
}

// =============================================================================
// Render - Jogo
// =============================================================================
static void render_game(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    
    // Header
    fb_text_rel(fb, MARGIN_X, MARGIN_TOP, "JOGO DE ADIVINHACAO", 0xFF, 0xCC, 0x00);
    
    char header_info[64];
    snprintf(header_info, sizeof(header_info), "Tentativa %d de %d", d->num_guesses + 1, MAX_GUESSES);
    fb_text_rel(fb, GAME_AREA_WIDTH - 280, MARGIN_TOP, header_info, 0x88, 0x88, 0x88);
    
    // ---- LEFT COLUMN ----
    int lx = MARGIN_X;
    int ly = MARGIN_TOP + 30;
    
    // "Seu Palpite" label
    fb_text_rel(fb, lx, ly, "SEU PALPITE:", 0xCC, 0xCC, 0xCC);
    
    // Guess input box
    int box_w = 240;
    int box_h = 50;
    fb_fill_rel(fb, lx, ly + 22, box_w, box_h, 0x11, 0x11, 0x22);
    
    unsigned char border_r = d->game_over ? 0x44 : 0x00;
    unsigned char border_g = d->game_over ? 0x44 : 0x88;
    unsigned char border_b = d->game_over ? 0x44 : 0xFF;
    fb_rect_rel(fb, lx, ly + 22, box_w, box_h, border_r, border_g, border_b);
    
    if (d->game_active && !d->game_over) {
        char display[32];
        if (d->guess_len == 0) {
            snprintf(display, sizeof(display), "_");
        } else {
            snprintf(display, sizeof(display), "%s", d->guess_str);
        }
        fb_text_rel(fb, lx + 20, ly + 38, display, 0xFF, 0xFF, 0xFF);
        
        if (d->cursor_visible) {
            int cxr = lx + 20 + (d->guess_len * 6);
            fb_fill_rel(fb, cxr, ly + 45, 6, 2, 0xFF, 0xFF, 0x00);
        }
    } else if (d->game_over) {
        fb_text_rel(fb, lx + 20, ly + 38, "FIM", 0x88, 0x88, 0x88);
    }
    
    // Instructions
    int inst_y = ly + 25;
    fb_text_rel(fb, lx + box_w + 20, inst_y, "Digite o numero:", 0x88, 0x88, 0x88);
    fb_text_rel(fb, lx + box_w + 20, inst_y + 18, "0-9 + ENTER", 0x66, 0x88, 0x66);
    fb_text_rel(fb, lx + box_w + 20, inst_y + 36, "CIMA/BAIXO: +1/-1", 0x55, 0x55, 0x66);
    fb_text_rel(fb, lx + box_w + 20, inst_y + 54, "BACK: Apagar  R: Reset", 0x55, 0x55, 0x66);
    
    // Message area
    int msg_y = ly + 90;
    unsigned char mr = 0xFF, mg = 0xFF, mb = 0xFF;
    if (strstr(d->message, "BAIXO") || strstr(d->message, "baixo")) { mr = 0xFF; mg = 0x66; mb = 0x66; }
    else if (strstr(d->message, "ALTO") || strstr(d->message, "alto")) { mr = 0xFF; mg = 0x66; mb = 0x66; }
    else if (strstr(d->message, "ACERTOU") || strstr(d->message, "PARABENS") || strstr(d->message, "INCRIVEL") || strstr(d->message, "FIM")) { mr = 0x00; mg = 0xFF; mb = 0x00; }
    fb_text_rel(fb, lx, msg_y, d->message, mr, mg, mb);
    
    // Hint
    if (d->game_active && !d->game_over) {
        fb_text_rel(fb, lx, msg_y + 25, d->hint_message, 0x00, 0xCC, 0xCC);
    }
    
    // Status
    fb_text_rel(fb, lx, msg_y + 50, d->status_message, 0x88, 0xAA, 0x88);
    
    // Remaining guesses indicator
    int remaining = MAX_GUESSES - d->num_guesses;
    char rem_msg[64];
    snprintf(rem_msg, sizeof(rem_msg), "Restam %d tentativas", remaining);
    fb_text_rel(fb, lx, msg_y + 72, rem_msg, 0xAA, 0xAA, 0x66);
    
    // ---- RIGHT COLUMN: HISTORY PANEL ----
    int rx = 490;
    int ry = MARGIN_TOP + 10;
    int rw = 270;
    int rh = GAME_AREA_HEIGHT - MARGIN_TOP - 100;
    
    fb_fill_rel(fb, rx, ry, rw, rh, 0x15, 0x15, 0x20);
    fb_rect_rel(fb, rx, ry, rw, rh, 0x33, 0x33, 0x44);
    fb_text_rel(fb, rx + 10, ry + 10, "HISTORICO:", 0xAA, 0xAA, 0xAA);
    
    int max_display = (rh - 40) / 22;
    if (max_display > MAX_GUESSES) max_display = MAX_GUESSES;
    
    for (int i = 0; i < d->num_guesses && i < max_display; i++) {
        char gs[32];
        const char* ind;
        if (d->guesses[i] < d->target) ind = "<<";
        else if (d->guesses[i] > d->target) ind = ">>";
        else ind = "**";
        
        snprintf(gs, sizeof(gs), "#%d: %d %s", i + 1, d->guesses[i], ind);
        
        unsigned char r, g, b;
        if (d->guesses[i] == d->target) {
            r = 0x00; g = 0xFF; b = 0x00;
        } else if (d->guesses[i] < d->target) {
            r = 0xFF; g = 0x88; b = 0x88;
        } else {
            r = 0xFF; g = 0x88; b = 0x88;
        }
        
        fb_text_rel(fb, rx + 10, ry + 35 + i * 22, gs, r, g, b);
    }
    
    // ---- GAME OVER BAR ----
    if (d->game_over) {
        int bar_y = GAME_AREA_HEIGHT - 70;
        int bar_w = GAME_AREA_WIDTH - MARGIN_X * 2;
        fb_fill_rel(fb, MARGIN_X, bar_y, bar_w, 50, 0x11, 0x33, 0x11);
        fb_rect_rel(fb, MARGIN_X, bar_y, bar_w, 50, 0x00, 0xFF, 0x00);
        fb_text_center_at(fb, GAME_AREA_WIDTH / 2, bar_y + 18, "ENTER: Novo Jogo  |  S: Estatisticas  |  ESC: Menu", 0x00, 0xFF, 0x00);
    }
}

// =============================================================================
// Render - Estatísticas - FIXED: Empty format string
// =============================================================================
static void render_stats(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    int cx = GAME_AREA_WIDTH / 2;
    int sy = MARGIN_TOP + 5;
    
    fb_text_center_at(fb, cx, sy, "ESTATISTICAS", 0xFF, 0xCC, 0x00);
    fb_text_rel(fb, GAME_AREA_WIDTH - 300, sy, "ESC:Menu  N:Jogar  SETAS:Scroll", 0x66, 0x66, 0x66);
    
    if (num_sessions == 0) {
        fb_text_center_at(fb, cx, 200, "Nenhuma partida registrada!", 0xFF, 0x88, 0x88);
        fb_text_center_at(fb, cx, 230, "Pressione N para jogar ou ESC para o menu", 0x88, 0x88, 0x88);
        return;
    }
    
    int panel_w = 380;
    int panel_h = 190;
    int panel_y = sy + 30;
    int left_x = cx - panel_w - 15;
    int right_x = cx + 15;
    
    // LEFT PANEL: Stats
    fb_fill_rel(fb, left_x, panel_y, panel_w, panel_h, 0x15, 0x15, 0x25);
    fb_rect_rel(fb, left_x, panel_y, panel_w, panel_h, 0x33, 0x55, 0x77);
    
    char lines[7][128];
    snprintf(lines[0], sizeof(lines[0]), "Total de sessoes:      %d", d->total_sessions);
    snprintf(lines[1], sizeof(lines[1]), "Media de tentativas:   %.1f", d->avg_guesses);
    snprintf(lines[2], sizeof(lines[2]), "Melhor sessao:         %d tentativa(s)", d->best_session);
    snprintf(lines[3], sizeof(lines[3]), "Pior sessao:           %d tentativa(s)", d->worst_session);
    snprintf(lines[4], sizeof(lines[4]), "Desvio padrao:         %.2f", d->std_dev);
    // FIXED: Empty format string replaced with space
    snprintf(lines[5], sizeof(lines[5]), " ");
    snprintf(lines[6], sizeof(lines[6]), "Estrategia: %s", get_strategy(d));
    
    for (int i = 0; i < 7; i++) {
        unsigned char r = 0xFF, g = 0xFF, b = 0xFF;
        if (i == 6) { r = 0x00; g = 0xCC; b = 0xCC; }
        fb_text_rel(fb, left_x + 20, panel_y + 20 + i * 22, lines[i], r, g, b);
    }
    
    // RIGHT PANEL: Rating
    fb_fill_rel(fb, right_x, panel_y, panel_w, panel_h, 0x15, 0x15, 0x25);
    fb_rect_rel(fb, right_x, panel_y, panel_w, panel_h, 0x33, 0x55, 0x77);
    fb_text_center_at(fb, right_x + panel_w / 2, panel_y + 20, "AVALIACAO", 0xFF, 0xCC, 0x00);
    
    const char* stars;
    if (d->avg_guesses < 6) stars = "***** EXPERT";
    else if (d->avg_guesses < 8) stars = "**** AVANCADO";
    else if (d->avg_guesses < 10) stars = "*** MEDIO";
    else if (d->avg_guesses < 15) stars = "** BASICO";
    else stars = "* INICIANTE";
    fb_text_center_at(fb, right_x + panel_w / 2, panel_y + 60, stars, 0xFF, 0xFF, 0x00);
    fb_text_center_at(fb, right_x + panel_w / 2, panel_y + 100, get_strategy(d), 0xAA, 0xCC, 0xAA);
    
    // BOTTOM: Recent sessions
    int hist_y = panel_y + panel_h + 20;
    fb_text_rel(fb, MARGIN_X, hist_y, "ULTIMAS PARTIDAS:", 0xAA, 0xAA, 0xAA);
    
    int start = d->history_scroll;
    int max_display = 10;
    if (start > num_sessions - max_display) start = num_sessions - max_display;
    if (start < 0) start = 0;
    
    for (int i = 0; i < max_display && (start + i) < num_sessions; i++) {
        Session* s = &sessions[start + i];
        char line[128];
        char ts[20];
        struct tm* tm_info = localtime(&s->timestamp);
        strftime(ts, sizeof(ts), "%d/%m %H:%M", tm_info);
        snprintf(line, sizeof(line), "%s  Alvo:%d  Tentativas:%d", ts, s->target, s->num_guesses);
        
        unsigned char r = (s->num_guesses == d->best_session) ? 0x00 : 0xAA;
        unsigned char g = (s->num_guesses == d->best_session) ? 0xFF : 0xAA;
        unsigned char b = (s->num_guesses == d->worst_session) ? 0x66 : 0xAA;
        fb_text_rel(fb, MARGIN_X + 10, hist_y + 25 + i * 22, line, r, g, b);
    }
}

// =============================================================================
// Render - Histórico
// =============================================================================
static void render_history(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    int cx = GAME_AREA_WIDTH / 2;
    int sy = MARGIN_TOP + 5;
    
    fb_text_center_at(fb, cx, sy, "HISTORICO COMPLETO", 0xFF, 0xCC, 0x00);
    fb_text_rel(fb, GAME_AREA_WIDTH - 250, sy, "ESC:Menu  SETAS:Scroll", 0x66, 0x66, 0x66);
    
    if (num_sessions == 0) {
        fb_text_center_at(fb, cx, 250, "Nenhum historico disponivel", 0xFF, 0x88, 0x88);
        return;
    }
    
    // Table header
    int table_x = MARGIN_X + 10;
    int table_y = sy + 35;
    int table_w = GAME_AREA_WIDTH - MARGIN_X * 2 - 20;
    
    fb_fill_rel(fb, table_x, table_y - 5, table_w, 25, 0x20, 0x20, 0x30);
    fb_text_rel(fb, table_x + 5, table_y, "DATA/HORA            ALVO   TENTATIVAS   PALPITES", 0xCC, 0xCC, 0xCC);
    
    int start = d->history_scroll;
    int max_display = 16;
    if (start > num_sessions - max_display) start = num_sessions - max_display;
    if (start < 0) start = 0;
    
    for (int i = 0; i < max_display && (start + i) < num_sessions; i++) {
        Session* s = &sessions[start + i];
        char line[512], ts[20];
        struct tm* tm_info = localtime(&s->timestamp);
        strftime(ts, sizeof(ts), "%d/%m/%Y %H:%M", tm_info);
        
        char gs[128] = "";
        int ms = s->num_guesses < 6 ? s->num_guesses : 6;
        for (int j = 0; j < ms; j++) {
            char num[8];
            snprintf(num, sizeof(num), "%d%s", s->guesses[j], j < ms - 1 ? "," : "");
            strncat(gs, num, sizeof(gs) - strlen(gs) - 1);
        }
        if (s->num_guesses > 6) strncat(gs, "...", sizeof(gs) - strlen(gs) - 1);
        
        snprintf(line, sizeof(line), "%-20s %-6d %-12d %s", ts, s->target, s->num_guesses, gs);
        
        unsigned char r = 0xAA, g = 0xAA, b = 0xAA;
        if (d->total_sessions > 1) {
            if (s->num_guesses == d->best_session) { r = 0x00; g = 0xFF; b = 0x00; }
            else if (s->num_guesses == d->worst_session) { r = 0xFF; g = 0x66; b = 0x66; }
        }
        
        // Alternating row colors
        if ((start + i) % 2 == 0) fb_fill_rel(fb, table_x, table_y + 25 + i * 22, table_w, 22, 0x12, 0x12, 0x18);
        fb_text_rel(fb, table_x + 5, table_y + 28 + i * 22, line, r, g, b);
    }
    
    // Scroll indicators
    if (start > 0) fb_text_center_at(fb, cx, table_y - 15, "[ Mais acima ]", 0xFF, 0xCC, 0x00);
    if (start + max_display < num_sessions) fb_text_center_at(fb, cx, table_y + 28 + max_display * 22, "[ Mais abaixo ]", 0xFF, 0xCC, 0x00);
    
    // Legend
    fb_text_center_at(fb, cx, GAME_AREA_HEIGHT - 30, "Verde = Melhor  |  Vermelho = Pior", 0x88, 0x88, 0x88);
}

// =============================================================================
// Render principal
// =============================================================================
static void guessing_render(Game* game, Framebuffer* fb) {
    if (!game || !game->data || !fb) return;
    GuessingData* d = (GuessingData*)game->data;
    
    // Fill entire game area with background
    fb_fill_rel(fb, 0, 0, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, 0x11, 0x11, 0x1A);
    
    switch (d->screen) {
        case SCREEN_MAIN_MENU: render_menu(game, fb); break;
        case SCREEN_GAME:      render_game(game, fb); break;
        case SCREEN_STATS:     render_stats(game, fb); break;
        case SCREEN_HISTORY:   render_history(game, fb); break;
    }
}

// =============================================================================
// Init/Cleanup
// =============================================================================
static void guessing_init(Game* game) {
    if (!game) return;
    
    GuessingData* d = (GuessingData*)calloc(1, sizeof(GuessingData));
    if (!d) return;
    
    srand((unsigned int)time(NULL));
    
    d->screen = SCREEN_MAIN_MENU;
    d->menu_selection = MENU_PLAY;
    d->game_active = 0;
    d->game_over = 0;
    d->cursor_visible = 1;
    d->frame_count = 0;
    d->history_scroll = 0;
    d->hint_level = 0;
    d->guess_len = 0;
    d->num_guesses = 0;
    d->target = 0;
    
    // Initialize arrays
    memset(d->guesses, 0, sizeof(d->guesses));
    memset(d->guess_str, 0, sizeof(d->guess_str));
    memset(d->message, 0, sizeof(d->message));
    memset(d->hint_message, 0, sizeof(d->hint_message));
    memset(d->status_message, 0, sizeof(d->status_message));
    
    load_history(d);
    if (num_sessions > 0) calc_stats(d);
    
    snprintf(d->message, sizeof(d->message), "Bem-vindo!");
    snprintf(d->status_message, sizeof(d->status_message), "Selecione JOGAR para comecar");
    
    game->data = d;
    game->active = 1;
}

static void guessing_cleanup(Game* game) {
    if (game && game->data) { 
        free(game->data); 
        game->data = NULL; 
    }
}

// =============================================================================
// ENTRY POINT
// =============================================================================
__attribute__((visibility("default"))) Game* create_game() {
    Game* game = (Game*)calloc(1, sizeof(Game));
    if (!game) return NULL;
    
    game->init = guessing_init;
    game->handle_key = guessing_handle_key;
    game->handle_click = guessing_handle_click;
    game->update = guessing_update;
    game->render = guessing_render;
    game->cleanup = guessing_cleanup;
    
    strncpy(game->name, "Jogo de Adivinhacao", sizeof(game->name) - 1);
    game->name[sizeof(game->name) - 1] = '\0';
    game->active = 0;
    game->data = NULL;
    
    return game;
}