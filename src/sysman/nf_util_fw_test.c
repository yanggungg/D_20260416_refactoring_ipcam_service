
1590 //just for test
1591 gint nf_fw_seq_update_test(void)
1592 {
1593     gint ret=0, is_first=0;
1594     guint ping_seq=0, pong_seq=0;
1595     NF_FW_IMAGE_LIST imglist;
1596     guchar dataBuf[FW_UPGRADE_NAND_PAGE_SIZE] = {0,};
1597     guchar oobBuf[FW_UPGRADE_NAND_OOB_SIZE] = {0,};
1598     gint i=0;
1599 
1600     //ping read
1601     memset(dataBuf, 0x0, FW_UPGRADE_NAND_PAGE_SIZE);
1602     nf_flash_read(FW_UPGRADE_MTD_PING_NUM, 0, 0,dataBuf, oobBuf);
1603 
1604     memset(&imglist, 0x0, FW_UPGRADE_LIST_SIZE);
1605     memcpy(&imglist, dataBuf, FW_UPGRADE_LIST_SIZE);
1606     ping_seq = imglist.fwheader.seq;
1607 
1608     //pong read
1609     memset(dataBuf, 0x0, FW_UPGRADE_NAND_PAGE_SIZE);
1610     nf_flash_read(FW_UPGRADE_MTD_PONG_NUM, 0, 0,dataBuf, oobBuf);
1611     memset(&imglist, 0x0, FW_UPGRADE_LIST_SIZE);
1612     memcpy(&imglist, dataBuf, FW_UPGRADE_LIST_SIZE);
1613     pong_seq = imglist.fwheader.seq;
1614 
1615     if((ping_seq == 0xffffffff) && (pong_seq == 0xffffffff))
1616     {
1617         g_message("%s Upgrade is first...", __FUNCTION__);
1618         g_message("%s Current Sequence Number ==> ping_seq [%u] pong_seq [%u]\n",
1619                 __FUNCTION__, ping_seq, pong_seq);
1620 
1621         ping_seq = 1;
1622         pong_seq = 0;
1623         is_first = TRUE;
1624     }
1625     else
1626     {
1627         g_message("%s Current Sequence Number ==> ping_seq [%u] pong_seq [%u]\n",
1628                 __FUNCTION__, ping_seq, pong_seq);
1629         is_first = FALSE;
1630     }
1631 
1632 //  nf_fw_get_seq(&ping_seq, &pong_seq, &is_first);
1633     nf_fw_set_seq(&ping_seq, &pong_seq, is_first, &imglist);
1634    
1635     return 0;
1636 }

1185 static char fw_seq_update_help [] = "fw_seq_update [count]";
1186 static int fw_seq_update(int argc, char **argv)
1187 {
1188     gint cnt = 0, i=0;
1189     if ( argc < 1 ) {
1190         printf("Invalid arguments\n%s\n", fw_seq_update_help);
1191         return -1;
1192     }
1193 
1194     if(argc == 1)
1195         cnt = 0;
1196     else
1197         cnt = strtol(argv[1] , NULL, 0);
1198 
1199     for(i = 0; i<=cnt; i++)
1200         nf_fw_seq_update_test();
1201 
1202     return 0;
1203 }
1204 __commandlist(fw_seq_update, "fw_seq_up", "fw_seq_up [count]", fw_seq_update_help);


