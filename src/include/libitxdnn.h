#ifndef	LIBITXDNN_H
#define	LIBITXDNN_H


typedef struct {
	int classid;		/**< Class id. */
	float confidence;	/**< Confidence, [0,1]. */
	float bb[4];		/**< Bounding box, [xmin,ymin,xmax,ymax]. */
} itxdnn_detector_result_t;

typedef struct {
	int classes;		/**< Number of classes, except for bg class. */
	float threshold;	/**< Detection threshold. */
	char **names;		/**< Class names. */
	int in_w;			/**< Input width. */
	int in_h;			/**< Input height. */
	int in_c;			/**< Input channels. */
	void *net;			/**< Private pointer to a network associated to the detector. */
	float *ibuf;		/**< Private input buffer. */
	int maxdet;			/**< Max. number of detection. */
	int ndet;			/**< Number of detected object in the last detection. */
	itxdnn_detector_result_t *res;	/**< Detection results. */
} itxdnn_detector_t;


/**
 * @brief  Initializes itxdnn.
 * @param[in] mode  Running mode, -1=cpu, 0,1,2,...=gpu
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed. (Initialization failed, or no gpu)
 */
int itxdnn_init(int mode);

/**
 * @brief  Creates an itxdnn detector.
 *
 * @param[in] datafile  Data file name(.cfg). default values (voc classes) will be used if NULL.
 * @param[in] cfgfile  Network config file name. (.cfg)
 * @param[in] weightfile  Weight file name. (.weights)
 * @param[in] threshold  Detection threshold, [0, 1]. Default value will be used if -1.
 * @param[in] maxdet  Maximum number of detections per image.
 *
 * @return
 *  - Detector handle if successful.
 *  - NULL if failed.
 */
itxdnn_detector_t *itxdnn_detector_create(char *datafile, char *cfgfile,
		char *weightfile, float threshold, int maxdet);

/**
 * @brief  Releases an itxdnn detector.
 *
 * @param[in] det  Pointer to the created detector handle.
 *
 * @return
 *  - 0 if successful.
 *  - -1 if failed.
 */
int itxdnn_detector_release(itxdnn_detector_t *det);

/**
 * @brief Perform detection for given input image.
 *
 * @param[in] det  Pointer to the detector handle.
 * @param[in] input  Pointer to the input image. (rgb planar image format)
 * @param[in] iw  Image width, set to -1 if input is already resized.
 * @param[in] ih  Image height, set to -1 if input is already resized.
 *
 * @remark  Input image formats:
 *   yolo: letterbox image.
 *   ssd: just resized image.
 *   input image will be automatically resized if [iw, ih] != [det->in_w, det->in_h].
 *
 * @TODO ssd use bgr, opt for ssd later...
 *
 * @remark  Detection results are stored in det->res & det->ndet.
 *
 * @return
 *  - Non negative number of detections.
 *  - -1 if failed.
 */
int itxdnn_detector_predict(itxdnn_detector_t *det, unsigned char *input, int iw, int ih);








// TODO APIs TBD.
/*
net *itxdnn_network_create(files...)
int itxdnn_network_release(net);
int itxdnn_network_predict(net, input); -> output = lastlayer->output
int itxdnn_detector_valid(files...)
int itxdnn_detector_predict(net) -> output = detection result
int itxdnn_detector_test(files...)
*/

#endif	/* LIBITXDNN_H */

