/**
 * @file app.c
 * @brief Modbus Application Layer
 */

/*********************
 *      INCLUDES
 *********************/
#include <stddef.h>
#include "err.h"
#include "filetype.h"
#include "appl.h"

/**********************
 *      DEFINES
 **********************/
#define REGISTER_SIZE   2
#define COILS_PER_BYTE  8

#define COILS_REF_FIRST             1
#define DISCRETE_INPUT_REF_FIRST    10001
#define INPUT_REGS_REF_FIRST        30001
#define HOLDING_REGS_REF_FIRST      40001

/**********************
 *      TYPEDEFS
 **********************/
typedef uint16_t mb_ref_t;
typedef uint16_t mb_cnt_t;

typedef enum {
    ERR_NONE,
    ERR_ILLEGAL_FUNC,
    ERR_ILLEGAL_DATA_ADDR,
    ERR_ILLEGAL_DATA_VALUE,
} mb_exception_t;

typedef enum {
    MBF_READ_COILS              = 1,
    MBF_READ_DISCRETE_INPUTS    = 2,
    MBF_WRITE_REG               = 6,
    MBF_WRITE_REGS              = 16,
    MBF_READ_HOLDING_REGS       = 3,
    MBF_READ_FILE               = 20,
    MBF_WRITE_FILE               = 21,
} mb_func_t;

typedef int (* mb_func_handler_t)(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz);

typedef struct {
    mb_func_t func;
    mb_func_handler_t handler;
} mb_func_handler_map_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static int read_coils_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz);
static int read_holding_regs_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz);
static int write_regs_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz);
static int write_regs_handlers(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz);
static int read_file_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz);
static int write_file_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz);

/**********************
 *  STATIC VARIABLES
 **********************/
static const mb_func_handler_map_t func_handlers[] = {
    {
        .func = MBF_READ_COILS,
        .handler = read_coils_handler,
    },
    {
        .func = MBF_READ_DISCRETE_INPUTS,
        .handler = read_coils_handler,
    },
    {
        .func = MBF_READ_HOLDING_REGS,
        .handler = read_holding_regs_handler,
    },
    {
        .func = MBF_WRITE_REG,
        .handler = write_regs_handler,
    },
    {
        .func = MBF_WRITE_REGS,
        .handler = write_regs_handlers,
    },
    {
        .func = MBF_READ_FILE,
        .handler = read_file_handler,
    },
    {
        .func = MBF_WRITE_FILE,
        .handler = write_file_handler,
    },
};

/**********************
 *   MACROS
 **********************/
#define rd_resp_header_len()    2
#define wr_resp_len()           5

#define chk_rd_req_size(func, req_len, resp_buf) \
    if ((req_len) != sizeof(mb_ref_t) + sizeof(mb_cnt_t)) \
        return make_exception((func), ERR_ILLEGAL_DATA_VALUE, (resp_buf))

#define chk_rd_resp_buf_size(func, buf_sz, resp_data_len, resp_buf) \
    if ((buf_sz) <  rd_resp_header_len() + (resp_data_len)) \
        return make_exception((func), ERR_ILLEGAL_DATA_ADDR, (resp_buf))

#define chk_wr_resp_buf_size(func, buf_sz, resp_buf) \
    if ((buf_sz) <  wr_resp_len()) \
        return make_exception((func), ERR_ILLEGAL_DATA_ADDR, (resp_buf))

#define catch_modbus_exception(func, err, resp_buf) \
    if ((err) < 0) return make_exception((func), -(err), (resp_buf));

#define rd_resp_header(func, resp_data_len, resp_buf) \
    (resp_buf)[0] = (func); \
    (resp_buf)[1] = (resp_data_len)

#define wr_resp(func, ref_start, word) \
    (resp_buf)[0] = (func); \
    (resp_buf)[1] = (ref_start) >> 8; \
    (resp_buf)[2] = (ref_start); \
    (resp_buf)[3] = (word) >> 8; \
    (resp_buf)[4] = (word)

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int make_exception(mb_func_t func, mb_exception_t expt, char *resp_buf)
{
    resp_buf[0] = func | 0x80;
    resp_buf[1] = expt;
    return 2;
}

/**
 * I image the slave devcie has a virtual memory, any portion of which
 * can be addressed through a reference address. This function is
 * invocated when we need to answer what's the content for a givin
 * portion of that virtual memory.
 *
 * @param start starting reference address
 * @param len length of the memory region 
 * @parm buf buffer to hold memory contents
 * @return negative if error, and zero otherwise
 */
static int load_ref_mem(mb_ref_t start, mb_size_t len, char *buf)
{
    regval_t val;
    const reg_t *reg;
    char *p = buf;
    int n;

    while (len) {
        if ((n = register_read(start, 0, &reg, &val)) < 0) return n;
        if (len < n * REGISTER_SIZE) return -1;

        if (regval_encode_mb(&val, p, reg->tag, reg->size, reg_mb_scale(reg)))
            return -REG_ERR_ADDRESS_NOT_FOUND;

        p += n * REGISTER_SIZE;
        len -= n * REGISTER_SIZE;
        start += n;
    }

    return 0;
}

static int load_ref_bitmap(mb_ref_t start, mb_size_t nbits, char *buf)
{
    regval_t val;
    const reg_t *reg;
    char *p = buf;
    unsigned char bit_offset;   /* inner byte */
    int n;

    bit_offset = 0;
    if (nbits) *p = 0;

    while (nbits) {
        if ((n = register_read(start, OPT_BITMAP, &reg, &val)) < 0) return n;

        while (n && nbits) {
            *p |=  (val.n & (1 << bit_offset));
            if (++bit_offset == 8) {
                bit_offset = 0;
                *++p = 0;
                val.n >>= 8;
            }
            n--;
            nbits--;
            start++;
        }
    }

    return 0;
}

static int store_ref_mem(mb_ref_t start, mb_size_t len, const char *buf)
{
    regval_t val;
    const reg_t *reg;
    const char *p = buf;
    int n;

    while (len) {
        if (register_find(start, 0, &reg) < 0
                || len < reg->size * REGISTER_SIZE)
            return -REG_ERR_ADDRESS_NOT_FOUND;

        if (regval_decode_mb(p, &val, reg->tag, reg->size, reg_mb_scale(reg)))
            return -REG_ERR_ADDRESS_NOT_FOUND;

        if ((n = register_write(start, 0, reg, &val)) < 0) return n;

        p += n * REGISTER_SIZE;
        len -= n * REGISTER_SIZE;
        start += n;
    }

    return 0;
}

static int read_coils_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz)
{
    int err;

    chk_rd_req_size(func, req_len, resp_buf);
    mb_ref_t ref_start = req_buf[0] * 256 + req_buf[1];
    mb_cnt_t read_cnt = req_buf[2] * 256 + req_buf[3];
    mb_size_t mem_sz = (read_cnt + COILS_PER_BYTE - 1) / 8;
    chk_rd_resp_buf_size(func, buf_sz, mem_sz, resp_buf);

    err = load_ref_bitmap(ref_start
            + (func == MBF_READ_COILS ? COILS_REF_FIRST
            : DISCRETE_INPUT_REF_FIRST),
            read_cnt, resp_buf + rd_resp_header_len());
    catch_modbus_exception(func, err, resp_buf);

    rd_resp_header(func, mem_sz, resp_buf);
    return rd_resp_header_len() + mem_sz;
}

static int write_regs_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz)
{
    int err;

    if (req_len < sizeof(mb_ref_t) + REGISTER_SIZE)
        goto illegal_req;

    mb_ref_t ref_start = req_buf[0] * 256 + req_buf[1];
    mb_size_t mem_sz = REGISTER_SIZE;

    chk_wr_resp_buf_size(func, buf_sz, resp_buf);

    err = store_ref_mem(ref_start + HOLDING_REGS_REF_FIRST,
            mem_sz,
            req_buf + sizeof(mb_ref_t));
    catch_modbus_exception(func, err, resp_buf);

    wr_resp(func, ref_start,
            req_buf[sizeof(mb_ref_t)] * 256
            + req_buf[sizeof(mb_ref_t) + 1]);
    return wr_resp_len();

illegal_req:
    return make_exception(func, ERR_ILLEGAL_DATA_VALUE, resp_buf);
}

static int write_regs_handlers(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz)
{
    int err;

    if (req_len < sizeof(mb_ref_t) + sizeof(mb_cnt_t) + 1)
        goto illegal_req;
    if (req_len < sizeof(mb_ref_t) + sizeof(mb_cnt_t) + 1
            + req_buf[sizeof(mb_ref_t) + sizeof(mb_cnt_t)])
        goto illegal_req;

    mb_ref_t ref_start = req_buf[0] * 256 + req_buf[1];
    mb_cnt_t write_cnt = req_buf[2] * 256 + req_buf[3];
    mb_size_t mem_sz = req_buf[sizeof(mb_ref_t) + sizeof(mb_cnt_t)];

    if (mem_sz != write_cnt * REGISTER_SIZE) goto illegal_req;

    chk_wr_resp_buf_size(func, buf_sz, resp_buf);

    err = store_ref_mem(ref_start + HOLDING_REGS_REF_FIRST,
            mem_sz,
            req_buf + sizeof(mb_ref_t) + sizeof(mb_cnt_t) + 1);
    catch_modbus_exception(func, err, resp_buf);

    wr_resp(func, ref_start, write_cnt);
    return wr_resp_len();

illegal_req:
    return make_exception(func, ERR_ILLEGAL_DATA_VALUE, resp_buf);
}

static int read_holding_regs_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz)
{
    int err;

    chk_rd_req_size(func, req_len, resp_buf);
    mb_ref_t ref_start = req_buf[0] * 256 + req_buf[1];
    mb_cnt_t read_cnt = req_buf[2] * 256 + req_buf[3];
    mb_size_t mem_sz = read_cnt * REGISTER_SIZE;
    chk_rd_resp_buf_size(func, buf_sz, mem_sz, resp_buf);

    err = load_ref_mem(ref_start + HOLDING_REGS_REF_FIRST,
                    mem_sz, resp_buf + rd_resp_header_len());
    catch_modbus_exception(func, err, resp_buf);

    rd_resp_header(func, mem_sz, resp_buf);
    return rd_resp_header_len() + mem_sz;
}

static int read_file_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz)
{
    if (req_len < 2) catch_modbus_exception(func, -REG_ERR_DATA_VALUE, resp_buf);

    size_t file_req_len = req_buf[0] - 1;
    int type = req_buf[1];

    filetype_t *filetype = filetype_get(type);
    if (! filetype)
        catch_modbus_exception(func, -REG_ERR_ADDRESS_NOT_FOUND, resp_buf);

    int n = filetype->read(type, req_buf + 2, file_req_len,
            resp_buf + 2, buf_sz - 2);
    if (n < 0) catch_modbus_exception(func, n, resp_buf);
    if (n > 255) catch_modbus_exception(func, -REG_ERR_INTERNAL, resp_buf);

    resp_buf[0] = func;
    resp_buf[1] = n;
    return 2 + n;
}

static int write_file_handler(mb_func_t func,
        const char *req_buf,
        mb_size_t req_len,
        char *resp_buf,
        mb_size_t buf_sz)
{
    if (req_len < 2) catch_modbus_exception(func, -REG_ERR_DATA_VALUE, resp_buf);

    size_t file_req_len = req_buf[0] - 1;
    int type = req_buf[1];

    filetype_t *filetype = filetype_get(type);
    if (! filetype)
        catch_modbus_exception(func, -REG_ERR_ADDRESS_NOT_FOUND, resp_buf);

    int n = filetype->write(type, req_buf + 2, file_req_len,
            resp_buf + 2, buf_sz - 2);
    if (n < 0) catch_modbus_exception(func, n, resp_buf);
    if (n > 255) catch_modbus_exception(func, -REG_ERR_INTERNAL, resp_buf);

    resp_buf[0] = func;
    resp_buf[1] = n;
    return 2 + n;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int yam_app_input(mb_dev_addr_t slave_addr,
        const mb_pbuf_t *req,
        char *resp_buf,
        mb_size_t buf_sz)
{
    size_t i;
    mb_func_t func = req->payload[0];

    for (i = 0;
            i < sizeof(func_handlers) / sizeof(mb_func_handler_map_t)
            && func_handlers[i].func != func;
            ++i);
    if (i == sizeof(func_handlers) / sizeof(mb_func_handler_map_t))
        return -YAM_ERR_UNKNOWN_MESSAGE;

    return func_handlers[i].handler(func,
            ((char *)req->payload) + 1, req->len - 1,
            resp_buf, buf_sz);
}
