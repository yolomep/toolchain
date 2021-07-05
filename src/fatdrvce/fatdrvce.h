/**
 * @file
 * @brief USB FAT Filesystem Driver
 *
 * This library can be used to communicate with Mass Storage Devices (MSD) which
 * have partitions formated as FAT32. (FAT16 support is implemented yet untested).
 * It currently only supports drives that have a sector size of 512, which is pretty
 * much every flash drive on the market, except for huge ones.
 * The drive must use MBR partitioning, GUID is not supported.
 *
 * @author Matt "MateoConLechuga" Waltz
 * @author Jacob "jacobly" Young
 */

#ifndef H_FATDRVCE
#define H_FATDRVCE

#include <stdint.h>
#include <stdbool.h>
#include <usbdrvce.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t signature;
    uint32_t tag;
    uint32_t length;
    uint8_t status;
} msd_csw_t;

typedef struct {
    uint8_t length;
    uint8_t opcode;
    uint8_t data[15];
} msd_cbd_t;

typedef struct {
    uint32_t signature;
    uint32_t tag;
    uint32_t length;
    uint8_t dir;
    uint8_t lun;
    msd_cbd_t cbd;
} msd_cbw_t;

typedef struct {
    usb_device_t dev;    /**< USB device */
    usb_endpoint_t in;   /**< USB bulk in endpoint */
    usb_endpoint_t out;  /**< USB bulk out endpoint */
    usb_endpoint_t ctrl; /**< USB Control endpoint */
    uint24_t tag;        /**< MSD Command Block Wrapper incrementing tag */
    uint32_t lba;        /**< Logical Block Address of LUN */
    uint32_t blocksize;  /**< Block size (usually 512) */
    uint8_t interface;   /**< USB Interface index */
    uint8_t maxlun;      /**< Maximum LUNs for MSD */
    void *buffer;        /**< User supplied buffer address */
} msd_device_t;

typedef struct {
    uint8_t dummy;
} fat_file_t;

typedef struct {
    uint32_t lba;       /**< Logical Block Address (LBA) of FAT partition */
    msd_device_t *msd;  /**< MSD containing FAT filesystem */
} fat_partition_t;

typedef struct {
    fat_partition_t *partition; /**< Disk partition used by FAT. */
    uint8_t cluster_size;
    uint32_t clusters;
    uint24_t fat_pos;
    uint24_t fs_info;
    uint32_t fat_base_lba;
    uint32_t root_dir_pos;
    uint32_t data_region;
    uint32_t working_sector;
    uint32_t working_cluster;
    uint24_t working_pointer;
} fat_t;

#define FAT_OPEN_WRONLY  2  /**< Open file in write-only mode. */
#define FAT_OPEN_RDONLY  1  /**< Open file in read-only mode. */
#define FAT_OPEN_RDWR    (FAT_OPEN_RDONLY | FAT_OPEN_WRONLY)  /**< Open file for reading and writing. */

#define FAT_RDONLY    (1 << 0)  /**< Entry is read-only. */
#define FAT_HIDDEN    (1 << 1)  /**< Entry is hidden. */
#define FAT_SYSTEM    (1 << 2)  /**< Entry is a system file / directory. */
#define FAT_VOLLABEL  (1 << 3)  /**< Entry is a volume label -- only for root directory. */
#define FAT_SUBDIR    (1 << 4)  /**< Entry is a subdirectory (or directory). */
#define FAT_DIR       (1 << 4)  /**< Entry is a directory (or subdirectory). */
#define FAT_ALL       (FAT_DIR | FAT_VOLLABEL | FAT_SYSTEM | FAT_HIDDEN | FAT_RDONLY)

/**
 * Locates any available FAT partitions detected on the mass storage device
 * (MSD). You must allocate space for \p partitions before calling this
 * function, as well as passing a valid msd_device_t from the msd_Init
 * function.
 * @param partitions Array of FAT partitions available, returned from function.
 * @param number The number of FAT partitions found.
 * @param max The maximum number of FAT partitions that can be found.
 * @return USB_SUCCESS on success, otherwise error.
 */
usb_error_t fat_Find(msd_device_t *msd,
                     fat_partition_t *partitions,
                     uint8_t *number,
                     uint8_t max);

/**
 * Initializes the FAT filesystem and allows other FAT functions to be used.
 * Before calling this function, you must use fat_Find in order to
 * locate a valid FAT partition.
 * @param fat Uninitialized FAT structure type.
 * @param partition Available FAT partition returned from fat_Find.
 * @return USB_SUCCESS on success, otherwise error.
 */
usb_error_t fat_Init(fat_t *fat,
                     fat_partition_t *partition);

/**
 * Opens a file for either reading or writing, or both.
 * @param fat Initialized FAT structure type.
 * @param filepath File path to open or write.
 * @return 0 if the file could not be opened, otherwise pointer
 *         to a file handle for other functions.
 */
fat_file_t *fat_Open(fat_t *fat,
                     const char *filepath,
                     uint8_t flags);

/**
 * Initialize a USB connected Mass Storage Device. Checks if the device is
 * a valid MSD device. Will perform all necessary MSD initialization.
 * A user-supplied buffer is needed for internal library use. This buffer must
 * be at least 512 bytes in size. It should not be the same buffer used
 * by other devices and/or functions.
 * @param msd MSD device structure.
 * @param dev USB device to initialize as MSD.
 * @param buffer The buffer's address. (must be at least 512 bytes).
 * @return USB_SUCCESS on success, otherwise error if initialization failed.
 */
usb_error_t msd_Init(msd_device_t *msd,
                     usb_device_t dev,
                     void *buffer);

/**
 * Gets the sector count of the device.
 * @param msd MSD device structure.
 * @param blockSize Pointer to store block size to.
 * @return USB_SUCCESS on success.
 */
usb_error_t msd_GetSectorCount(msd_device_t *msd,
                               uint32_t *sectorCount);

/**
 * Gets the sector size of each sector on the device.
 * @param msd MSD device structure.
 * @param blockSize Pointer to store block size to.
 * @return USB_SUCCESS on success.
 */
usb_error_t msd_GetSectorSize(msd_device_t *msd,
                              uint24_t *sectorSize);

/**
 * Reads a single sector from a Mass Storage Device.
 * @param msd MSD device structure.
 * @param lba Logical Block Address (LBA) of starting sector to read.
 * @param sectors Number of sectors to read.
 * @param buffer Buffer to read into. Should be at least a sector size.
 * @return USB_SUCCESS on success.
 */
usb_error_t msd_ReadSector(msd_device_t *msd,
                           uint32_t lba,
                           void *data);

/**
 * Writes a single sector of a Mass Storage Device.
 * @param msd MSD device structure.
 * @param lba Logical Block Address (LBA) of sector to write.
 * @param buffer Buffer to write to MSD. Should be at least one sector size.
 * @return USB_SUCCESS on success.
 */
usb_error_t msd_WriteSectors(msd_device_t *msd,
                             uint32_t lba,
                             const void *data);

#ifdef __cplusplus
}
#endif

#endif
