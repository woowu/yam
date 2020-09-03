/**
 * @file err.h
 */

#ifndef __YAM_ERR_H
#define __YAM_ERR_H

/**********************
 *      TYPEDEFS
 **********************/
enum {
    YAM_ERR_NONE,
    YAM_ERR_ADDR,
    YAM_ERR_UNKNOWN_MESSAGE,
    YAM_ERR_FRAME,
};

/**
 * This map to the modbus expection code
 * list.
 */
enum {
    REG_ERR_NONE,
    REG_ERR_INTERNAL,
    REG_ERR_ADDRESS_NOT_FOUND,
    REG_ERR_DATA_VALUE,
};

#endif /* __YAM_ERR_H */
