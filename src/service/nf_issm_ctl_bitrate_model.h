#if defined(_IPX_32P4E)|| defined(_IPX_32M4E) || defined(_IPX_32P5)
#define MAX_BANDWIDTH	(49152)			// KBps => 384Mbps
#elif defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E)
#define MAX_BANDWIDTH	(65536)			// KBps => 512Mbps 
#elif defined(_IPX_0824P4E) || defined(_IPX_1648P4E)
#define MAX_BANDWIDTH	(16384)			// KBps => 128Mbps
#else
#define MAX_BANDWIDTH	(16384)			// KBps	
#endif

#define BITRATE_CTL_ON_SETUP (0)		// 0 : Off, 1 : On

#define MAIN_RATIO (2)					// Max : 4 Mbps
#define SECOND_RATIO (1)				// Max : 2 Mbps

#define BITRATE_CTL_LAN_EXCEPTION	(0)	// 0 : Off, 1 : On
