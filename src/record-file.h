/**
 * @file record-file.h.h
 */

#ifndef __YAM_RECORD_FILE_H
#define __YAM_RECORD_FILE_H

/**********************
 *      TYPEDEFS
 **********************/
#define FILE_REC_MAX_LEN    255

/**********************
 *      TYPEDEFS
 **********************/

/**
 * PIE's packet file record format.
 */
typedef struct {
    char content[FILE_REC_MAX_LEN];
    size_t len;
    int remaining_recs_num;
} packet_file_rec_t;

/**
 * A yam_file_rec_io_t is an object which contains read and write method
 * for file records.
 */
typedef struct {
    /**
     * Record read method of a record filetype.
     * @param filetype type code of the filetype.
     * @param file_number identity of the file.
     * @param rec_start the starting index of the record to read.
     * @param rec_num number of records to read.
     * @param record a pointer to the returned record, which will be modified
     *      to point to a record. The caller should never free the
     *      returned record. For different filetype, the record will
     *      be cast to different data type coresponding to that
     *      filetype.
     * @return 0 on success; negative on failure.
     */
    int (* read)(int filetype, int file_number, int rec_start, size_t rec_num,
            void **record);

    /**
     * Record write method of a record filetype.
     * @param filetype type code of the filetype.
     * @param file_number identity of the file.
     * @param rec_start the starting index of the record to read.
     * @param rec_num number of records to read.
     * @param record a pointer to the record that need to be write
     *      into the file.
     * @return 0 on success; negative on failure.
     */
    int (* write)(int filetype, int file_number, int rec_start, size_t rec_num,
            void *record);
} yam_file_rec_io_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register a yam_file_rec_io_t for a given filetype.
 * @param io the rec_io_t object.
 * @return 0 on success or -1 on failure.
 *
 * Note: each filetype can be registered only only once, otherwise
 * an error will return.
 */
int yam_register_file_rec_io(int filetype, yam_file_rec_io_t *io);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __YAM_RECORD_FILE_H */
