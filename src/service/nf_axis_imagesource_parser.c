#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "nf_ipcam_driver_axis.h"

#define XML_MAX (15360)	//Max 15Kbyte - ImageSource > node 
#define xml_Debug (0)	//print xml current value
enum PARS_STATE
{
	_FAIL = -1,
	_OK = 0,
	_RUNNING = 1
};

enum defineType
{
	DATA_TYPE_ENUM,
	DATA_TYPE_INT,
	DATA_TYPE_BOOL,
};

static struct DataType *axis_head;
static struct DataType *axis_tail;

static xmlChar*  Data;

int Head_Tail_init();
static void push_next(struct DataType *cur_ptr);
static void getEnumProp(xmlNodePtr cur, char* findName, char* dest);
static int parseDoc(const char* document);
static void parseGroup(xmlNodePtr cur);
static void parseSubGroup(xmlNodePtr cur);
static void parseParam(xmlNodePtr cur);
static struct DataType* enumParser(xmlNodePtr parentsPTR, xmlNodePtr cur, int flag);
static struct DataType* intParser(xmlNodePtr parentsPTR, xmlNodePtr cur, int flag);
struct DataType* boolParser(xmlNodePtr parentsPTR,xmlNodePtr cur, int flag);
static void printList(struct DataType *ptr);
static int getIntProp(xmlNodePtr node, char *prop_name);

extern int nf_axis_get_imagesource(char* source);
extern int nf_axis_get_appearance(char* source);
extern void nf_axis_image_free();

extern struct DataType* getAxisImageListHead(void)
{
	return	axis_head; 
}

extern struct DataType* getAxisImageListTail(void)
{
	return	axis_tail; 
}

void nf_axis_image_free()
{
	struct DataType *pre = NULL;
	struct DataType *end = NULL;
	int cnt = 0;

	if(axis_head == NULL)
		goto ends_label;

	if(axis_tail == NULL)
		goto ends_label;
	
	pre = axis_head->next;

	while(pre != axis_tail)
	{
		end = pre;
		pre = pre->next;
		if(end != NULL)
			free(end);
		cnt ++;
	}
	
ends_label:

	if(axis_head != NULL) 
	{
		free(axis_head);
		axis_head = NULL;
	}
	if(axis_tail != NULL) 
	{
		free(axis_tail);
		axis_tail = NULL;
	}

	return;
}

int Head_Tail_init(void)
{
	axis_head = NULL;
	axis_tail = NULL;

	axis_head = (struct DataType *)malloc(sizeof(struct DataType));
	if(axis_head == NULL)
		goto ends_label;
	axis_tail = (struct DataType *)malloc(sizeof(struct DataType));
	if(axis_tail == NULL)
		goto ends_label;

	memset(axis_head, 0x00, sizeof(struct DataType));
	memset(axis_tail, 0x00, sizeof(struct DataType));

	axis_head->next = axis_tail;
	axis_tail->next = NULL;

	return 0;

ends_label:

	if(axis_head != NULL) 
	{
		free(axis_head);
		axis_head = NULL;
	}
	if(axis_tail != NULL) 
	{
		free(axis_tail);
		axis_tail = NULL;
	}

	return -1;
}


// * AXIS GET ImageSource LIST * //

int nf_axis_get_imagesource(char* source)
{
	int rtn = _FAIL;
	if(source == NULL)
	{ 
		return _FAIL;
	}
	rtn = parseDoc(source);
	
	return rtn;
}

int nf_axis_get_appearance(char *source)
{
	int rtn = _FAIL;
	if(source == NULL)
	{ 
		return _FAIL;
	}
	rtn = parseDoc(source);
	return rtn;
}



static char* firstxml_filter(const char *data, char* buff)
{
	char *xml = NULL;

	xml = strstr(data, "<?xml");

	if(xml == NULL)
	{
		return NULL;
	}

	memset(buff, 0x00, XML_MAX);
	strncpy(buff, xml, XML_MAX -1);

	return buff;
}


static void push_next(struct DataType *cur_ptr)
{
	struct DataType *temp_ptr = NULL;

	if (cur_ptr == NULL)
	{
		return;
	}

	temp_ptr = axis_head;

	while (temp_ptr->next != axis_tail)
	{
		temp_ptr = temp_ptr->next;
	}

	cur_ptr->next = axis_tail;
	temp_ptr->next = cur_ptr;

	return;
}

static int parseDoc(const char* document)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlNodePtr root_node = NULL;
	char *tester = NULL;
	char *XML = NULL;
	char xml_data[XML_MAX] = {0, };
	int rtn = -1;
	int data_len = 0;
		
	if (document == NULL)
	{
		return rtn;
	}

	tester = firstxml_filter(document, xml_data);
	
	if(tester == NULL)
		return rtn;

	Data = (xmlChar*)tester;
	data_len = strlen(Data);

	doc = xmlReadMemory(Data, data_len, NULL, NULL, XML_PARSE_HUGE  | XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NOBLANKS );
	//doc = xmlParseDoc(Data);
	
	if(doc == NULL)
	{
		printf("Document not parsed successfully.\n");	
		return rtn;
	}

	root_node = xmlDocGetRootElement(doc);
	
	if (root_node == NULL)
	{	
		xmlFreeDoc(doc);
		return rtn;
	}
	
	if (xmlStrcmp(root_node->name, (const xmlChar *)  "parameterDefinitions"))
	{
		xmlFreeDoc(doc);
		return rtn;
	}

	rtn = Head_Tail_init();
	
	if(rtn != 0)
	{
		xmlFreeDoc(doc);
		return rtn;
	}

	cur = root_node->xmlChildrenNode;

	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *)  "group"))
		{
			parseGroup(cur);
		}
		cur = cur->next;
	}
	
	xmlFreeDoc(doc);
#if xml_Debug
	printList(axis_head);	
#endif
	return 0;
}

static void parseGroup(xmlNodePtr cur)
{
	xmlChar *check1 = NULL;
	xmlChar *check2 = NULL;
	xmlNodePtr next_cur = NULL;

	cur = cur->xmlChildrenNode;

	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"group")))
		{
			check1 = xmlGetProp(cur, (const xmlChar*)"name");

			if (check1 != NULL)
			{
				if (xmlStrEqual(check1, (xmlChar*)"ImageSource"))
				{
					next_cur = cur->xmlChildrenNode;

					while (next_cur != NULL)
					{
						if ((!xmlStrcmp(next_cur->name, (const xmlChar *)"group"))) //I0 routine 
						{
							check2 = xmlGetProp(next_cur, (const xmlChar*)"name");

							if (xmlStrEqual(check2, (xmlChar*)"I0"))
							{
								parseSubGroup(next_cur);
								//break;
							}

							xmlFree(check2);	
						}
						next_cur = next_cur->next;
					}
				}
				check2 = NULL;	// check2 
				
				if (xmlStrEqual(check1, (xmlChar*)"Image"))
				{
					next_cur = cur->xmlChildrenNode;

					while (next_cur != NULL)
					{
						if ((!xmlStrcmp(next_cur->name, (const xmlChar *)"group"))) //I0 routine 
						{
							check2 = xmlGetProp(next_cur, (const xmlChar*)"name");

							if (xmlStrEqual(check2, (xmlChar*)"I0"))
							{
								parseSubGroup(next_cur);
								//break;
							}

							xmlFree(check2);	
						}
						next_cur = next_cur->next;
					}
				}
				check2 = NULL;
			}
			xmlFree(check1);
		}
		cur = cur->next;
	}

	return;
}

static void parseSubGroup(xmlNodePtr cur)
{
	int i;
	xmlChar *sub_group[4] = {
		"Sensor",
		"DayNight",
		"DCIris",
		"Appearance",
	};
	xmlChar *check = NULL;

	cur = cur->xmlChildrenNode;

	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"group"))) //search for sub_group 
		{
			check = xmlGetProp(cur, (const xmlChar*)"name");

			for (i = 0; i < 4; i++)
			{
				if (xmlStrEqual(check, sub_group[i]))
				{
#if xml_Debug	
					printf("\e[33m >> pars current subGroup : %s \e[0m\n", sub_group[i]);
#endif
					parseParam(cur);
				}
			}
			xmlFree(check);
		}

		cur = cur->next;
	}

	return;
}

static void parseParam(xmlNodePtr cur)
{
	int paramFlag = 0;
	xmlNodePtr temp = NULL;
	xmlNodePtr cur_1 = NULL;
	xmlNodePtr cur_2 = NULL;
	xmlNodePtr parentsPTR = NULL;

	cur = cur->xmlChildrenNode;

	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"parameter"))) //search for sub_group 
		{
			parentsPTR = cur;
			cur_1 = cur->xmlChildrenNode;
			
			while(cur_1 != NULL)
			{
				if ((!xmlStrcmp(cur_1->name, (const xmlChar *)"type")))
				{
					cur_2 = cur_1->xmlChildrenNode;

					while (cur_2 != NULL)
					{
						if ((!xmlStrcmp(cur_2->name, (const xmlChar *)"enum")))
						{
							paramFlag = 0;
							push_next(enumParser(parentsPTR, cur_2, paramFlag));
						}

						if ((!xmlStrcmp(cur_2->name, (const xmlChar *)"int")))
						{
							paramFlag = 1;
							push_next(intParser(parentsPTR, cur_2, paramFlag));
						}

						if ((!xmlStrcmp(cur_2->name, (const xmlChar *)"bool")))
						{
							paramFlag = 2;
							push_next(boolParser(parentsPTR, cur_2, paramFlag));
						}

						cur_2 = cur_2->next;
					}
				}
				cur_1 = cur_1->next;
			}
		}
		cur = cur->next;
	}
	
	return;
}

static void getEnumProp(xmlNodePtr cur, char* findName, char* dest)
{
	xmlChar *dummy = NULL;

	dummy = xmlGetProp(cur, (const char*)findName);
	if(dummy == NULL)
	{
		return NULL;
	}
	strcpy(dest, (char *)dummy);

	xmlFree(dummy);
}

static int getIntProp(xmlNodePtr node, char *prop_name)
{
	xmlChar *temp = NULL;
	int value;

	temp = xmlGetProp(node, (xmlChar*)prop_name);
	if(temp == NULL)
	{
		return NULL;
	}
	value = (int)strtol((char*)temp, NULL, 10);

	xmlFree(temp);
	
	return value;
}

struct DataType* enumParser(xmlNodePtr parentsPTR, xmlNodePtr cur, int flag)
{
	struct DataType *E_Data = NULL;
	int cnt = 0;
	
	E_Data = (struct DataType *)malloc(sizeof(struct DataType));
	if(E_Data == NULL)
		return NULL;
	memset(E_Data, 0x00, sizeof(struct DataType));

	E_Data->type = DATA_TYPE_ENUM;

	if (parentsPTR == NULL || flag != 0)
	{
		free(E_Data);
		return NULL;
	}

	cur = cur->xmlChildrenNode;
	
	if ((xmlStrcmp(cur->name, (const xmlChar *)"entry")))
	{
		free(E_Data);
		return NULL;
	}
	
	getEnumProp(parentsPTR, "name", E_Data->name);
	getEnumProp(parentsPTR, "value", E_Data->e_value);
	getEnumProp(parentsPTR, "value", E_Data->nice_name);

	while (cur != NULL && cnt < 35)	//exception Overflow cnt +1 margin 
	{
		getEnumProp(cur, "value", E_Data->eData.value[cnt]);
		getEnumProp(cur, "niceValue", E_Data->eData.nice_value[cnt]);

		cur = cur->next;
		cnt++;
	}

	E_Data->eData.enumCnt = cnt;
	
	return E_Data;
}

struct DataType* intParser(xmlNodePtr parentsPTR,xmlNodePtr cur, int flag)
{
	struct DataType *I_Data = NULL;

	I_Data = (struct DataType *)malloc(sizeof(struct DataType));
	if(I_Data == NULL)
		return NULL;
	memset(I_Data, 0x00, sizeof(struct DataType));

	I_Data->type = DATA_TYPE_INT;

	if (parentsPTR == NULL || flag != 1)
	{
		free(I_Data);
		return NULL;
	}

	getEnumProp(parentsPTR, "name",I_Data->name);
	
	I_Data->i_value = getIntProp(parentsPTR, "value"); 
	getEnumProp(parentsPTR, "niceName",I_Data->nice_name);

	I_Data->iData.min = getIntProp(cur, "min");
	I_Data->iData.max = getIntProp(cur, "max");

	return I_Data;
}

struct DataType* boolParser(xmlNodePtr parentsPTR,xmlNodePtr cur, int flag)
{
	struct DataType *B_Data = NULL;

	B_Data = (struct DataType *)malloc(sizeof(struct DataType));
	if(B_Data == NULL)
		return NULL;
	memset(B_Data, 0x00, sizeof(struct DataType));
	B_Data->type = DATA_TYPE_BOOL;

	if (parentsPTR == NULL || flag != 2)
	{
		free(B_Data);
		return NULL;
	}

	getEnumProp(parentsPTR, "name", B_Data->name);
	getEnumProp(parentsPTR, "value", B_Data->b_value);
	getEnumProp(parentsPTR, "niceName",B_Data->nice_name);

	return B_Data;
}


#if xml_Debug
static void printList(struct DataType *ptr)
{
	struct DataType *tempPTR = NULL;
	int i = 0;
	tempPTR = ptr->next;
	
	while(tempPTR != axis_tail)
	{
		printf("\e[31m \n\n>> ================ Type : \t%d ===========================\e[0m\n", tempPTR->type);	
		printf("\e[31m >>                  parameter name : %s  \e[0m\n", tempPTR->name);	
		printf("\e[31m >>                  parameter nice_name : %s  \e[0m\n", tempPTR->nice_name);
		printf("\e[31m >> =========================================================   \e[0m\n");	

		if((tempPTR->type) == 0 )
		{
			printf("\e[31m >>              current value : %s \e[0m\n", tempPTR->e_value);	
/*
			for(i = 0; i < tempPTR->eData.enumCnt; i++)
			{
				printf("\e[31m >>          option value : %s  \e[0m\n", tempPTR->eData.value[i]);	
				printf("\e[31m >>          option name  : %s  \e[0m\n", tempPTR->eData.nice_value[i]);	
				printf("\e[31m >> =========================================================   \e[0m\n");	
			}
			*/
		}
		else if((tempPTR->type) == 1 )
		{
			printf("\e[31m >>              optional min :\t%d  \e[0m\n", tempPTR->iData.min);	
			printf("\e[31m >>              optional max :\t%d  \e[0m\n", tempPTR->iData.max);	
			printf("\e[31m >> =========================================================   \e[0m\n");	
		}
		else if((tempPTR->type) == 2 )
		{
			printf("\e[31m >>              current value :\t%s  \e[0m\n", tempPTR->b_value);	
			printf("\e[31m >> =========================================================   \e[0m\n");	
		}

		tempPTR = tempPTR->next;
	}
}
#endif
