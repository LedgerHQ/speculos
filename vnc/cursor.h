#ifndef CURSOR_H
#define CURSOR_H

#define bits_per_sample   8
#define samples_per_pixel 3
#define bytes_per_pixel   4

void set_cursor(rfbScreenInfoPtr screen, char *p);
void set_cursor_from_keysym(rfbScreenInfoPtr screen, unsigned int index);
void handle_konami_code(rfbBool down, rfbKeySym keySym);

extern rfbScreenInfoPtr screen;

#endif
