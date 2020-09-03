/**
 * @file app.h
 */

#ifndef __AMBS_APPL_H
#define __AMBS_APPL_H

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include "register.h"

/*********************
 *      DEFINES
 *********************/
#define MODBUS_PDU_LEN_MAX      253

/**********************
 *      TYPEDEFS
 **********************/
typedef uint8_t mb_dev_addr_t;

typedef struct {
    char *payload;
    mb_size_t len;
} mb_pbuf_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hanle ModBus request PDU to the slave and produce
 * the resposne PDU.
 * @param slave_addr the target slave device address
 * @param req request PDU
 * @param resp_buf buffer to hold the response PDU
 * @param buf_sz size of the response buffer
 * @return negative if error or the length of the
 *         resposne PDU.
 */
int yam_app_input(mb_dev_addr_t slave_addr,
        const mb_pbuf_t *req,
        char *resp_buf,
        mb_size_t buf_sz);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __AMBS_APPL_H */
