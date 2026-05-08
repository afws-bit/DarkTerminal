#!/bin/sh

# =============================================================================
# SPANE GAME ENGINE - INSTALLATION SCRIPT
# =============================================================================

# PROJECT BASIC INFO
PROJECT_NAME="Spane"
PROJECT_DESCRIPTION="SPANE Game Engine - Multi-Game Platform"

# INSTALLATION PATHS
INSTALL_DIR="/usr/local/etc/$PROJECT_NAME"
BIN_DIR="/usr/local/bin"
GAMES_DIR="$INSTALL_DIR/games"

# SOURCE PATHS
REPO_DIR=$(pwd)
MAIN_SOURCE_DIR="$REPO_DIR"

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================
MAIN_FILE="Spane.c"
BINARY_NAME="spane"
BUILD_DIR="/tmp/spane_build_$$"

WEB_MODE=false

if [ "$(id -u)" = "0" ]; then
    SUDO=""
else
    SUDO="sudo"
fi

# =============================================================================
# FUNCTIONS
# =============================================================================

log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

detect_package_manager() {
    if command_exists apk; then
        echo "apk"
    elif command_exists apt-get; then
        echo "apt-get"
    elif command_exists dnf; then
        echo "dnf"
    elif command_exists yum; then
        echo "yum"
    elif command_exists pacman; then
        echo "pacman"
    elif command_exists zypper; then
        echo "zypper"
    else
        echo "unknown"
    fi
}

install_build_tools() {
    log_message "Checking build tools..."
    
    if command_exists gcc; then
        echo 'int main(){return 0;}' > /tmp/spane_test.c
        if gcc /tmp/spane_test.c -o /tmp/spane_test 2>/dev/null; then
            log_message "✓ Build tools working"
            rm -f /tmp/spane_test.c /tmp/spane_test
            return 0
        fi
        rm -f /tmp/spane_test.c /tmp/spane_test
        log_message "GCC found but cannot compile (missing headers)"
    fi
    
    log_message "Installing build tools..."
    local pkg_manager=$(detect_package_manager)
    
    case "$pkg_manager" in
        apk)
            log_message "Detected Alpine Linux"
            $SUDO apk update 2>/dev/null && $SUDO apk add build-base 2>/dev/null && { log_message "✓ Build tools installed"; return 0; }
            $SUDO apk add gcc musl-dev 2>/dev/null && { log_message "✓ Minimal build tools installed"; return 0; }
            log_message "✗ Failed to install via apk"
            exit 1
            ;;
        apt-get)
            $SUDO apt-get update -y 2>/dev/null && $SUDO apt-get install -y gcc build-essential 2>/dev/null && { log_message "✓ Build tools installed"; return 0; }
            exit 1
            ;;
        dnf)
            $SUDO dnf install -y gcc make 2>/dev/null && { log_message "✓ Build tools installed"; return 0; }
            exit 1
            ;;
        yum)
            $SUDO yum install -y gcc make 2>/dev/null && { log_message "✓ Build tools installed"; return 0; }
            exit 1
            ;;
        pacman)
            $SUDO pacman -S --noconfirm base-devel 2>/dev/null && { log_message "✓ Build tools installed"; return 0; }
            exit 1
            ;;
        zypper)
            $SUDO zypper install -y gcc make 2>/dev/null && { log_message "✓ Build tools installed"; return 0; }
            exit 1
            ;;
        *)
            log_message "✗ Unknown package manager"
            exit 1
            ;;
    esac
}

install_x11_libs() {
    log_message "Checking X11 libraries..."
    
    if [ -f "/usr/include/X11/Xlib.h" ] || [ -f "/usr/local/include/X11/Xlib.h" ]; then
        log_message "✓ X11 found"
        return 0
    fi
    
    log_message "Installing X11..."
    local pkg_manager=$(detect_package_manager)
    
    case "$pkg_manager" in
        apk)
            $SUDO apk add libx11-dev 2>/dev/null
            ;;
        apt-get)
            $SUDO apt-get install -y libx11-dev 2>/dev/null
            ;;
        dnf)
            $SUDO dnf install -y libX11-devel 2>/dev/null
            ;;
        yum)
            $SUDO yum install -y libX11-devel 2>/dev/null
            ;;
        pacman)
            $SUDO pacman -S --noconfirm libx11 2>/dev/null
            ;;
        zypper)
            $SUDO zypper install -y libX11-devel 2>/dev/null
            ;;
    esac
    
    [ -f "/usr/include/X11/Xlib.h" ] && { log_message "✓ X11 installed"; return 0; }
    log_message "⚠ No X11 - web-only build"
    return 1
}

install_zenity() {
    log_message "Checking zenity..."
    
    if command_exists zenity; then
        log_message "✓ zenity already installed"
        return 0
    fi
    
    log_message "Installing zenity..."
    local pkg_manager=$(detect_package_manager)
    
    case "$pkg_manager" in
        apk)
            if $SUDO apk add zenity 2>/dev/null; then
                log_message "✓ zenity installed"
                return 0
            fi
            ;;
        apt-get)
            if $SUDO apt-get update -y 2>/dev/null && $SUDO apt-get install -y zenity 2>/dev/null; then
                log_message "✓ zenity installed"
                return 0
            fi
            ;;
        dnf)
            if $SUDO dnf install -y zenity 2>/dev/null; then
                log_message "✓ zenity installed"
                return 0
            fi
            ;;
        yum)
            if $SUDO yum install -y zenity 2>/dev/null; then
                log_message "✓ zenity installed"
                return 0
            fi
            ;;
        pacman)
            if $SUDO pacman -S --noconfirm zenity 2>/dev/null; then
                log_message "✓ zenity installed"
                return 0
            fi
            ;;
        zypper)
            if $SUDO zypper install -y zenity 2>/dev/null; then
                log_message "✓ zenity installed"
                return 0
            fi
            ;;
        *)
            log_message "⚠ Could not install zenity - unknown package manager"
            return 1
            ;;
    esac
    
    log_message "⚠ Failed to install zenity"
    return 1
}

has_x11() {
    [ -f "/usr/include/X11/Xlib.h" ] || [ -f "/usr/local/include/X11/Xlib.h" ]
}

# Use sed to replace all X11-specific code in one pass
create_web_source() {
    local src="$1"
    local dst="$2"
    
    log_message "Creating web-only source..."
    
    # Create header with all stubs
    cat > "$dst" << 'HEADER'
// SPANE Engine - Web-only build
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
#include <sys/select.h>

// X11 type stubs
typedef unsigned long Window;
typedef unsigned long GC;
typedef unsigned long KeySym;
typedef unsigned long Visual;
typedef struct _XDisplay Display;
typedef struct _XImage XImage;
typedef struct { int type; int x, y; int xbutton_button; int xmotion_x, xmotion_y; int xkey_keycode; } XEvent;
struct _XImage { int width, height; char *data; };

// X11 macros
#define ZPixmap 2
#define KeyPress 2
#define KeyRelease 3
#define ButtonPress 4
#define ButtonRelease 5
#define MotionNotify 6
#define MapNotify 19
#define Expose 12
#define ExposureMask 0
#define KeyPressMask 0
#define ButtonPressMask 0
#define PointerMotionMask 0
#define StructureNotifyMask 0

#define XK_Escape 0xFF1B
#define XK_F1 0xFFBE
#define XK_F2 0xFFBF
#define XK_F3 0xFFC0
#define XK_F4 0xFFC1
#define XK_Up 0xFF52
#define XK_Down 0xFF54
#define XK_Left 0xFF51
#define XK_Right 0xFF53
#define XK_Return 0xFF0D
#define XK_space 0x0020
#define XK_w 0x0077
#define XK_W 0x0057
#define XK_s 0x0073
#define XK_S 0x0053
#define XK_a 0x0061
#define XK_A 0x0041
#define XK_d 0x0064
#define XK_D 0x0044
#define XK_r 0x0072
#define XK_R 0x0052

// X11 function stubs
static Display* XOpenDisplay(void* a) { return 0; }
static int XCloseDisplay(void* a) { return 0; }
static int DefaultScreen(void* a) { return 0; }
static Window XRootWindow(void* a, int b) { return 0; }
static unsigned long XBlackPixel(void* a, int b) { return 0; }
static Window XCreateSimpleWindow(void* a, Window b, int c, int d, unsigned int e, unsigned int f, unsigned int g, unsigned long h, unsigned long i) { return 0; }
static int XSelectInput(void* a, Window b, long c) { return 0; }
static int XStoreName(void* a, Window b, char* c) { return 0; }
static int XMapWindow(void* a, Window b) { return 0; }
static GC XCreateGC(void* a, Window b, unsigned long c, void* d) { return 0; }
static int XFreeGC(void* a, GC b) { return 0; }
static int XDestroyWindow(void* a, Window b) { return 0; }
static int XPending(void* a) { return 0; }
static int XNextEvent(void* a, void* b) { return 0; }
static KeySym XLookupKeysym(void* a, int b) { return 0; }
static Visual* XDefaultVisual(void* a, int b) { return 0; }
static XImage* XCreateImage(void* a, void* b, int c, int d, int e, char* f, int g, int h, int i, int j) { return 0; }
static int XPutImage(void* a, Window b, GC c, XImage* d, int e, int f, int g, int h, unsigned int i, unsigned int j) { return 0; }
static int XFlush(void* a) { return 0; }
static int XDestroyImage(void* a) { return 0; }

// X11 function stubs
static void x11_mirror_frame(void* gm) {}
static int x11_init(void* gm) { return 0; }
static int x11_to_keycode(void* ks) { return 0; }
static void x11_process(void* gm, int* running) {}
static void x11_cleanup(void* gm) {}

HEADER

    # Now copy the original file, skipping X11 includes and X11 function bodies
    # Use sed to delete lines between markers
    sed \
        -e '/^#include <X11\//d' \
        -e '/^\/\/ Compile:.*-lX11/d' \
        -e '/^\/\/ Run:.*\[--web\]/d' \
        -e '/^\/\/ Games are loaded/d' \
        -e '/^\/\/ Place compiled/d' \
        -e '/^static void x11_mirror_frame/,/^}$/d' \
        -e '/^static int x11_init/,/^}$/d' \
        -e '/^static int x11_to_keycode/,/^}$/d' \
        -e '/^static void x11_process/,/^}$/d' \
        -e '/^static void x11_cleanup/,/^}$/d' \
        -e 's/DefaultVisual/XDefaultVisual/g' \
        -e 's/RootWindow(/XRootWindow(/g' \
        -e 's/BlackPixel(/XBlackPixel(/g' \
        "$src" >> "$dst"
    
    [ -f "$dst" ] && [ -s "$dst" ]
}

compile_engine() {
    local src="$1"
    
    if has_x11; then
        log_message "Building with X11"
        
        local cflags=""
        local ldflags="-lX11"
        if command_exists pkg-config && pkg-config --exists x11 2>/dev/null; then
            cflags=$(pkg-config --cflags x11)
            ldflags=$(pkg-config --libs x11)
        fi
        
        cd "$BUILD_DIR" || exit 1
        
        gcc -O3 -march=native -pipe $cflags \
            -c "$src" -o spane_engine.o 2>&1 || {
            log_message "✗ Compilation failed"
            return 1
        }
        
        gcc -O3 spane_engine.o $ldflags -lm -ldl -lpthread \
            -o "$BINARY_NAME" 2>&1 || {
            log_message "✗ Linking failed"
            return 1
        }
        
        log_message "✓ Built with X11"
    else
        log_message "Building web-only"
        
        local web_src="$BUILD_DIR/Spane_web.c"
        if ! create_web_source "$src" "$web_src"; then
            log_message "✗ Failed to create web source"
            return 1
        fi
        
        cd "$BUILD_DIR" || exit 1
        
        gcc -O3 -march=native -pipe \
            -o "$BINARY_NAME" "$web_src" \
            -lm -ldl -lpthread 2>&1 || {
            log_message "✗ Compilation failed"
            return 1
        }
        
        log_message "✓ Built web-only"
    fi
    
    strip "$BINARY_NAME" 2>/dev/null || true
    return 0
}

compile_games() {
    log_message "Compiling games..."
    
    local dir="$MAIN_SOURCE_DIR/games"
    mkdir -p "$BUILD_DIR/games" "$dir"
    
    local count=0
    for f in "$dir"/*.c; do
        [ ! -f "$f" ] && continue
        local name=$(basename "$f" .c)
        if gcc -shared -fPIC -O3 -march=native -o "$BUILD_DIR/games/${name}.so" "$f" 2>&1; then
            log_message "  ✓ $name.so"
            count=$((count + 1))
        else
            log_message "  ✗ $name"
        fi
    done
    
    log_message "Games: $count"
    return 0
}

install_spane() {
    log_message "Installing..."
    
    $SUDO mkdir -p "$INSTALL_DIR" "$GAMES_DIR" "$BIN_DIR"
    $SUDO cp "$BUILD_DIR/$BINARY_NAME" "$INSTALL_DIR/"
    $SUDO chmod 755 "$INSTALL_DIR/$BINARY_NAME"
    
    if ls "$BUILD_DIR/games/"*.so >/dev/null 2>&1; then
        $SUDO cp "$BUILD_DIR/games/"*.so "$GAMES_DIR/" 2>/dev/null
        $SUDO chmod 644 "$GAMES_DIR/"*.so 2>/dev/null
    fi
    
    $SUDO tee "$INSTALL_DIR/run_spane.sh" > /dev/null << 'EOF'
#!/bin/sh
cd /usr/local/etc/Spane/games 2>/dev/null || cd /usr/local/etc/Spane
exec /usr/local/etc/Spane/spane "$@"
EOF
    $SUDO chmod 755 "$INSTALL_DIR/run_spane.sh"
    
    [ -L "$BIN_DIR/$BINARY_NAME" ] && $SUDO rm -f "$BIN_DIR/$BINARY_NAME"
    $SUDO ln -sf "$INSTALL_DIR/run_spane.sh" "$BIN_DIR/$BINARY_NAME"
    
    log_message "✓ Installed"
}

cleanup() { rm -rf "$BUILD_DIR" 2>/dev/null; }

uninstall_spane() {
    log_message "Uninstalling..."
    [ -L "$BIN_DIR/$BINARY_NAME" ] && $SUDO rm -f "$BIN_DIR/$BINARY_NAME"
    [ -d "$INSTALL_DIR" ] && $SUDO rm -rf "$INSTALL_DIR"
    log_message "✓ Uninstalled"
}

show_help() {
    echo "SPANE Game Engine Installer"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo "  --install    Install (default)"
    echo "  --web        Web-only install (skip X11)"
    echo "  --uninstall  Remove"
    echo "  --help       This help"
}

# =============================================================================
# MAIN
# =============================================================================

case "${1:-}" in
    --uninstall|-u) uninstall_spane; exit 0 ;;
    --help|-h) show_help; exit 0 ;;
    --web) WEB_MODE=true ;;
esac

echo ""
echo "╔════════════════════════════════╗"
echo "║  SPANE Game Engine Installer   ║"
[ "$WEB_MODE" = true ] && echo "║  Mode: Web Server Only         ║"
echo "╚════════════════════════════════╝"
echo ""

if [ -d "$INSTALL_DIR" ]; then
    log_message "Existing installation found"
    printf "[1]=Update [2]=Remove [3]=Exit: "
    read choice
    case "$choice" in
        1) $SUDO rm -rf "$INSTALL_DIR"; [ -L "$BIN_DIR/$BINARY_NAME" ] && $SUDO rm -f "$BIN_DIR/$BINARY_NAME" ;;
        2) uninstall_spane; exit 0 ;;
        *) exit 0 ;;
    esac
fi

install_build_tools
install_zenity

if [ "$WEB_MODE" = false ]; then
    install_x11_libs
fi

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

src="$MAIN_SOURCE_DIR/$MAIN_FILE"
if [ ! -f "$src" ]; then
    src=$(find "$MAIN_SOURCE_DIR" -maxdepth 1 -name "*.c" -exec grep -l "int main" {} \; | head -1)
fi

if [ ! -f "$src" ]; then
    log_message "Error: No Spane.c found"
    exit 1
fi

log_message "Source: $src"

if compile_engine "$src"; then
    compile_games
    install_spane
    cleanup
    
    games=$(ls "$GAMES_DIR"/*.so 2>/dev/null | wc -l)
    
    echo ""
    echo "╔════════════════════════════════╗"
    echo "║    Installation Complete!      ║"
    echo "║                                ║"
    has_x11 && echo "║  spane          X11 mode       ║"
    echo "║  spane --web    Web mode       ║"
    echo "║  Games: $games                    ║"
    echo "╚════════════════════════════════╝"
    echo ""
else
    log_message "Compilation failed!"
    cleanup
    exit 1
fi