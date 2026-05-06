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

# SOURCE PATHS
REPO_DIR=$(pwd)
MAIN_SOURCE_DIR="$REPO_DIR"

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================
MAIN_FILE="Spane.c"
BINARY_NAME="spane"
BUILD_DIR="/tmp/spane_build_$$"

# =============================================================================
# FUNCTION DEFINITIONS
# =============================================================================

log_message() {
    message="$1"
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] $message"
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check if a package is installed (dpkg based)
package_installed() {
    dpkg -l "$1" 2>/dev/null | grep -q "^ii"
}

# Install package with fallback: direct → apt update → retry
install_package() {
    package="$1"
    
    log_message "Installing $package..."
    
    # First attempt: direct install without update
    if sudo apt-get install -y "$package" 2>/dev/null; then
        log_message "✓ Installed $package (direct)"
        return 0
    fi
    
    # Second attempt: update and retry
    log_message "Direct install failed, updating package lists..."
    if sudo apt-get update -y 2>/dev/null && sudo apt-get install -y "$package" 2>/dev/null; then
        log_message "✓ Installed $package (after update)"
        return 0
    fi
    
    log_message "✗ Failed to install $package"
    return 1
}

# Verify and install build dependencies
install_dependencies() {
    log_message "Checking build dependencies..."
    
    # Check GCC
    if command_exists gcc; then
        log_message "✓ GCC found: $(gcc --version | head -n1)"
    else
        log_message "GCC not found, installing..."
        install_package "gcc" || {
            log_message "Error: Failed to install GCC"
            exit 1
        }
    fi
    
    # Check libx11-dev
    if package_installed "libx11-dev"; then
        log_message "✓ libx11-dev found"
    else
        log_message "libx11-dev not found, installing..."
        install_package "libx11-dev" || {
            log_message "Error: Failed to install libx11-dev"
            exit 1
        }
    fi
}

# Check if a C file includes X11 headers
needs_x11() {
    c_file="$1"
    grep -q "#include.*<X11/" "$c_file" 2>/dev/null
    return $?
}

# Recursively find all .c files, excluding .git
find_c_files() {
    find "$1" -type f -name "*.c" ! -path "*/.git/*" 2>/dev/null
}

# Compile all C files recursively
compile_spane() {
    log_message "Starting compilation..."
    
    # Create build directory
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    
    # Find main file
    main_file=""
    if [ -f "$MAIN_SOURCE_DIR/$MAIN_FILE" ]; then
        main_file="$MAIN_SOURCE_DIR/$MAIN_FILE"
    else
        # Search for main function
        main_file=$(find_c_files "$MAIN_SOURCE_DIR" | while read f; do
            if grep -q "int main\s*(" "$f" 2>/dev/null; then
                echo "$f"
                break
            fi
        done)
    fi
    
    if [ -z "$main_file" ]; then
        log_message "Error: No main file found (looking for $MAIN_FILE or any file with main function)"
        exit 1
    fi
    
    log_message "Main file: $main_file"
    
    # Find all C files
    all_files=$(find_c_files "$MAIN_SOURCE_DIR")
    file_count=$(echo "$all_files" | wc -l)
    log_message "Found $file_count C source files"
    
    # Copy files to build directory maintaining structure
    echo "$all_files" | while IFS= read -r file; do
        [ -z "$file" ] && continue
        rel_path="${file#$MAIN_SOURCE_DIR/}"
        mkdir -p "$BUILD_DIR/$(dirname "$rel_path")"
        cp "$file" "$BUILD_DIR/$rel_path"
    done
    
    cd "$BUILD_DIR" || exit 1
    
    # Detect X11 flags once
    X11_CFLAGS=""
    X11_LDFLAGS=""
    if pkg-config --exists x11 2>/dev/null; then
        X11_CFLAGS=$(pkg-config --cflags x11 2>/dev/null)
        X11_LDFLAGS=$(pkg-config --libs x11 2>/dev/null)
    else
        X11_CFLAGS="-I/usr/include/X11"
        X11_LDFLAGS="-L/usr/lib/x86_64-linux-gnu -lX11"
    fi
    
    # Common flags
    COMMON_CFLAGS="-O3 -march=native -pipe -flto -fomit-frame-pointer"
    
    # Compile each file, detecting if it needs X11
    object_files=""
    echo "$all_files" | while IFS= read -r file; do
        [ -z "$file" ] && continue
        rel_path="${file#$MAIN_SOURCE_DIR/}"
        obj_file="${rel_path%.c}.o"
        src_file="$BUILD_DIR/$rel_path"
        
        # Determine if this file needs X11
        if needs_x11 "$src_file"; then
            CFLAGS="$COMMON_CFLAGS $X11_CFLAGS"
            log_message "  Compiling with X11: $rel_path"
        else
            CFLAGS="$COMMON_CFLAGS"
            log_message "  Compiling: $rel_path"
        fi
        
        mkdir -p "$(dirname "$BUILD_DIR/$obj_file")"
        if gcc -c $CFLAGS "$src_file" -o "$BUILD_DIR/$obj_file" 2>/dev/null; then
            echo "$obj_file" >> "$BUILD_DIR/objects.list"
        else
            log_message "  ✗ Failed to compile: $rel_path"
        fi
    done
    
    # Check if we have objects
    if [ ! -f "$BUILD_DIR/objects.list" ]; then
        log_message "Error: No files compiled successfully"
        exit 1
    fi
    
    object_files=$(cat "$BUILD_DIR/objects.list" | tr '\n' ' ')
    obj_count=$(echo "$object_files" | wc -w)
    log_message "Successfully compiled $obj_count object files"
    
    # Link everything
    log_message "Linking $BINARY_NAME..."
    if gcc -O3 -flto $object_files $X11_LDFLAGS -lm -o "$BUILD_DIR/$BINARY_NAME" 2>/dev/null; then
        log_message "✓ Successfully built $BINARY_NAME"
        strip "$BUILD_DIR/$BINARY_NAME" 2>/dev/null || true
        return 0
    else
        log_message "✗ Linking failed"
        return 1
    fi
}

# Install binary globally
install_binary() {
    log_message "Installing SPANE engine globally..."
    
    # Create installation directory
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$BIN_DIR"
    
    # Copy binary
    cp "$BUILD_DIR/$BINARY_NAME" "$INSTALL_DIR/$BINARY_NAME"
    chmod 755 "$INSTALL_DIR/$BINARY_NAME"
    
    # Remove old symlink if exists
    [ -L "$BIN_DIR/$BINARY_NAME" ] && rm -f "$BIN_DIR/$BINARY_NAME"
    
    # Create symlink
    ln -sf "$INSTALL_DIR/$BINARY_NAME" "$BIN_DIR/$BINARY_NAME"
    
    log_message "✓ Installed to $INSTALL_DIR/$BINARY_NAME"
    log_message "✓ Global command created: $BINARY_NAME"
}

# Cleanup build files
cleanup() {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        log_message "Cleaned up build directory"
    fi
}

# Uninstall
uninstall_spane() {
    log_message "Uninstalling SPANE engine..."
    
    [ -L "$BIN_DIR/$BINARY_NAME" ] && rm -f "$BIN_DIR/$BINARY_NAME"
    [ -d "$INSTALL_DIR" ] && rm -rf "$INSTALL_DIR"
    
    log_message "✓ SPANE engine uninstalled"
}

# Show help
show_help() {
    echo "SPANE Game Engine - Installation Script"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --install     Install SPANE engine (default)"
    echo "  --uninstall   Uninstall SPANE engine"
    echo "  --help        Show this help"
    echo ""
    echo "After installation, run: spane"
}

# =============================================================================
# MAIN EXECUTION
# =============================================================================

# Parse arguments
case "${1:-}" in
    --uninstall|-u)
        uninstall_spane
        exit 0
        ;;
    --help|-h)
        show_help
        exit 0
        ;;
esac

echo ""
echo "╔══════════════════════════════════════════╗"
echo "║     SPANE Game Engine - Installer        ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# Check for existing installation
if [ -d "$INSTALL_DIR" ]; then
    log_message "Existing installation found"
    printf "Choose: [1]=Update [2]=Remove [3]=Exit: "
    read choice
    case "$choice" in
        1) log_message "Updating..."; rm -rf "$INSTALL_DIR"; [ -L "$BIN_DIR/$BINARY_NAME" ] && rm -f "$BIN_DIR/$BINARY_NAME" ;;
        2) uninstall_spane; exit 0 ;;
        *) exit 0 ;;
    esac
fi

# Install dependencies
install_dependencies

# Compile
if compile_spane; then
    install_binary
    cleanup
    
    echo ""
    echo "╔══════════════════════════════════════════╗"
    echo "║       Installation Complete!             ║"
    echo "║                                          ║"
    echo "║  Run with: spane                         ║"
    echo "║                                          ║"
    echo "║  Controls:                               ║"
    echo "║  - Arrow Keys/WASD: Move                 ║"
    echo "║  - F1/F2: Switch games                   ║"
    echo "║  - Click sidebar: Switch games           ║"
    echo "║  - ESC: Quit                             ║"
    echo "║                                          ║"
    echo "╚══════════════════════════════════════════╝"
    echo ""
else
    log_message "Compilation failed!"
    cleanup
    exit 1
fi