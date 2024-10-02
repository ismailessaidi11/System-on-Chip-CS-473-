#ifndef __DMA_H__
#define __DMA_H__

#define DMA_BASE_ADDRESS 0x50000040

#define MEMORY_ADDRESS_ID 0
#define SPM_ADDRESS_ID 1
#define TRANSFER_SIZE_ID 2
#define START_STATUS_ID 3

#define DMA_FROM_SPM_TO_MEM 1 << 8
#define DMA_FROM_MEM_TO_SPM 1 << 9

#define DMA_BUSY_BIT 1
#define DMA_ERROR_BIT 2
#define DMA_MEM_ALLIGN_ERROR_BIT 4
#define DMA_SPM_ALLIGN_ERROR_BIT 8
#define DMA_SPM_OUT_OF_RANGE_ERROR_BIT 16

#endif