struct _CellLayout
{
	char Rows[5];
	char Columns[5];
};

struct _AnalyticsModules
{
	char Rows[5];
	char Columns[5];
	int Enable;
	int DayNight;
};
extern int nf_onvif_va_set_motion(NFIPCamSetupMotionArea* info, int ch);
extern int nf_onvif_va_set_motion_cells(NFIPCamSetupMotionArea* info, int ch);
extern int nf_onvif_va_get_analytics_modules(struct _AnalyticsModules *analyticsModule, int ch);
extern int nf_onvif_va_set_motion_window(NFIPCamSetupMotionArea* info, int ch);

#define DEBUG_ANALYTICS

#ifdef DEBUG_ANALYTICS
#define DEBUG_PRINT(STRING) printf("[D/ONVIF/ANALYTICS] FILE:<%-25s:%-4d>\t>>>> %s\n",__FILE__,__LINE__,(STRING));
#define DEBUG_PRINT_LINE printf("[D/ONVIF/ANALYTICS] FILE:<%-25s:%-4d>\t ##\n",__FILE__,__LINE__); \
	if(1)
#else
#define DEBUG_PRINT
#endif
