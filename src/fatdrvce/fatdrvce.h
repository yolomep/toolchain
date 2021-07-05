/**
 * @file
 * @brief MSD FAT Filesystem Driver
 *
 * This library can be used to communicate with Mass Storage Devices (MSD) which
 * have partitions formated as FAT32. It is recommended to make the cluster
 * allocation size at least 4KiB, however 64KiB will provide the best
 * performance.
 *
 * Currently only drives with a logical block size of 512 bytes are supported,
 * which is the most common block size available. Larger drives and SSDs may
 * not work.
 *
 * The drive must use MBR partitioning, GPT and others are not yet supported.
 *
 * @author Matt "MateoConLechuga" Waltz
 * @author Jacob "jacobly" Young
 */

#ifndef FATDRVCE_H
#define FATDRVCE_H

#include <stdint.h>
#include <stdbool.h>
#include <usbdrvce.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MSD_BLOCK_SIZE 512 /**< Block size in bytes */

typedef enum {
    MSD_SUCCESS = 0, /**< Operation was successful */
    MSD_ERROR_INVALID_PARAM, /**< An invalid argument was provided */
    MSD_ERROR_USB_FAILED, /**< An error occurred in usbdrvce */
    MSD_ERROR_SCSI_FAILED, /**< An error occurred in scsi transfer */
    MSD_ERROR_NOT_SUPPORTED, /**< The operation is not supported */
    MSD_ERROR_INVALID_DEVICE, /**< An invalid usb device was specified */
} msd_error_t;

typedef struct {
    usb_device_t dev; /**< USB device */
    uint8_t bulkin; /**< USB bulk in endpoint address */
    uint8_t bulkout; /**< USB bulk out endpoint address */
    uint8_t config; /**< USB config descriptor index */
    uint8_t interface; /**< USB Interface index */
    uint32_t tag; /**< Internal library use */
    void *last; /**< Internal library use */
    uint8_t haslast; /**< Internal library use */
    uint8_t userbuf[1024]; /**< Internal library use */
} msd_t;

typedef struct msd_transfer_t {
    msd_t *msd; /**< Initialized MSD device */
    uint32_t lba; /**< Logical block address */
    void *buffer; /**< Pointer to data location to read/write */
    uint24_t count; /**< Number of blocks to transfer */
    void (*callback)(msd_error_t error, struct msd_transfer_t *xfer); /**< Called when transfer completes */
    void *userptr; /**< Custom user data for callback (optional) */
    void *next; /**< Internal library use */
    uint8_t stall; /**< Internal library use */
    uint8_t cbw[31+1]; /**< Internal library use */
    uint8_t csw[13+19]; /**< Internal library use */
} msd_transfer_t;

/**
 * Initialize a Mass Storage Device.
 * @param msd Uninitilaized MSD device structure.
 * @param usb Initialized USB device structure.
 * @return MSD_SUCCESS on success, otherwise error.
 */
msd_error_t msd_Open(msd_t *msd, usb_device_t usb);

/**
 * Closes and deinitializes a Mass Storage Device. This function should be
 * called in the \c USB_DEVICE_DISCONNECTED_EVENT in the USB handler callback.
 * @param msd Initialized MSD device structure.
 */
void msd_Close(msd_t *msd);

/**
 * Attempts to reset and restore normal working order of the device.
 * @param msd Initialized MSD device structure.
 * @return MSD_SUCCESS on success, otherwise error.
 */
msd_error_t msd_Reset(msd_t *msd);

/**
 * Gets the number of and size of each block on the device.
 * @param msd Initialized MSD device structure.
 * @param block_count Pointer to store number of blocks to.
 * @param block_size Pointer to store block size to.
 * @return MSD_SUCCESS on success.
 */
msd_error_t msd_Info(msd_t *msd, uint32_t *block_count, uint32_t *block_size);

/**
 * Synchronous block read.
 * @param msd Iniailized MSD structure.
 * @param lba Logical Block Address (LBA) of starting block to read.
 * @param num Number of blocks to read.
 * @param data Buffer to read into. Must be at least block size * count bytes.
 * @return MSD_SUCCESS on success.
 */
msd_error_t msd_Read(msd_t *msd, uint32_t lba,
                     uint24_t count, void *buffer);

/**
 * Synchronous block write.
 * @param msd Iniailized MSD structure.
 * @param lba Logical Block Address (LBA) of starting block to read.
 * @param num Number of blocks to read.
 * @param data Buffer to read into. Must be at least block size * count bytes.
 * @return MSD_SUCCESS on success.
 */
msd_error_t msd_Write(msd_t *msd, uint32_t lba,
                      uint24_t count, const void *buffer);

/**
 * Asynchronous block read. You must set the following \p xfer elements:
 * \p lba, \p buffer, \p count \p callback. The optional element \p userptr
 * can be used to store user-defined data for access in the callback.
 * The \p xfer argument must remain valid (cannot be free'd or lose scope)
 * until the callback is issued. You can free \xfer inside the callback as
 * needed.
 * @param xfer Initialized transaction structure.
 * @return MSD_SUCCESS if the transfer has been added to the queue.
 */
msd_error_t msd_ReadAsync(msd_transfer_t *xfer);

/**
 * Asynchronous block write. You must set the following \p xfer elements:
 * \p lba, \p buffer, \p count \p callback. The optional element \p userptr
 * can be used to store user-defined data for access in the callback.
 * The \p xfer argument must remain valid (cannot be free'd or lose scope)
 * until the callback is issued. You can free \xfer inside the callback as
 * needed.
 * @param xfer Initialized transaction structure.
 * @return MSD_SUCCESS if the transfer has been added to the queue.
 */
msd_error_t msd_WriteAsync(msd_transfer_t *xfer);

#ifdef __cplusplus
}
#endif

#endif
