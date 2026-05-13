#ifndef _ONVIF_DISCOVERY_H_
#define _ONVIF_DISCOVERY_H_

unsigned int is_https_required(void);
void multi_bye();

static char hello_response[] =
"<?xml version='1.0' encoding='utf-8'?>\
<soap:Envelope \
xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
xmlns:wsadis=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" \
xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\">\
<soap:Header>\
<wsadis:Action>\
http://schemas.xmlsoap.org/ws/2005/04/discovery/Hello\
</wsadis:Action>\
<wsadis:MessageID>\
uuid:b53e8e74-89e8-4d4b-b178-134a15854725\
</wsadis:MessageID>\
<wsadis:RelatesTo>\
</wsadis:RelatesTo>\
<wsadis:To>\
http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous\
</wsadis:To>\
<d:AppSequence InstanceId=\"1077004800\" MessageNumber=\"2\" />\
</soap:Header>\
<soap:Body>\
<d:Hello>\
<wsadis:EndpointReference>\
<wsadis:Address>\
</wsadis:Address>\
</wsadis:EndpointReference>\
<d:Types>tds:Device dn:NetworkVideoTransmitter</d:Types>\
<d:Scopes>\
onvif://www.onvif.org/name/NCX \
</d:Scopes>\
<d:XAddrs>http://192.168.100.5</d:XAddrs>\
<d:MetadataVersion>1</d:MetadataVersion>\
</d:Hello>\
</soap:Body>\
</soap:Envelope>\n";


#if 1
static char  bye_templet[] = \
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\">\
<SOAP-ENV:Header>\
<wsa:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Bye</wsa:Action>\
<wsa:MessageID>%s</wsa:MessageID>\
<wsa:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>\
<d:AppSequence InstanceId=\"1077004800\" MessageNumber=\"5\" />\
</SOAP-ENV:Header>\
<SOAP-ENV:Body>\
<d:Bye>\
<wsa:EndpointReference><wsa:Address>%s</wsa:Address></wsa:EndpointReference>\
<d:Types>tds:Device dn:NetworkVideoTransmitter</d:Types>\
<d:Scopes>%s</d:Scopes>\
<d:XAddrs>%s</d:XAddrs>\
<d:MetadataVersion>1</d:MetadataVersion>\
</d:Bye>\
</SOAP-ENV:Body></SOAP-ENV:Envelope>";

static char bye_response[] = \
"<?xml version='1.0' encoding='utf-8'?>\
<soap:Envelope xmlns:wsadis=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" >\
<soap:Header>\
<wsadis:Action>\
http://schemas.xmlsoap.org/ws/2005/04/discovery/Bye\
</wsadis:Action>\
<wsadis:MessageID>\
\%s \
</wsadis:MessageID> \
<wsadis:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsadis:To> \
<d:AppSequence InstanceId=\"1077004800\" MessageNumber=\"4\" /> \
</soap:Header> \
<soap:Body> \
<d:Bye>\
<wsadis:EndpointReference>\
<wsadis:Address>\%s</wsadis:Address>\
</wsadis:EndpointReference>\
</d:Bye> \
</soap:Body> \
</soap:Envelope>\n";
#else
static char bye_response[] =
{
"<?xml version='1.0' encoding='utf-8'?>"
"<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
"xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
"xmlns:wsadis=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
"xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\">"
"<soap:Header>"
"<wsadis:Action>"
"http://schemas.xmlsoap.org/ws/2005/04/discovery/Bye"
"</wsadis:Action>"
"<wsadis:MessageID>"
"%s"
"</wsadis:MessageID>"
"<wsadis:RelatesTo>"
"%s"
"</wsadis:RelatesTo>"
"<wsadis:To>"
"http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous"
"</wsadis:To>"
"<d:AppSequence InstanceId=\"1077004800\" MessageNumber=\"2\" />"
"</soap:Header>"
"<soap:Body>"
"<d:Bye>"
"<wsadis:EndpointReference>"
"<wsadis:Address>"
"</wsadis:Address>"
"</wsadis:EndpointReference>"
"<d:Types>tds:Device dn:NetworkVideoTransmitter</d:Types>"
"<d:Scopes>"
"%s"
"</d:Scopes>"
"<d:XAddrs>"
"%s"
"</d:XAddrs>"
"<d:MetadataVersion>1</d:MetadataVersion>"
"</d:Bye>"
"</soap:Body>"
"</soap:Envelope>\n"
};
#endif



static char probe_match[] =
"<?xml version='1.0' encoding='utf-8'?>\
<soap:Envelope \
xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
xmlns:wsadis=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" \
xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\"> \
<soap:Header>\
<wsadis:Action>\
http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches\
</wsadis:Action>\
<wsadis:MessageID>\
uuid:b53e8e74-89e8-4d4b-b178-134a15854725\
</wsadis:MessageID>\
<wsadis:RelatesTo>\
</wsadis:RelatesTo>\
<wsadis:To>\
http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous\
</wsadis:To>\
<d:AppSequence InstanceId=\"1077004800\" MessageNumber=\"2\" />\
</soap:Header>\
<soap:Body>\
<d:ProbeMatches>\
<d:ProbeMatch>\
<wsadis:EndpointReference>\
<wsadis:Address>\
urn:uuid:98190dc2-0890-4ef8-ac9a-5940995e6119\
</wsadis:Address>\
</wsadis:EndpointReference>\
<d:Types>tds:Device dn:NetworkVideoTransmitter</d:Types>\
<d:Scopes>\
onvif://www.onvif.org/type/video_encoder \
onvif://www.onvif.org/location/country/korea \
onvif://www.onvif.org/hardware/ARM \
onvif://www.onvif.org/name/NCX \
</d:Scopes>\
<d:XAddrs>http://192.168.10.170/cgi-bin/onvif.cgi</d:XAddrs>\
<d:MetadataVersion>1</d:MetadataVersion>\
</d:ProbeMatch>\
</d:ProbeMatches>\
</soap:Body>\
</soap:Envelope>\n";

#if 1
static char genetec_response[] =
"<GenetecProtocolDiscoveryReply>\
<IpAddress>192.168.100.100</IpAddress>\
<HttpPort>8080</HttpPort>\
<MacAddress>00-11-22-33-44-55</MacAddress>\
<CompanyName></CompanyName>\
<ModelName>NCX-xxxx(P)</ModelName>\
<HttpsSupported>true</HttpsSupported>\
</GenetecProtocolDiscoveryReply>";
#else
static char genetec_response[] =
"<?xml version='1.0' encoding='utf-8'?>\
<GenetecProtocolDiscoveryReply>\
<IpAddress>192.168.100.100</IpAddress>\
<HttpPort>8080</HttpPort>\
<MacAddress>00-11-22-33-44-55</MacAddress>\
<CompanyName>ITX Security, Inc</CompanyName>\
<ModelName>NCX-xxxx(P)</ModelName>\
<HttpsSupported>true</HttpsSupported>\
</GenetecProtocolDiscoveryReply>";
#endif

static const char probe_error_response[] =
{
	"<?xml version='1.0' encoding='utf-8'?>"
	"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" "
	"xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" " 
	"xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
	"xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
	"xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" "
	"xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\">"
	"<SOAP-ENV:Header>"
	"<wsa:RelatesTo>"
	"%s"
	"</wsa:RelatesTo>"
	"<wsa:To SOAP-ENV:mustUnderstand=\"true\">"
	"http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous"
	"</wsa:To>"
	"<wsa:Action SOAP-ENV:mustUnderstand=\"true\">"
	"http://schemas.xmlsoap.org/ws/2005/04/discovery/fault"
	"</wsa:Action>"
	"</SOAP-ENV:Header>"
	"<SOAP-ENV:Body>"
	"<SOAP-ENV:Fault>"
	"<SOAP-ENV:Code>"
	"<SOAP-ENV:Value>"
	"SOAP-ENV:Sender"
	"</SOAP-ENV:Value>"
	"<SOAP-ENV:Subcode>"
	"<SOAP-ENV:Value>"
	"d:MatchingRuleNotSupported"
	"</SOAP-ENV:Value>"
	"<SOAP-ENV:Subcode>"
	"<SOAP-ENV:Value>"
	"d:MatchingRuleNotSupported"
	"</SOAP-ENV:Value>"
	"</SOAP-ENV:Subcode>"
	"</SOAP-ENV:Subcode>"
	"</SOAP-ENV:Code>"
	"<SOAP-ENV:Reason>"
	"<SOAP-ENV:Text xml:lang=\"en\">"
	"The matching rule specified is not supported."
	"</SOAP-ENV:Text>"
	"</SOAP-ENV:Reason>"
	"<SOAP-ENV:Detail>"
	"<d:SupportedMatchingRules>"
	"http://schemas.xmlsoap.org/ws/2005/04/discovery/rfc3986"
	"</d:SupportedMatchingRules>"
	"</SOAP-ENV:Detail>"
	"</SOAP-ENV:Fault>"
	"</SOAP-ENV:Body>"
	"</SOAP-ENV:Envelope>"
};

#if 0
static const char probe_error_response[] =
{
	"<?xml version='1.0' encoding='utf-8'?>"
	"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" "
	"xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" " 
	"xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" "
	"xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" "
	"xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" "
	"xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\"> "
	"<SOAP-ENV:Header> "
	"<wsa:RelatesTo> "
	"%s "
	"</wsa:RelatesTo> "
	"<wsa:To SOAP-ENV:mustUnderstand=\"true\"> "
	"http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous"
	"</wsa:To>"
	"<wsa:Action SOAP-ENV:mustUnderstand=\"true\">"
	"http://schemas.xmlsoap.org/ws/2005/04/discovery/fault"
	"</wsa:Action>"
	"</SOAP-ENV:Header>"
	"<SOAP-ENV:Body>"
	"<SOAP-ENV:Fault>"
	"<SOAP-ENV:Code>"
	"<SOAP-ENV:Value>"
	"SOAP-ENV:Sender"
	"</SOAP-ENV:Value>"
	"<SOAP-ENV:Subcode>"
	"<SOAP-ENV:Value>"
	"d:MatchingRuleNotSupported"
	"</SOAP-ENV:Value>"
	"</SOAP-ENV:Subcode>"
	"</SOAP-ENV:Code>"
	"<SOAP-ENV:Reason>"
	"<SOAP-ENV:Text xml:lang=\"en\">"
	"The matching rule specified is not supported."
	"</SOAP-ENV:Text>"
	"</SOAP-ENV:Reason>"
	"<SOAP-ENV:Detail>"
	"<d:SupportedMatchingRules>"
	"http://schemas.xmlsoap.org/ws/2005/04/discovery/rfc3986"
	"</d:SupportedMatchingRules>"
	"</SOAP-ENV:Detail>"
	"</SOAP-ENV:Fault>"
	"</SOAP-ENV:Body>"
	"</SOAP-ENV:Envelope>\r\n\r\n"
};
#endif
#endif
