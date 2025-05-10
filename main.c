#include <stdio.h>
#include <ctype.h>
#include <string.h>


#define DEBUG 1

#define STACK 10
#define RSTACK 10

int stack[STACK];
int rstack[RSTACK];
int sp = 0;
int rp = 0;

struct item {
    const char* word;
    int address;
};

#define MTABLE 10
struct item mtable[MTABLE] = {};
int mtp = 0;

#define STABLE 100
char stable[STABLE];
int stp = 0;

enum {COMPILE, INTREPRET} pmode = INTREPRET;

/* VM */

#define FOREACH_BYTECODE(f) \
    f(CALL) \
    f(EXIT) \
    f(LIT) \
    f(IF) \
    f(UNTIL) \
    f(DUP) \
    f(DROP) \
    f(SWAP) \
    f(OVER) \
    f(SUB) \
    f(EQ) \
    f(OR) \
    f(NOT) \
    f(READ) \
    f(WRITE)

#define F(b) BC_##b,
enum bytecode {
    FOREACH_BYTECODE(F)
    _BC_END
};
#undef F
#define F(b) #b,
const char* bytecode_str[] = {
    FOREACH_BYTECODE(F)
    0
};
#undef F

#define CODE 50
int code[CODE] = {};
int pc = 0;
/* by my convention this is code pointer which is the index into code array
   but pc sounds better */


int intrinsic();
int execute(int address);
int compile(const char* word);
int intrepret(const char* word);

const char* get_next();

FILE* fp = NULL;

void code_push(int b) {
    if (pc < CODE) {
        code[pc] = b;
        pc++;
    } else
        printf("code overflow\n");
}

int code_get(int a) {
    if (a > 0 && a < CODE) {
        return code[a];
    } else
        printf("code overflow\n");
}

void stack_push(int v) {
    if (sp < STACK) {
        stack[sp] = v;
        sp++;
    } else
        printf("stack overflow\n");
}

int stack_view() {
    if (sp > 0) {
        return stack[sp - 1];
    } else
        printf("stack underflow\n");
}

int stack_pop() {
    if (sp > 0) {
        sp--;
        return stack[sp];
    } else
        printf("stack underflow\n");
}

void rstack_push(int v) {
    if (rp < STACK) {
        rstack[rp] = v;
        rp++;
    } else
        printf("rstack overflow\n");
}

int rstack_view() {
    if (rp > 0) {
        return rstack[rp - 1];
    } else
        printf("rstack underflow\n");
}

int rstack_pop() {
    if (rp > 0) {
        rp--;
        return rstack[rp];
    } else
        printf("rstack underflow\n");
}

int loop() {
    const char *word = 0;
    int r = 0;
    while(( word = get_next(fp) )){
        if (r > 0 && ! word[0]) {
            printf(" no.\n");
            r = 0;
            continue;
        }
        else if (r == 0 && ! word[0]) {
            printf(" ok.\n");
            continue;
        }
        else if (r < 0)
            return 1;

        switch(pmode) {
        case COMPILE:
            r = compile(word);
            break;
        case INTREPRET:
            r = intrepret(word);
            break;
        }
    }
    return 0;
}

int main() {
    int r = 0;
    if(( r = intrinsic() ))
        return r;
    while(1) {
        r = loop();
        if( fp == NULL )
            break;
        else {
            fclose(fp);
            fp = NULL;
        }
    }
    return r;
}

#define WORDMAX 10
const char* get_next(FILE* fp)
{
    static char buf[WORDMAX];
    static int i = WORDMAX;
    /* reset internal buffer */
    if(i) {
        memset(buf, 0, i);
        i = 0;
    }
    if( ! fp) {
        fp = stdin;
    }
    char ch = 0;
    /* skip spaces */
    do {
        ch = getc(fp);
        /* return empty to intrepreter on newline */
        if (pmode == INTREPRET && ch == '\n') {
            buf[i++] = 0;
            return buf;
        }
    } while(ch > 0 && isspace(ch) );
    /* no word EOF */
    if(! (ch > 0) )
        return NULL;
    /* capture until next space */
    while(ch > 0 && ! isspace(ch) ){
        buf[i++] = ch;
        ch = getc(fp);
        /* save newline for intrepreter */
        if (pmode == INTREPRET && ch == '\n')
            ungetc(ch, fp);
    }
    /* add null terminator */
    buf[i++] = 0;
    return buf;
}

int is_number(const char* word)
{
    char ch = 0;
    /* do loop to make sure first char is not null */
    ch = *word;
    do {
        if( ! isdigit(ch) )
            return 0;
        word++;
    /* continue till null */
    } while(( ch = *word ));

    return 1;
}

int make_number(const char* word)
{
    int n = 0;
    char ch = 0;
    while(( ch = *word )) {
        n = n * 10 + ch - '0';
        word++;
    }
    return n;
}

void fprint_stacks(FILE* fp)
{
    int i = 0;
    fprintf(fp, "[");
    while(i + 1 < sp) {
        fprintf(fp, "%d ", stack[i]);
        i++;
    }
    if(i < sp)
        fprintf(fp, "%d", stack[i]);
    fprintf(fp, "] r[");
    i = 0;
    while(i + 1 < rp) {
        fprintf(fp, "%d ", rstack[i]);
        i++;
    }
    if(i < rp)
        fprintf(fp, "%d", rstack[i]);
    fprintf(fp, "]");
}

#define print_stacks() fprint_stacks(stdout)

void fprint_mtable(FILE* fp) {
    for (int i = 0; i < mtp; i++) {
        fprintf(fp, "%s -> %3d\n", mtable[i].word, mtable[i].address);
    }
}

#define print_mtable() fprint_mtable(stdout)

int execute(int address) {
    int i = 0;
    do {
        int top = 0;
        int prev = 0;
        int bc = code_get(address);
#if DEBUG
        fprintf(stderr, "%3d ", address);
        fprint_stacks(stderr);
        switch(bc) {
        case BC_EXIT:
        case BC_DUP:
        case BC_DROP:
        case BC_SWAP:
        case BC_OVER:
        case BC_SUB:
        case BC_EQ:
        case BC_NOT:
        case BC_OR:
        case BC_READ:
        case BC_WRITE:
            fprintf(stderr, " %s\n", bytecode_str[bc]);
            break;
        case BC_LIT:
        case BC_CALL:
        case BC_UNTIL:
        case BC_IF:
            fprintf(stderr, " %s %3d\n", bytecode_str[bc], code_get(address + 1));
            break;
        default:
            fprintf(stderr, "ERR\n");
        }
#endif
        switch(bc) {
        case BC_CALL:
            rstack_push(address + 2);
            address = code_get(address + 1);
            break;
        case BC_LIT:
            address++;
            stack_push(code_get(address));
            address++;
            break;
        case BC_EXIT:
            /* return to intrepreter? */
            if(rstack_view() == -1) {
                rstack_pop();
                return 1; /* ok. */
            }
            else
                address = rstack_pop();
            break;
        case BC_IF:
        case BC_UNTIL:
            address++;
            /* jump if false */
            if( ! stack_pop())
                address = code_get(address);
            else
                address++;
            break;
        case BC_DUP:
            stack_push(stack_view());
            address++;
            break;
        case BC_DROP:
            stack_pop();
            address++;
            break;
        case BC_SWAP:
            top = stack_pop();
            prev = stack_pop();
            stack_push(top);
            stack_push(prev);
            address++;
            break;
        case BC_OVER:
            top = stack_pop();
            prev = stack_view();
            stack_push(top);
            stack_push(prev);
            address++;
            break;
        case BC_SUB:
            top = stack_pop();
            stack_push(stack_pop() - top);
            address++;
            break;
        case BC_EQ:
            top = stack_pop();
            stack_push(stack_pop() == top);
            address++;
            break;
        case BC_OR:
            top = stack_pop();
            stack_push(stack_pop() || top);
            address++;
            break;
        case BC_NOT:
            stack_push( ! stack_pop() );
            address++;
            break;
        case BC_READ:
            stack_push( getc(stdin) );
            address++;
            break;
        case BC_WRITE:
            putc(stack_pop(),stdout);
            address++;
            break;
        default:
            return -1; /* no. */
        }
    } while( address < CODE && i++ < 200);
    return -1; /* no. */
}

int table_find(const char* word)
{
    int i = 0;
    while(i < mtp) {
        if (strcmp(mtable[i].word, word) == 0) {
            return mtable[i].address;
        }
        i++;
    }
    return -1;
}

int table_try(const char* word)
{
    int address = table_find(word);
    if (address < 0)
        return 0;
#if DEBUG
    fprintf(stderr, "executing %s.\n", word);
#endif
    /* return to intrepreter */
    rstack[rp] = -1;
    rp++;
    return execute(address);
}

int open_file(const char* filename) {
    fp = fopen(filename, "r");
    if( ! fp)
        return 1;
    return 0;
}

int intrepret(const char* word)
{
    int r = 0;
    if(is_number(word))
        stack[sp++] = make_number(word);
    else if(strcmp(word, ":") == 0)
        pmode = COMPILE;
    else if(( r=table_try(word) ))
        r = r < 0 ? r : 0;
    else if(strcmp(word, "print") == 0)
        print_stacks();
    else if(strcmp(word, "load") == 0)
        r = open_file(get_next(fp));
    else if(strcmp(word, "words") == 0)
        print_mtable();
    else
        r = 1;
    return r;
}

const char *cword = 0;

int init_compile(const char* word)
{
    int l = strlen(word) + 1;
    char *new = stable + stp;
    if (mtp >= MTABLE) {
        printf("table overflow\n");
        return -1;
    }
    stp += l;
    if (stp >= STABLE) {
        printf("table overflow\n");
        return -1;
    }
    memcpy(new, word, l);
    cword = new;
    mtable[mtp].word = cword;
    mtable[mtp].address = pc;
    mtp++;
#if DEBUG
    fprintf(stderr, "compiling %s. ", cword);
#endif
    return 0;
}

int fini_compile(int r)
{
    if (r != -1) {
        code_push(BC_EXIT);
#if DEBUG
        fprintf(stderr, "compiled. \n");
        fprint_mtable(stderr);
#endif
        if (r == 0)
            printf(" ok.\n");
        r = 0;
    } else {
        printf(" no.\n");
    }
    pmode = INTREPRET;
    cword = 0;
    return r;
}

int compile(const char* word)
{
    int r = 0;
    if (! cword)
        r = init_compile(word);
    else if(is_number(word)) {
        code_push(BC_LIT);
        code_push(make_number(word));
    }
    else if (strcmp(word, "if") == 0) {
        code_push(BC_IF);
        rstack_push(pc);
        code_push(0);
    }
    else if (strcmp(word, "then") == 0) {
        code[rstack_pop()] = pc;
    }
    else if (strcmp(word, "begin") == 0) {
        rstack_push(pc);
    }
    else if (strcmp(word, "until") == 0) {
        code_push(BC_UNTIL);
        code_push(rstack_pop());
    }
    else if (strcmp(word, "exit") == 0) {
        code_push(BC_EXIT);
    }
    else if (strcmp(word, "dup") == 0) {
        code_push(BC_DUP);
    }
    else if (strcmp(word, "drop") == 0) {
        code_push(BC_DROP);
    }
    else if (strcmp(word, "swap") == 0) {
        code_push(BC_SWAP);
    }
    else if (strcmp(word, "over") == 0) {
        code_push(BC_OVER);
    }
    else if (strcmp(word, "sub") == 0) {
        code_push(BC_SUB);
    }
    else if (strcmp(word, "eq") == 0) {
        code_push(BC_EQ);
    }
    else if (strcmp(word, "or") == 0) {
        code_push(BC_OR);
    }
    else if (strcmp(word, "not") == 0) {
        code_push(BC_NOT);
    }
    else if (strcmp(word, "read") == 0) {
        code_push(BC_READ);
    }
    else if (strcmp(word, "write") == 0) {
        code_push(BC_WRITE);
    }
    /* hacky way to avoid -1 return */
    else if(( r = table_find(word) ) >= 0 || (r = 0)) {
        code_push(BC_CALL);
        code_push(r);
    }
    else if (strcmp(word, ";") == 0)
        r = fini_compile(0);
    else {
#ifdef DEBUG
        fprintf(stderr, "\nerror: %s\n", word);
#endif
        r = fini_compile(-1);
    }
    return r;
}

int intrinsic()
{
    const char* intrinsics[] = {"read", "write", "dup", "drop", "swap", "over", "sub", NULL};
    const char** i = NULL;
    int r = 0;
    for (i = intrinsics; *i != NULL; i++) {
        pmode = COMPILE;
        if(( r = init_compile(*i) ))
            break;
        else if(( r = compile(*i) ))
            break;
        else if(( r = fini_compile(1) ))
            break;
    }
    return r;
}
