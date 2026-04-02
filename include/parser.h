#ifndef _YIPSCRIPT_PARSER_H
#define _YIPSCRIPT_PARSER_H

#include "lexer.h"
#include <stdbool.h>
#include <stdint.h>

#define YIP_PARSEMSG_DATATERM 0xFF

#define YIP_PARSERESULT_OK (uint8_t)0b1
#define YIP_PARSERESULT_WARNS (uint8_t)0b10
#define YIP_PARSERESULT_ERROR (uint8_t)0b100
#define YIP_PARSERESULT_INTERR (uint8_t)0b1000

typedef enum: uint8_t {
    YIP_AST_VARIABLE_DECL,
} yip_parser_ast_node_type_t;

typedef struct {
    uint32_t column;
    uint32_t line;
} yip_parser_location_t;

typedef enum: uint16_t {
    YIP_PARSEMSG_HINT_MASK = 0x0100,
    YIP_PARSEMSG_INFO_MASK = 0x0200,
    YIP_PARSEMSG_WARN_MASK = 0x0400,
    YIP_PARSEMSG_ERR_MASK = 0x0800,
    YIP_PARSEMSG_INTERR_MASK = 0x1000, // internal error

    YIP_PARSEMSG_INFO_OK = 0x0201,

    YIP_PARSEMSG_INTERR_NO_IDENTTABLE_TKN = 0x1001,
} yip_parser_msg_type_t;

typedef union {
    struct {
        bool ok: 1, warns: 1, error: 1, interr: 1;
    };
    uint8_t number;
} yip_parser_result_t;

typedef struct {
    yip_parser_msg_type_t type;
    yip_parser_location_t location;
    uintptr_t area_start_offset;
    uintptr_t area_size;
    uintptr_t data_size;
    char data[];
} yip_parser_message_t;

typedef struct {
    yip_parser_ast_node_type_t type;
    void* next; // yip_parser_ast_node_t*
    union {
        struct {
            uint8_t child_count;
            void* children[]; // yip_parser_ast_node_t*
        };
        struct {
            uintptr_t data_size;
            char data[];
        };
    };
} yip_parser_ast_node_t;

yip_parser_result_t yip_parser_parse(const yip_lexer_token_t* in, yip_parser_ast_node_t** ast_out_ptr, yip_parser_message_t** msg_out_ptr);
void yip_parser_sprint_ast(yip_parser_ast_node_t* ast_root, uint8_t max_depth);
void yip_parser_sprint_msg(yip_parser_message_t* msg, bool file_loc, bool description);

#endif
