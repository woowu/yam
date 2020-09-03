/**
 * @file filetype.h
 */

#ifndef __YAM_FILE_TYPE_H
#define __YAM_FILE_TYPE_H

/**********************
 *      INCLUDES
 **********************/
#include <stddef.h>

/**********************
 *      TYPEDEFS
 **********************/

/**
 * Code of filetypes.
 */
enum {
    MODBUS_PACKET_FILE =  16,
};


/**
 * A filetype defines a read and a write methods, each of which
 * will handling modbus request message and gives out response.
 */
typedef struct {
    int (* read)(int type, const char *req_buf, size_t req_len,
        char *resp_buf, size_t resp_buf_sz);
    int (* write)(int type, const char *req_buf, size_t req_len,
        char *resp_buf, size_t resp_buf_sz);
} filetype_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a filetype_t object by a givin filetype code.
 * @param type type code of a filetype.
 * @return pointer to a filetype object or NULL if not found.
 */
filetype_t* filetype_get(int type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __YAM_FILE_TYPE_H */
