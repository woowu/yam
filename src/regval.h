/**
 * @file regval.h
 */

#ifndef __AMBS_REGVAL_H
#define __AMBS_REGVAL_H

/**********************
 *      INCLUDES
 **********************/
#include <stdint.h>
#include "../options.h"

/**********************
 *      TYPEDEFS
 **********************/
typedef uint8_t type_tag_t;
typedef uint8_t mb_size_t;
typedef int8_t scale_t;

/**
 * The actual datatype of a regval_t, it will
 * be assigned to the 'tag' field of a regval_t.
 */
enum {
    _integer,
    _float,
};

typedef struct {
    union {
        int32_t n;
        float f;
    };
    type_tag_t tag;
} regval_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Put an integer value into a regvl.
 * @param val the regval
 * @param v the integer
 */
void regval_put_integer(regval_t *val, int32_t v);

/**
 * Put a float value into a regvl.
 * @param val the regval
 * @param v the float
 */
void regval_put_float(regval_t *val, float v);

/**
 * Compare a regval (a) to an integer (b).
 * @param a the regval
 * @param b the integer
 * @return -1: a < b; 0: a == b; 1: a > b
 */
int regval_compare(const regval_t *a, int32_t b);

/**
 * Compare a regval (a) to a float (b).
 * @param a the regval
 * @param b the float
 * @return -1: a < b; 0: a == b; 1: a > b
 */
int regval_compare_f(const regval_t *a, float b);

/**
 * Encode a regval to modbus buffer.
 * @param val the regval to encode
 * @param buf the encoded octets will be put in the buffer
 * @param tag the value type of regval
 * @param mb_size size of the registers
 * @param mb_scale the scale of encoded modbus value
 * @return zero on success or negaive when fail
 */
int regval_encode_mb(const regval_t *val, char *buf,
        type_tag_t tag, mb_size_t mb_size, scale_t mb_scale);

/**
 * Decode from modbus a regval.
 * See: regval_encode_mb()
 */
int regval_decode_mb(const char *buf, regval_t *val, 
        type_tag_t mb_tag, mb_size_t mb_size, scale_t mb_scale);

/**
 * Set float codec format used for Modbus representation.
 *
 * @param fmt a four-tuple of integers.
 * @return zero on success, negative if error.
 *
 * All the possible formats:
 * b:   {3, 2, 1, 0}
 * bb:  {2, 3, 0, 1}
 * l:   {0, 1, 2, 3}
 * lb:  {1, 0, 3, 2}
 */
int regval_set_float_fmt(const short *fmt);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __AMBS_REGVAL_H */
