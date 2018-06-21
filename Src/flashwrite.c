

#include "stm32f4xx.h"
#include <stm32f4xx_hal_flash.h>



#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

#define FLASH_Sector_0     ((uint16_t)0x0000) /*!< Sector Number 0 */
#define FLASH_Sector_1     ((uint16_t)0x0008) /*!< Sector Number 1 */
#define FLASH_Sector_2     ((uint16_t)0x0010) /*!< Sector Number 2 */
#define FLASH_Sector_3     ((uint16_t)0x0018) /*!< Sector Number 3 */
#define FLASH_Sector_4     ((uint16_t)0x0020) /*!< Sector Number 4 */
#define FLASH_Sector_5     ((uint16_t)0x0028) /*!< Sector Number 5 */
#define FLASH_Sector_6     ((uint16_t)0x0030) /*!< Sector Number 6 */
#define FLASH_Sector_7     ((uint16_t)0x0038) /*!< Sector Number 7 */
#define FLASH_Sector_8     ((uint16_t)0x0040) /*!< Sector Number 8 */
#define FLASH_Sector_9     ((uint16_t)0x0048) /*!< Sector Number 9 */
#define FLASH_Sector_10    ((uint16_t)0x0050) /*!< Sector Number 10 */
#define FLASH_Sector_11    ((uint16_t)0x0058) /*!< Sector Number 11 */


const uint32_t Stm407FlashSectorMap[13] = 
{
    0x08000000,
    0x08004000,
    0x08008000,
    0x0800C000,        
    0x08010000,
    0x08020000,
    0x08040000,
    0x08060000, 
    0x08080000,
    0x080A0000,
    0x080C0000,
    0x080E0000,
    0x08100000
};


uint16_t GetFlashSector(uint32_t addr)
{
    int i = 0;
    
    for (i=0; i<12; i++)
    {
        if ((addr>=Stm407FlashSectorMap[i]) && (addr<Stm407FlashSectorMap[i+1]) )
        {
            return i;
        }
    }
}



#define FLASH_TIMEOUT_VALUE 1000




void writeFlash(uint32_t addr,  uint32_t *pData, uint32_t len)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef earasInit;
    uint32_t PageError = 0; 
    uint16_t sector = GetFlashSector(addr);
    uint32_t len32 = (len+3)/4;
    uint32_t *ptr32 = (uint32_t*)(pData);
    uint32_t  i = 0;
    
    
    HAL_FLASH_Unlock();

    earasInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    earasInit.Sector = sector;
    earasInit.NbSectors = 1;
    earasInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
 
    HAL_FLASHEx_Erase(&earasInit, &PageError);  
    
    for (i=0; i<len32; i++)
    {
        HAL_FLASH_Program(TYPEPROGRAM_WORD, addr+i*4, ptr32[i]);
        
        status = FLASH_WaitForLastOperation((uint32_t)FLASH_TIMEOUT_VALUE);

        /* If the program operation is completed, disable the PG Bit */
        CLEAR_BIT(FLASH->CR, FLASH_CR_PG);

        /* In case of error, stop programation procedure */
        if (status != HAL_OK)
        {
            break;
        }
    }

    HAL_FLASH_Lock();
}

