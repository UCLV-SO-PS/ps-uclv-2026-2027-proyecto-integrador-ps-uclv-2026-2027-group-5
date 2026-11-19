#include "monitor.h"
#include <stdio.h>
#include <string.h>

static Episode db[6];
static int initialized = 0;

static const Episode seed[6] = {
    {"S01E01", "The Hedge Knight", 9.2, 45120L},
    {"S01E02", "Hard Salt Beef", 8.8, 38900L},
    {"S01E03", "The Squire", 9.5, 52400L},
    {"S01E04", "Seven", 9.1, 41200L},
    {"S01E05", "In the Name of the Mother", 9.7, 68300L},
    {"S01E06", "The Morrow", 9.4, 49800L}
};

void monitor_init(void) {
    if (!initialized) {
        memcpy(db, seed, sizeof(seed));
        initialized = 1;
    }
}

const Episode *monitor_find(const char *code) {
    int i;
    monitor_init();
    if (code == NULL) return NULL;
    for (i = 0; i < 6; i++) {
        if (strcmp(db[i].code, code) == 0) {
            return &db[i];
        }
    }
    return NULL;
}

static void format_votes(long value, char *out, size_t out_size) {
    char tmp[32];
    size_t len, i, j = 0;

    if (out_size == 0) return;

    snprintf(tmp, sizeof(tmp), "%ld", value);
    len = strlen(tmp);

    for (i = 0; i < len; i++) {
        if (i > 0 && ((len - i) % 3 == 0) && j + 1 < out_size) {
            out[j++] = ',';
        }
        if (j + 1 < out_size) {
            out[j++] = tmp[i];
        }
    }
    out[j] = '\0';
}

int monitor_format(const char *code, char *out, size_t out_size) {
    const Episode *ep;
    char votes[32];

    if (out == NULL || out_size == 0) return -1;

    ep = monitor_find(code);
    if (ep == NULL) {
        snprintf(out, out_size, "ERROR: Episodio '%s' no encontrado\n",
                 code ? code : "");
        return -1;
    }

    format_votes(ep->votes, votes, sizeof(votes));

    return snprintf(out, out_size,
                    "Código: %s\n"
                    "Título: %s\n"
                    "Puntuación IMDB: %.1f\n"
                    "Votos registrados: %s\n",
                    ep->code, ep->title, ep->rating, votes);
}