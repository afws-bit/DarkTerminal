// =============================================================================
// GUESSING GAME - SPANE Engine SDK (Final Fixed Version)
// =============================================================================
// Compile: gcc -shared -fPIC -O3 -o jogodeadvinhacao.so jogodeadvinhacao.c
// =============================================================================
// CONTROLS:
//   0-9         - Digitar número diretamente (acumula dígitos)
//   BACKSPACE   - Apagar último dígito
//   CIMA/BAIXO  - Ajustar palpite (+1/-1)
//   ENTER/ESPAÇO- Confirmar / Submeter palpite
//   ESC         - Voltar ao menu
//   N           - Novo jogo
//   S           - Estatísticas (funciona em qualquer tela)
//   R           - Resetar palpite
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
    
    // Input simples: string de dígitos
    char guess_str[8];      // String do palpite atual
    int guess_len;          // Número de dígitos digitados
    
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
// Desenho
// =============================================================================
static inline void fb_pixel(Framebuffer* fb, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= MAIN_WINDOW_WIDTH || y < 0 || y >= MAIN_WINDOW_HEIGHT) return;
    int i = (y * MAIN_WINDOW_WIDTH + x) * 4;
    fb->pixels[i] = r; fb->pixels[i+1] = g; fb->pixels[i+2] = b; fb->pixels[i+3] = 255;
}

static void fb_fill(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            fb_pixel(fb, x+dx, y+dy, r, g, b);
}

static void fb_rect(Framebuffer* fb, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int dx = 0; dx < w; dx++) { fb_pixel(fb, x+dx, y, r, g, b); fb_pixel(fb, x+dx, y+h-1, r, g, b); }
    for (int dy = 1; dy < h-1; dy++) { fb_pixel(fb, x, y+dy, r, g, b); fb_pixel(fb, x+w-1, y+dy, r, g, b); }
}

static void fb_char(Framebuffer* fb, int x, int y, char c, unsigned char r, unsigned char g, unsigned char b) {
    if (c < 32 || c > 126) return;
    const unsigned char* glyph = font_5x7[c-32];
    for (int row = 0; row < 7; row++)
        for (int col = 0; col < 5; col++)
            if (glyph[col] & (1<<row)) fb_pixel(fb, x+col, y+row, r, g, b);
}

static void fb_text(Framebuffer* fb, int x, int y, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    if (!s) return;
    for (int i = 0; s[i]; i++) fb_char(fb, x+i*6, y, s[i], r, g, b);
}

static void fb_text_center(Framebuffer* fb, int x, int y, int w, const char* s, unsigned char r, unsigned char g, unsigned char b) {
    if (!s) return;
    int len = strlen(s);
    fb_text(fb, x + (w - len*6)/2, y, s, r, g, b);
}

// =============================================================================
// sqrt customizada
// =============================================================================
static double my_sqrt(double x) {
    if (x <= 0.0) return 0.0;
    double guess = x / 2.0;
    for (int i = 0; i < 20; i++) {
        double ng = (guess + x/guess) / 2.0;
        if (ng == guess) break;
        guess = ng;
    }
    return guess;
}

// =============================================================================
// Histórico e Arquivo
// =============================================================================
static void save_session(GuessingData* d) {
    FILE* f = fopen(HISTORY_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    fprintf(f, "%ld %d %d", (long)now, d->target, d->num_guesses);
    for (int i = 0; i < d->num_guesses; i++) fprintf(f, " %d", d->guesses[i]);
    fprintf(f, "\n");
    fclose(f);
    printf("Saved: target=%d, guesses=%d\n", d->target, d->num_guesses);
}

static void load_history(GuessingData* d) {
    FILE* f = fopen(HISTORY_FILE, "r");
    if (!f) {
        num_sessions = 0;
        if (d) { d->history_scroll = 0; d->total_sessions = 0; }
        return;
    }
    
    num_sessions = 0;
    char line[1024];
    
    while (fgets(line, sizeof(line), f) && num_sessions < MAX_SCORES) {
        Session* s = &sessions[num_sessions];
        char* token = strtok(line, " \n");
        if (!token) continue;
        s->timestamp = (time_t)atol(token);
        token = strtok(NULL, " \n");
        if (!token) continue;
        s->target = atoi(token);
        token = strtok(NULL, " \n");
        if (!token) continue;
        s->num_guesses = atoi(token);
        int i = 0;
        while ((token = strtok(NULL, " \n")) != NULL && i < MAX_GUESSES) {
            s->guesses[i++] = atoi(token);
        }
        if (s->num_guesses > 0) num_sessions++;
    }
    fclose(f);
    if (d) { d->history_scroll = 0; d->total_sessions = num_sessions; }
    printf("Loaded %d sessions\n", num_sessions);
}

// =============================================================================
// Recursão
// =============================================================================
static int rec_sum(int* arr, int n) { return n <= 0 ? 0 : arr[0] + rec_sum(arr+1, n-1); }
static int rec_min(int* arr, int n) { 
    if (n <= 0) return 999999;
    if (n == 1) return arr[0];
    int m = rec_min(arr+1, n-1);
    return arr[0] < m ? arr[0] : m;
}
static int rec_max(int* arr, int n) {
    if (n <= 0) return -1;
    if (n == 1) return arr[0];
    int m = rec_max(arr+1, n-1);
    return arr[0] > m ? arr[0] : m;
}
static double rec_sum_sq(int* arr, int n, double mean) {
    if (n <= 0) return 0.0;
    double d = arr[0] - mean;
    return d*d + rec_sum_sq(arr+1, n-1, mean);
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
    for (int i = 0; i < num_sessions; i++) gc[i] = sessions[i].num_guesses;
    
    d->total_sessions = num_sessions;
    int sum = rec_sum(gc, num_sessions);
    d->avg_guesses = (double)sum / num_sessions;
    d->best_session = rec_min(gc, num_sessions);
    d->worst_session = rec_max(gc, num_sessions);
    double var = rec_sum_sq(gc, num_sessions, d->avg_guesses) / num_sessions;
    d->std_dev = my_sqrt(var);
    
    snprintf(d->status_message, sizeof(d->status_message), 
             "%d sessoes | Media: %.1f | Melhor: %d | Pior: %d", 
             num_sessions, d->avg_guesses, d->best_session, d->worst_session);
}

static const char* get_strategy(GuessingData* d) {
    if (num_sessions == 0) return "Sem dados - use busca binaria!";
    if (d->avg_guesses < 6) return "Excelente! Busca binaria perfeita!";
    if (d->avg_guesses < 10) return "Bom! Tente melhorar a estrategia.";
    return "Use busca binaria: comece em 50";
}

// =============================================================================
// Dica progressiva
// =============================================================================
static void gen_hint(GuessingData* d) {
    if (d->num_guesses == 0) {
        snprintf(d->hint_message, sizeof(d->hint_message), "Digite um numero entre %d e %d", MIN_GUESS, MAX_GUESS);
        return;
    }
    int lo = MIN_GUESS, hi = MAX_GUESS;
    for (int i = 0; i < d->num_guesses; i++) {
        if (d->guesses[i] < d->target && d->guesses[i] >= lo) lo = d->guesses[i] + 1;
        if (d->guesses[i] > d->target && d->guesses[i] <= hi) hi = d->guesses[i] - 1;
    }
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
// Jogo
// =============================================================================
static void new_game(GuessingData* d) {
    d->game_active = 1;
    d->target = (rand() % MAX_GUESS) + 1;
    d->num_guesses = 0;
    d->game_over = 0;
    d->guess_str[0] = 0;
    d->guess_len = 0;
    snprintf(d->message, sizeof(d->message), "Nova partida! Adivinhe o numero entre %d e %d", MIN_GUESS, MAX_GUESS);
    snprintf(d->status_message, sizeof(d->status_message), "Digite um numero e pressione ENTER");
    gen_hint(d);
    printf("New game: target=%d\n", d->target);
}

static int get_current_guess(GuessingData* d) {
    if (d->guess_len == 0) return 0;
    return atoi(d->guess_str);
}

static void submit_guess(GuessingData* d) {
    if (d->game_over) return;
    
    int guess = get_current_guess(d);
    if (guess == 0 && d->guess_len == 0) {
        snprintf(d->message, sizeof(d->message), "Digite um numero primeiro!");
        return;
    }
    if (guess < MIN_GUESS || guess > MAX_GUESS) {
        snprintf(d->message, sizeof(d->message), "Numero deve estar entre %d e %d!", MIN_GUESS, MAX_GUESS);
        d->guess_str[0] = 0;
        d->guess_len = 0;
        return;
    }
    
    d->guesses[d->num_guesses++] = guess;
    
    if (guess < d->target) {
        int diff = d->target - guess;
        if (diff > 30) snprintf(d->message, sizeof(d->message), "MUITO BAIXO! Tente um numero bem maior.");
        else if (diff > 10) snprintf(d->message, sizeof(d->message), "BAIXO! Tente um numero maior.");
        else snprintf(d->message, sizeof(d->message), "Um pouco baixo... Esta perto!");
        snprintf(d->status_message, sizeof(d->status_message), "Tentativa #%d", d->num_guesses);
    } else if (guess > d->target) {
        int diff = guess - d->target;
        if (diff > 30) snprintf(d->message, sizeof(d->message), "MUITO ALTO! Tente um numero bem menor.");
        else if (diff > 10) snprintf(d->message, sizeof(d->message), "ALTO! Tente um numero menor.");
        else snprintf(d->message, sizeof(d->message), "Um pouco alto... Esta perto!");
        snprintf(d->status_message, sizeof(d->status_message), "Tentativa #%d", d->num_guesses);
    } else {
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
    
    gen_hint(d);
    d->guess_str[0] = 0;
    d->guess_len = 0;
}

// =============================================================================
// Input Handler - CORRIGIDO
// =============================================================================
static void guessing_handle_key(Game* game, int keycode, int pressed) {
    if (!game || !game->data || !pressed) return;
    GuessingData* d = (GuessingData*)game->data;
    
    printf("Keycode=%d screen=%d active=%d over=%d guess='%s'\n", 
           keycode, d->screen, d->game_active, d->game_over, d->guess_str);
    
    // ===== TECLAS GLOBAIS =====
    
    // S - Estatísticas (sempre funciona)
    if (keycode == KEY_S || keycode == 115) {
        printf(">>> Abrindo estatisticas <<<\n");
        load_history(d);
        calc_stats(d);
        d->screen = SCREEN_STATS;
        d->history_scroll = 0;
        return;
    }
    
    // N - Novo jogo (sempre funciona)
    if (keycode == KEY_N || keycode == 110) {
        printf(">>> Novo jogo <<<\n");
        new_game(d);
        d->screen = SCREEN_GAME;
        return;
    }
    
    // ===== ESC =====
    if (keycode == KEY_ESC) {
        printf(">>> ESC: screen=%d <<<\n", d->screen);
        if (d->screen == SCREEN_GAME) {
            // Salvar estado e ir para menu
            d->screen = SCREEN_MAIN_MENU;
            if (d->game_active && !d->game_over) {
                d->game_active = 0; // Pausar jogo
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
                d->menu_selection = (d->menu_selection - 1 + MENU_COUNT) % MENU_COUNT;
                break;
            case KEY_DOWN:
                d->menu_selection = (d->menu_selection + 1) % MENU_COUNT;
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
        if (keycode == KEY_UP) { if (d->history_scroll > 0) d->history_scroll--; }
        if (keycode == KEY_DOWN) { d->history_scroll++; }
        return;
    }
    
    // ===== TELA DO JOGO =====
    if (d->screen == SCREEN_GAME) {
        // Game over
        if (d->game_over) {
            if (keycode == KEY_ENTER || keycode == KEY_SPACE) {
                new_game(d);
            }
            return;
        }
        
        // Jogo ativo
        if (d->game_active) {
            // Dígitos 0-9 (keycodes 48-57)
            if (keycode >= 48 && keycode <= 57) {
                int digit = keycode - 48;
                if (d->guess_len < 7) {
                    d->guess_str[d->guess_len++] = '0' + digit;
                    d->guess_str[d->guess_len] = 0;
                    
                    // Limitar a no máximo 2 dígitos (1-100)
                    if (d->guess_len >= 3) {
                        // Manter só os últimos 2 dígitos
                        d->guess_str[0] = d->guess_str[1];
                        d->guess_str[1] = d->guess_str[2];
                        d->guess_str[2] = 0;
                        d->guess_len = 2;
                    }
                    
                    // Se o número for > 100, truncar para 2 dígitos
                    if (atoi(d->guess_str) > MAX_GUESS) {
                        d->guess_str[1] = 0;
                        d->guess_len = 1;
                    }
                    
                    printf("Digit %d -> guess='%s' (len=%d)\n", digit, d->guess_str, d->guess_len);
                }
                return;
            }
            
            switch (keycode) {
                case KEY_BACK:
                case KEY_DEL:
                    // Backspace - apagar último dígito
                    if (d->guess_len > 0) {
                        d->guess_str[--d->guess_len] = 0;
                        printf("Backspace -> guess='%s'\n", d->guess_str);
                    }
                    break;
                    
                case KEY_UP:
                    // Incrementar palpite
                    {
                        int g = get_current_guess(d);
                        g = (g + 1) > MAX_GUESS ? MAX_GUESS : g + 1;
                        snprintf(d->guess_str, sizeof(d->guess_str), "%d", g);
                        d->guess_len = strlen(d->guess_str);
                        printf("UP -> guess=%d\n", g);
                    }
                    break;
                    
                case KEY_DOWN:
                    // Decrementar palpite
                    {
                        int g = get_current_guess(d);
                        g = (g - 1) < MIN_GUESS ? MIN_GUESS : g - 1;
                        snprintf(d->guess_str, sizeof(d->guess_str), "%d", g);
                        d->guess_len = strlen(d->guess_str);
                        printf("DOWN -> guess=%d\n", g);
                    }
                    break;
                    
                case KEY_R:
                    // Reset
                    d->guess_str[0] = 0;
                    d->guess_len = 0;
                    printf("Reset guess\n");
                    break;
                    
                case KEY_ENTER:
                case KEY_SPACE:
                    printf("Submit: '%s'\n", d->guess_str);
                    submit_guess(d);
                    break;
            }
        }
    }
}

// =============================================================================
// Click
// =============================================================================
static void guessing_handle_click(Game* game, int x, int y) {
    if (!game || !game->data) return;
    GuessingData* d = (GuessingData*)game->data;
    
    int gx = x - GAME_AREA_X;
    int gy = y - GAME_AREA_Y;
    
    if (d->screen == SCREEN_MAIN_MENU) {
        if (gx >= 80 && gx <= 520) {
            int item = (gy - 140) / 70;
            if (item >= 0 && item < MENU_COUNT) {
                d->menu_selection = item;
                if (item == MENU_PLAY) { new_game(d); d->screen = SCREEN_GAME; }
                else if (item == MENU_ANALYZE) { load_history(d); calc_stats(d); d->screen = SCREEN_STATS; }
                else if (item == MENU_HISTORY) { load_history(d); calc_stats(d); d->screen = SCREEN_HISTORY; }
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
// Render - Menu
// =============================================================================
static void render_menu(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    int cy = GAME_AREA_Y + 60;
    
    fb_text_center(fb, GAME_AREA_X, cy + 10, GAME_AREA_WIDTH, "JOGO DE ADIVINHACAO", 0xFF, 0xCC, 0x00);
    fb_text_center(fb, GAME_AREA_X, cy + 45, GAME_AREA_WIDTH, "Use CIMA/BAIXO e ENTER para selecionar", 0x88, 0x88, 0x88);
    
    const char* items[] = { "[ JOGAR ]", "[ ANALISAR ESTATISTICAS ]", "[ VER HISTORICO ]" };
    const char* descs[] = { "Iniciar nova partida", "Ver estatisticas de desempenho", "Visualizar partidas anteriores" };
    
    for (int i = 0; i < MENU_COUNT; i++) {
        int my = cy + 85 + i * 75;
        int sel = (i == d->menu_selection);
        
        fb_fill(fb, GAME_AREA_X + 80, my, 440, 50, sel ? 0x00 : 0x22, sel ? 0x66 : 0x22, sel ? 0xAA : 0x33);
        if (sel) fb_rect(fb, GAME_AREA_X + 80, my, 440, 50, 0x00, 0xAA, 0xFF);
        
        fb_text_center(fb, GAME_AREA_X + 80, my + 8, 440, items[i], sel ? 0xFF : 0xAA, sel ? 0xFF : 0xAA, sel ? 0x00 : 0xAA);
        fb_text_center(fb, GAME_AREA_X + 80, my + 28, 440, descs[i], sel ? 0xAA : 0x66, sel ? 0xCC : 0x66, sel ? 0xFF : 0x66);
        
        if (sel && d->cursor_visible) fb_text(fb, GAME_AREA_X + 55, my + 15, ">>", 0xFF, 0xFF, 0x00);
    }
    
    fb_text_center(fb, GAME_AREA_X, GAME_AREA_Y + GAME_AREA_HEIGHT - 30, GAME_AREA_WIDTH,
                   "ESC: Sair  |  S: Estatisticas  |  N: Novo Jogo", 0x66, 0x66, 0x66);
    
    if (num_sessions > 0) {
        char info[128];
        snprintf(info, sizeof(info), "%d partidas | Melhor: %d tentativa(s)", num_sessions, d->best_session);
        fb_text_center(fb, GAME_AREA_X, GAME_AREA_Y + GAME_AREA_HEIGHT - 15, GAME_AREA_WIDTH, info, 0x55, 0x55, 0x55);
    }
}

// =============================================================================
// Render - Jogo
// =============================================================================
static void render_game(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    int bx = GAME_AREA_X + 30, by = GAME_AREA_Y + 15;
    
    fb_text(fb, bx, by, "JOGO DE ADIVINHACAO", 0xFF, 0xCC, 0x00);
    fb_text(fb, bx + 350, by, "ESC:Menu  S:Stats  N:Novo", 0x66, 0x66, 0x66);
    
    // Palpite atual
    int gy = by + 35;
    fb_text(fb, bx, gy, "SEU PALPITE:", 0xCC, 0xCC, 0xCC);
    
    fb_fill(fb, bx, gy + 20, 220, 50, 0x11, 0x11, 0x22);
    fb_rect(fb, bx, gy + 20, 220, 50, d->game_over ? 0x44 : 0x00, d->game_over ? 0x44 : 0x88, d->game_over ? 0x44 : 0xFF);
    
    if (d->game_active && !d->game_over) {
        char display[32];
        if (d->guess_len == 0) {
            snprintf(display, sizeof(display), "_");
        } else {
            snprintf(display, sizeof(display), "%s", d->guess_str);
        }
        fb_text(fb, bx + 20, gy + 35, display, 0xFF, 0xFF, 0xFF);
        
        // Cursor piscante
        if (d->cursor_visible) {
            int cx = bx + 20 + (d->guess_len * 6);
            fb_fill(fb, cx, gy + 42, 6, 2, 0xFF, 0xFF, 0x00);
        }
    } else if (d->game_over) {
        fb_text(fb, bx + 20, gy + 35, "FIM", 0x88, 0x88, 0x88);
    }
    
    // Ajuda
    fb_text(fb, bx + 240, gy + 25, "Digite o numero:", 0x88, 0x88, 0x88);
    fb_text(fb, bx + 240, gy + 42, "0-9 + ENTER", 0x66, 0x88, 0x66);
    fb_text(fb, bx + 240, gy + 58, "BACK: Apagar  R: Reset", 0x55, 0x55, 0x66);
    
    // Mensagem
    int my = gy + 90;
    unsigned char mr = 0xFF, mg = 0xFF, mb = 0xFF;
    if (strstr(d->message, "BAIXO") || strstr(d->message, "baixo")) { mr = 0xFF; mg = 0x66; mb = 0x66; }
    else if (strstr(d->message, "ALTO") || strstr(d->message, "alto")) { mr = 0xFF; mg = 0x66; mb = 0x66; }
    else if (strstr(d->message, "ACERTOU") || strstr(d->message, "PARABENS") || strstr(d->message, "INCRIVEL")) { mr = 0x00; mg = 0xFF; mb = 0x00; }
    fb_text(fb, bx, my, d->message, mr, mg, mb);
    
    // Dica
    if (d->game_active && !d->game_over) {
        fb_text(fb, bx, my + 25, d->hint_message, 0x00, 0xCC, 0xCC);
    }
    
    // Status
    fb_text(fb, bx, my + 50, d->status_message, 0x88, 0xAA, 0x88);
    
    // Histórico
    int hx = bx + 420;
    fb_fill(fb, hx, by + 30, 220, 380, 0x15, 0x15, 0x20);
    fb_rect(fb, hx, by + 30, 220, 380, 0x33, 0x33, 0x44);
    fb_text(fb, hx + 10, by + 40, "HISTORICO:", 0xAA, 0xAA, 0xAA);
    
    for (int i = 0; i < d->num_guesses; i++) {
        char gs[32];
        const char* ind = d->guesses[i] < d->target ? "<<" : d->guesses[i] > d->target ? ">>" : "**";
        snprintf(gs, sizeof(gs), "#%d: %d %s", i+1, d->guesses[i], ind);
        unsigned char r = d->guesses[i] < d->target ? 0xFF : d->guesses[i] > d->target ? 0xFF : 0x00;
        unsigned char g = d->guesses[i] < d->target ? 0x88 : d->guesses[i] > d->target ? 0x88 : 0xFF;
        unsigned char b = d->guesses[i] < d->target ? 0x88 : d->guesses[i] > d->target ? 0x88 : 0x00;
        fb_text(fb, hx + 10, by + 65 + i*18, gs, r, g, b);
    }
    
    // Game over
    if (d->game_over) {
        int oy = GAME_AREA_Y + GAME_AREA_HEIGHT - 70;
        fb_fill(fb, GAME_AREA_X + 10, oy, GAME_AREA_WIDTH - 20, 50, 0x11, 0x33, 0x11);
        fb_rect(fb, GAME_AREA_X + 10, oy, GAME_AREA_WIDTH - 20, 50, 0x00, 0xFF, 0x00);
        fb_text_center(fb, GAME_AREA_X + 10, oy + 15, GAME_AREA_WIDTH - 20,
                       "ENTER: Novo Jogo  |  S: Estatisticas  |  ESC: Menu", 0x00, 0xFF, 0x00);
    }
}

// =============================================================================
// Render - Estatísticas
// =============================================================================
static void render_stats(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    int sx = GAME_AREA_X + 30, sy = GAME_AREA_Y + 20;
    
    fb_text(fb, sx, sy, "ESTATISTICAS", 0xFF, 0xCC, 0x00);
    fb_text(fb, sx + 300, sy, "ESC:Menu  N:Jogar  SETAS:Scroll", 0x66, 0x66, 0x66);
    
    if (num_sessions == 0) {
        fb_text_center(fb, GAME_AREA_X, sy + 120, GAME_AREA_WIDTH,
                       "Nenhuma partida registrada!", 0xFF, 0x88, 0x88);
        fb_text_center(fb, GAME_AREA_X, sy + 150, GAME_AREA_WIDTH,
                       "Pressione N para jogar ou ESC para o menu", 0x88, 0x88, 0x88);
        return;
    }
    
    // Box de estatísticas
    fb_fill(fb, sx, sy + 30, 450, 180, 0x15, 0x15, 0x25);
    fb_rect(fb, sx, sy + 30, 450, 180, 0x33, 0x55, 0x77);
    
    char lines[7][128];
    snprintf(lines[0], sizeof(lines[0]), "Total de sessoes:      %d", d->total_sessions);
    snprintf(lines[1], sizeof(lines[1]), "Media de tentativas:   %.1f", d->avg_guesses);
    snprintf(lines[2], sizeof(lines[2]), "Melhor sessao:         %d tentativa(s)", d->best_session);
    snprintf(lines[3], sizeof(lines[3]), "Pior sessao:           %d tentativa(s)", d->worst_session);
    snprintf(lines[4], sizeof(lines[4]), "Desvio padrao:         %.2f", d->std_dev);
    snprintf(lines[5], sizeof(lines[5]), "");
    snprintf(lines[6], sizeof(lines[6]), "Estrategia: %s", get_strategy(d));
    
    for (int i = 0; i < 7; i++) {
        unsigned char r = 0xFF, g = 0xFF, b = 0xFF;
        if (i == 6) { r = 0x00; g = 0xCC; b = 0xCC; }
        fb_text(fb, sx + 15, sy + 45 + i*22, lines[i], r, g, b);
    }
    
    // Rating
    fb_fill(fb, sx + 470, sy + 30, 160, 180, 0x15, 0x15, 0x25);
    fb_rect(fb, sx + 470, sy + 30, 160, 180, 0x33, 0x55, 0x77);
    fb_text(fb, sx + 480, sy + 40, "AVALIACAO:", 0xFF, 0xCC, 0x00);
    
    const char* stars;
    if (d->avg_guesses < 6) stars = "***** EXPERT";
    else if (d->avg_guesses < 8) stars = "**** AVANCADO";
    else if (d->avg_guesses < 10) stars = "*** MEDIO";
    else if (d->avg_guesses < 15) stars = "** BASICO";
    else stars = "* INICIANTE";
    fb_text(fb, sx + 480, sy + 70, stars, 0xFF, 0xFF, 0x00);
    
    // Histórico compacto
    fb_text(fb, sx, sy + 220, "ULTIMAS PARTIDAS:", 0xAA, 0xAA, 0xAA);
    int start = d->history_scroll;
    if (start > num_sessions - 8) start = num_sessions - 8;
    if (start < 0) start = 0;
    
    for (int i = 0; i < 8 && (start+i) < num_sessions; i++) {
        Session* s = &sessions[start+i];
        char line[128];
        char ts[20];
        struct tm* tm = localtime(&s->timestamp);
        strftime(ts, sizeof(ts), "%d/%m %H:%M", tm);
        snprintf(line, sizeof(line), "%s  Alvo:%d  Tentativas:%d", ts, s->target, s->num_guesses);
        
        unsigned char r = s->num_guesses == d->best_session ? 0x00 : 0xAA;
        unsigned char g = s->num_guesses == d->best_session ? 0xFF : 0xAA;
        unsigned char b = s->num_guesses == d->worst_session ? 0x66 : 0xAA;
        fb_text(fb, sx + 10, sy + 245 + i*18, line, r, g, b);
    }
}

// =============================================================================
// Render - Histórico
// =============================================================================
static void render_history(Game* game, Framebuffer* fb) {
    GuessingData* d = (GuessingData*)game->data;
    int sx = GAME_AREA_X + 20, sy = GAME_AREA_Y + 20;
    
    fb_text(fb, sx + 10, sy, "HISTORICO COMPLETO", 0xFF, 0xCC, 0x00);
    fb_text(fb, sx + 300, sy, "ESC:Menu  SETAS:Scroll", 0x66, 0x66, 0x66);
    
    if (num_sessions == 0) {
        fb_text_center(fb, GAME_AREA_X, sy + 100, GAME_AREA_WIDTH,
                       "Nenhum historico disponivel", 0xFF, 0x88, 0x88);
        return;
    }
    
    // Cabeçalho
    char hdr[128];
    snprintf(hdr, sizeof(hdr), "%-18s %-6s %-10s %s", "DATA/HORA", "ALVO", "TENTATIVAS", "PALPITES");
    fb_text(fb, sx + 5, sy + 35, hdr, 0xCC, 0xCC, 0xCC);
    for (int i = 0; i < 72; i++) fb_char(fb, sx+5+i*6, sy+43, '-', 0x44,0x44,0x44);
    
    int start = d->history_scroll;
    int maxd = 14;
    if (start > num_sessions - maxd) start = num_sessions - maxd;
    if (start < 0) start = 0;
    
    for (int i = 0; i < maxd && (start+i) < num_sessions; i++) {
        Session* s = &sessions[start+i];
        char line[512], ts[20];
        struct tm* tm = localtime(&s->timestamp);
        strftime(ts, sizeof(ts), "%d/%m/%Y %H:%M", tm);
        
        char gs[128] = "";
        int ms = s->num_guesses < 5 ? s->num_guesses : 5;
        for (int j = 0; j < ms; j++) {
            char num[8];
            snprintf(num, sizeof(num), "%d%s", s->guesses[j], j < ms-1 ? "," : "");
            strcat(gs, num);
        }
        if (s->num_guesses > 5) strcat(gs, "...");
        
        snprintf(line, sizeof(line), "%-18s %-6d %-10d %s", ts, s->target, s->num_guesses, gs);
        
        unsigned char r = 0xAA, g = 0xAA, b = 0xAA;
        if (d->total_sessions > 1) {
            if (s->num_guesses == d->best_session) { r = 0x00; g = 0xFF; b = 0x00; }
            else if (s->num_guesses == d->worst_session) { r = 0xFF; g = 0x66; b = 0x66; }
        }
        fb_text(fb, sx + 5, sy + 50 + i*18, line, r, g, b);
    }
    
    if (start > 0) fb_text(fb, sx + 350, sy + 35, "^ MAIS ^", 0xFF, 0xCC, 0x00);
    if (start + maxd < num_sessions) fb_text(fb, sx + 350, sy + 50 + maxd*18, "v MAIS v", 0xFF, 0xCC, 0x00);
    
    fb_text(fb, sx + 5, sy + 320, "Verde=Melhor  Vermelho=Pior", 0x88, 0x88, 0x88);
}

// =============================================================================
// Render principal
// =============================================================================
static void guessing_render(Game* game, Framebuffer* fb) {
    if (!game || !game->data || !fb) return;
    GuessingData* d = (GuessingData*)game->data;
    
    fb_fill(fb, GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, 0x11, 0x11, 0x1A);
    fb_rect(fb, GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, 0x33, 0x55, 0x77);
    
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
    
    srand(time(NULL));
    d->screen = SCREEN_MAIN_MENU;
    d->menu_selection = MENU_PLAY;
    d->game_active = 0;
    d->game_over = 0;
    d->cursor_visible = 1;
    
    load_history(d);
    if (num_sessions > 0) calc_stats(d);
    
    snprintf(d->message, sizeof(d->message), "Bem-vindo!");
    snprintf(d->status_message, sizeof(d->status_message), "Selecione JOGAR para comecar");
    
    game->data = d;
    game->active = 1;
    printf("Jogo inicializado com sucesso!\n");
}

static void guessing_cleanup(Game* game) {
    if (game && game->data) { free(game->data); game->data = NULL; }
}

// =============================================================================
// ENTRY POINT
// =============================================================================
__attribute__((visibility("default"))) Game* create_game() {
    printf("Criando Jogo de Adivinhacao\n");
    Game* game = (Game*)calloc(1, sizeof(Game));
    if (!game) return NULL;
    
    game->init = guessing_init;
    game->handle_key = guessing_handle_key;
    game->handle_click = guessing_handle_click;
    game->update = guessing_update;
    game->render = guessing_render;
    game->cleanup = guessing_cleanup;
    
    strncpy(game->name, "Jogo de Adivinhacao", sizeof(game->name) - 1);
    game->active = 0;
    game->data = NULL;
    return game;
}