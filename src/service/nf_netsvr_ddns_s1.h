#ifndef _s1_ddns2_manager_h_
#define _s1_ddns2_manager_h_

int _s1_register_write_data(const int sock, const int portnumber, const int rtsp_port, const unsigned char *mac_addr);
int _s1_register_read_data(const int sock);

int _s1_query_write_data(const int sock, const unsigned char *mac_addr);
int _s1_query_read_data(const int sock, unsigned int *ipv4_addr);

#endif
