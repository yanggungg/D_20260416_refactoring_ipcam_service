#ifndef __ITX_MICOM_UPGRADE_H_
#define __ITX_MICOM_UPGRADE_H_

#define MICOM_FW_HEADER_MAGIC		"micom"

#if defined(SUPPORT_NAND_512M)
#define MICOM_NAND_ERASE_SIZE				0x40000
#elif defined(SUPPORT_NAND_256M)
#define MICOM_NAND_ERASE_SIZE				0x20000
#elif defined(SUPPORT_NAND_128M)
#define MICOM_NAND_ERASE_SIZE				0x20000
#endif

#define MICOM_DATA_SIZE 512
#define MICOM_HEADER_SIZE 1
#define MICOM_WRITE_SIZE (MICOM_DATA_SIZE + MICOM_HEADER_SIZE)
#define MICOM_CMP_DATA_BYTE 4
#define MICOM_CMP_DATA_CNT (MICOM_CMP_DATA_BYTE * 1)

void micom_upgrade_led_blink(int event);
void micom_upgrade_thread_create(void);

#endif
