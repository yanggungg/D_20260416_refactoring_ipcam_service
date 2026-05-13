
#include "nf_ui_image.h"
#include "nf_ui_color.h"
#include "nf_ui_font.h"
#include "event_loop.h"


#include "nfobject.h"
#include "nfwindow.h"
#include "nfbutton.h"
#include "nfcheckbutton.h"
#include "nffixed.h"
#include "nfspinbutton.h"
#include "nfcombobox.h"
#include "nfhscale.h"
#include "nftimeline.h"
#include "nftile.h"
#include "nflistbox.h"




static gboolean btn_exit_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	switch(event->type) {
		case GDK_BUTTON_RELEASE:
			gtk_main_quit();
			break;
		default:
			break;
	}
	return FALSE;
}



int
main(int argv, char* argc[])
{
	NFOBJECT *nfwin;
	NFOBJECT *nffixed;
	NFOBJECT *nfbtn;
	NFOBJECT *nfcheck;
	NFOBJECT *nfspin;
	NFOBJECT *nfcombo;
	NFOBJECT *nfhscale;
	NFOBJECT *nftimeline;
	NFOBJECT *nftile;
	NFOBJECT *nflistbox;
	NF_TIMELINE_ELEM *t_elem;
	GdkPixbuf *image[3] = {NULL, NULL, NULL};
	GdkPixbuf *image2[3] = {NULL, NULL, NULL};
	GdkFont *font = NULL;
	guint font_color;
	GSList *slist = NULL;
	gchar *str[5] = {"aaaaa", "bbbbb", "ccccc", "ddddd", "eeeee"};
#if 1
	gchar *tmp[26] = {"01", "02","03","04","05","06","07","08","09","10","11","12","13","14","15",
					"16","17","18",	"19","20","21","22","23","24","25","26"};
#else
	gchar *tmp[17] = {"01", "02","03","04","05","06","07","08","09","10","11","12","13","14","15",
					"16","17"};
#endif
	gchar *tmp2 = "abcdefghijklmnopqrstuvwxyz";
	gint i, j;

	gtk_init(&argv, &argc);
	
	/* window */	
	nfwin = (NFOBJECT*)nfui_nfwindow_new(GTK_WINDOW_TOPLEVEL, 200, 100, 1200, 800);
	
	/* fixed */	
	nffixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_show(nffixed);

	/* button */
	image[0] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/05_00_bu_01.bmp", NULL);
	image[1] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/05_00_bu_03.bmp", NULL);
	image[2] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/05_00_bu_02.bmp", NULL);
	
	for(i=0; i<8; i++) {	
		nfbtn = (NFOBJECT*)nfui_nfbutton_new();
		if(i==7)
			nfui_regi_pre_event_callback(nfbtn, (gpointer)btn_exit_cb);
		nfui_nfbutton_set_image(NF_BUTTON(nfbtn), image);
		nfui_nfobject_set_size(nfbtn, 140, 24);
		nfui_nfobject_show(nfbtn);

		nfui_nffixed_put((NFFIXED*)nffixed, nfbtn, 30 + 145 * i, 50);

	}

	/* check button */
	image[0] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/04_00_check_01.bmp", NULL);
	image[1] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/04_00_check_02.bmp", NULL);
	image[2] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/04_00_check_01.bmp", NULL);

	for(i=0; i<8; i++) {	
		if(i == 2)
			nfcheck = (NFOBJECT*)nfui_checkbutton_new(TRUE);
		else
			nfcheck = (NFOBJECT*)nfui_checkbutton_new(FALSE);
		nfui_check_button_set_image(NF_CHECKBUTTON(nfcheck), image);
		nfui_nfobject_set_size(nfcheck, 14, 14);
		nfui_nfobject_show(nfcheck);


		nfui_nffixed_put((NFFIXED*)nffixed, nfcheck, 30 + 70 * i, 80);
	}


	/* radio button	*/
	image[0] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/04_01_radio_01.bmp", NULL);
	image[1] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/04_01_radio_02.bmp", NULL);
	image[2] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/04_01_radio_01.bmp", NULL);
	for(i=0; i<8; i++) {	
		nfbtn = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(nfbtn), image);
		nfui_nfobject_set_size(nfbtn, 14, 14);
		nfui_nfobject_show(nfbtn);

		nfui_nffixed_put((NFFIXED*)nffixed, nfbtn, 600 + 70 * i, 80);

		if(i==0) {
			slist = nfui_radio_button_get_group(NF_BUTTON(nfbtn));
		}else {
			if(i == 3)
				nfui_radio_button_set_toggled(NF_BUTTON(nfbtn), TRUE);
			nfui_radio_button_add_group(NF_BUTTON(nfbtn), slist);
		}
	}

	/* spin button */	
	font = gdk_font_load("-misc-fixed-medium-*-*-*-*-*-*-*-*-*-*-*");

	for(i=0; i<8; i++) {
		nfspin = nfui_spinbutton_new(str, 5, 0);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)nfspin, NFSPINBUTTON_TYPE_1);
		nfui_nfobject_set_size(nfspin, 140, 20);
		nfui_nffixed_put((NFFIXED*)nffixed, nfspin, 30 + 145 * i, 110);
		nfui_nfobject_show(nfspin);
	}

	/* combo */	
	image[0] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/03_00_combo_01.bmp", NULL);
	image[1] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/03_00_combo_03.bmp", NULL);
	image[2] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/03_00_combo_02.bmp", NULL);

	for(i=0; i<8; i++) {
		nfcombo = nfui_combo_box_new(str, 5, 0);
		nfui_nfobject_set_size(nfcombo, 140, 22);

		nfui_combo_box_set_arrow_image(NF_COMBOBOX(nfcombo), image, 16, 22);
		nfui_combo_box_set_font(NF_COMBOBOX(nfcombo), font, 12);

		nfui_nffixed_put((NFFIXED*)nffixed, nfcombo, 30 + 145 * i, 140);
		nfui_nfobject_show(nfcombo);
	}

	/* hscale */	
	image[0] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/13_01_slider.bmp", NULL);
	image[1] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/13_03_slider_bu_01.bmp", NULL);
	
	for(i=0; i<6; i++) {
		nfhscale = nfui_hscale_new();
		if(i == 0) 
			nfui_hscale_set_value(NF_HSCALE(nfhscale), 0);
		else if(i == 1) 
			nfui_hscale_set_value(NF_HSCALE(nfhscale), 1);
		else if(i == 2) 
			nfui_hscale_set_value(NF_HSCALE(nfhscale), 3);
		else if(i == 3) 
			nfui_hscale_set_value(NF_HSCALE(nfhscale), 4);
		else if(i == 4) 
			nfui_hscale_set_value(NF_HSCALE(nfhscale), 5);
		else if(i == 5) 
			nfui_hscale_set_value(NF_HSCALE(nfhscale), 6);

		nfui_nfobject_set_size(nfhscale, 160, 12);
		nfui_hscale_set_bg_image(NF_HSCALE(nfhscale), image[0]);
		nfui_hscale_set_bar_image(NF_HSCALE(nfhscale), image[1], 12, 12);

		nfui_nffixed_put((NFFIXED*)nffixed, nfhscale, 60 + 180 * i, 170);
		nfui_nfobject_show(nfhscale);
	}


	/* timeline */	
	t_elem = g_malloc0(sizeof(NF_TIMELINE_ELEM) * (1090 * 4));
	
	for(i=0; i<(1090 * 4); i++) {
		if(i < 272)
			t_elem[i].reason = NF_RECORD_REASON_TIMER;	
		else if(i < 545)
			t_elem[i].reason = NF_RECORD_REASON_NOTHING;	
		else if(i < 1090)
			t_elem[i].reason = NF_RECORD_REASON_ALARM;	
		else if(i < 1390)
			t_elem[i].reason = NF_RECORD_REASON_USER;	
		else if(i < 1630)
			t_elem[i].reason = NF_RECORD_REASON_ALARM;	
		else if(i < 2180)
			t_elem[i].reason = NF_RECORD_REASON_MOTION;	
		else if(i < 3270)
			t_elem[i].reason = NF_RECORD_REASON_MANUAL;	
		else if(i < 3970)
			t_elem[i].reason = NF_RECORD_REASON_PRE;	
	}

	nftimeline = nfui_timeline_new(4);
	nfui_nfobject_set_size(nftimeline, 1090, 200);
	nfui_timeline_set_data(NF_TIMELINE(nftimeline), t_elem);
	nfui_nffixed_put((NFFIXED*)nffixed, nftimeline, 80, 200);
	nfui_nfobject_show(nftimeline);

	g_free(t_elem);
	
#if 0

	/* tile */	
	nftile = nfui_tile_new(24, 24);
	nfui_nfobject_set_size(nftile, 1140, 380);
	nfui_nffixed_put((NFFIXED*)nffixed, nftile, 30, 430);
	nfui_nfobject_show(nftile);
	nfui_tile_set_selectArea(NF_TILE(nftile), 1, 1, 10, 10);
	nfui_tile_set_selectArea(NF_TILE(nftile), 11, 11, 20, 20);
	nfui_tile_set_selectArea(NF_TILE(nftile), 1, 1, 2, 2);
#else

	/* listbox */
	image[0] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/03_03_spin_up_01.bmp", NULL);
	image[1] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/03_03_spin_up_03.bmp", NULL);
	image[2] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/03_03_spin_up_02.bmp", NULL);

	image2[0] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/03_04_spin_down_01.bmp", NULL);
	image2[1] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/03_04_spin_down_03.bmp", NULL);
	image2[2] = gdk_pixbuf_new_from_file("../../data/gui/bmp/4D1/03_04_spin_down_02.bmp", NULL);

	nflistbox = nfui_listbox_new();	
	nfui_nfobject_set_size(nflistbox, 800, 360);
#if 0
	nfui_listbox_set_text(NF_LISTBOX(nflistbox), tmp, 17);
#else
	nfui_listbox_set_text(NF_LISTBOX(nflistbox), tmp, 26);
#endif
	nfui_listbox_set_font(NF_LISTBOX(nflistbox), font, COLOR_IDX(NOT_CARE));
	nfui_listbox_set_arrow_image(NF_LISTBOX(nflistbox), SCROLL_UP, image, 22, 10);
	nfui_listbox_set_arrow_image(NF_LISTBOX(nflistbox), SCROLL_DOWN, image2, 22, 10);
	nfui_nffixed_put((NFFIXED*)nffixed, nflistbox, 80, 430);
	nfui_nfobject_show(nflistbox);
#endif


	nfui_nfwindow_add((NFWINDOW*)nfwin, nffixed);

	nfui_run_main_event_handler(nfwin);

	nfui_nfobject_show(nfwin);

	gtk_main();

	return 0;
}
