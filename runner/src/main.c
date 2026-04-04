#include "align.h"
#include "arena.h"
#include "lexer.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>



int main(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: %s SCRIPTFILE\n", basename(argv[0]));
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        printf("could not open file: %s [%d]\n", strerror(errno), errno);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    uintptr_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buf = malloc(fsize + 1);
    assert(buf != NULL);
    fread(buf, fsize, 1, fp);
    buf[fsize] = '\0';

    void* tokens = NULL;
    yip_lexer_lex(buf, &tokens);

    free(buf);

    return 0;
}
