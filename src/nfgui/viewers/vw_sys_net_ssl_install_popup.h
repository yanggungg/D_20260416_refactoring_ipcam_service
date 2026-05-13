#ifndef __VW_SYS_NET_INSTALL_SSL_CERTIFICATE_H__
#define __VW_SYS_NET_INSTALL_SSL_CERTIFICATE_H__


gboolean VW_SSL_Installer_Open(NFWINDOW *parent);
gboolean VW_Show_SSL_Installer_main();
void vw_ssl_installer_builtin_page(NFOBJECT *parent);
void vw_ssl_installer_ca_page(NFOBJECT *parent);
void vw_ssl_installer_self_page(NFOBJECT *parent);

#endif
