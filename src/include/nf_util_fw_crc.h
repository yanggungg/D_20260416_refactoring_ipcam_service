#ifndef __NF_UTIL_FW_CRC_H__
#define __NF_UTIL_FW_CRC_H__

gulong fwup_chk_crc(gulong crc, gint mtd_num, guint len, gboolean is_header, gint img_type, gint prgt_type);
gulong fwup_chk_crc_ext4fs(FILE *fp, guint location_fs, gulong crc, gint mtd_num, guint len, gboolean is_header,
										gint img_type, gint prgt_type);
gulong header_crc_check(gulong crc, guchar *buf, guint len);
gulong data_crc_check(gulong crc, FILE *fp, guint len, gint img_type);

#endif

