#ifndef MONITOR_H
#define MONITOR_H

#include <stddef.h>

typedef struct {
    char code[7];
    char title[128];
    double rating;
    long votes;
} Episode;

void monitor_init(void);
const Episode *monitor_find(const char *code);
int monitor_format(const char *code, char *out, size_t out_size);

#endif