#ifndef __VW_SYS_NET_8021X_SECURITY_H__
#define __VW_SYS_NET_8021X_SECURITY_H__

void VW_Init_Security_8021x_Page(NFOBJECT *parent);
gboolean check_net_security_8021x_changed();
void save_net_security_8021x_data();
void restore_net_security_8021x_data();

#endif