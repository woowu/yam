/**
 * @file filetype.c
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <string.h>
#include "err.h"
#include "record-file.h"
#include "filetype.h"
#include "stdlib.h"

/**********************
 *   STATIC PROTOTYPES
 **********************/
static int packet_file_read(int type, const char *req_buf, size_t req_len,
        char *resp_buf, size_t resp_buf_sz);
static int packet_file_write(int type, const char *req_buf, size_t req_len,
        char *resp_buf, size_t resp_buf_sz);

/**********************
 *   TYPEDEFS
 **********************/
typedef struct {
    int type_code;
    filetype_t *filetype;
} filetype_map_t;

typedef struct {
    int type_code;
    yam_file_rec_io_t *io;
} rec_io_map_t;

/**********************
 *   STATIC VARIABLES
 **********************/
static filetype_t packet_file = {
    .read = packet_file_read,
    .write = packet_file_write,
};

static const filetype_map_t filetype_table[] = {
    {
        .type_code = MODBUS_PACKET_FILE,
        .filetype = &packet_file,
    },
};

static rec_io_map_t rec_io_table[] = {
    {
        .type_code = MODBUS_PACKET_FILE,
    },
};

/**********************
 *   STATIC FUNCTIONS
 **********************/
inline static yam_file_rec_io_t *find_rec_io(int type)
{
    for (size_t i = 0; i < sizeof(rec_io_table) / sizeof(filetype_map_t); ++i)
        if (rec_io_table[i].type_code == type)
            return rec_io_table[i].io;
    return NULL;
}

static int packet_file_read(int type, const char *req_buf, size_t req_len,
        char *resp_buf, size_t resp_buf_sz)
{
    if (req_len != 6) return -REG_ERR_DATA_VALUE;

    uint16_t file_number = req_buf[0] * 256 + req_buf[1];
    uint8_t packet_id = 0;
    packet_file_rec_t *rec = NULL;

    if (req_buf[2] == 0xFF && req_buf[3] == 0xFF) /* read file */ {
        packet_id = req_buf[5];
    } else /* read register */ {
        packet_id = 0xFF;
    }

    yam_file_rec_io_t *rec_io = find_rec_io(type);
    if (! rec_io) return -REG_ERR_INTERNAL;
    if (rec_io->read(type, file_number, packet_id, 1, (void**)&rec) < 0 || ! rec)
        return -REG_ERR_INTERNAL;

    if (resp_buf_sz < rec->len + 1) return -REG_ERR_INTERNAL;
    resp_buf[0] = rec->remaining_recs_num;
    memcpy(resp_buf + 1, rec->content, rec->len);

    return rec->len + 1;
}

static int packet_file_write(int type, const char *req_buf, size_t req_len,
        char *resp_buf, size_t resp_buf_sz)
{
    if (req_len <= 0) return -REG_ERR_DATA_VALUE;

    uint16_t file_number = req_buf[0] * 256 + req_buf[1];
    uint8_t packet_id = 0;
    packet_file_rec_t rec;

    rec.len = req_len - 6;
    rec.remaining_recs_num = req_buf[5];
    memcpy(rec.content, req_buf+6, rec.len);
    if (req_buf[2] == 0xFF && req_buf[3] == 0xFF) /* write file */ {
        packet_id = req_buf[4];
    } else /* write register */ {
        packet_id = 0xFF;
    }

    yam_file_rec_io_t *rec_io = find_rec_io(type);
    if (! rec_io) return -REG_ERR_INTERNAL;
    if (rec_io->write(type, file_number, packet_id, 1, &rec) < 0)
        return -REG_ERR_INTERNAL;

    if (resp_buf_sz < 6) return -REG_ERR_INTERNAL;
    resp_buf[0] = packet_id;
    resp_buf[1] = file_number >> 8;
    resp_buf[2] = file_number;

    return 6;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
filetype_t* filetype_get(int type)
{
    for (size_t i = 0; i < sizeof(filetype_table) / sizeof(filetype_map_t); ++i)
        if (filetype_table[i].type_code == type)
            return filetype_table[i].filetype;
    return NULL;
}

int yam_register_file_rec_io(int type, yam_file_rec_io_t *io)
{
    for (size_t i = 0; i < sizeof(rec_io_table) / sizeof(filetype_map_t); ++i)
        if (rec_io_table[i].type_code == type && ! rec_io_table[i].io)
            rec_io_table[i].io = io;
    return -1;
}
