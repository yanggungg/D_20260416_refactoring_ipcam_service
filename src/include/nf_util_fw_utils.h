#ifndef __NF_UTIL_FW_UTILS_H__
#define __NF_UTIL_FW_UTILS_H__

void nf_fw_imgh_print(image_header_t *img_header);
void hexa_print(guchar *dataBuf, guint offs);
void hexa_print_with_oob(guchar *dataBuf, guint offs);
gboolean nf_fw_ls(gchar *path);

#endif

