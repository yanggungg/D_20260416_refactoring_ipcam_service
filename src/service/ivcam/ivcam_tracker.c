// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2013 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2013/02/18 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  ivcam_tracker.c
 * @brief  This file contains implementation of meta data tracker.
 * @author  Jongbin Yim.
 * @data  2013/02/18
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include "ivcam_tracker.h"

#include <math.h>

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static const unsigned char sqq_table[] = {
   0,  16,  22,  27,  32,  35,  39,  42,  45,  48,  50,  53,  55,  57,
  59,  61,  64,  65,  67,  69,  71,  73,  75,  76,  78,  80,  81,  83,
  84,  86,  87,  89,  90,  91,  93,  94,  96,  97,  98,  99, 101, 102,
 103, 104, 106, 107, 108, 109, 110, 112, 113, 114, 115, 116, 117, 118,
 119, 120, 121, 122, 123, 124, 125, 126, 128, 128, 129, 130, 131, 132,
 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 144, 145,
 146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155, 155, 156, 157,
 158, 159, 160, 160, 161, 162, 163, 163, 164, 165, 166, 167, 167, 168,
 169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 178,
 179, 180, 181, 181, 182, 183, 183, 184, 185, 185, 186, 187, 187, 188,
 189, 189, 190, 191, 192, 192, 193, 193, 194, 195, 195, 196, 197, 197,
 198, 199, 199, 200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206,
 207, 208, 208, 209, 209, 210, 211, 211, 212, 212, 213, 214, 214, 215,
 215, 216, 217, 217, 218, 218, 219, 219, 220, 221, 221, 222, 222, 223,
 224, 224, 225, 225, 226, 226, 227, 227, 228, 229, 229, 230, 230, 231,
 231, 232, 232, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238,
 239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246,
 246, 247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253,
 253, 254, 254, 255
};

unsigned int
sqrti(unsigned int x)
{
	unsigned int xn;

	if (x >= 0x10000) {
		if (x >= 0x1000000) {
			if (x >= 0x10000000) {
				if (x >= 0x40000000) {
					if (x >= 65535UL*65535UL)
						return 65535;
					xn = sqq_table[x>>24] << 8;
				}
				else
					xn = sqq_table[x>>22] << 7;
			}
			else {
				if (x >= 0x4000000)
					xn = sqq_table[x>>20] << 6;
				else
					xn = sqq_table[x>>18] << 5;
			}
			xn = (xn + 1 + x / xn) >> 1;
		}
		else {
			if (x >= 0x100000)
				if (x >= 0x400000)
					xn = sqq_table[x>>16] << 4;
				else
					xn = sqq_table[x>>14] << 3;
			else
				if (x >= 0x40000)
					xn = sqq_table[x>>12] << 2;
				else
					xn = sqq_table[x>>10] << 1;

		}
		xn = (xn + 1 + x / xn) >> 1;
		return xn - (xn * xn > x);
	}
	else {
		if (x >= 0x100) {
			if (x >= 0x1000)
				if (x >= 0x4000)
					xn = (sqq_table[x>>8] >> 0) + 1;
				else
					xn = (sqq_table[x>>6] >> 1) + 1;
			else
				if (x >= 0x400)
					xn = (sqq_table[x>>4] >> 2) + 1;
				else
					xn = (sqq_table[x>>2] >> 3) + 1;

			return xn - (xn * xn > x);
		}
		else
			return sqq_table[x] >> 4;
	}
}	/* sqrti(... */


ivca_mat_t *
ivca_mat_init(ivca_ptr_t buf, ivca_s32_t nrows, ivca_s32_t ncols)
{
	ivca_s32_t i;
	ivca_mat_t * __restrict mat = (ivca_mat_t *)buf;

	if ( !mat )
		return NULL;
	mat->nrows = nrows;
	mat->ncols = ncols;
	mat->mdata = (ivca_s32_t **)(mat + 1);
	mat->rdata = (ivca_s32_t *)&((ivca_s32_t **)(mat + 1))[nrows];
	for (i = 0; i < nrows; i++)
		mat->mdata[i] = &mat->rdata[i * ncols];
	return mat;
}	

ivca_mat_t *
ivca_mat_create(ivca_s32_t nrows, ivca_s32_t ncols)
{
	ivca_mat_t *mat;

	mat = (ivca_mat_t *)calloc(1,MAT_SIZE(nrows, ncols));
	if ( mat )
		ivca_mat_init(mat, nrows, ncols);
	return mat;
}

void
ivca_mat_release(ivca_mat_t *mat)
{
	if ( mat )
		free(mat);
}	

static ivca_s32_t
_compute_homography(ai_ivcam_tracker_t *tracker, ivca_calib_t *calib)
{
	ivca_f32_t h, phi, f, u0, v0;
	ivca_s32_t *rHg = tracker->Hg->rdata;
	ivca_s32_t *riHg = tracker->Hginv->rdata;
	ivca_s32_t qh = 22, qhi = 28;

	if ( !calib->paramvalid )
		return -1;

	u0 = (ivca_f32_t )calib->p_width / 2;
	v0 = (ivca_f32_t )calib->p_height / 2;
	h = calib->height;
	phi = (calib->tilt - 90.0F) * (ivca_f32_t)M_PI / 180.0F;
	f = calib->focal;

	/* Compute Hg. */
	rHg[0] = FIXED(1 / h * tracker->rx, qh);
	rHg[1] = FIXED(u0 * sinf(phi) / f / h * tracker->rx, qh);
	rHg[2] = FIXED(u0 * cosf(phi) / f * tracker->rx, qh);
	rHg[3] = 0;
	rHg[4] = FIXED(v0 * sinf(phi) / f / h + cosf(phi) / h * tracker->ry, qh);
	rHg[5] = FIXED(v0 * cosf(phi) / f - sinf(phi) * tracker->ry, qh);
	rHg[6] = 0;
	rHg[7] = FIXED(sinf(phi) / f / h, qh);
	rHg[8] = FIXED(cosf(phi) / f, qh);

	/* Compute iHg. */
	riHg[0] = FIXED(1 / f / tracker->rx, qhi);
	riHg[1] = 0;
	riHg[2] = FIXED(-u0 / f, qhi);
	riHg[3] = 0;
	riHg[4] = FIXED(cosf(phi) / f / tracker->ry, qhi);
	riHg[5] = FIXED(sinf(phi) - v0 * cosf(phi) / f, qhi);
	riHg[6] = 0;
	riHg[7] = FIXED(-sinf(phi) / f / h / tracker->ry, qhi);
	riHg[8] = FIXED(cosf(phi) / h + v0 * sinf(phi) / f / h, qhi);

	tracker->uv0.x = (ivca_s16_t)u0;
	tracker->uv0.y = (ivca_s16_t)v0;

	/* Compute rho vector. */
	tracker->rho[0] = FIXED(-sinf(phi) * sinf(phi), 16);	/* Q.16 */
	tracker->rho[1] = FIXED(f * sinf(phi) * cosf(phi), 0);	/* Q.0 */
	tracker->rho[2] = FIXED(sinf(phi) * cosf(phi) / f, 28);	/* Q.28 */
	tracker->rho[3] = FIXED(-cosf(phi) * cosf(phi), 16);	/* Q.16 */
	tracker->rho_s = FIXED(h, 8);							/* Q.8 */
	return 0;
}

static void
_convert_to_ground(ai_ivcam_tracker_t *tracker, ivca_s16_t u, ivca_s16_t v,
		ivca_s32_t *x, ivca_s32_t *y)
{
	ivca_s32_t s, *riHg = tracker->Hginv->rdata;
	ivca_s32_t tmp, *_x = x ? x : &tmp, *_y = y ? y : &tmp;

	s = FCONVD(riHg[7] * v + riHg[8], 8);
	if ( s != 0 ) {
		*_x = (riHg[0] * u + riHg[2]) / s;
		*_y = (riHg[4] * v + riHg[5]) / s;
	}
	else
		*_x = *_y = 0;
}	/* _convert_to_ground(... */

static void
_convert_to_gspeed(ai_ivcam_tracker_t *tracker,
		ivca_s32_t u, ivca_s32_t v, ivca_s32_t x, ivca_s32_t y,
		ivca_s32_t vu, ivca_s32_t vv, ivca_s32_t *vx, ivca_s32_t *vy)
{
	ivca_s32_t s, *riHg = tracker->Hginv->rdata;

	s = FCONVD(riHg[7] * v + riHg[8], 8);
	if ( s != 0 ) {
		*vx = ((riHg[0] * vu - FMUL(riHg[7], x, 8) * vu) >> Q_ST)*15 / s;
		*vy = ((riHg[4] * vv - FMUL(riHg[7], y, 8) * vv) >> Q_ST)*15 / s;
	}
	else
		*vx = *vy = 0;
}	/* _convert_to_gspeed(... */


static void
_compute_3d_info(ai_ivcam_tracker_t *tracker, ai_ivcam_track_obj_t *obj)
{
	ivca_s16_t vh, vf;
	ivca_s32_t x1, s, *r, vx, vy, dx, dy;
	ai_meta_obj_t mobj;

	if ( !tracker->Hginv )
		return;

	mobj = obj->mobj;

	obj->rect.x = mobj.bbx_position[0]*3840;
	obj->rect.y = mobj.bbx_position[1]*2160;
	obj->rect.w = (mobj.bbx_position[2] - mobj.bbx_position[0])*3840;
	obj->rect.h = (mobj.bbx_position[3] - mobj.bbx_position[1])*2160;

	obj->gndp.x = obj->rect.x + (obj->rect.w >> 1);
	obj->gndp.y = obj->rect.y + obj->rect.h;

	obj->cent.x = obj->rect.x + (obj->rect.w >> 1);
	obj->cent.y = obj->rect.y + (obj->rect.h >> 1);

	/* Estimate velocity. */
	dx = FCONVI(obj->cent.x - obj->old_cent.x, Q_ST);
	dy = FCONVI(obj->cent.y - obj->old_cent.y, Q_ST);
	obj->vx = (obj->vx + dx) >> 1;
	obj->vy = (obj->vy + dy) >> 1;

	/* Maximum possible height. */
	r = tracker->rho;
	vh = obj->rect.y / tracker->ry - tracker->uv0.y;
	vf = (obj->rect.y + obj->rect.h) / tracker->ry - tracker->uv0.y;
	s = (FMUL(vf, FMUL(vh, r[2], 8), 20) + FMUL(vh, r[3], 16) -
			FMUL(vf, r[0], 16) - r[1]) * tracker->ry;
	obj->gh = s > 0 ? (tracker->rho_s * obj->rect.h / s) : 0;
	obj->gh = min(obj->gh, FIXED(255.999, 8));

	/* Ground coordinate. */
	_convert_to_ground(tracker, obj->gndp.x, obj->gndp.y, &obj->gx, &obj->gy);
	_convert_to_ground(tracker, obj->rect.x, obj->gndp.y, &x1, NULL);
	/* Diameter. */
	obj->gw = abs(obj->gx - x1) * 2;
	obj->gw = min(obj->gw, FIXED(255, 8));

	/* Speed. */
	_convert_to_gspeed(tracker, obj->gndp.x, obj->gndp.y, obj->gx, obj->gy,
			obj->vx, obj->vy, &vx, &vy);
	// apply time gap
	//vx = vx*1000/obj->gt;
	//vy = vy*1000/obj->gt;
	//printf("_compute_3d_info dx %d dy %d cent.x %d %d cent.y %d %d obj->gx %d obj->gy %d\n",obj->vx,obj->vy,obj->cent.x,obj->old_cent.x,obj->cent.y,obj->old_cent.y, obj->gx, obj->gy);
	
	obj->gvx = (7*obj->gvx + vx) / 8;
	obj->gvy = (7*obj->gvy + vy) / 8;
	obj->gspeed = sqrti(obj->gvx * obj->gvx + obj->gvy * obj->gvy) *
			3600 / 1000;
	obj->gspeed = min(obj->gspeed, FIXED(SPEED3D_MAX, 8));

	//printf("_compute_3d_info ID %d W=%.1fm H=%.1fm S=%.1fkm/h\n",obj->mobj.id,(float) obj->gw/256,(float) obj->gh/256,(float) obj->gspeed/256);
	
}	/* _compute_3d_info(... */




ivca_s32_t
ai_tracker_set_homography(ai_ivcam_tracker_t *tracker, ivca_calib_t *calib)
{
	if ( !tracker )
		return -1;

	if ( calib && calib->paramvalid ) {
		if ( !tracker->Hg )
			tracker->Hg = ivca_mat_create(3, 3);
		if ( !tracker->Hginv )
			tracker->Hginv = ivca_mat_create(3, 3);
		if ( !tracker->Hg || !tracker->Hginv )
			return -1;

		if ( _compute_homography(tracker, calib) < 0 )
			return -1;
	}
	else {
		if ( tracker->Hg )
			ivca_mat_release(tracker->Hg);
		if ( tracker->Hginv )
			ivca_mat_release(tracker->Hginv);
		tracker->Hg = tracker->Hginv = NULL;
	}
	return 0;
}	


ai_ivcam_tracker_t *
ai_ivcam_tracker_create(ivca_s32_t id, ivca_s32_t nmaxobjs)
{
	ai_ivcam_tracker_t *tracker;

	tracker = (ai_ivcam_tracker_t *)calloc(1, sizeof(*tracker));
	if ( !tracker )
		return NULL;
	tracker->size = sizeof(*tracker);
	tracker->id = id;
	tracker->nmaxobjs = nmaxobjs;
	tracker->rx = 3840/320;
	tracker->ry = 2160/180;
	tracker->Hg = tracker->Hginv = NULL;
	
	list_init(&tracker->tlist);
	list_init(&tracker->list_static);

	tracker->static_cnt = 0;

	tracker->mtbl = (ai_meta_obj_t **)calloc(nmaxobjs,
			sizeof(ai_meta_obj_t *));
	if ( !tracker->mtbl ) {
		ai_ivcam_tracker_release(tracker);
		return NULL;
	}

	return tracker;
}	/* ai_ivcam_tracker_create(... */

ivca_s32_t
ai_ivcam_tracker_release(ai_ivcam_tracker_t *tracker)
{
	ai_ivcam_track_obj_t *obj, *tmp;
	ai_static_region_t *static_region, *tmp1;

	if ( !tracker || tracker->size != sizeof(*tracker) )
		return -1;

	list_for_each_entry_safe(obj, tmp, &tracker->tlist,
			ai_ivcam_track_obj_t, list) {
		list_del(&obj->list);
		free(obj);
	}
	
	list_for_each_entry_safe(static_region, tmp1, &tracker->list_static,
			ai_static_region_t, list) {
		list_del(&static_region->list);
		free(static_region);
	}
	
	if ( tracker->mtbl )
		free(tracker->mtbl);
	free(tracker);
	return 0;
}	/* ivcam_tracker_release(... */

static __inline void
_ai_update_trajectory(ivca_u08_t tref,
		ai_ivcam_track_obj_t *obj, ai_meta_obj_t *mobj)
{
	//captainnn
	//obj->traj[obj->t_tail].x = mobj->rc.x + mobj->rc.w / 2;
	obj->traj[obj->t_tail].x = mobj->bbx_position[0]*3840 + ((mobj->bbx_position[2] - mobj->bbx_position[0])*3840)/2;
	if ( tref == IVCA_TREF_CENT )
		obj->traj[obj->t_tail].y = mobj->bbx_position[1]*2160 + ((mobj->bbx_position[3] - mobj->bbx_position[1])*2160)/2;//mobj->rc.y + mobj->rc.h / 2;
	else
		obj->traj[obj->t_tail].y = mobj->bbx_position[1]*2160 + (mobj->bbx_position[3] - mobj->bbx_position[1])*2160;//mobj->rc.y + mobj->rc.h;
	obj->t_tail = (obj->t_tail + 1) % NR_REFP_HIST;
	if ( obj->t_head == obj->t_tail )
		obj->t_head = (obj->t_head + 1) % NR_REFP_HIST;
}

ivca_u08_t
conv_static_filter_sen(ivca_u08_t sense)
{
	switch(sense){
		case 2:
			return 3;
		case 1:
			return 5;
		case 0:
			return 10;
		default:
			return 10;
	}
}

#define MAX_STATIC_REGION 30
ivca_s32_t
ai_ivcam_tracker_process(ai_ivcam_tracker_t *tracker,
		ai_meta_hdr_t *pMTH, ai_meta_ckh_t *pCKH, ivca_option_t *opt)
{
	ai_meta_obj_t *pCMObj, *pCKB = (ai_meta_obj_t *)(pCKH + 1);
	ai_ivcam_track_obj_t *obj, *tmp;
	ivca_u32_t i, count = pCKH ? pCKH->priv : 0;
	ivca_u08_t sens = conv_static_filter_sen(opt->static_filter_sense);

	if ( !tracker || tracker->size != sizeof(*tracker) )
		return -1;

	/* Store pointers of the current meta objects for indexing. */
	for (i = 0, pCMObj = pCKB; i < count; i++) {
		tracker->mtbl[i] = pCMObj;
		pCMObj = !pCMObj->hvalid ? pCMObj + 1 :
				(ai_meta_obj_t *)((ai_meta_objhist_t *)(pCMObj + 1) + 1);
	}

	list_for_each_entry_safe(obj, tmp, &tracker->tlist,
			ai_ivcam_track_obj_t, list) {
		/* Find a match for obj from the current meta obj. */
		for (i = 0; i < count; i++) {
			pCMObj = tracker->mtbl[i];
			if ( pCMObj && obj->mobj.id == pCMObj->id ) {
				tracker->mtbl[i] = NULL;
				break;
			}
		}
		if ( i < count ) {
			ivca_u08_t tmp_static;
			
			/* obj is matched to pCMObj: Update obj. */
			obj->match = 1;
			obj->old_cent = obj->cent;
			tmp_static = obj->mobj.isstatic;
			obj->mobj = *pCMObj;
			obj->mobj.isstatic = tmp_static;
			_ai_update_trajectory(pMTH->track_ref, obj, pCMObj);
			if ( pCMObj->hvalid ) {
				memcpy(obj->hdata, pCMObj + 1, sizeof(obj->hdata));
				obj->hvalid = 1;
			}
			//obj->fn_last = pMTH->framenum;
			if ( obj->npts < NR_REFP_HIST - 1 )
				obj->npts++;

			//static check
			if(obj->static_object == 0 && (abs(obj->rect.x - pCMObj->bbx_position[0]*3840) >  (obj->rect.w)/sens ||
							abs(obj->rect.y - pCMObj->bbx_position[1]*2160) > (obj->rect.h)/sens ||
							abs(obj->rect.w - ( pCMObj->bbx_position[2] -  pCMObj->bbx_position[0])*3840) > (obj->rect.w)/sens ||
							abs(obj->rect.h - ( pCMObj->bbx_position[3] -  pCMObj->bbx_position[1])*2160) > (obj->rect.h)/sens)) {
				obj->static_check_time = 0;
			}

			if ( tracker->Hginv ) {
				if(pMTH->timestamp == obj->timestamp){
					obj->gt = (pMTH->timestampl - obj->timestampl)/1000;
				}
				else{
					obj->gt = (pMTH->timestamp - obj->timestamp)*1000;
					if(pMTH->timestampl >= obj->timestampl){
						obj->gt += (pMTH->timestampl - obj->timestampl)/1000;
					}
					else{
						obj->gt -= (obj->timestampl - pMTH->timestampl)/1000;
					}
				}
				
				/* Compute 3d information. */
				_compute_3d_info(tracker, obj);

				obj->mobj.width3d = (ivca_u16_t)obj->gw;
				obj->mobj.height3d = (ivca_u16_t)obj->gh;
				obj->mobj.speed3d = (ivca_u16_t)obj->gspeed;
				obj->mobj.vx3d = (ivca_s16_t)obj->gvx;
				obj->mobj.vy3d = (ivca_s16_t)obj->gvy;
				//printf("_compute_3d_info ID %d W=%.1fm H=%.1fm S=%.1fkm/h\n",obj->mobj.id,(float) obj->mobj.width3d/256,(float) obj->mobj.height3d/256,(float) obj->mobj.speed3d/256);

			}
			else{
				
				obj->rect.x = pCMObj->bbx_position[0]*3840;
				obj->rect.y =  pCMObj->bbx_position[1]*2160;
				obj->rect.w = ( pCMObj->bbx_position[2] -  pCMObj->bbx_position[0])*3840;
				obj->rect.h = ( pCMObj->bbx_position[3] -  pCMObj->bbx_position[1])*2160;
	
				obj->mobj.width3d = obj->mobj.height3d = obj->mobj.speed3d = 0;
				obj->mobj.vx3d = obj->mobj.vy3d = 0;
			}
			
			if(obj->static_object == 0 && obj->static_check_time== 0){
				obj->static_check_time = pMTH->timestamp;
				obj->static_object = 0;
			}
			else if(obj->static_object == 0){
				if(pMTH->timestamp - obj->static_check_time >= 30){
					
					//static object
					obj->static_object = 1;
					obj->static_check_time = 0;

					if(opt->en_static_filter){
						ai_static_region_t * static_region;
						char tmp_check =0;
						
						list_for_each_entry_reverse(static_region,&tracker->list_static,ai_static_region_t,list){
							if(abs(obj->rect.x - static_region->x) <=  (static_region->w)/sens &&
								abs(obj->rect.y - static_region->y) <= (static_region->h)/sens &&
								abs(obj->rect.w - static_region->w) <= (static_region->w)/sens &&
								abs(obj->rect.h - static_region->h) <= (static_region->h)/sens){
								tmp_check = 1;
								break;
							}
						}

						if(!tmp_check){
						
							if(tracker->static_cnt >= MAX_STATIC_REGION){
								list_for_each_entry_reverse(static_region,&tracker->list_static,ai_static_region_t,list){
									list_del(&static_region->list);
									tracker->static_cnt--;
									free(static_region);
									break;
								}
							}
							
							static_region = (ai_static_region_t *)malloc(sizeof(ai_static_region_t));
							static_region->x = obj->rect.x;
							static_region->y = obj->rect.y;
							static_region->w = obj->rect.w;
							static_region->h = obj->rect.h;
							list_add_head(&static_region->list, &tracker->list_static);
							tracker->static_cnt++;
							
							printf("SET static object ID %d %d %d %d %d static_cnt %d \n",pCMObj->id,static_region->x,static_region->y,static_region->w,static_region->h,tracker->static_cnt);
						}
					}
				}
			}
			else if(obj->static_object == 1){
				obj->static_del_time = 0;
			}
			obj->timestamp = pMTH->timestamp;
			obj->timestampl = pMTH->timestampl;

			if(opt->en_static_filter && obj->mobj.isstatic){
				// check static region
				int static_flag = 0;
				ai_static_region_t* static_region;

				list_for_each_entry(static_region, &tracker->list_static, ai_static_region_t, list) {
					if(abs(obj->rect.x - static_region->x) <=  (static_region->w)/sens &&
								abs(obj->rect.y - static_region->y) <= (static_region->h)/sens &&
								abs(obj->rect.w - static_region->w) <= (static_region->w)/sens &&
								abs(obj->rect.h - static_region->h) <= (static_region->h)/sens) {
						static_flag = 1;
						break;
					}
				}

				if(static_flag ==0){
					obj->mobj.isstatic = 0;
					//printf("isstatic clear %d %d %d %d \n",obj->rect.x,obj->rect.y,obj->rect.w,obj->rect.h);
				}
				else{
					//printf("isstatic continue \n");
				}

			}

			if ( tracker->Hginv && opt->en_usecalib){
				if((opt->min_width3d > (float) (obj->mobj.width3d*1000/256))
					|| (opt->min_height3d > (float) (obj->mobj.height3d*1000/256))){
					list_del(&obj->list);
					tracker->tcount--;
					free(obj);
				}		
			}
		}
		else {
			obj->match = 0;
			if(obj->static_object && obj->static_del_time==0){
				obj->static_del_time = pMTH->timestamp;
				continue;
			}
			else if(obj->static_object){
				if(pMTH->timestamp - obj->static_del_time < 70){
					continue;
				}
				//printf("DEL static object ID %d \n",obj->mobj.id);
			}
			//obj->mobj.tframes = _fn_diff(pMTH->framenum, obj->fn_first);
			/* obj is not matched. */
			//dfn = _fn_diff(pMTH->framenum, obj->fn_last);
			/* Rollback to previous code. */
			//captainnn
			/*
			if ( dfn > (ivca_u32_t)5 * pMTH->framerate ) {
				list_del(&obj->list);
				tracker->tcount--;
				free(obj);
			}*/
			list_del(&obj->list);
			tracker->tcount--;
			free(obj);
		}
	}

	for (i = 0; i < count; i++) {
		pCMObj = tracker->mtbl[i];
		if ( pCMObj ) {
			
			/* Make a new object and add to the track list. */
			obj = (ai_ivcam_track_obj_t *)malloc(sizeof(ai_ivcam_track_obj_t));
			obj->mobj = *pCMObj;
			obj->t_head = obj->t_tail = 0;
			_ai_update_trajectory(pMTH->track_ref, obj, pCMObj);
			//obj->fn_first = pMTH->framenum - pCMObj->tframes;
			//obj->fn_last = pMTH->framenum;
			obj->hvalid = pCMObj->hvalid;
			if ( pCMObj->hvalid )
				memcpy(obj->hdata, pCMObj + 1, sizeof(obj->hdata));
			memset(obj->zstate, 0, sizeof(obj->zstate));
			obj->zevent_count = 0;
			obj->npts = 1;
			obj->match = 1;
			
			obj->gx = obj->gy = 0;
			obj->vx = obj->vy = 0;
			obj->gvx = obj->gvy = 0;
			obj->timestamp = pMTH->timestamp;
			obj->timestampl = pMTH->timestampl;
			obj->static_check_time = 0;
			obj->static_object = 0;
			obj->static_del_time = 0;
			obj->removed = 0;
			obj->minimum_size = 0;
			
			obj->rect.x = pCMObj->bbx_position[0]*3840;
			obj->rect.y =  pCMObj->bbx_position[1]*2160;
			obj->rect.w = ( pCMObj->bbx_position[2] -  pCMObj->bbx_position[0])*3840;
			obj->rect.h = ( pCMObj->bbx_position[3] -  pCMObj->bbx_position[1])*2160;

			obj->cent.x = obj->rect.x + (obj->rect.w >> 1);
			obj->cent.y = obj->rect.y + (obj->rect.h >> 1);

			if(opt->en_static_filter){
				// check static region
				int static_flag = 0;
				ai_static_region_t* static_region;
					
				list_for_each_entry(static_region, &tracker->list_static, ai_static_region_t, list) {
					if(abs(obj->rect.x - static_region->x) <=  (static_region->w)/sens &&
								abs(obj->rect.y - static_region->y) <= (static_region->h)/sens &&
								abs(obj->rect.w - static_region->w) <= (static_region->w)/sens &&
								abs(obj->rect.h - static_region->h) <= (static_region->h)/sens) {
						static_flag = 1;
						break;
					}
				}	

				if(static_flag){
					//printf("static start ID %d %d %d %d %d \n",(*pCMObj).id,obj->rect.x,obj->rect.y,obj->rect.w,obj->rect.h);
					obj->mobj.isstatic = 1;
					//continue;
				}
			}

			if ( tracker->Hginv && opt->en_usecalib){

				/* Compute 3d information. */
				_compute_3d_info(tracker, obj);

				obj->mobj.width3d = (ivca_u16_t)obj->gw;
				obj->mobj.height3d = (ivca_u16_t)obj->gh;
				obj->gx = obj->gy = 0;
				obj->vx = obj->vy = 0;
				obj->gvx = obj->gvy = 0;
				
				if((opt->min_width3d > (float) (obj->mobj.width3d*1000/256))
					|| (opt->min_height3d > (float) (obj->mobj.height3d*1000/256))){
				//printf("min_width3d free %d obj->mobj.width3d %f en_static_filter %d \n",opt->min_width3d,(float) obj->mobj.width3d/256, opt->en_static_filter);
					free(obj);
				}
				else{
					list_add_head(&obj->list, &tracker->tlist);
					tracker->tcount++;
					tracker->totcount++;	
				}
			}
			else{
				list_add_head(&obj->list, &tracker->tlist);
				tracker->tcount++;
				tracker->totcount++;
			}
		}
	}

	return 0;
}	



/**
 * @brief  Creates a meta data tracker.
 *
 * @param[in] id  Tracker id. (i.e. channel number)
 * @param[in] nmaxobjs  Maximum number of tracking objects in a frame.
 *
 * @return
 *  - Pointer to the created meta data tracker if successful.
 *  - NULL if failed.
 *
 * @sa  ivcam_tracker_release(), ivcam_tracker_process()
 */
ivcam_tracker_t *
ivcam_tracker_create(ivca_s32_t id, ivca_s32_t nmaxobjs)
{
	ivcam_tracker_t *tracker;

	tracker = (ivcam_tracker_t *)calloc(1, sizeof(*tracker));
	if ( !tracker )
		return NULL;
	tracker->size = sizeof(*tracker);
	tracker->id = id;
	tracker->nmaxobjs = nmaxobjs;
	list_init(&tracker->tlist);

	tracker->mtbl = (ivca_meta_obj_t **)calloc(nmaxobjs,
			sizeof(ivca_meta_obj_t *));
	if ( !tracker->mtbl ) {
		ivcam_tracker_release(tracker);
		return NULL;
	}

	return tracker;
}	/* ivcam_tracker_create(... */

/**
 * @brief  Releases a meta data tracker.
 *
 * @param[in] tracker  Pointer to the meta data tracker to release.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 *
 * @sa  ivcam_tracker_create(), ivcam_tracker_process()
 */
ivca_s32_t
ivcam_tracker_release(ivcam_tracker_t *tracker)
{
	ivcam_track_obj_t *obj, *tmp;

	if ( !tracker || tracker->size != sizeof(*tracker) )
		return -1;

	list_for_each_entry_safe(obj, tmp, &tracker->tlist,
			ivcam_track_obj_t, list) {
		list_del(&obj->list);
		free(obj);
	}
	if ( tracker->mtbl )
		free(tracker->mtbl);
	free(tracker);
	return 0;
}	/* ivcam_tracker_release(... */

/**
 * @brief  Updates trajectory of an object.
 */
static __inline void
_update_trajectory(ivca_u08_t tref,
		ivcam_track_obj_t *obj, ivca_meta_obj_t *mobj)
{
	obj->traj[obj->t_tail].x = mobj->rc.x + mobj->rc.w / 2;
	if ( tref == IVCA_TREF_CENT )
		obj->traj[obj->t_tail].y = mobj->rc.y + mobj->rc.h / 2;
	else
		obj->traj[obj->t_tail].y = mobj->rc.y + mobj->rc.h;
	obj->t_tail = (obj->t_tail + 1) % NR_REFP_HIST;
	if ( obj->t_head == obj->t_tail )
		obj->t_head = (obj->t_head + 1) % NR_REFP_HIST;
}	/* _update_trajectory(... */

/**
 * @breif  Gets difference between frame numbers.
 */
static ivca_u32_t
_fn_diff(ivca_u32_t fn_curr, ivca_u32_t fn_prev)
{
	return fn_curr >= fn_prev ? (fn_curr - fn_prev) :
			((0xFFFFFFFF - fn_prev) + fn_curr + 1);
}	/* _fn_diff(... */

/**
 * @brief  Performs meta data tracking for the current frame.
 *
 * @param[in] tracker  Pointer to the meta data tracker.
 * @param[in] pMTH  Pointer to the meta data header.
 * @param[in] pCKH  Pointer to the object chunk header for the current frame.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 *
 * @sa  ivcam_tracker_create(), ivcam_tracker_release()
 *
 * @todo  We may speed up the tracking by using hash, since the metadata
 *  tracking is just one-to-one matching.
 */
ivca_s32_t
ivcam_tracker_process(ivcam_tracker_t *tracker,
		ivca_meta_hdr_t *pMTH, ivca_meta_ckh_t *pCKH)
{
	ivca_meta_obj_t *pCMObj, *pCKB = (ivca_meta_obj_t *)(pCKH + 1);
	ivcam_track_obj_t *obj, *tmp;
	ivca_u32_t dfn, i, count = pCKH ? pCKH->priv : 0;

	if ( !tracker || tracker->size != sizeof(*tracker) )
		return -1;

	/* Store pointers of the current meta objects for indexing. */
	for (i = 0, pCMObj = pCKB; i < count; i++) {
		tracker->mtbl[i] = pCMObj;
		pCMObj = !pCMObj->hvalid ? pCMObj + 1 :
				(ivca_meta_obj_t *)((ivca_meta_objhist_t *)(pCMObj + 1) + 1);
	}

	list_for_each_entry_safe(obj, tmp, &tracker->tlist,
			ivcam_track_obj_t, list) {
		/* Find a match for obj from the current meta obj. */
		for (i = 0; i < count; i++) {
			pCMObj = tracker->mtbl[i];
			if ( pCMObj && obj->mobj.id == pCMObj->id ) {
				tracker->mtbl[i] = NULL;
				break;
			}
		}
		if ( i < count ) {
			/* obj is matched to pCMObj: Update obj. */
			obj->match = 1;
			obj->mobj = *pCMObj;
			_update_trajectory(pMTH->track_ref, obj, pCMObj);
			if ( pCMObj->hvalid ) {
				memcpy(obj->hdata, pCMObj + 1, sizeof(obj->hdata));
				obj->hvalid = 1;
			}
			obj->fn_last = pMTH->framenum;
			if ( obj->npts < NR_REFP_HIST - 1 )
				obj->npts++;
		}
		else {
			obj->match = 0;
			obj->mobj.tframes = _fn_diff(pMTH->framenum, obj->fn_first);
			/* obj is not matched. */
			dfn = _fn_diff(pMTH->framenum, obj->fn_last);
			/* Rollback to previous code. */
			if ( dfn > (ivca_u32_t)5 * pMTH->framerate ) {
//			if(dfn > 2){ /* nonsense... */
				list_del(&obj->list);
				tracker->tcount--;
				free(obj);
			}
		}
	}

	for (i = 0; i < count; i++) {
		pCMObj = tracker->mtbl[i];
		if ( pCMObj ) {
			/* Make a new object and add to the track list. */
			obj = (ivcam_track_obj_t *)malloc(sizeof(ivcam_track_obj_t));
			obj->mobj = *pCMObj;
			obj->t_head = obj->t_tail = 0;
			_update_trajectory(pMTH->track_ref, obj, pCMObj);
			obj->fn_first = pMTH->framenum - pCMObj->tframes;
			obj->fn_last = pMTH->framenum;
			obj->hvalid = pCMObj->hvalid;
			if ( pCMObj->hvalid )
				memcpy(obj->hdata, pCMObj + 1, sizeof(obj->hdata));
			memset(obj->zstate, 0, sizeof(obj->zstate));
			obj->zevent_count = 0;
			obj->npts = 1;
			obj->match = 1;
			list_add_head(&obj->list, &tracker->tlist);
			tracker->tcount++;
			tracker->totcount++;
		}
	}

	return 0;
}	/* ivcam_tracker_process(... */

