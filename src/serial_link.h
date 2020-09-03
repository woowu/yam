/**
 * @file yam_serial_link.h
 */
#ifndef __YAM_SERIAL_LINK_H
#define __YAM_SERIAL_LINK_H

/*********************
 *      INCLUDES
 *********************/
#include "../options.h"

/**********************
 *      TYPEDEFS
 **********************/
typedef struct yam_slink yam_slink_t; /* obscure object */

/* -- callbacks -- */
typedef void (* yam_send_frame_cb_t)(const char *frame, size_t len);

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new serial link object.
 */
yam_slink_t * yam_create_slink(int slave_id);

/**
 * Put a new ingress character into the yam link.
 * @param link the link object
 * @param c the ingress char
 *
 * Note: This api is safe to call from ISR.
 */
void yam_slink_putchar(yam_slink_t *link, char c);

/**
 * Yam should be called with this api when the concrete serial link
 * implementation detected a frame delimiter (some length of idle
 * time after the last char was received).  Once this api is called,
 * yam will begin verify and parse current accumulated char buffer
 * trying to handle a modbuf frame.
 * @return negative if there is no frame can be parsed with success,
 *         zero otherwise.
 * 
 * Note: this api should *not* be called from inside an ISR.
 */
int yam_slink_put_frame_delimiter(yam_slink_t *link);

/**
 * Caller should register this callback which will be called
 * when yam needs to send out a frame.
 * @param link the link object
 * @param cb the callback
 *
 * Note: Inside the callback, it's user's responsibility to
 * make the START char as the protocol required.
 */
void yam_slink_set_send_frame_cb(yam_slink_t *link, yam_send_frame_cb_t cb);

/**
 * Set slave address
 *
 * @param link the link object
 * @param slave_id the slave address associated to the link.
 */
void yam_set_slink_slave_id(yam_slink_t *link, int slave_id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __YAM_SERIAL_LINK_H */
