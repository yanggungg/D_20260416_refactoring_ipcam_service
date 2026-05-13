/*******************************************************************************
*  (c) COPYRIGHT 2010 ITXSecurity                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
********************************************************************************

DESCRIPTION:

................................................................................

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
31/03/2014 MinJung Kim    Created. (ONVIF Device and DeviceIO Service ver 2.3)
*/
#ifndef __NF_ONVIF_DEVICEIO_C__
#define __NF_ONVIF_DEVICEIO_C__

#define DEBUG_ONVIF_LOG

#include <stdsoap2.h>
#include <onvifH.h>

#include "wsaapi.h"
#include <nf_ipcam_defs.h>


// TODO: Move Device Servce to nf_onvif_device.c

// MACRO FUNCTION
// ============================================================================
//#define SOAP_ERR(soap, str) { char buff[1024]; soap_sprintf_fault((soap), buff, 1024); fprintf(stderr, "[%s:%d] ERROR: %s\n",__func__,__LINE__,buff); }
#define SOAP_ERROR_PRINT(soap) \
{ \
	char buff[1024]; \
	soap_sprint_fault((soap), buff, 1024); \
	fprintf(stderr, "[%s:%d] ERROR \n========================================\n\
%s\n========================================\n\n" \
			, __func__, __LINE__, buff); \
}


// STRUCTURE: RELAY OUTPUT 
// ============================================================================
#define TOKEN_LEN 256
#define MAX_RELAYOUT 16


// STRUCTURE
// ============================================================================
typedef struct _NFIPCamRelayOutput NFIPCamRelayOutput;
struct _NFIPCamRelayOutput 
{
  char token[TOKEN_LEN];
  long long DelayTime;
  unsigned int Mode;
  unsigned IdleState;
};

typedef struct _NFIPCamRelayOutputs NFIPCamRelayOutputs;
struct _NFIPCamRelayOutputs
{
  int size;
  struct _NFIPCamRelayOutput relayoutput[MAX_RELAYOUT];
};

typedef struct _NFIPCamDeviceIOCapabilities NFIPCamDeviceIOCapabilities;
struct _NFIPCamDeviceIOCapabilities 
{
	int VideoSources;
	int VideoOutputs;
	int AudioSources;
	int AudioOutputs;
	int RelayOutputs;
	int SerialPorts;
	int DigitalInputs;
};

NFIPCamDeviceIOCapabilities cap[AVAILABLE_MAX_CH];

// EXTERN
// ============================================================================
extern int SOAP_SSL_CLIENT_CONTEXT(struct soap* soap, unsigned short a, const char* b, const char* c, const char* d, const char* e, const char* f);


// CONSTANT VALUE
// ============================================================================
enum __IPCAM_RELAY_MODE_E_
{
 IPCAM_RELAYOUT_MODE_MONOSTABLE = 0,
 IPCAM_RELAYOUT_MODE_BISTABLE = 1
};

enum __IPCAM_RELAY_IDLE_STATE_E_
{
  IPCAM_RELAY_IDLE_STATE_CLOSED = 0,
  IPCAM_RELAY_IDLE_STATE_OPEN =1
};

enum __IPCAM_LOGICAL_STATE_E_
{
  IPCAM_LOGICAL_STATE_ACTIVE = 0 ,
  IPCAM_LOGICAL_STATE_INACTIVE = 1
};


// STATIC FUNCTION
// ============================================================================

//ONVIF_MSG nf_onvif_deviceio_get_capabilities(int ch);

ONVIF_MSG nf_onvif_device_get_relayoutputs(int ch, NFIPCamRelayOutputs *relayout);
ONVIF_MSG nf_onvif_device_set_relayoutput(int ch, NFIPCamRelayOutput *relayoutput);
ONVIF_MSG nf_onvif_device_set_relayoutput_state(int ch, char *token, int logical_state);

// GSOAP CALL FUNC
//ONVIF_MSG _nf_onvif_deviceio_get_capabilitie(int auth, const char *endpoint, const char* user, const char* pass, const int ch);
ONVIF_MSG _nf_onvif_device_get_relayoutput(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const int ch, NFIPCamRelayOutputs *relayoutputs);
ONVIF_MSG _nf_onvif_device_set_relayoutput_settings(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const int ch, NFIPCamRelayOutput *relayoutput);
ONVIF_MSG _nf_onvif_device_set_relayoutput_state(ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const int ch, const char *token, const int LogicalState);
//ONVIF_MSG _nf_onvif_deviceio_get_digital_input(int auth, const char *endpoint, const char* user, const char* pass, char *token); 


// EXTERN FUNCTION
// ============================================================================

// Support RelayOutput : Return(Number Relay Output, 0 : Not Support)

ONVIF_API nf_onvif_set_relayout(cam_info *info, int ch)
{
	int rtn;
	mtable *runtime = get_runtime();
	
	// Print Argument
#ifdef DEBUG_ONVIF_LOG 
	IPCAM_DBG(MINOR,"\truntime[ch].alarm [ .alarm_out_type.value(%d) | alarm_out(%d) ] \n",
			runtime[ch].alarm.alarm_out_type.value, 
			runtime[ch].alarm.alarm_out);

	IPCAM_DBG(MINOR,"\tinfo->alarm [ .in_onoff(%d) | .in_type(%d) | .out_onoff(%d) | .out_type(%d) ]\n",
			info->alarm.in_onoff,
			info->alarm.in_type,
			info->alarm.out_onoff,
			info->alarm.out_type
			);
#endif


	NFIPCamRelayOutputs relayoutputs;

	rtn = nf_onvif_device_get_relayoutputs(ch, &relayoutputs);
	if(rtn != 0)
	{
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto exit_err;
	}

	if(relayoutputs.size < 1)
	{
		// Not Support Relay Output
		printf("[ERROR]>>>>>>>>>> Relay Output Size 0");
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto exit_err;
	}

#ifdef DEBUG_ONVIF_LOG
	IPCAM_DBG(MINOR,"\tsize(%d) | token(%s) | IdleState(%s(%d)) | Mode(%s(%d)) | DelayTime(%lld) \n", 
			relayoutputs.size,
			relayoutputs.relayoutput[0].token,
			relayoutputs.relayoutput[0].IdleState ? "OPEN" : "CLOSED",
			relayoutputs.relayoutput[0].IdleState,
			relayoutputs.relayoutput[0].Mode ? "BISTABLE" : "MONOSTABLE" ,
			relayoutputs.relayoutput[0].Mode,
			relayoutputs.relayoutput[0].DelayTime
			);
#endif


	// Set Relay Output N/C or N/O
	NFIPCamRelayOutput relayoutput;
	memcpy(&relayoutput, &relayoutputs.relayoutput[0], sizeof(NFIPCamRelayOutput));
	relayoutput.Mode = IPCAM_RELAYOUT_MODE_BISTABLE;  // IPX Default Mode Bistable
	relayoutput.IdleState = (info->alarm.out_type == 1) ? IPCAM_RELAY_IDLE_STATE_CLOSED : IPCAM_RELAY_IDLE_STATE_OPEN;
	relayoutput.DelayTime = 5; // Not Used, Bistable Mode ?// TODO: not found from event argument

	if(relayoutputs.relayoutput[0].IdleState != (info->alarm.out_type ? 0 : 1) )
	{
		IPCAM_DBG(MINOR,"\tIdleState = %d, out_type = %d \n", relayoutputs.relayoutput[0].IdleState, (info->alarm.out_type ? 0 : 1));
		// Set Default Relay Output(Mode, 
		rtn = nf_onvif_device_set_relayoutput(ch, &relayoutput);
		if(rtn != 0)
		{
			IPCAM_DBG(ERROR,"SET RELAY OUTPUT FAILE (rtn:%d) \n",rtn);
			rtn = IPCAM_SETUP_RTN_FAILED;
			goto exit_err;
		}
	}


	// Set After Value Print
	rtn = nf_onvif_device_get_relayoutputs(ch, &relayoutputs);
	if(rtn != 0)
	{
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto exit_err;
	}

	if(relayoutputs.size < 1)
	{
		// Not Support Relay Output
		printf("[ERROR]>>>>>>>>>> Relay Output Size 0");
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto exit_err;
	}

#ifdef DEBUG_ONVIF_LOG
	IPCAM_DBG(MINOR,"\tsize(%d) | token(%s) | IdleState(%s(%d)) | Mode(%s(%d)) | DelayTime(%lld) \n", 
			relayoutputs.size,
			relayoutputs.relayoutput[0].token,
			relayoutputs.relayoutput[0].IdleState ? "OPEN" : "CLOSED",
			relayoutputs.relayoutput[0].IdleState,
			relayoutputs.relayoutput[0].Mode ? "BISTABLE" : "MONOSTABLE" ,
			relayoutputs.relayoutput[0].Mode,
			relayoutputs.relayoutput[0].DelayTime
			);
#endif


	// Set Relay Output Status (ON/OFF)
	rtn = nf_onvif_device_set_relayoutput_state( ch, relayoutput.token, info->alarm.out_onoff );
	if(rtn != 0)
	{
		IPCAM_DBG(ERROR,"SET RELAY OUTPUT STATE FAILE (rtn:%d)\n",rtn);
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto exit_err;
	}
	rtn = IPCAM_SETUP_RTN_DONE;

exit_err:
	return rtn;
}

#if 0
ONVIF_API nf_onvif_deviceio_is_support_relayoutputs(int ch)
{
  int rtn;
  rtn = nf_onvif_deviceio_get_capabilities(ch);

  if(cap[ch].RelayOutputs > 0)
  {
    rtn = cap[ch].RelayOutputs; // Support Number Relay Output
  }
  else
  {
    rtn = 0; // Not Support Relay Output 
  }

  return rtn;

}
#endif

// STATIC FUNCTION 
// ============================================================================
#if 0
ONVIF_MSG nf_onvif_deviceio_get_capabilities(int ch)
{
	char endpoint[256];
	int rtn;
	mtable *runtime;

  // START FUNCTION

  // GET RUNTIME AND CHECK
	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT), 0);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE] != NULL, (-1));

  // COPY XAddr
	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE]);

  // CALL CAPABILITY
	rtn = _nf_onvif_deviceio_get_capabilitie(
      runtime[ch].onvif.auth_method, endpoint, runtime[ch].username, runtime[ch].password,  ch);
  if(rtn != 0)
  {
    goto exit_end;
  }

exit_end:

	return rtn;
}

#endif
ONVIF_MSG nf_onvif_device_set_relayoutput_state(int ch, char *token, int logical_state)
{
	char endpoint[256];
	int rtn;
	int state;
	mtable *runtime;

	ipcam_onvif_auth_info_t auth_info;

	// START FUNCTION

  	// GET RUNTIME AND CHECK
	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT), 0);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE] != NULL, (-1));

  // COPY XAddr
	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE]);
 	
	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];
	
	if(logical_state == 0)
    state = IPCAM_LOGICAL_STATE_INACTIVE;
    else
    state = IPCAM_LOGICAL_STATE_ACTIVE;

    rtn =  _nf_onvif_device_set_relayoutput_state(
      &auth_info, endpoint, ch, token, state );

 return rtn;

}

ONVIF_MSG nf_onvif_device_get_relayoutputs(int ch, NFIPCamRelayOutputs *relayout)
{	
  char endpoint[256];
	int rtn;
	mtable *runtime;

	ipcam_onvif_auth_info_t auth_info;

  // START FUNCTION

  // GET RUNTIME AND CHECK
	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT), 0);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT] != NULL, (-1));

  // COPY XAddr
	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE]);
	
	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

  // CALL CAPABILITY
	rtn = _nf_onvif_device_get_relayoutput(&auth_info, endpoint, ch, relayout);

  if(rtn != 0)
  {
    goto exit_end;
  }

exit_end:
  return rtn;

}

ONVIF_MSG nf_onvif_device_set_relayoutput(int ch, NFIPCamRelayOutput *relayoutput)
{
	char endpoint[256];
	int rtn;
	mtable *runtime;

	ipcam_onvif_auth_info_t auth_info;

	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT), 0);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT] != NULL, (-1));

  // COPY XAddr
	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE]);

  rtn = _nf_onvif_device_set_relayoutput_settings(&auth_info, endpoint, ch, relayoutput );

  return rtn;
}

#if 0
ONVIF_API nf_onvif_deviceio_get_digital_input(int ch, char *token)
{
	char endpoint[256];
	int rtn;
  char local_token[5][TOKEN_LEN];
	mtable *runtime;

	runtime = get_runtime();
	g_return_val_if_fail(runtime != NULL, (-1));
	g_return_val_if_fail(runtime[ch].username != NULL, (-1));
	g_return_val_if_fail(runtime[ch].password != NULL, (-1));
	g_return_val_if_fail(runtime[ch].onvif.onvif_service & __OFM(NF_ONVIF_SERVICE_EVENT), 0);
	g_return_val_if_fail(runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT] != NULL, (-1));

  // COPY XAddr
	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE]);
  rtn = _nf_onvif_deviceio_get_digital_input(
      runtime[ch].onvif.auth_method, endpoint, runtime[ch].username, runtime[ch].password, local_token[0]);

  if(rtn != 0)
  {
    // Get Fail Digital Input Token
    goto exit_err;
  }

  if(local_token == NULL)
  {
    rtn = 1; // TOKEN NULL
  }

exit_err:
  return rtn;
}
#endif

// STATIC FUNCTION : GSOAP CALL LAYER
// ============================================================================

// Get Device IO Capabilitie
#if 0
ONVIF_MSG _nf_onvif_deviceio_get_capabilitie(
    int auth, const char *endpoint, const char* user, const char* pass, const int ch)
{
	int rtn;             // Return Number
	struct soap *soap;
	struct _devio__GetServiceCapabilities req;
	struct _devio__GetServiceCapabilitiesResponse res;

	// soap init
	soap =  soap_new();
	nf_onvif_soap_init_set(soap);

	// add auth
	if(soap == NULL || endpoint == NULL || user == NULL || pass == NULL)
	{
		// input argument error
		goto exit_err;
	}
	rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);	
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto exit_err;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			printf("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto exit_err;
		}
	}



	memset(&req, 0x00, sizeof(struct _devio__GetServiceCapabilities));
	rtn = soap_call___devio__GetServiceCapabilities(soap, endpoint, NULL, &req, &res);

	cap[ch].VideoSources = res.Capabilities->VideoSources;
	cap[ch].VideoOutputs = res.Capabilities->VideoOutputs;
	cap[ch].AudioSources = res.Capabilities->AudioSources;
	cap[ch].AudioOutputs = res.Capabilities->AudioOutputs;
	cap[ch].RelayOutputs = res.Capabilities->RelayOutputs;
	cap[ch].SerialPorts =  res.Capabilities->SerialPorts;
	cap[ch].DigitalInputs = res.Capabilities->DigitalInputs;


	printf("cap[ch].VideoSources = %d\n",cap[ch].VideoSources);
	printf("cap[ch].AudioSources = %d\n",cap[ch].VideoSources);
	printf("cap[ch].AudioOutputs = %d\n",cap[ch].VideoSources);
	printf("cap[ch].RelayOutputs = %d\n",cap[ch].VideoSources);
	printf("cap[ch].SerialPorts = %d\n",cap[ch].VideoSources);
	printf("cap[ch].DigitalInputs = %d\n",cap[ch].VideoSources);

exit_err:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	soap_done(soap);


	return rtn;
}

#endif
// Get Relay Output
ONVIF_MSG _nf_onvif_device_get_relayoutput(
    ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const int ch, NFIPCamRelayOutputs *relayoutputs)
{
	int rtn;             // Return Number
    int count;
	struct soap *soap;
	struct _device__GetRelayOutputs req;
	struct _device__GetRelayOutputsResponse res;
	// soap init
	soap =  soap_new();
	nf_onvif_soap_init_set(soap);


	// add auth
	if(soap == NULL || auth_info->endpoint == NULL || auth_info->username == NULL || auth_info->password == NULL)
	{
		// input argument error
		goto exit_err;
	}

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto exit_err;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			printf("[%s:%d] ERROR - %s\n", __FUNCTION__, __LINE__, buf);
			goto exit_err;
		}
	}

	memset(&req, 0x00, sizeof(struct _device__GetRelayOutputs));

	rtn = soap_call___device__GetRelayOutputs(soap, endpoint, NULL, &req, &res);
  if(rtn != 0)
  {
    SOAP_ERROR_PRINT(soap);
    goto exit_err;
  }

  struct tt__RelayOutput *ro;
  relayoutputs->size = res.__sizeRelayOutputs;
  ro = res.RelayOutputs;

  for(count = 0; count < relayoutputs->size; count++)
  {
    strcpy(relayoutputs->relayoutput[count].token, ro->token);
    relayoutputs->relayoutput[count].Mode = ro->Properties->Mode;
    relayoutputs->relayoutput[count].IdleState = ro->Properties->IdleState;
    relayoutputs->relayoutput[count].DelayTime = ro->Properties->DelayTime;
  }

	exit_err:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

// Set Relay Output Settings
ONVIF_MSG _nf_onvif_device_set_relayoutput_settings(
    ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const int ch, NFIPCamRelayOutput *relayoutput)
{
	int rtn;             // Return Number

  char token[TOKEN_LEN];

	struct soap *soap;
	struct _device__SetRelayOutputSettings req;
	struct _device__SetRelayOutputSettingsResponse res;

	struct tt__RelayOutputSettings properties;
  struct tt__RelayOutput output;


	// soap init
	soap =  soap_new();
	nf_onvif_soap_init_set(soap);


	// add auth
	if(soap == NULL || auth_info->endpoint == NULL || auth_info->username == NULL || auth_info->password == NULL)
	{
		// input argument error
		goto exit_err;
	}
	
	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto exit_err;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
      SOAP_ERROR_PRINT(soap);
			goto exit_err;
		}
	}

	memset(&req, 0x00, sizeof(struct _device__SetRelayOutputSettings));
	memset(&properties, 0x00, sizeof(struct tt__RelayOutputSettings));

  properties.Mode = relayoutput->Mode ? 1 : 0;  // 1 or 0
  properties.IdleState = relayoutput->IdleState ? 1 : 0; // 1 or 0
  properties.DelayTime = relayoutput->DelayTime;

  strcpy(token, relayoutput->token);

/* DeviceIO
  output.token = token;
  output.Properties = &properties;

  req.RelayOutput = &output;
  */

  req.RelayOutputToken = token;
  req.Properties = &properties;

	rtn = soap_call___device__SetRelayOutputSettings(soap, endpoint, NULL, &req, &res);
  if(rtn != 0)
  {
		printf("[%s] Failed request(%d)\n", __FUNCTION__, rtn);
		SOAP_ERROR_PRINT(soap);
		goto exit_err;
  }

exit_err:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;
}

// Set Relay Output State
ONVIF_MSG _nf_onvif_device_set_relayoutput_state(
    ipcam_onvif_auth_info_t* auth_info, const char *endpoint, const int ch, const char *token, const int LogicalState)
{
  int rtn;
  char local_token[TOKEN_LEN];
  struct soap *soap;
  struct _device__SetRelayOutputState req;
  struct _device__SetRelayOutputStateResponse res;

  // SOAP INIT
	soap =  soap_new();
	nf_onvif_soap_init_set(soap);

  // AUTH
	if(soap == NULL || auth_info->endpoint == NULL || auth_info->username == NULL || auth_info->password == NULL)
	{
		// input argument error
		goto exit_err;
	}
	
	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto exit_err;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
      SOAP_ERROR_PRINT(soap);
			goto exit_err;
		}
	}

  // SET
  memset(&req, 0x00, sizeof(struct _device__SetRelayOutputState));

  strcpy(local_token,token);
  req.RelayOutputToken = local_token;
  req.LogicalState = LogicalState;

  soap_call___device__SetRelayOutputState(soap, endpoint, NULL, &req, &res);
 if(rtn != 0)
  {
		printf("[%s] Failed request(%d)\n", __FUNCTION__, rtn);
		SOAP_ERROR_PRINT(soap);
		goto exit_err;
  }

exit_err:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

return rtn;
}

#if 0
// Get Digital Input
ONVIF_MSG _nf_onvif_deviceio_get_digital_input( int auth, const char *endpoint, const char* user, const char* pass,  char *token)
{
  int rtn;
  int count;
  char local_token[TOKEN_LEN];
  struct soap *soap;

  struct _devio__GetDigitalInputs req; struct _devio__GetDigitalInputsResponse res;

	soap =  soap_new();
	nf_onvif_soap_init_set(soap);

	if(soap == NULL || endpoint == NULL || user == NULL || pass == NULL)
	{
		// input argument error
		goto exit_err;
	}
	rtn = _nf_onvif_add_auth(soap, auth, user, pass, endpoint);	
	if(rtn != 0)
	{
		SOAP_ERROR_PRINT(soap);
		goto exit_err;
	}

	if (strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
      SOAP_ERROR_PRINT(soap);
			goto exit_err;
		}
	}

  memset(&req, 0x00, sizeof(struct _devio__GetDigitalInputs));

  rtn = soap_call___devio__GetDigitalInputs(soap, endpoint, NULL, &req, &res); 
 if(rtn != 0)
  {
		printf("[%s] Failed request(%d)\n", __FUNCTION__, rtn);
		SOAP_ERROR_PRINT(soap);
		goto exit_err;
  }

 if(res.__sizeDigitalInputs > 0)
 {
   for(count = 0; count < res.__sizeDigitalInputs; count++)
   {
     strcpy((token+count),res.DigitalInputs->token+count);
   }
 }
 else
 {
   token ==  NULL;
   /* No Digital Inputs */
 }

exit_err:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

  return rtn;
}
#endif
#endif //__NF_ONVIF_DEVICEIO_C__
