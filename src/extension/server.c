#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <pthread.h>
#include "onvif_common.h"
#include "onvif_gen.h"

#define MAX_CLNT_CNT 1000
extern void *onvif_discovery(void *arg);
#ifdef JS_FEATURE_GENETEC_SDK
extern void *genetec_discovery(void *arg);
#endif
extern unsigned int nf_sysdb_get_uint(const char *property_name);

// nf_service/onvif.c
extern void init_onvif_table(void);

static ssize_t writen(int fd, const void *ptr, size_t n);
static ssize_t readn(int fd, void *ptr, size_t n);
static int general_recv(int clnt_sock, onvif_packet *buff);
static int general_send(int clnt_sock, onvif_packet *packet);
static void onvif_net_init(onvif_base *self);
static void onvif_select_init(onvif_base *self);
static int onvif_accept(onvif_base *self);
static void onvif_receive(onvif_base *self);
static void *onvif_instance(void *arg);


#if 0 //[[ Jeonghun_0130801_BEGIN --  del

static void *onvif_instance(void *arg)
{
	onvif_base self;

	memset(&self, 0x00, sizeof(onvif_base));

       while( dvrReady_info() == 0 ){
            _TTY_LOG_ONVIF("wait for dvr_ready");
            sleep(2);
       }

	onvif_net_init(&self);
	onvif_select_init(&self);

	onvif_receive(&self);

	pthread_exit(NULL);
}

static void onvif_packet_code(int fd, onvif_packet *packet)
{

	switch(packet->header.cmd)
	{
		case CMD_GetHostName:
			factory_GetHostName(packet);
			general_send(fd, packet);
			break;
		case CMD_SetHostName:
			factory_SetHostName(packet);
			general_send(fd, packet);            
			break;
		case CMD_GetDNS:
			factory_GetDNS(packet);
			general_send(fd, packet);
			break;
		case CMD_SetDNS:
			factory_SetDNS(packet);
			general_send(fd, packet);            
			break;
		case CMD_GetNTP:
			factory_GetNTP(packet);
			general_send(fd, packet);
			break;
		case CMD_SetNTP:
			factory_SetNTP(packet);
			general_send(fd, packet);
			break;
		case CMD_GetDateTime:
			factory_GetDateTime(packet);
			general_send(fd, packet);
			break;
		case CMD_SetDateTime:
			factory_SetDateTime(packet);
			general_send(fd, packet);
			break;
		case CMD_GetDeviceInformation:
			factory_GetDeviceInformation(packet);
			general_send(fd, packet);
			break;
		case CMD_GetProfilesDB:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetProfilesDB(packet);
			general_send(fd, packet);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			break;
		case CMD_GetProfileDB:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetProfileDB(packet);
			general_send(fd, packet);
			break;
		case CMD_GetVideoEncoder:
			factory_GetVideoEncoder(packet);
			general_send(fd, packet);
			break;
		case CMD_GetVideoEncoders:
			factory_GetVideoEncoders(packet);
			general_send(fd, packet);
			break;
		case CMD_SetVideoEncoder:
			factory_SetVideoEncoder(packet);
			general_send(fd, packet);
			break;
		case CMD_FactoryDefault:
			factory_FactoryDefault(packet);
			general_send(fd, packet);            
			break;
		case CMD_SystemReboot:
			factory_SystemReboot(packet);
			general_send(fd, packet);
			break;
		case CMD_GetStreamUri:
			factory_GetStreamUri(packet);
			general_send(fd, packet);
			break;
		case CMD_GetVideoEncoderOption:
			factory_GetVideoEncoderOption(packet);
			general_send(fd, packet);
			break;
		case CMD_GetImagingOption:
			factory_GetImagingOption(packet);
			general_send(fd, packet);
			break;
		case CMD_SetImagingOption:
			factory_SetImagingOption(packet);
			general_send(fd, packet);            
			break;
		case CMD_GetNetworkInfo:
			factory_GetNetworkInfo(packet);
			general_send(fd, packet);
			break;
		case CMD_GetNetworkPort:
			factory_GetNetworkPort(packet);
			general_send(fd, packet);
			break;
		case CMD_SetNetworkPort:
			factory_SetNetworkPort(packet);
//			general_send(fd, packet);            
			break;
		case CMD_SetVideoSource:
			factory_SetVideoSource(packet);
			general_send(fd, packet);
			break;
		case CMD_SetNetworkInfo:
			factory_SetNetworkInfo(packet);
//			general_send(fd, packet);            
			break;
		case CMD_SetScopes:
			factory_SetScope(packet);
			general_send(fd, packet);
			break;
		case CMD_GetScopes:
			factory_GetScope(packet);
			general_send(fd, packet);
			break;
		case CMD_AddScopes:
			factory_AddScope(packet);
			general_send(fd, packet);
			break;
		case CMD_RemoveScopes:
			factory_RemoveScope(packet);
			general_send(fd, packet);
			break;
		case CMD_GetDiscovery:
			factory_GetDiscovery(packet);
			general_send(fd, packet);
			break;
		case CMD_SetDiscovery:
			factory_SetDiscovery(packet);
			general_send(fd, packet);            
			break;
		case CMD_CreateProfile:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_CreateProfile(packet);
			general_send(fd, packet);
			break;
		case CMD_AddVideoSource:
			factory_AddVideoSource(packet);
			general_send(fd, packet);
			break;
		case CMD_AddVideoEncoder:
			factory_AddVideoEncoder(packet);
			general_send(fd, packet);
			break;
		case CMD_RemoveVideoEncoder:
			factory_RemoveVideoEncoder(packet);
			general_send(fd, packet);
			break;
		case CMD_RemoveVideoSource:
			factory_RemoveVideoSource(packet);
			general_send(fd, packet);
			break;
		case CMD_DeleteProfile:
			factory_DeleteProfile(packet);
			general_send(fd, packet);
			break;
		case CMD_GetCompatibleVideoSource:
			factory_GetCompatibleVideoSource(packet);
			general_send(fd, packet);
			break;
		case CMD_GetVideoSourceConfiguration:
			factory_GetVideoSourceConfiguration(packet);
			general_send(fd, packet);
			break;
		case CMD_GetVideoSourceConfigurations:
			factory_GetVideoSourceConfigurations(packet);
			general_send(fd, packet);
			break;
		case CMD_GetVideoSourceOption:
			factory_GetVideoSourceOption(packet);
			general_send(fd, packet);
			break;
		case CMD_CheckTokens:
			factory_CheckTokens(packet);
			general_send(fd, packet);
			break;
		case CMD_SetAudioEncoder:
			factory_SetAudioEncoder(packet);
			general_send(fd, packet);
			break;
		case CMD_SetAudioSource:
			factory_SetAudioSource(packet);
			general_send(fd, packet);
			break;
		case CMD_GetCompatibleAudioEncoder:
			factory_GetCompatibleAudioEncoder(packet);
			general_send(fd, packet);
			break;
		case CMD_GetCompatibleAudioSource:
			factory_GetCompatibleAudioSource(packet);
			general_send(fd, packet);
			break;
		case CMD_AddAudioSource:
			factory_AddAudioSource(packet);
			general_send(fd, packet);
			break;
		case CMD_AddAudioEncoder:
			factory_AddAudioEncoder(packet);
			general_send(fd, packet);
			break;
		case CMD_RemoveAudioEncoder:
			factory_RemoveAudioEncoder(packet);
			general_send(fd, packet);
			break;
		case CMD_RemoveAudioSource:
			factory_RemoveAudioSource(packet);
			general_send(fd, packet);
			break;
		case CMD_GetAudioSourceConfiguration:
			factory_GetAudioSourceConfiguration(packet);
			general_send(fd, packet);
			break;
		case CMD_GetAudioSourceConfigurations:
			factory_GetAudioSourceConfigurations(packet);
			general_send(fd, packet);
			break;
		case CMD_GetAudioSourceOption:
			factory_GetAudioSourceOption(packet);
			general_send(fd, packet);
			break;
		case CMD_GetAudioEncoderConfiguration:
			factory_GetAudioEncoderConfiguration(packet);
			general_send(fd, packet);
			break;
		case CMD_GetAudioEncoderConfigurations:
			factory_GetAudioEncoderConfigurations(packet);
			general_send(fd, packet);
			break;
		case CMD_GetVideoSources:
			factory_GetVideoSources(packet);
			general_send(fd, packet);
			break;
		case CMD_GetAudioSources:
			factory_GetAudioSources(packet);
			general_send(fd, packet);
			break;
		case CMD_GetCompatibleVideoEncoder:
			factory_GetCompatibleVideoEncoder(packet);
			general_send(fd, packet);
			break;
		case CMD_GetMetadataConfigurations:
			factory_GetMetadataConfigurations(packet);
			general_send(fd, packet);
			break;
		case CMD_GetCompatibleMetadataConfigurations:
			factory_GetCompatibleMetadataConfigurations(packet);
			general_send(fd, packet);
			break;
		case CMD_RemoveMetadataConfiguration:
			factory_RemoveMetadataConfiguration(packet);
			general_send(fd, packet);
			break;
		case CMD_AddMetadataConfiguration:
			factory_AddMetadataConfiguration(packet);
			general_send(fd, packet);
			break;
		case CMD_GetMetadataConfiguration:
			factory_GetMetadataConfiguration(packet);
			general_send(fd, packet);
			break;
		case CMD_RTP_HTTP:
			factory_RtpHttp(fd, packet);
			break;
		case CMD_GetUser:
			factory_GetUser(packet);
			general_send(fd, packet);
			break;
		case CMD_SetUser:
			factory_SetUser(packet);
			general_send(fd, packet);
			break;
		case CMD_CreateUsers:
			factory_CreateUser(packet);
			general_send(fd, packet);
			break;
		case CMD_DeleteUsers:
			factory_DeleteUser(packet);
			general_send(fd, packet);
			break;
		case CMD_GetImagingSettings:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetImagingSettings(packet);
			general_send(fd, packet);
			break;
		case CMD_SetImagingSettings:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_SetImagingSettings(packet);
			general_send(fd, packet);
			break;
		case CMD_GetRelays:
			factory_GetRelays(packet);
			general_send(fd, packet);
			break;
		case CMD_SetRelaySettings:
			factory_SetRelaySettings(packet);
			general_send(fd, packet);
			break;
		case CMD_SetRelayState:
			factory_SetRelayState(packet);
			general_send(fd, packet);
			break;
		case CMD_GetOptions:
			factory_GetOptions(packet);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			general_send(fd, packet);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			break;
		case CMD_Move:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_Move(packet);
			general_send(fd, packet);
			break;
		case CMD_GetStatus:
			factory_GetStatus(packet);
			general_send(fd, packet);
			break;
		case CMD_GetCapa:
			factory_GetCapa(packet);
			general_send(fd, packet);
			break;
		case CMD_GotoHomePosition:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GotoHomePosition(packet);
			general_send(fd, packet);
			break;
		case CMD_AbsoluteMove:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_AbsoluteMove(packet);
			general_send(fd, packet);
			break;
		case CMD_ContinuousMove:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_ContinuousMove(packet);
			general_send(fd, packet);
			break;
		case CMD_CreatePresetTour:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_CreatePresetTour(packet);
			general_send(fd, packet);
			break;
		case CMD_GetConfigurationOptions:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetConfigurationOptions(packet);
			general_send(fd, packet);
			break;
		case CMD_SetPTZConfiguration:
			_TTY_LOG_ONVIF_DEBUG("CMD_SetPTZConfiguration: %d ",CMD_SetPTZConfiguration);
			factory_SetPTZConfiguration(packet);
			general_send(fd, packet);
			break;
		case CMD_GetNode:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetNode(packet);
			general_send(fd, packet);
			break;
		case CMD_GetNodes:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetNodes(packet);
			general_send(fd, packet);
			break;
		case CMD_GetPresets:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetPresets(packet);
			general_send(fd, packet);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			break;
		case CMD_GetPresetTour:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetPresetTour(packet);
			general_send(fd, packet);
			break;
		case CMD_GetPresetTourOptions:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetPresetTourOptions(packet);
			general_send(fd, packet);
			break;
		case CMD_GetPresetTours:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetPresetTours(packet);
			general_send(fd, packet);
			break;
		case CMD_GetServiceCapabilities:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetServiceCapabilities(packet);
			general_send(fd, packet);
			break;
		case CMD_PTZGetStatus:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_PTZGetStatus(packet);
			general_send(fd, packet);
			break;
		case CMD_GotoPreset:
			_TTY_LOG_ONVIF_DEBUG("CMD_GotoPreset Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GotoPreset(packet);
			general_send(fd, packet);
			break;
		case CMD_ModifyPresetTour:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_ModifyPresetTour(packet);
			general_send(fd, packet);
			break;
		case CMD_OperatePresetTour:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_OperatePresetTour(packet);
			general_send(fd, packet);
			break;
		case CMD_RelativeMove:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_RelativeMove(packet);
			general_send(fd, packet);
			break;
		case CMD_RemovePreset:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_RemovePreset(packet);
			general_send(fd, packet);
			break;
		case CMD_RemovePresetTour:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_RemovePresetTour(packet);
			general_send(fd, packet);
			break;
		case CMD_SendAuxiliaryCommand:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_SendAuxiliaryCommand(packet);
			general_send(fd, packet);
			break;
		case CMD_SetConfiguration:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_SetConfiguration(packet);
			general_send(fd, packet);
			break;
		case CMD_SetHomePosition:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_SetHomePosition(packet);
			general_send(fd, packet);
			break;
		case CMD_SetPreset:
			factory_SetPreset(packet);
			general_send(fd, packet);
			break;
		case CMD_Stop:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_Stop(packet);
			general_send(fd, packet);
			break;
		case CMD_FocusStop:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_FocusStop(packet);
			general_send(fd, packet);
					break;
		case CMD_SetMetadataConfiguration:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_SetMetadataConfiguration(packet);
			general_send(fd, packet);
					break;
		case CMD_GetPTZNodes:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetPTZNodes(packet);
			general_send(fd, packet);
					break;
		case CMD_GetPTZNode:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetPTZNode(packet);
			general_send(fd, packet);
					break;
		case CMD_GetPTZConfigurations:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetPTZConfigurations(packet);
			general_send(fd, packet);
					break;
		case CMD_GetPTZConfiguration:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_GetPTZConfiguration(packet);
			general_send(fd, packet);
					break;
		case CMD_AddPTZConfiguration:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			factory_AddPTZConfiguration(packet);
			general_send(fd, packet);
			break;
		default:
			break;
	}
}



static void onvif_net_init(onvif_base *self)
{
	struct sockaddr_un server_addr;
	int option;
	socklen_t optlen = sizeof(int);


	if(0 == access(ONVIF_FILE_SERVER, F_OK)) 
		unlink(ONVIF_FILE_SERVER);

	self->serv_sock = socket(PF_FILE, SOCK_STREAM, 0);
	if(-1 == self->serv_sock)
	{
		fprintf(stderr, "%s server socket failed\n", __FUNCTION__);
		return;
	}

	setsockopt(self->serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, optlen);

	memset(&server_addr, 0x00, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, ONVIF_FILE_SERVER);
	if(-1 == bind(self->serv_sock, (struct sockaddr *)&server_addr,
				sizeof(server_addr)))
	{
		fprintf(stderr, "%s bind failed\n", __FUNCTION__);
		close(self->serv_sock);
		return;
	}
	if(-1 == listen(self->serv_sock, MAX_CLNT_CNT))
	{
		fprintf(stderr, "%s listen failed\n", __FUNCTION__);
		close(self->serv_sock);
		return;
	}
}


static void onvif_select_init(onvif_base *self)
{
	onvif_select *selval = &self->select_val;

	FD_ZERO(&selval->rset);

	FD_SET(self->serv_sock, &selval->rset);

	selval->tv.tv_sec = 2;
	selval->tv.tv_usec = 0;

	selval->clntCnt = self->serv_sock;
}



static void onvif_receive(onvif_base *self)
{
	onvif_select *selval = &self->select_val;
	struct timeval timeout;
	int *clntCnt = &selval->clntCnt;
	int serv_sock = self->serv_sock;
	int select_ret;
	fd_set rsetTmp;
	int fd;
	int ret;

	for(;;)
	{
		while(1)
		{
			rsetTmp = selval->rset;
			timeout.tv_sec = selval->tv.tv_sec;
			timeout.tv_usec = selval->tv.tv_usec;

			select_ret = select(*clntCnt+1, &rsetTmp, 0, 0, &timeout);
			

			if(select_ret == -1)
			{
				if(errno == EINTR) continue;

				fprintf(stderr, "%s select error\n", __FUNCTION__);
				return;
			}
			else if(select_ret == 0) {
				continue;
			}

			break;
		}

		for(fd = 0; fd < *clntCnt+1;fd++)
		{
			if(FD_ISSET(fd, &rsetTmp))
			{
				if(fd == serv_sock)
				{
					if(onvif_accept(self) < 0) {
						fprintf(stderr, "%s accept error\n", __FUNCTION__);                     
						return; // todo recovery 
					}
				}
				else
				{
					onvif_packet packet;
					memset(&packet, 0x00, sizeof(packet));

					ret = general_recv(fd, &packet);  // 무한대기....
						_TTY_LOG_ONVIF("%s (%d error)",__FUNCTION__, __LINE__);
					if(ret < 0) 
					{
						if(errno == EINTR) {
							continue;
						}
						else 
						{
							FD_CLR(fd, &self->select_val.rset);
							close(fd);
							continue;
						}
					}
					else if(ret == 0)
					{
						FD_CLR(fd, &self->select_val.rset);
						close(fd);
						continue;
					}
					else
					{
                                        /* wait for dvr_ready */
                                        while( dvrReady_info() == 0 ){
                                            _TTY_LOG_ONVIF("wait for dvr_ready");
                                            sleep(2);
                                        }                                        
						onvif_packet_code(fd, &packet);
					}
				}
			}
		}
	}
}




static int onvif_accept(onvif_base *self)
{
	int serv_sock = self->serv_sock;
	struct sockaddr_un client_addr;
	socklen_t clnt_addr_size;
	int clnt_socket;
	onvif_select *selval = &self->select_val;


	while(1)
	{
		clnt_addr_size = sizeof(client_addr);
		clnt_socket = accept(serv_sock, (struct sockaddr *)&client_addr, &clnt_addr_size);
		if(clnt_socket < 0)
		{
			if(errno == EINTR)
				continue;

			return -1;
		}
		else if(clnt_socket >= MAX_CLNT_CNT)
		{
			close(clnt_socket);
			fprintf(stderr, "%s client Max Count Over Error!", __FUNCTION__);
			return -1;
		}
		break;
	}

	FD_SET(clnt_socket, &selval->rset);

	if(clnt_socket > selval->clntCnt)
	{
		selval->clntCnt = clnt_socket;
	}

	return 0;
}


static int general_send(int clnt_sock, onvif_packet *packet)
{
#if 0
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	if(writen(clnt_sock, packet, sizeof(onvif_packet)) < 0) 
	{
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		return -1;
	}
#else
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
       //baek.debug
	nf_debug_hexdump_tty( packet->data, packet->header.dlen);
	if(writen(clnt_sock, packet, sizeof(onvif_packet_header)) < 0) {
		return -1;
	}
	if(writen(clnt_sock, packet->data, packet->header.dlen) < 0) {
		return -1;
	}
#endif
	
	return 0;
}


static int general_recv(int clnt_sock, onvif_packet *packet)
{
	int ret;
#if 0
	ret = readn(clnt_sock, packet, sizeof(onvif_packet));
	if(ret < 0) 
		return -1;
       //baek.debug
	//nf_debug_hexdump_tty( packet->data, 108);
#else
	ret = readn(clnt_sock, packet, sizeof(onvif_packet_header));
	if(ret < 0) 
		return -1;
	ret = readn(clnt_sock, packet->data, packet->header.dlen);
	if(ret < 0) 
		return -1;
       //baek.debug
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);       
	nf_debug_hexdump_tty( packet->data, packet->header.dlen);
#endif
       
	return ret;
}




static ssize_t readn(int fd, void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nread;

	nleft = n;

	while(nleft > 0) {
		if((nread = read(fd, ptr, nleft)) < 0) {
			if(errno == EINTR)
				continue;
			if(nleft == n)
				return -1;
			else
				break;
		}
		else if(nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}

	return (n-nleft);
}


static ssize_t writen(int fd, const void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;

	nleft = n;
	while(nleft > 0) {
		if((nwritten = write(fd, ptr, nleft)) < 0) {
			if(errno == EINTR)
				continue;

			if(nleft == n)
				return -1;
			else
				break;
		}
		else if(nwritten == 0)
			break;

		nleft -= nwritten;
		ptr += nwritten;
	}

	return (n-nleft);
}

#endif //]] Jeonghun_0130801_END --  del
