#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rfb/rfb.h>
#include <sys/time.h>

#define WIDTH 320
#define HEIGHT 240
#define BPP 4  // 32-bit color depth (RGBA)

static rfbScreenInfoPtr server;

void update_framebuffer(uint32_t* framebuffer) {
    // Update the framebuffer content for the VNC server
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            uint32_t color = framebuffer[y * WIDTH + x];
            server->frameBuffer[(y * WIDTH + x) * BPP] = (color >> 16) & 0xFF;  // Red
            server->frameBuffer[(y * WIDTH + x) * BPP + 1] = (color >> 8) & 0xFF;  // Green
            server->frameBuffer[(y * WIDTH + x) * BPP + 2] = color & 0xFF;  // Blue
            server->frameBuffer[(y * WIDTH + x) * BPP + 3] = 0xFF;  // Alpha (opaque)
        }
    }
    rfbMarkRectAsModified(server, 0, 0, WIDTH, HEIGHT);  // Notify the VNC server about the update
}

int main(int argc, char *argv[]) {
    // Initialize the VNC server
    server = rfbGetScreen(&argc, argv, WIDTH, HEIGHT, 8, 3, BPP);
    if (server == NULL) {
        fprintf(stderr, "Failed to initialize VNC server\n");
        return 1;
    }

    server->frameBuffer = (char*)malloc(WIDTH * HEIGHT * BPP);  // Allocate framebuffer memory
    if (server->frameBuffer == NULL) {
        fprintf(stderr, "Failed to allocate framebuffer\n");
        return 1;
    }

    rfbInitServer(server);

    while (1) {
        rfbProcessEvents(server, 100);  // Process events and send framebuffer updates to clients
    }

    return 0;
}
