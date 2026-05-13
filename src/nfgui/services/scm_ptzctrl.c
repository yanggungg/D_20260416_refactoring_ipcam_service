/*
 * scm_ptzctrl.c
 * 	- ptz control services
 *	- dependency :
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Oct 19, 2011
 *
 */

#include "scm.h"
#include "iux_afx.h"
#include "nf_ptz.h"
#include "scm_internal.h"
#include "ix_mem.h"
#include "evt.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_PTZCTRL"

#define ZOOM_SPEED_DEFAULT		(50)
#define FOCUS_SPEED_DEFAULT		(1)


typedef struct _PTZCTRL_T {
	NF_PTZ_CMD	ptz_cmd;
	int			zoom_speed;
	int			focus_speed;
   	int			pt_speed;
} PTZCTRL_T;

static PTZCTRL_T g_ptzctrl;



////////////////////////////////////////////////////////////
//
// private functions
//

static void _ptz_cmd_init(guint ch)
{
	PtzData ptzdata;
	NF_PTZ_CMD auto_focus;
	NF_PTZ_CMD auto_iris;
	NF_PTZ_CMD pt_speed;
	NF_PTZ_CMD focus_speed;
	NF_PTZ_CMD zoom_speed;
	NF_PTZ_CMD iris_speed;

	DAL_get_ptz_data(&ptzdata, ch);

	/* cmd cam set */
	auto_focus.ch = ch;
	auto_iris.ch = ch;
	pt_speed.ch = ch;
	focus_speed.ch = ch;
	zoom_speed.ch = ch;
	iris_speed.ch = ch;

	/* cmd set */
	auto_focus.cmd = NF_PTZ_CMD_SET_AUTO_FOCUS;
	auto_iris.cmd = NF_PTZ_CMD_SET_AUTO_IRIS;
	pt_speed.cmd = NF_PTZ_CMD_SET_PANTILT_SPEED;
	focus_speed.cmd = NF_PTZ_CMD_SET_FOCUS_SPEED;
	zoom_speed.cmd = NF_PTZ_CMD_SET_ZOOM_SPEED;
	iris_speed.cmd = NF_PTZ_CMD_SET_IRIS_SPEED;

	/* set parma from DB */
	auto_focus.params[0] = ptzdata.autoFocus;
	auto_iris.params[0] = ptzdata.autoIris;
	pt_speed.params[0] = ((ptzdata.PTSpeed)+1)*10;
	focus_speed.params[0] = ((ptzdata.focusSpeed)+1)*10;
	zoom_speed.params[0] = ((ptzdata.zoomSpeed)+1)*10;
	iris_speed.params[0] = ((ptzdata.irisSpeed)+1)*10;

	/* cmd */
	nf_ptz_cmd(&auto_focus);
	nf_ptz_cmd(&auto_iris);
	nf_ptz_cmd(&pt_speed);
	nf_ptz_cmd(&focus_speed);
	nf_ptz_cmd(&zoom_speed);
	nf_ptz_cmd(&iris_speed);

	memset(&g_ptzctrl, 0x00, sizeof(PTZCTRL_T));
	g_ptzctrl.ptz_cmd.ch = ch;
	g_ptzctrl.zoom_speed = zoom_speed.params[0];
	g_ptzctrl.focus_speed = focus_speed.params[0];
    g_ptzctrl.pt_speed = pt_speed.params[0];
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//


////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_init_ptz_param(int ch)
{
	_ptz_cmd_init(ch);
	return 0;	
}

int scm_run_ptz_cmd_left()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.pt_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_PAN_LEFT;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_left_up()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.pt_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_PT_LEFTUP;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_up()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.pt_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_TILT_UP;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_right_up()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.pt_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_PT_RIGHTUP;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_right()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.pt_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_PAN_RIGHT;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_right_down()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.pt_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_PT_RIGHTDOWN;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_down()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.pt_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_TILT_DOWN;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_left_down()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.pt_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_PT_LEFTDOWN;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_set_ptz_cmd_pt_speed(int speed)
{
	g_ptzctrl.pt_speed = speed;
	return 0;
}

int scm_run_ptz_cmd_zoom_in()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.zoom_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_ZOOM_TELE;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_zoom_out()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.zoom_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_ZOOM_WIDE;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_set_ptz_cmd_zoom_speed(int speed)
{
	g_ptzctrl.zoom_speed = speed;
	return 0;
}

int scm_run_ptz_cmd_focus_near()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.focus_speed;	
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_FOCUS_NEAR;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_focus_far()
{
	g_ptzctrl.ptz_cmd.params[0] = g_ptzctrl.focus_speed;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_FOCUS_FAR;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_set_ptz_cmd_focus_speed(int speed)
{
	g_ptzctrl.focus_speed = speed;
	return 0;
}

int scm_run_ptz_cmd_focus_auto()
{

	return 0;
}

int scm_run_ptz_cmd_iris_open()
{
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_IRIS_OPEN;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_iris_close()
{
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_IRIS_CLOSE;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_iris_auto()
{


	return 0;
}

int scm_stop_ptz_cmd()
{
	g_ptzctrl.ptz_cmd.params[0] = 0;
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_STOP;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_get_protocol_cnt()
{
	return nf_ptz_protocol_get_cnt();
}

int scm_set_ptz_preset(int param0)
{
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_SET_PRESET;
	g_ptzctrl.ptz_cmd.params[0] = param0;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_unset_ptz_preset(int param0)
{
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_CLEAR_PRESET;
	g_ptzctrl.ptz_cmd.params[0] = param0;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}

int scm_run_ptz_cmd_goto(int param0)
{
	g_ptzctrl.ptz_cmd.cmd = NF_PTZ_CMD_GOTO_PRESET;
	g_ptzctrl.ptz_cmd.params[0] = param0;
	nf_ptz_cmd(&g_ptzctrl.ptz_cmd);
	return 0;
}
