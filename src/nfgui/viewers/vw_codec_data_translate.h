#ifndef _VW_CODEC_DATA_TRANSLATE_H_
#define _VW_CODEC_DATA_TRANSLATE_H_

extern const gchar* g_resol_info[];
extern const gchar* g_fps_info[];
extern const gchar* g_quality_info[];


gchar* vw_codec_get_resol_info(gchar data);
gchar* vw_codec_get_fps_info(gchar data, gint is_ntsc);
gchar* vw_codec_get_quality_info(gchar data);

gchar vw_codec_get_resol_data(gchar *info);
gchar vw_codec_get_fps_data(gchar *info);
gchar vw_codec_get_quality_data(gchar *info);

void vw_codec_translate_capable_data_resol(guint64 capable, gchar *buf);
void vw_codec_translate_capable_data_fps(guint capable, gchar *buf);

#endif

