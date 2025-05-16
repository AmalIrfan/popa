#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct popa_state popa_state;

typedef struct popa_word {
    char name[256];
    int(* func)(popa_state*);
} popa_word;

#define POPA_WORDS 10

typedef struct popa_state {
    const char* token;
    enum {
        POPA_START,
        POPA_FAIL,
        POPA_CAT,
        POPA_EXIT
    } stage;
    popa_word words[POPA_WORDS];
    int end;
} popa_state;


int popa_init(popa_state* popa);
int popa_interpret(popa_state* popa);
int popa_try(popa_state* popa);
const char* popa_next(popa_state* popa);
int _popa_skipspaces(popa_state* popa);
int _popa_read_nonspace(popa_state* popa, char *buf, int size);

popa_word popa_word_make(const char* name, int(* func)(popa_state*));

int popa_word_cat(popa_state* popa);

int main() {
    popa_state popa = {0};
    popa_init(&popa);
    popa_interpret(&popa);
}

int popa_init(popa_state* popa) {
    popa->stage = POPA_START;
    popa->words[popa->end++] = popa_word_make("cat", popa_word_cat);
    return 0;
}

int popa_interpret(popa_state* popa) {
    while (popa->stage != POPA_EXIT) {
        switch (popa->stage) {
        case POPA_START:
            if (popa_try(popa) >= 0)
                (void)0;
            else
                popa->stage = POPA_FAIL;
            break;
        case POPA_FAIL:
            fprintf(stderr, "Error: token `%s`\n", popa->token);
            popa->stage = POPA_EXIT;
            break;
        default:
            fprintf(stderr, "Error: unrecognised stage\n");
            break;
        }
    }
    return 0;
}

popa_word popa_word_make(const char* name, int(* func)(popa_state*)) {
    popa_word word = {0};
    strcpy(word.name, name);
    word.func = func;
    return word;
}

int popa_try(popa_state* popa) {
    int i = 0;
    popa->token = popa_next(popa);
    for (i = popa->end - 1; i >= 0; i--) {
        if (strcmp(popa->words[i].name, popa->token) == 0) {
            return popa->words[i].func(popa);
        }
    }
    return -1;
}

const char* popa_next(popa_state* popa) {
    static char buf[256];
    int n = 0;
    _popa_skipspaces(popa);
    n = _popa_read_nonspace(popa, buf, 255);
    buf[n] = 0;
    return buf;
}

int _popa_skipspaces(popa_state* popa) {
    char ch = getc(stdin);
    int i = 0;
    while (isspace(ch) && ch > 0) {
        i++;
        ch = getc(stdin);
    }
    if (!isspace(ch) && ch > 0) {
        ungetc(ch, stdin);
    }
    return i;
}

int _popa_read_nonspace(popa_state* popa, char *buf, int size) {
    char ch = getc(stdin);
    int i = 0;
    while (!isspace(ch) && ch > 0 && i < size) {
        buf[i++] = ch;
        ch = getc(stdin);
    }
    if (isspace(ch) || (ch > 0 && i >= size)) {
        ungetc(ch, stdin);
    }
    return i;
}

int popa_word_cat(popa_state* popa) {
    const char* filename = popa_next(popa);
    FILE* fh = fopen(filename, "r");
    char ch = 0;
    if (!fh) {
        perror(filename);
        return 1;
    }
    ch = getc(fh);
    while (ch > 0) {
        putc(ch, stdout);
        ch = getc(fh);
    }
    fclose(fh);
    return 0;
}
