typedef struct {
	guint ch;
	guint person;
	guint vehicle;
	guint animal;
} DVA_OBJ_COUNTER;

DVA_OBJ_COUNTER *nf_dva_get_obj_count(gint ch);
void nf_dva_detector_cb_func(struct objects *objs);
void nf_dva_event_init(void);
