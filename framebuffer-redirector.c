#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

extern void update_framebuffer(uint32_t* framebuffer);

void redirect_framebuffer(uint32_t* framebuffer) {
    // Pass the framebuffer to the VNC server
    update_framebuffer(framebuffer);
}
