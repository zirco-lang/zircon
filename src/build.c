#include "build.h"

int build_rust_project(const char* source_dir) {
    printf("Building (this may take several minutes)...\n");
    
    char cmd[MAX_CMD];
    snprintf(cmd, sizeof(cmd), "cd '%s' && cargo build --release", source_dir);
    
    int status = system(cmd);
    if (status != 0) {
        fprintf(stderr, "Build failed (exit code: %d)\n", status);
        return -1;
    }
    
    printf("Build complete!\n");
    return 0;
}

int check_cargo(void) {
    char version[256];
    FILE* pipe = popen("cargo --version 2>&1", "r");
    if (!pipe) {
        fprintf(stderr, "cargo not found. Please install Rust from https://rustup.rs/\n");
        return -1;
    }
    
    if (fgets(version, sizeof(version), pipe) != NULL) {
        size_t len = strlen(version);
        if (len > 0 && version[len - 1] == '\n') version[len - 1] = '\0';
        printf("Found cargo: %s\n", version);
    }
    
    int status = pclose(pipe);
    if (status != 0) {
        fprintf(stderr, "cargo --version failed\n");
        return -1;
    }
    
    return 0;
}
