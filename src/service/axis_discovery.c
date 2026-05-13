#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <nf_ipcam_defs.h>


#if 0
#define AXIS_DBG printf
#else
#define AXIS_DBG while(0) printf
#endif

static const char str_http_req_msg_raw[] =
		"GET %s HTTP/1.1\r\n"
		"Authorization: Basic %s\r\n"
		"\r\n"
		;

size_t b64_encode_string_to_buffer(char *p_dst, size_t i_dst, const char *p_src);
char* makeHTTPReqMsg(const char *pSendMsg);
void parsingHTTPRes(char* pHTTPResponse, int pType, axis_Discovery* pStrAxisDiscovery);
int sendHTTPReqMsg(char *pIpAddr, int pType, axis_Discovery* pStrAxisDiscovery);


axis_Discovery IsAxisCamera(char *pIpAddr)
{
	int rtnVal;
	axis_Discovery strAxisDiscovery;
	memset(&strAxisDiscovery, 0x00, sizeof(strAxisDiscovery));

	// Set root's password
	// ok or unauthorization
	rtnVal = sendHTTPReqMsg(pIpAddr, 1, &strAxisDiscovery);
	if (!rtnVal)
		return strAxisDiscovery;

	// Get Product Code
	rtnVal = sendHTTPReqMsg(pIpAddr, 2, &strAxisDiscovery);
	if (!rtnVal)
		return strAxisDiscovery;

	// Enable webservice
	rtnVal = sendHTTPReqMsg(pIpAddr, 3, &strAxisDiscovery);
	if (!rtnVal)
		return strAxisDiscovery;

	return strAxisDiscovery;
}

int sendHTTPReqMsg(char *pIpAddr, int pType, axis_Discovery* pStrAxisDiscovery)
{
	int rtnVal = 0, htmlstart = 0, recvLen = 0, totalRecvLen = 0;
	char* sendSocketBuf;
	char* recvSocketBuf = (char*)malloc(sizeof(char)*1000);
	char* totalRecvSocket = (char*)malloc(sizeof(char)*2000);

	// Socket Initilize
	int sockfd;
	struct sockaddr_in sin;

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family 		= AF_INET;
	sin.sin_port   		= htons(80);
	sin.sin_addr.s_addr = inet_addr(pIpAddr);

	if (sin.sin_addr.s_addr == INADDR_NONE) {
		AXIS_DBG("Invalid Address\n");
		goto HTTP_IMPL_END;
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
		AXIS_DBG("%d\n", sockfd);
		AXIS_DBG("socket() error!\n");
		goto HTTP_IMPL_END;
	}

	if (connect(sockfd, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
		AXIS_DBG("connect() error!\n");
		goto HTTP_IMPL_END;
	}

	// Set HTTP Request Message
	switch (pType) {
	case 1 : // Set Root Password
		sendSocketBuf = makeHTTPReqMsg("/axis-cgi/pwdroot/pwdroot.cgi?action=update&user=root&pwd=pass");
		break;
	case 2 : // Get Product Code
		sendSocketBuf = makeHTTPReqMsg("/axis-cgi/param.cgi?action=list&group=Brand.ProdFullName");
		break;
	case 3 : // Enable Webservice
		sendSocketBuf = makeHTTPReqMsg("/axis-cgi/param.cgi?action=update&root.WebService.Enabled=yes");
		break;
	}

	// Send HTTP Request Message
	AXIS_DBG("%s\n\n", sendSocketBuf);
	if (send(sockfd, sendSocketBuf, strlen(sendSocketBuf), 0) < 0) {
		AXIS_DBG("send() error!\n");
		goto HTTP_IMPL_END;
	}
	free(sendSocketBuf);

	// Receive HTTP Response
	memset(recvSocketBuf, 0x00, 1000);
	memset(totalRecvSocket, 0x00, 1000);
	while ((recvLen = recv(sockfd, recvSocketBuf, 1000, 0)) > 0) {
		memcpy(totalRecvSocket + totalRecvLen, recvSocketBuf, recvLen);
		totalRecvLen += recvLen;
	}
	AXIS_DBG("recv :\n%s\n", totalRecvSocket);

	// Parsing HTTP Response and Set pStrAxisDiscovery
	parsingHTTPRes(totalRecvSocket, pType, pStrAxisDiscovery);
	rtnVal = pStrAxisDiscovery->isAxisCamera;

HTTP_IMPL_END:

	free(recvSocketBuf);
	free(totalRecvSocket);
	close(sockfd);

	return rtnVal;
}

void parsingHTTPRes(char* pHTTPResponse, int pType, axis_Discovery* pStrAxisDiscovery)
{
	char* rbuf;
	char* modelNameFirstPos;
	char* modelName;

	// Parsing HTTP STATUS CODE
	int httpStatus;
	if (strstr(pHTTPResponse, "HTTP/1.1 200 OK") != NULL)
		httpStatus = 200;
	else if (strstr(pHTTPResponse, "HTTP/1.1 401 Unauthorized") != NULL)
		httpStatus = 401;
	else
		httpStatus = 0;

	
	switch (pType) {
	case 1 : // Set Root Password
		if (httpStatus != 0)
			pStrAxisDiscovery->isAxisCamera = 1;

		break;
	case 2 : // Get Product Code
		if (httpStatus == 0) {
			pStrAxisDiscovery->isAxisCamera = 0;
			break;
		}

		if (httpStatus == 401) {
			pStrAxisDiscovery->discoveryResult = AD_NOT_MATCH_ID_PASS;
			break;
		}

		if (strstr(pHTTPResponse, "Brand.ProdFullName=AXIS ") == NULL)
			break;

		modelNameFirstPos = strstr(pHTTPResponse, "Brand.ProdFullName=AXIS ");
		modelName = strtok_r (modelNameFirstPos," ", &rbuf );
		modelName = strtok_r (NULL," ", &rbuf);
		memcpy(pStrAxisDiscovery->modelName, modelName, strlen(modelName));
		break;
	case 3 : // Enable Webservice
		if (httpStatus == 0) {
			pStrAxisDiscovery->isAxisCamera = 0;
			break;
		}

		if (httpStatus == 401) {
			pStrAxisDiscovery->discoveryResult = AD_NOT_MATCH_ID_PASS;
			break;
		}

		if (strstr(strstr(pHTTPResponse, "200 OK"), "OK") != NULL)
			pStrAxisDiscovery->discoveryResult = AD_ENABLED_WEBSERVICE;
		else
			pStrAxisDiscovery->discoveryResult = AD_DISABLE_WEBSERVICE;

		break;
	}

	return;
}

// Make HTTP request message with basic authorization.
char* makeHTTPReqMsg(const char *pSendMsg)
{
	char auth_encbuf[256];
	char* str_send_msg = (char*)malloc(sizeof(char)*1000);
	
	b64_encode_string_to_buffer(auth_encbuf, 256, "root:pass");
	snprintf(str_send_msg, 1000, str_http_req_msg_raw, pSendMsg, auth_encbuf);

	return str_send_msg;
}

// Make base64 encoding text.
size_t b64_encode_string_to_buffer(char *p_dst, size_t i_dst, const char *p_src)
{
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned char in[3], out[4];
	int i, len, src_id, src_len = 0;
	size_t dst_len = 0;

	src_len = strlen(p_src);
	memset(p_dst, 0x00, i_dst);

	for (src_id = 0; src_id < src_len;)
	{
		len = 0;
		for (i=0; i<3; i++)
		{
			in[i] = *(p_src+src_id++);
			if (src_id <= src_len)
				len++;
			else
				in[i] = 0;
		}
		if (len)
		{
			out[0] = b64[in[0] >> 2];
			out[1] = b64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
			out[2] = (len > 1 ? b64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
			out[3] = (len > 2 ? b64[in[2] & 0x3f] : '=');
			for (i=0; i<4; i++)
			{
				*(p_dst+i) = out[i];
				dst_len++;
			}
			p_dst += 4;
		}
	}

	return dst_len;
}
