#include <stdint.h>
#include <stdio.h>
#include "svm/svm.h"

uint32_t compiler = SVM_CAL << 24 | /* COMPILER */38 << 8 | SVM_HLT;

struct definition {
    uint8_t name_index;
    uint8_t code_index;
    uint8_t flags;
    uint8_t reserve;
};

#define _DICTIONARY 216

#define DICTIONARY (_DICTIONARY / sizeof(struct definition))

struct definition dictionary[DICTIONARY];
uint8_t dictionary_used = 0;

#define NAMES 292

int8_t names[NAMES];
uint8_t names_used = 0;

#define CODES 512

uint8_t codes[CODES];
uint8_t codes_used = 0;

uint8_t new_name(const int8_t* name) {
    uint8_t i = names_used;
    while (*name) {
        names[names_used++] = *name++;
    }
    names[names_used++] = 0;
    return i;
}

uint8_t new_code(const int8_t* code) {
    uint8_t i = codes_used;
    while (*code) {
        codes[codes_used++] = *code++;
    }
    codes[codes_used++] = 0;
    return i;
}

int define(const int8_t* name, const uint8_t* code, uint8_t flags) {
    struct definition def = {0};
    def.name_index = new_name(name);
    def.code_index = new_code(code);
    def.flags = flags;
    dictionary[dictionary_used++] = def;
    return 0;
}

int define_simple(const int8_t* name, const uint8_t code, uint8_t flags) {
    const uint8_t code_[] = {code, 0};
    return define(name, code_, flags);
}


int main() {
    define_simple("+", SVM_ADD, 0);
    define_simple("-", SVM_SUB, 0);
    define_simple("?", SVM_BNZ, 0);
    define_simple("-?", SVM_BNG, 0);
    define_simple("RET", SVM_RET, 0);
    define_simple("DUP", SVM_DUP, 0);
    define_simple("DROP", SVM_DRP, 0);
    define_simple("OVER", SVM_OVR, 0);
    define_simple("POP", SVM_POP, 0);
    define_simple("PUSH", SVM_PSH, 0);
    define_simple("@", SVM_FCH, 0);
    define_simple("!", SVM_PUT, 0);
    {
        const uint8_t code[] = {SVM_LIT, SVM_CAL, /* GETNEXT */0, /* FIND */ 0, 2, SVM_RET, 0};
        define("CALL", code, 1);
    }
    {
        const uint8_t code[] = {SVM_LIT, SVM_LIT, SVM_CAL, /* GETNEXT */0, 2, SVM_RET, 0};
        define("LIT", code, 1);
    }
    {
        const uint8_t code[] = {SVM_LIT, SVM_LAD, SVM_CAL, /* GETADDR */0, 3, SVM_RET, 0};
        define("LAD", code, 1);
    }
    {
        const uint8_t code[] = {SVM_GET, SVM_RET, 0};
        define("GETNEXT", code, 0);
    }
    {
        const uint8_t code[] = {SVM_LAD, 0xFFFF - 0x301, SVM_FCH, SVM_DRP, SVM_LAD, 0xFFFF - 0x300, SVM_FCH, SVM_RET, 0};
        define("GETCH", code, 0);
    }
    {
        const uint8_t code[] = {SVM_CALL, //};
        define("GETADDR", code, 0);
    }
    {
        const uint8_t code[] = {0};
        define("FIND", code, 0);
    }
    {
        const uint8_t code[] = {0};
        define("COMPILER", code, 0);
    }
    fwrite(&compiler, 1, sizeof(compiler), stdout);
    fwrite(dictionary, 1, _DICTIONARY, stdout);
    fwrite(names, 1, NAMES, stdout);
    fwrite(codes, 1, CODES, stdout);
    for (int i = 0; i < dictionary_used; i++) {
        fprintf(stderr, "%s %hd\n",
            names + dictionary[i].name_index,
            dictionary[i].code_index);
    }
}
