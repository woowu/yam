/**
 * @file serial_link.c
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include "circ_buf.h"
#include "lib/log.h"
#include "frame_tool.h"
#include "appl.h"
#include "err.h"
#include "serial_link.h"
#include "string.h"

/*********************
 *      DEFINES
 *********************/
#define MODBUS_SERIAL_APDU_LEN_MAX      256
#define MODBUS_ADDR_SIZE                1 
#define MODBUS_CRC_SIZE                 2
#define MODBUS_SERIAL_APDU_LEN_MIN      (MODBUS_ADDR_SIZE + MODBUS_CRC_SIZE + 2)
//#define VERBOSE

/* The size of the circular buf has to be power of two and not
 * less than (MODBUS_SERIAL_APDU_LEN_MAX + 1) because of the
 * bitwise operations used.
 */
#define CIRC_BUF_SZ                     512

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    char buf[CIRC_BUF_SZ];
    int head;
    int tail;
    size_t size;
} recv_buf_t;

typedef struct {
    unsigned int rx_chars;
    unsigned int tx_chars;
    unsigned int bad_frames;
    unsigned int good_frames;
} serial_link_stats_t ;

struct yam_slink {
    recv_buf_t recv_buf;

    char in_frame[MODBUS_SERIAL_APDU_LEN_MAX];
    size_t in_frame_len;
    int slave_id;
    char out_frame[MODBUS_SERIAL_APDU_LEN_MAX];

    yam_send_frame_cb_t send_frame_cb;

    serial_link_stats_t stats;
};

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void yam_slink_init(yam_slink_t *link)
{
    link->recv_buf.head = 0;
    link->recv_buf.tail = 0;
    link->recv_buf.size = CIRC_BUF_SZ;
    link->send_frame_cb = NULL;
}

static int yam_slink_process_in_frame(yam_slink_t *link)
{
    mb_pbuf_t pbuf;
    uint16_t crc;
    int n;

    pbuf.payload = &link->in_frame[MODBUS_ADDR_SIZE];
    pbuf.len = link->in_frame_len - MODBUS_ADDR_SIZE - MODBUS_CRC_SIZE;
    if ((n = yam_app_input(link->in_frame[0],
                    &pbuf,
                    link->out_frame + MODBUS_ADDR_SIZE,
                    MODBUS_PDU_LEN_MAX))
            < 0)
        return n;
    link->out_frame[0] = link->in_frame[0];
    ++n;
    crc = modbus_crc(link->out_frame, n);
    link->out_frame[n++] = crc;
    link->out_frame[n++] = crc >> 8;

    if (link->send_frame_cb) {
        link->stats.tx_chars += n;
        link->send_frame_cb(link->out_frame, n);
    }
    return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
yam_slink_t * yam_create_slink(int slave_id)
{
    yam_slink_t *link = calloc(1, sizeof(yam_slink_t));
    yam_slink_init(link);
    link->slave_id = slave_id;
    return link;
}

inline void yam_slink_set_send_frame_cb(yam_slink_t *link, yam_send_frame_cb_t cb)
{
    link->send_frame_cb = cb;
}

void yam_slink_putchar(yam_slink_t *link, char c)
{
    int head = link->recv_buf.head;
    int tail = link->recv_buf.tail;
    if (CIRC_SPACE(head, tail, link->recv_buf.size) <= 0) return;

    link->recv_buf.buf[head] = c;
    link->recv_buf.head = (head + 1) & (link->recv_buf.size - 1);

    ++link->stats.rx_chars;
}

int yam_slink_put_frame_delimiter(yam_slink_t *link)
{
    int head;
    char *p;
    uint16_t crc;

    head = link->recv_buf.head;
    p = link->in_frame;

    while (CIRC_CNT(head, link->recv_buf.tail, link->recv_buf.size) > 0) {
        *p++ = link->recv_buf.buf[link->recv_buf.tail];
        link->recv_buf.tail = (link->recv_buf.tail + 1) & (link->recv_buf.size - 1);
    }

    link->in_frame_len = p - link->in_frame;
    
#ifdef VERBOSE
    log_dump_memory(link->in_frame, link->in_frame_len,
            "modbus-485", "ingress frame");
#endif

    if (link->in_frame_len < MODBUS_SERIAL_APDU_LEN_MIN) {
        ++link->stats.bad_frames;
        return -YAM_ERR_FRAME;
    }
    if (link->in_frame[0] != link->slave_id) {
        ll_info("yam: unrecognized slave address %u", link->in_frame[0]);
        return -YAM_ERR_ADDR;
    }

    crc = modbus_crc(link->in_frame, link->in_frame_len - MODBUS_CRC_SIZE);
    if ((crc >> 8) != link->in_frame[link->in_frame_len - 1]
            || (crc & 0xff)
            != link->in_frame[link->in_frame_len - MODBUS_CRC_SIZE]
            ) {
        ++link->stats.bad_frames;
        return -YAM_ERR_FRAME;
    }

    ++link->stats.good_frames;

    return yam_slink_process_in_frame(link);
}

void yam_set_slink_slave_id(yam_slink_t *link, int slave_id)
{
    link->slave_id = slave_id;
}
