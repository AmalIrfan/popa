#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define WORD 23
#define WORDS 32
#define HELP \
    "words defined:\n"\
    "   help  show this message\n"\
    "   exit  exit from the program\n"\
    "   echo  print next word\n"\
    "   ''    make word with spaces eg: 'a b'\n"\
    "   ?     print the return value of previous command\n"\
    "   12    push a number to stack eg: 20\n"\
    "   .     pop and print a number on stack\n"\
    "   +     add two numbers\n"\
    "   -     sub two numbers\n"\
    "   *     mul two numbers\n"\
    "   /     div two numbers\n"\
    "   %     mod two numbers\n"\
    "   dup   duplicate top\n"\
    "   over  duplicate over\n"\
    "   swap  swap top and over\n"\
    "   drop  drop top\n"\
    "   .s    print stack\n"\

#define STACK 512

struct state;

char* getnext(struct state* sp);

struct word {
    char name[WORD + 1];
    int (*func)(struct state*);
};

int initwords(struct state* sp);

struct state {
    char token[WORD + 1];
    struct word words[WORDS];
    int words_index;
    bool running;
    char stack[STACK];
    int sp;
    bool num;
    int ret;
};

int (*findword(struct state* sp))(struct state* sp);
int push(struct state*, int num);
int pop(struct state*);
int makenum(struct state*);

int main() {
    struct state state = {0};

    initwords(&state);

    state.running = true;

    while (state.running) {
        fwrite("> ", 1, 2, stderr);
        if (!getnext(&state)) {
            fprintf(stderr, "\nError: EOF\n");
            break;
        }
        if (state.num) {
            push(&state, makenum(&state));
            continue;
        }
        int (*func)(struct state*) = findword(&state);
        if (func)
            state.ret = func(&state);
        else
            fprintf(stderr, "Error: Unrecognised `%s`\n", state.token);
    }

    return state.ret;
}


char* makestr(struct state* sp) {
    char ch = 0;
    int i = 0;

    ch = getc(stdin);

    while (ch != '\'' && ch > 0 && i <= WORD) {
        sp->token[i++] = ch;
        ch = getc(stdin);
    }

    if (ch > 0 && ch != '\'')
        ungetc(ch, stdin);

    sp->token[i] = 0;

    return sp->token;
}

char* getnext(struct state* sp) {
    char ch = 0;
    int i = 0;

    do {
        ch = getc(stdin);
    } while(isspace(ch));

    sp->num = isdigit(ch);

    if (ch <= 0)
        return NULL;

    if (ch == '\'') {
        return makestr(sp);
    }

    do {
        sp->token[i++] = ch;
        sp->num = sp->num && isdigit(ch);
        ch = getc(stdin);
    } while (!isspace(ch) && ch > 0 && i <= WORD);

    if (ch > 0)
        ungetc(ch, stdin);

    sp->token[i] = 0;

    return sp->token;
}

int push(struct state* sp, int num) {
    sp->stack[sp->sp++] = (num >> 0) & 0xFF;
    sp->stack[sp->sp++] = (num >> 8) & 0xFF;
    sp->stack[sp->sp++] = (num >> 16) & 0xFF;
    sp->stack[sp->sp++] = (num >> 24) & 0xFF;
}

int pop(struct state* sp) {
    int n = 0;
    n |= (sp->stack[--sp->sp] & 0xFF) << 24;
    n |= (sp->stack[--sp->sp] & 0xFF) << 16;
    n |= (sp->stack[--sp->sp] & 0xFF) <<  8;
    n |= (sp->stack[--sp->sp] & 0xFF) <<  0;
    return n;
}

int view(struct state* sp) {
    int n = 0;
    n |= (sp->stack[sp->sp-1] & 0xFF) << 24;
    n |= (sp->stack[sp->sp-2] & 0xFF) << 16;
    n |= (sp->stack[sp->sp-3] & 0xFF) <<  8;
    n |= (sp->stack[sp->sp-4] & 0xFF) <<  0;
    return n;
}

int makenum(struct state* sp) {
    int n = 0;
    char *ch = sp->token;
    while (*ch) {
        n = n * 10 + (*ch) - '0';
        ch++;
    }
    return n;
}

int (*findword(struct state* sp))(struct state* sp) {
    int i;
    for (i = 0; i < sp->words_index; i++)
        if (strcmp(sp->words[i].name, sp->token) == 0)
            return sp->words[i].func;
    return NULL;
}

int _addword(struct state* sp, const char* token, int (*func)(struct state* sp)) {
    struct word word = {0};
    strcpy(word.name, token);
    word.func = func;
    sp->words[sp->words_index++] = word;
    return 0;
}

int exitword(struct state* sp) {
    sp->running = false;
    return 0;
}

int helpword(struct state* sp) {
    fprintf(stderr, "%s", HELP);
    return 0;
}

int echoword(struct state* sp) {
    const char* arg = getnext(sp);
    if (arg)
        printf("%s\n", arg);
    return 0;
}

int qmarkword(struct state* sp) {
    printf("%d\n", sp->ret);
    return 0;
}

int canbinop(struct state* sp) {
    if (sp->sp >= 8)
        return 0;
    return 1;
}

int canuniop(struct state* sp) {
    if (sp->sp >= 4)
        return 0;
    return 1;
}

int addword(struct state* sp) {
    if (canbinop(sp))
        return 1;
    int a = pop(sp);
    int b = pop(sp);
    push(sp, a + b);
    return 0;
}

int subword(struct state* sp) {
    if (canbinop(sp))
        return 1;
    int a = pop(sp);
    int b = pop(sp);
    push(sp, b - a);
    return 0;
}

int mulword(struct state* sp) {
    if (canbinop(sp))
        return 1;
    int a = pop(sp);
    int b = pop(sp);
    push(sp, a * b);
    return 0;
}

int divword(struct state* sp) {
    if (canbinop(sp))
        return 1;
    int a = pop(sp);
    int b = pop(sp);
    push(sp, b / a);
    return 0;
}

int modword(struct state* sp) {
    if (canbinop(sp))
        return 1;
    int a = pop(sp);
    int b = pop(sp);
    push(sp, b % a);
    return 0;
}

int dotword(struct state* sp) {
    if (canuniop(sp))
        return 1;
    int a = pop(sp);
    printf("%d\n", a);
    return 0;
}

int dupword(struct state* sp) {
    if (canuniop(sp))
        return 1;
    int a = view(sp);
    push(sp, a);
    return 0;
}

int overword(struct state* sp) {
    if (canbinop(sp))
        return 1;
    int a = pop(sp);
    int b = view(sp);
    push(sp, a);
    push(sp, b);
    return 0;
}

int swapword(struct state* sp) {
    if (canbinop(sp))
        return 1;
    int a = pop(sp);
    int b = pop(sp);
    push(sp, a);
    push(sp, b);
    return 0;
}

int dropword(struct state* sp) {
    if (canuniop(sp))
        return 1;
    int a = pop(sp);
    return 0;
}

int pstackword(struct state* sp) {
    int i = 0;
    putchar('[');
    for (i = 0; i < sp->sp; i++) {
        if (i != 0) putchar(' ');
        printf("%02x", sp->stack[i]);
    }
    printf("]\n");
    return 0;
}

int initwords(struct state* sp) {
    _addword(sp, "exit", exitword);
    _addword(sp, "help", helpword);
    _addword(sp, "echo", echoword);
    _addword(sp, "?", qmarkword);
    _addword(sp, "+", addword);
    _addword(sp, "-", subword);
    _addword(sp, "*", mulword);
    _addword(sp, "/", divword);
    _addword(sp, "%", modword);
    _addword(sp, ".", dotword);
    _addword(sp, "dup", dupword);
    _addword(sp, "over", overword);
    _addword(sp, "swap", swapword);
    _addword(sp, "drop", dropword);
    _addword(sp, ".s", pstackword);
    return 0;
}
