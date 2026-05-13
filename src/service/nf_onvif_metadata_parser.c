#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <nf_ipcam_defs.h>


#define DEBUG if(0)

// Topic Name List
static char *motion_topic_name[] = {
	"tns1:VideoSource/MotionAlarm",					// Onvif Default
	"tns1:VideoAnalytics/tnsht:MotionDetection", 	// HITRON
	"tns1:VideoAnalytics/tnss1:MotionDetection", 	// S1
	"tns1:VideoAnalytics/tnsaxis:MotionDetection", 	// Axis
	NULL,
};

static char *alarm_topic_name[] = {
	"tns1:Device/Trigger/DigitalInput",
	NULL,
};


// GLOBAL VARIABLE
//static xmlChar *g_data;

 
//
static int firstxml_filter(char *data, char *pars_buf);
static int SearchXmlParsingElement(char *);

enum EVENT_STATE
{
	EVENT_FAIL	= 0,
	EVENT_OFF	= 1,
	EVENT_ON	= 2
};


//
// Metadata Event Parser Main

extern int nf_onvif_metadata_parser(char *meta_data)
{
	char *first_xml;
	char pars_buf[META_STREAM_LEN_MAX];
	
	if(meta_data == NULL) return -1;
	if(strnlen(meta_data,2) < 1) return -1;

	memset(pars_buf, 0x00, META_STREAM_LEN_MAX);

	if(firstxml_filter(meta_data, pars_buf) != 0)
	{
		//printf("\e[31m[%s:%d] NULL\e[0m\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return SearchXmlParsingElement(pars_buf);
}

// Get First XML Ducument From Multi XML Ducument (ex; HUVIRON)
static int firstxml_filter(char *data, char *pars_buf)
{
	char *xml, *xml2;
	int data_st;
	int xml_size;
	int rtn = 0;

	if(pars_buf == NULL) {
		IPCAM_DBG(MINOR, "pars_buf is NULL\n");
		rtn = -1;
		goto ends_label;
	}

	xml = strstr(data, "<?xml");
	xml_size = strnlen(data, META_STREAM_LEN_MAX);

	if(xml == NULL)
	{
		IPCAM_DBG(MINOR, "\"<?xml\" Search Fail\n");
		rtn = -1;
		goto ends_label;
	}

	xml2 = strstr(xml+5, "<?xml");
	if(xml2 != NULL)
	{
		xml_size = xml2-xml;
		if(xml_size > META_STREAM_LEN_MAX)
		{
			IPCAM_DBG(MINOR, "xml data Size Over\n");
			rtn = -1;
			goto ends_label;
		}

		memset(pars_buf,0x00, META_STREAM_LEN_MAX);
		strncpy(pars_buf, xml, xml_size);

		rtn = 0;
		goto ends_label;
	}

	if(xml_size > META_STREAM_LEN_MAX)
	{
		IPCAM_DBG(MINOR, "xml data size over\n");
		rtn = -1;
		goto ends_label;
	}

	memcpy(pars_buf, xml, xml_size);
ends_label:

	return rtn;
}

// Parsing Data
static int DataParsing(xmlNodePtr data)
{
	int rtn = EVENT_FAIL; // Return Value
	xmlNodePtr item;
	xmlChar *event;



	if(data->xmlChildrenNode == NULL)
	{
		DEBUG printf("\e[31mData Children Element NULL\e[0m\n");
		return EVENT_FAIL;
	}
	DEBUG printf("Data Parsing: %s\n", data->name);

	item = data->xmlChildrenNode;

	for(;item != NULL; item = item->next)
	{
		if(xmlIsBlankNode(item)) continue;
		break;
	}

	DEBUG printf("Item Name: %s\n", item->name);


	event = xmlGetProp(item, (const xmlChar*)"Type");
	if(event != NULL)
	{
		if(xmlStrEqual(event, (xmlChar*)"true") || xmlStrEqual(event, (xmlChar*)"1"))
		{
			DEBUG printf("Type Event On : %s\n",(char*)event);
			rtn = EVENT_ON;
		}
		else if(xmlStrEqual(event, (xmlChar*)"false") || xmlStrEqual(event, (xmlChar*)"0"))
		{
			DEBUG printf("Type Event Off : %s\n",(char*)event);
			rtn = EVENT_OFF;
		}

		xmlFree(event);
		return rtn;
	}
	
	event = xmlGetProp(item, (const xmlChar*)"Value");
	if(event != NULL)
	{
		if(xmlStrEqual(event, (xmlChar*)"true") || xmlStrEqual(event, (xmlChar*)"1"))
		{
			DEBUG printf("Value Event On : %s\n",(char*)event);
			rtn = EVENT_ON;
		}
		else if(xmlStrEqual(event, (xmlChar*)"false") || xmlStrEqual(event, (xmlChar*)"0"))
		{
			DEBUG printf("Value Event Off : %s\n",(char*)event);
			rtn = EVENT_OFF;
		}
		xmlFree(event);
		return rtn;
	}

	
	return EVENT_FAIL;

}
// Parsing Motion Message
static int MessageParsing(xmlNodePtr noti)
{
	xmlNodePtr message;
	xmlNodePtr mess2;
	xmlNodePtr data;

	int rtn;

	message = noti->xmlChildrenNode;

	//DEBUG printf("noti-> name : %s \n", noti->name);

	do
	{

		//DEBUG printf("message->name = %s\n",message->name);
		if((!xmlStrcmp(message->name, (const xmlChar *)"Message"))) // Search NotificationMessage
		{
			mess2 = message->xmlChildrenNode;
			//DEBUG printf("message2->name = %s\n", mess2->name);
			do
			{

				if((!xmlStrcmp(mess2->name, (const xmlChar *)"Message"))) // Search Topic(is Topic Motion ?)
				{
					data = mess2->xmlChildrenNode;
					do{

						if((!xmlStrcmp(data->name, (const xmlChar *)"Data"))) // Search Topic(is Topic Motion ?)
						{
							DEBUG printf("OnvifMetaData::Find Event Data\n");
							rtn = DataParsing(data);
							if(rtn)
							{
								return rtn;
							}
						}

						data = data->next;
					}while(data != NULL);
				}
				mess2 = mess2->next;

			}while(mess2 !=NULL);
		}
		message = message->next;

	}while( message != NULL);

	return EVENT_FAIL; // Message Find Fail

}


// Search Motion Alarm Topic 
static int SearchXmlParsingElement(char *data){
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlNodePtr temp;
	xmlNodePtr noti;
	xmlNodePtr topic;
	int data_len;

	int rtn;

	int motion_flag = 0;
	int alarm_flag = 0;

	/* create xml document object */
	//doc = xmlParseDoc(data);
	data_len = strlen(data);

	doc = xmlReadMemory(data, data_len, "meta.xml", NULL, XML_PARSE_NOERROR|XML_PARSE_NOWARNING);

	/* create xml document object exception */
	if(doc == NULL){
		DEBUG printf("Document not parsed successfully.\n");
		return 0;
	}

	/* get Root Element*/ 
	cur = xmlDocGetRootElement(doc);

	/* check Root Element */
	if(cur == NULL){
		DEBUG printf("empty document.\n");
		xmlFreeDoc(doc);
		return 0;
	}

	/* Root Element Name Check*/
	//DEBUG printf("cur->name = %s \n", cur->name);
	if(xmlStrcmp(cur->name, (const xmlChar *)  "MetaDataStream"))
	{
		if(xmlStrcmp(cur->name, (const xmlChar *)  "MetadataStream"))
		{
			DEBUG printf("document of the wrong type, root node != reginfo \n");
			xmlFreeDoc(doc);
			return 0;
		}

	}

	/* Get Childen Element*/
	cur = cur->xmlChildrenNode;


	while(cur != NULL) // Search Event Loop
	{

		//DEBUG printf("depth1->name = %s \n", cur->name);
		if((!xmlStrcmp(cur->name, (const xmlChar *)"Event"))) // Search Event
		{
			noti = cur->xmlChildrenNode;

			while(noti != NULL) // Search Notificaton
			{  

				//DEBUG printf("depth2->name = %s\n",noti->name);
				if((!xmlStrcmp(noti->name, (const xmlChar *)"NotificationMessage"))) // Search NotificationMessage
				{
					topic = noti->xmlChildrenNode;

					while(topic != NULL)
					{
						//DEBUG printf("depth3->name = %s\n", topic->name);
						if((!xmlStrcmp(topic->name, (const xmlChar *)"Topic"))) // Search Topic(is Topic Motion ?)
						{
							xmlChar *key;
							temp = topic;

							key = xmlNodeGetContent(temp);
							DEBUG printf("OnvifMetaData::Find Topic = %s\n",key);

							int i = 0;
							if(motion_flag == 0 || alarm_flag != EVENT_ON)
							{
								do // Topic Motion Check
								{

									if(!strcmp((char*)key, motion_topic_name[i]))	// Topic Motion 
									{
										DEBUG printf("OnvifMetaData::Motion Event\n");
										rtn  = MessageParsing(noti);

										if(rtn)
										{
											motion_flag = rtn;
											//xmlFree(key);
											//xmlFreeDoc(doc);

											//return rtn;
										}

										break;
									}

									i++;

								}while(motion_topic_name[i] != NULL); // Search Tpoic Loop
							}

							i = 0;
							if(alarm_flag == 0 || alarm_flag != EVENT_ON)
							{
								do
								{
									if(!strcmp((char*)key, alarm_topic_name[i]))
									{
										DEBUG printf("OnvifMetadata::Alarm Event\n");
										rtn = MessageParsing(noti);
										if(rtn)
										{
											alarm_flag = rtn;
											//xmlFree(key);
											//xmlFreeDoc(doc);
										}

										break;
									}
									i++;
								}while(alarm_topic_name[i] != NULL);
							}

							xmlFree(key);

						}

						topic = topic->next;
					}
				}
				noti = noti->next;

			}

			break; // Event 
		} // Event End
		else
		{
			cur = cur->next;
		}

	}

	xmlFreeDoc(doc);

	if(alarm_flag > 0 || motion_flag > 0)
	{
		alarm_flag = alarm_flag<<2;
		rtn = alarm_flag | motion_flag;

		return rtn;
	}



	// Search End
	return EVENT_FAIL;

}

