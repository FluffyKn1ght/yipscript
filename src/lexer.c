#include "lexer.h"
#include "arena.h"
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "align.h"

typedef struct {
    char* src;
    uint64_t src_length;
    uint64_t src_offset;

    char* buffer;
    uintptr_t buf_capacity;

    char* ident_buf;
    uintptr_t ident_buf_capacity;
    uint32_t ident_buf_idents;
    uintptr_t last_ident_end;

    void** output_ptr;

    bool last_tkn_was_nop;

    char* identtbl_last_buf;
    uintptr_t identtbl_last_offset;
    uintptr_t identtbl_last_idx;
} yip_lexer;
static yip_lexer lexer = {};

#define CLEAR_BUF() memset(lexer.buffer, 0, lexer.buf_capacity)
#define BUF_MATCHES(what) strcmp(lexer.buffer, (what)) == 0
#define CASE_CHAR_TKN(c, t) \
    case (c): { \
        _push_token((t), false); \
        lexer.last_tkn_was_nop = false; \
        break; \
    }

static inline bool _isalnum(char c) {
    return isalnum(c) != 0;
}

static inline bool _isnum(char c) {
    return c > 47 && c < 58;
}

static inline bool _isspecial(char c) {
    return !_isalnum(c) || c == '\n' || c == ' ';
}

static inline bool _isempty(char c) {
    return c == ' ' || c == '\n' || c == '\0';
}

static void _buf_ensure(uintptr_t capacity) {
    if (capacity > lexer.buf_capacity) {
        uintptr_t old_capacity = lexer.buf_capacity;
        lexer.buf_capacity = capacity;
        lexer.buffer = realloc(lexer.buffer, lexer.buf_capacity);
        assert(lexer.buffer != NULL);
        memset(lexer.buffer + old_capacity, 0, lexer.buf_capacity - old_capacity);
    }
}

static void _buf_read_n(uintptr_t chars) {
    if (lexer.src_offset + chars >= lexer.src_length) {
        chars = lexer.src_length - lexer.src_offset;
    }

    char* buf = malloc(chars + 1);
    strncpy(buf, lexer.src + lexer.src_offset, chars);
    lexer.src_offset += chars;
    buf[chars] = '\0';

    _buf_ensure(lexer.buf_capacity + chars);
    lexer.buffer = strcat(lexer.buffer, buf);
    free(buf);
}

static void _buf_read_until_cond(bool(*condition)(char)) {
    uint64_t offset = 0;

    while (true) {
        if (condition(lexer.src[lexer.src_offset + offset])) {
            break;
        }
        offset++;
        if (lexer.src_offset + offset >= lexer.src_length) {
            break;
        }
    }

    _buf_read_n(offset);
}

static void _buf_read_until_char(char c) {
    uint64_t offset = 0;

    while (lexer.src_offset + offset < lexer.src_length) {
        if (lexer.src[lexer.src_offset + offset] == c) {
            //offset++;
            break;
        }
        offset++;
    }

    _buf_read_n(offset);
}

static void _push_token(yip_lexer_token_type_t type, bool include_buffer) {
    yip_lexer_token_t* tkn = arena_alloc(
        lexer.output_ptr,
        sizeof(yip_lexer_token_t) + (include_buffer ? strlen(lexer.buffer) + 2 : 1)
    );
    tkn->type = type;
    if (include_buffer) {
        strcpy(tkn->data, lexer.buffer);
        tkn->data[strlen(lexer.buffer) + 1] = YIP_TOKEN_DATATERM;
    } else {
        tkn->data[0] = YIP_TOKEN_DATATERM;
    }
}

char* yip_lexer_get_identtable_string(char* idtable_data, int idx);
static void _push_ident() {
    bool is_in_identbuf = false;
    uint32_t i;
    char* ident = NULL;
    for (i = 0; i < lexer.ident_buf_idents; i++) {
        ident = yip_lexer_get_identtable_string(lexer.ident_buf, i);
        if (strcmp(ident, lexer.buffer) == 0) {
            is_in_identbuf = true;
            break;
        }
        free(ident);
        ident = NULL;
    }
    if (ident) {
        free(ident);
    }

    if (!is_in_identbuf) {
        uint64_t ident_size = strlen(lexer.buffer);

        if (lexer.ident_buf_capacity < lexer.last_ident_end + ident_size + 1) {
            lexer.ident_buf_capacity += (ident_size + 1 > 64 ? ident_size + 1 : 64);
            lexer.ident_buf = realloc(lexer.ident_buf, lexer.ident_buf_capacity);
            assert(lexer.ident_buf != NULL);
        }

        strcpy(lexer.ident_buf + lexer.last_ident_end, lexer.buffer);
        i = lexer.ident_buf_idents;
        lexer.ident_buf_idents++;
        lexer.last_ident_end += ident_size + 1;
    }

    yip_lexer_token_t* ident_tkn = arena_alloc(lexer.output_ptr, sizeof(yip_lexer_token_t) + sizeof(uint32_t) + 1);
    ident_tkn->type = YIP_TOKEN_IDENTIFIER;
    memcpy(ident_tkn->data, &i, sizeof(uint32_t));
    ident_tkn->data[sizeof(uint32_t)] = YIP_TOKEN_DATATERM;
}

void yip_lexer_lex(const char* src, void** out) {
    if (lexer.src) {
        free(lexer.src);
    }
    if (lexer.buffer) {
        free(lexer.buffer);
    }
    if (lexer.ident_buf) {
        free(lexer.ident_buf);
    }

    memset(&lexer, 0, sizeof(yip_lexer));

    lexer.buf_capacity = 32;
    lexer.buffer = calloc(1, lexer.buf_capacity);
    assert(lexer.buffer != NULL);

    lexer.src_length = strlen(src);
    lexer.src = malloc(lexer.src_length);
    assert(lexer.src != NULL);
    memcpy(lexer.src, src, lexer.src_length);

    lexer.ident_buf_capacity = 64;
    lexer.ident_buf = calloc(64, lexer.ident_buf_capacity);
    assert(lexer.ident_buf != NULL);

    lexer.output_ptr = out;

    _push_token(YIP_TOKEN_NOP, false);

    while (true) {
        if (lexer.src_offset >= lexer.src_length) {
            _push_token(YIP_TOKEN_EOF, false);
            break;
        }

        CLEAR_BUF();
        _buf_read_until_cond(_isspecial);

        if (strlen(lexer.buffer)) {
            if (BUF_MATCHES("var")) {
                _push_token(YIP_TOKEN_KW_VAR, false);
            } else if (BUF_MATCHES("const")) {
                _push_token(YIP_TOKEN_KW_CONST, false);
            } else if (BUF_MATCHES("if")) {
                _push_token(YIP_TOKEN_KW_IF, false);
            } else if (BUF_MATCHES("else")) {
                _push_token(YIP_TOKEN_KW_ELSE, false);
            } else if (BUF_MATCHES("func")) {
                _push_token(YIP_TOKEN_KW_FUNC, false);
            } else if (BUF_MATCHES("class")) {
                _push_token(YIP_TOKEN_KW_CLASS, false);
            } else if (BUF_MATCHES("is")) {
                _push_token(YIP_TOKEN_KW_IS, false);
            } else if (BUF_MATCHES("in")) {
                _push_token(YIP_TOKEN_KW_IN, false);
            } else if (BUF_MATCHES("for")) {
                _push_token(YIP_TOKEN_KW_FOR, false);
            } else if (BUF_MATCHES("while")) {
                _push_token(YIP_TOKEN_KW_WHILE, false);
            } else if (BUF_MATCHES("do")) {
                _push_token(YIP_TOKEN_KW_CONST, false);
            } else if (BUF_MATCHES("continue")) {
                _push_token(YIP_TOKEN_KW_CONTINUE, false);
            } else if (BUF_MATCHES("break")) {
                _push_token(YIP_TOKEN_KW_BREAK, false);
            } else if (BUF_MATCHES("static")) {
                _push_token(YIP_TOKEN_KW_STATIC, false);
            } else if (BUF_MATCHES("enum")) {
                _push_token(YIP_TOKEN_KW_ENUM, false);
            } else if (BUF_MATCHES("return")) {
                _push_token(YIP_TOKEN_KW_RETURN, false);
            } else {
                if (_isnum(lexer.buffer[0])) {
                    char* buf2 = strdup(lexer.buffer);
                    CLEAR_BUF();
                    _buf_ensure(sizeof(lexer.buffer));
                    sscanf(buf2, "%d", (uint32_t*)lexer.buffer);
                    _push_token(YIP_TOKEN_LIT_NUMBER, true);
                    free(buf2);
                } else {
                    bool empty = true;
                    uint64_t size = strlen(lexer.buffer);
                    for (int i = 0; i < size; i++) {
                        if (!_isempty(lexer.buffer[i])) {
                            empty = false;
                            break;
                        }
                    }

                    if (!empty) {
                        _push_ident();
                    } else {
                        _push_token(YIP_TOKEN_NOP, false);
                    }
                }
            }
        } else {
            switch (lexer.src[lexer.src_offset]) {
                CASE_CHAR_TKN(':', YIP_TOKEN_COLON)
                CASE_CHAR_TKN('.', YIP_TOKEN_DOT)
                CASE_CHAR_TKN('(', YIP_TOKEN_BRACKET_OPEN)
                CASE_CHAR_TKN(')', YIP_TOKEN_BRACKET_CLOSED)
                CASE_CHAR_TKN('[', YIP_TOKEN_SQUAREBRACKET_OPEN)
                CASE_CHAR_TKN(']', YIP_TOKEN_SQUAREBRACKET_CLOSED)
                CASE_CHAR_TKN('{', YIP_TOKEN_CURLYBRACKET_OPEN)
                CASE_CHAR_TKN('}', YIP_TOKEN_CURLYBRACKET_CLOSED)
                CASE_CHAR_TKN(',', YIP_TOKEN_COMMA)
                CASE_CHAR_TKN('?', YIP_TOKEN_QUESTIONMARK)
                CASE_CHAR_TKN('!', YIP_TOKEN_EXCLAMATIONMARK)
                CASE_CHAR_TKN('@', YIP_TOKEN_ATSYMBOL)
                CASE_CHAR_TKN('#', YIP_TOKEN_HASHTAG)
                CASE_CHAR_TKN('+', YIP_TOKEN_PLUS)
                CASE_CHAR_TKN('-', YIP_TOKEN_MINUS)
                CASE_CHAR_TKN('*', YIP_TOKEN_STAR)
                CASE_CHAR_TKN('/', YIP_TOKEN_SLASH)
                CASE_CHAR_TKN('^', YIP_TOKEN_CARET)
                CASE_CHAR_TKN('&', YIP_TOKEN_AMPERSAND)
                CASE_CHAR_TKN('|', YIP_TOKEN_PIPE)
                CASE_CHAR_TKN('~', YIP_TOKEN_TILDE)
                CASE_CHAR_TKN('=', YIP_TOKEN_EQUALS)
                case ' ': {
                    if (!lexer.last_tkn_was_nop) {
                        _push_token(YIP_TOKEN_NOP, false);
                        lexer.last_tkn_was_nop = true;
                    }
                    break;
                }
                case '"': {
                    CLEAR_BUF();
                    lexer.src_offset++;
                    _buf_read_until_char('"');
                    _push_token(YIP_TOKEN_LIT_STRING, true);
                    break;
                }
                case '\'': {
                    CLEAR_BUF();
                    lexer.src_offset++;
                    _buf_read_until_char('"');
                    _push_token(YIP_TOKEN_LIT_STRING, true);
                    break;
                }
            }
        }

        lexer.src_offset++;
    }

    yip_lexer_token_t* identtbl = arena_alloc(lexer.output_ptr, sizeof(yip_lexer_token_t) + lexer.last_ident_end + 1);
    identtbl->type = YIP_TOKEN_IDENTTABLE;
    memcpy(identtbl->data, lexer.ident_buf, lexer.last_ident_end);
    identtbl->data[lexer.last_ident_end] = YIP_TOKEN_DATATERM;

    free(lexer.src);
    free(lexer.buffer);
    free(lexer.ident_buf);
}

char* yip_lexer_get_identtable_string(char* idtable_data, int idx) {
    if (idtable_data != lexer.identtbl_last_buf) {
        lexer.identtbl_last_buf = idtable_data;
        lexer.identtbl_last_offset = 0;
        lexer.identtbl_last_idx = 0;
    }

    int curidx = idx >= lexer.identtbl_last_idx ? idx : 0;
    uintptr_t offset = lexer.identtbl_last_offset;
    while (curidx < idx) {
        if ((uint8_t)idtable_data[offset] == YIP_TOKEN_DATATERM) {
            goto fail;
        } else if (idtable_data[offset] == '\0') {
            curidx++;
        }

        offset++;
    }

    uintptr_t start = (uintptr_t)offset;
    uint64_t size = 0;
    while (true) {
        if ((uint8_t)idtable_data[start + size] == 0xFF) {
            goto fail;
        } else if (idtable_data[start + size] == '\0') {
            break;
        }
        size++;
    }

    char* buf = malloc(size + 1);
    strcpy(buf, idtable_data + start);
    return buf;

    fail:
    lexer.identtbl_last_buf = NULL;
    lexer.identtbl_last_offset = 0;
    lexer.identtbl_last_idx = 0;
    return NULL;
}
