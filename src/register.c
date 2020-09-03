/**
 * @file udc_register.c
 */

/*********************
 *      INCLUDES
 *********************/
#include "err.h"
#include "register.h"

extern uint32_t __register_start;
extern uint32_t __register_end;

/**********************
 *  STATIC VARIABLES
 **********************/
static const reg_t *reg_start = (const reg_t *) &__register_start;
static const reg_t *reg_end = (const reg_t *) &__register_end;
static regstore_cb_t store_cb;

/**********************
 *   STAITC FUNCTIONS
 **********************/
static inline int read_reg(const reg_t *reg, regval_t *val)
{
    val->tag = reg->tag;
    if (! store_cb.load_register) return -REG_ERR_INTERNAL;
    return store_cb.load_register(val, reg->ref);
}

static inline int
write_reg(const reg_t *reg, const regval_t *val)
{
    if (! store_cb.save_register) return -REG_ERR_INTERNAL;
    return store_cb.save_register(val, reg->ref);
}

#if YAM_REG_RANGE_CONTROL
static inline int
register_chk_value_range(const reg_t *reg, const regval_t *val)
{
    return ((reg->lower_bound && regval_compare_f(val, reg->min) < 0)
            || (reg->upper_bound && regval_compare_f(val, reg->max) > 0));
}
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void register_install_store_cb(const regstore_cb_t *_store_cb)
{
    store_cb = *_store_cb;
}

int register_find(mb_ref_t ref, int options, const reg_t **reg)
{
    const reg_t *i;

    if (options & OPT_BITMAP)
        for (i = reg_start
                ; i < reg_end && (ref < i->ref || ref >= i->ref + i->size)
                ; ++i);
    else
        for (i = reg_start; i < reg_end && i->ref != ref; ++i);

    if (i == reg_end) return -1;
    *reg = i;
    return 0;
}

int register_read(mb_ref_t ref, int options,
        const reg_t **reg, regval_t *val)
{
    int err;

    if (register_find(ref, options, reg) < 0)
        return -REG_ERR_ADDRESS_NOT_FOUND;

    if (! ((*reg)->perm & REG_PERM_RD)) return  -REG_ERR_ADDRESS_NOT_FOUND;

#if YAM_REG_LOAD_STORE_SPECIAL_HANDLING
    if (! (*reg)->read_cb && (err = read_reg(*reg, val)) < 0)
        return err;
    if ((*reg)->read_cb && (err = (*reg)->read_cb(*reg, val)))
        return err;
#else
    if ((err = read_reg(*reg, val)) < 0)
        return err;
#endif

    if (options & OPT_BITMAP) {
        regval_put_integer(val, val->n >> (ref - (*reg)->ref));
        return (*reg)->size - (ref - (*reg)->ref);
    } else
        return (*reg)->size;
}

int register_write(mb_ref_t ref, int options,
        const reg_t *reg, const regval_t *val)
{
    int err;

    if (! (reg->perm & REG_PERM_WR)) return -REG_ERR_ADDRESS_NOT_FOUND;
#if YAM_REG_RANGE_CONTROL
    if (register_chk_value_range(reg, val)) return -REG_ERR_DATA_VALUE; 
#endif

#if YAM_REG_LOAD_STORE_SPECIAL_HANDLING
    if (! reg->write_cb)
        return (err = write_reg(reg, val)) < 0
            ? err : reg->size;
    else
        return (err = reg->write_cb(reg, val))
            ? err : reg->size;
#else
    return (err = write_reg(reg, val)) < 0
        ? err : reg->size;
#endif

    /*TODO: for OPT_BITMAP */
}
