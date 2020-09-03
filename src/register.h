/**
 * @file register.h
 */

#ifndef __YAM_REGISTER_H
#define __YAM_REGISTER_H

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include "../options.h"
#include "regval.h"

/*********************
 *      DEFINES
 *********************/
#define REG_PERM_MASK           0x03
#define REG_PERM_RD             0x02
#define REG_PERM_WR             0x01
#define REG_PERM_RW             0x03

#define OPT_BITMAP              1

/**********************
 *      TYPEDEFS
 **********************/
typedef uint32_t reg_mask_t;
typedef uint16_t mb_ref_t;

struct _reg;

typedef int (* rd_reg_t)(const struct _reg *, regval_t *val);
typedef int (* wr_reg_t)(const struct _reg *, const regval_t *val);

typedef struct _reg {
    /* five decimal digits of reference id, e.g., 40001 */ 
    mb_ref_t ref;

    /* number of refs this register spans */
    mb_size_t size;

    /* internal data type */
    type_tag_t tag;

    int8_t mb_scale         :5;  /* -16 to 15. value = mb-vale x 10^scale */
    int8_t reserved         :3;
    int8_t perm             :2;  /* bits xx = RW */
#if YAM_REG_RANGE_CONTROL
    int8_t lower_bound      :1;  /* 'min' takes effect */
    int8_t upper_bound      :1;  /* 'max' takes effect */

    float min;
    float max;
#endif

    /**
     * load/store callbacks, when appears, provide
     * customized load/store service.
     */
#if YAM_REG_LOAD_STORE_SPECIAL_HANDLING
    rd_reg_t read_cb;
    wr_reg_t write_cb;
#endif

    const char *desc;
    const char *group;
} reg_t;

typedef struct {
    int (* load_register)(regval_t *val, mb_ref_t ref);
    int (* save_register)(const regval_t *val, mb_ref_t ref);
} regstore_cb_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Install store callback methods.
 * @param store_cb it contains store callback methods: load_register and
 *                 save_register.
 */
void register_install_store_cb(const regstore_cb_t *store_cb);

/**
 * Read register value.
 * @param ref ref of the register.
 * @param options if contains OPT_BITMAP means the ref is referencing 
 *                a coil or discrete inputs.
 * @param reg if success, reg will be filled with address of the matched
 *           register.
 * @param val if success, val will be filled with value of the matched
 *            register
 * @return how many actual ref's that was read.
 */
int register_read(mb_ref_t ref, int options,
        const reg_t **reg, regval_t *val);

int register_write(mb_ref_t ref, int options,
        const reg_t *reg, const regval_t *val);

int register_find(mb_ref_t ref, int options, const reg_t **reg);

#define REG_IO_NONE                     0
#define REG_IO_ILLEGAL_DATA_ADDRESS     2
#define REG_IO_ILLEGAL_DATA_VALUE       3
#define REG_IO_SERVER_DEVICE_ERR        4
#define REG_IO_SERVER_DEVICE_BUSY       6
#define REG_IO_OTHERS                   255

#ifdef __cplusplus
} /* extern "C" */
#endif

/**********************
 *      MACROS
 **********************/

/**
 * Get the scale of a register.
 * It's needed since the five-bits scale value is in two's complement.
 */
#define reg_mb_scale(reg) \
    ((reg)->mb_scale >= 16 ? -(32 - (reg)->mb_scale) : (reg)->mb_scale)

#define __register__ __attribute__ ((section(".register"))) const reg_t

#endif /* __YAM_REGISTER_H */
