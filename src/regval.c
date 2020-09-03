/**
 * @file udc_register.c
 */

/*********************
 *      INCLUDES
 *********************/
#include <stddef.h>
#include "regval.h"

/**********************
 *      TYPEDEFS
 **********************/
typedef void (* regval_encoder_t)(const regval_t *val, short mb_scale, char *buf);
typedef void (* regval_decoder_t)(const char *buf, regval_t *val, short mb_scale);

typedef struct {
    char tag;
    char reg_size;
    regval_encoder_t encoder;
    regval_decoder_t decoder;
} val_codec_spec_t;

/**********************
 *   STATIC PROTOTYPES
 **********************/
static void integer_to_mb_short(const regval_t *val, short mb_scale, char *buf);
static void mb_short_to_integer(const char *buf, regval_t *val, short mb_scale);
static void integer_to_mb_long(const regval_t *val, short mb_scale, char *buf);
static void mb_long_to_integer(const char *buf, regval_t *val, short mb_scale);
static void float_to_mb_short(const regval_t *val, short mb_scale, char *buf);
static void mb_short_to_float(const char *buf, regval_t *val, short mb_scale);
static void float_to_mb_float(const regval_t *val, short mb_scale, char *buf);
static void mb_float_to_float(const char *buf, regval_t *val, short mb_scale);

/**********************
 *   STATIC VARIABLES
 **********************/
static const val_codec_spec_t val_codec_spec_table[] = {
    {
        .tag = _integer, 
        .reg_size = 1,
        .encoder = integer_to_mb_short,
        .decoder = mb_short_to_integer,
    },
    {
        .tag = _integer, 
        .reg_size = 2,
        .encoder = integer_to_mb_long,
        .decoder = mb_long_to_integer,
    },
    {
        .tag = _float, 
        .reg_size = 1,
        .encoder = float_to_mb_short,
        .decoder = mb_short_to_float,
    },
    {
        .tag = _float, 
        .reg_size = 2,
        .encoder = float_to_mb_float,
        .decoder  = mb_float_to_float,
    },
};

static const short float_fmt_b[] = {3, 2, 1, 0};
static const short *float_fmt = float_fmt_b;

/**********************
 *   STATIC FUNCTIONS
 **********************/
/**
 * raw = val * 10^scale, this function calculates the 'val'
 * from 'raw' and 'scale'.
 */
static float float_prescale(float raw, short scale)
{
    if (scale < 0)
        while (scale) {
            raw *= 10;
            scale++;
        }
    else if (scale > 0)
        while (scale) {
            raw /= 10;
            scale--;
        }
    return raw;
}

/**
 * raw = val * 10^scale, this function calculates the 'val'
 * from 'raw' and 'scale'.
 */
static float integer_prescale(int32_t raw, short scale)
{
    float f = raw;
    if (scale < 0)
        while (scale) {
            f *= 10;
            scale++;
        }
    else if (scale > 0)
        while (scale) {
            f /= 10;
            scale--;
        }
    return f;
}

/**
 * raw = val * 10^(scale), this function calculates the 'raw'
 * from 'val' and 'scale'.
 */
static float integer_scale(int32_t val, short scale)
{
    float f = val;
    if (scale > 0)
        while (scale) {
            f *= 10;
            scale--;
        }
    else if (scale < 0)
        while (scale) {
            f /= 10;
            scale++;
        }
    return f;
}

/**
 * raw = val * 10^(scale), this function calculates the 'raw'
 * from 'val' and 'scale'.
 */
static float float_scale(float val, short scale)
{
    if (scale > 0)
        while (scale) {
            val *= 10;
            scale--;
        }
    else if (scale < 0)
        while (scale) {
            val /= 10;
            scale++;
        }
    return val;
}

static void integer_to_mb_short(const regval_t *val, short mb_scale, char *buf)
{
    int32_t n = integer_prescale(val->n, mb_scale);
    buf[0] = n >> 8;
    buf[1] = n;
}

static void mb_short_to_integer(const char *buf, regval_t *val, short mb_scale)
{
    int32_t n = buf[0] * 256 + buf[1];
    n = integer_scale(n, mb_scale);
    regval_put_integer(val, n);
}

static void float_to_mb_short(const regval_t *val, short mb_scale, char *buf)
{
    float f = float_prescale(val->f, mb_scale);
    buf[0] = (int)f >> 8;
    buf[1] = (int)f;
}

static void mb_short_to_float(const char *buf, regval_t *val, short mb_scale)
{
    int32_t n = buf[0] * 256 + buf[1];
    float f = integer_scale(n, mb_scale);
    regval_put_float(val, f);
}

static void integer_to_mb_long(const regval_t *val, short mb_scale, char *buf)
{
    int32_t n = integer_prescale(val->n, mb_scale);
    const char *p = (const char*) &n;

    buf[0] = *(p + 3);
    buf[1] = *(p + 2);
    buf[2] = *(p + 1);
    buf[3] = *p;
}

static void mb_long_to_integer(const char *buf, regval_t *val, short mb_scale)
{
    int n;
    uint8_t *p = (uint8_t*)&n;

    *(p + 3) = buf[0];
    *(p + 2) = buf[1];
    *(p + 1) = buf[2];
    *(p + 0) = buf[3];

    n = integer_scale(n, mb_scale);
    regval_put_integer(val, n);
}

static void float_to_mb_float(const regval_t *val, short mb_scale, char *buf)
{
    float f = float_prescale(val->f, mb_scale);
    const char *p = (const char*) &f;

    buf[0] = *(p + float_fmt[0]);
    buf[1] = *(p + float_fmt[1]);
    buf[2] = *(p + float_fmt[2]);
    buf[3] = *(p + float_fmt[3]);
}

static void mb_float_to_float(const char *buf, regval_t *val, short mb_scale)
{
    float f;
    uint8_t *p = (uint8_t*)&f;

    /* FB B format, other formats will be suported late */
    *(p + float_fmt[0]) = buf[0];
    *(p + float_fmt[1]) = buf[1];
    *(p + float_fmt[2]) = buf[2];
    *(p + float_fmt[3]) = buf[3];

    f = float_scale(f, mb_scale);
    regval_put_float(val, f);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int regval_encode_mb(const regval_t *val, char *buf,
        type_tag_t tag, mb_size_t mb_size, scale_t mb_scale)
{
    size_t i;

    for (i = 0;
            i < sizeof(val_codec_spec_table) / sizeof(val_codec_spec_t)
            && (tag != val_codec_spec_table[i].tag
                || mb_size != val_codec_spec_table[i].reg_size);
            ++i);
    if (i == sizeof(val_codec_spec_table) / sizeof(val_codec_spec_t)) return -1;
    if (! val_codec_spec_table[i].encoder) return -1;

    val_codec_spec_table[i].encoder(val, mb_scale, buf);
    return 0;
}

int regval_decode_mb(const char *buf, regval_t *val,
        type_tag_t tag, mb_size_t mb_size, scale_t mb_scale)
{
    size_t i;

    for (i = 0;
            i < sizeof(val_codec_spec_table) / sizeof(val_codec_spec_t)
            && (tag != val_codec_spec_table[i].tag
                || mb_size != val_codec_spec_table[i].reg_size);
            ++i);
    if (i == sizeof(val_codec_spec_table) / sizeof(val_codec_spec_t)) return -1;
    if (! val_codec_spec_table[i].decoder) return -1;

    val_codec_spec_table[i].decoder(buf, val, mb_scale);
    return 0;
}

inline int regval_compare(const regval_t *val, int32_t a)
{
    return val->tag == _integer ? val->n - a : val->f - a;
}

inline int regval_compare_f(const regval_t *val, float a)
{
    return val->tag == _float ? val->f - a : val->n - a;
}

inline void regval_put_integer(regval_t *val, int32_t v)
{
    val->n = v;
    val->tag = _integer;
}

inline void regval_put_float(regval_t *val, float v)
{
    val->f = v;
    val->tag = _float;
}

int regval_set_float_fmt(const short *fmt)
{
    float_fmt = fmt;
    return 0;
}
