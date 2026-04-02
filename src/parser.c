#include "parser.h"
#include "arena.h"
#include "lexer.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    yip_lexer_token_t* in;
    uintptr_t in_size;
    uintptr_t in_offset;

    yip_parser_ast_node_t** ast_out_ptr;
    uintptr_t ast_out_offset;

    yip_parser_message_t** msg_out_ptr;
    uintptr_t msg_out_offset;

    char** identifiers;
    uint32_t ident_size;
    uint32_t ident_capacity;
} yip_parser_state_t;

static yip_parser_state_t parser = {};

static void _push_message(
    yip_parser_msg_type_t type,
    uintptr_t area_start_offset,
    uintptr_t area_size,
    void* data,
    uintptr_t dsize
) {
    arena_alloc((void**)parser.msg_out_ptr, sizeof(yip_parser_message_t) + dsize + 1);

    yip_parser_message_t* msg = *parser.msg_out_ptr + parser.msg_out_offset;
    msg->type = type;

    msg->area_start_offset = area_start_offset;
    msg->area_size = area_size;

    if (data) {
        msg->data_size = dsize;
        memcpy(msg->data, data, msg->data_size);
        msg->data[msg->data_size] = YIP_PARSEMSG_DATATERM;
    } else {
        msg->data_size = 0;
    }
    parser.msg_out_offset += sizeof(yip_parser_message_t) + dsize + 1;
}

yip_parser_result_t yip_parser_parse(const yip_lexer_token_t* in, yip_parser_ast_node_t** ast_out_ptr, yip_parser_message_t** msg_out_ptr) {
    memset(&parser, 0, sizeof(parser));

    parser.ast_out_ptr = ast_out_ptr;
    parser.msg_out_ptr = msg_out_ptr;

    parser.in = in;
    parser.in_size = arena_sizeof(in);

    #pragma region Parse ident table

        yip_lexer_token_t* ident_table = parser.in + parser.in_offset;
        bool ident_table_found = false;
        while (parser.in_offset < parser.in_size) {
            if (in[parser.in_offset].type == YIP_TOKEN_IDENTTABLE) {
                ident_table_found = true;
                break;
            }
            do {
                ident_table++;
                parser.in_offset++;
            } while (in[parser.in_offset].type != YIP_TOKEN_DATATERM);
            ident_table++;
            parser.in_offset++;
        }

        if (!ident_table_found) {
            _push_message(YIP_PARSEMSG_INTERR_NO_IDENTTABLE_TKN, 0, 0, NULL, 0);
            return (yip_parser_result_t)YIP_PARSERESULT_INTERR;
        }

        uintptr_t ident_tbl_offset = 0;
        while (true) {
            if (parser.ident_capacity <= parser.ident_size) {
                parser.ident_capacity += 8;
                parser.identifiers = realloc(parser.identifiers, parser.ident_capacity * sizeof(char*));
                assert(parser.identifiers != NULL);
            }

            char* ident_str = strdup(ident_table->data + ident_tbl_offset);
            parser.identifiers[parser.ident_size] = ident_str;
            ident_tbl_offset += strlen(ident_table->data + ident_tbl_offset) + 1;
            parser.ident_size++;

            if ((uint8_t)ident_table->data[ident_tbl_offset] == YIP_TOKEN_DATATERM) {
                break;
            }
        }

    #pragma endregion

    for (int i = 0; i < parser.ident_size; i++) {
        printf("'%s'\n", parser.identifiers[i]);
    }

    return (yip_parser_result_t)YIP_PARSERESULT_OK;
}
