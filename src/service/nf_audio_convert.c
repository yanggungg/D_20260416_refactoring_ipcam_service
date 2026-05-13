#include "nf_common.h"

#include "nf_audio_convert.h"

/**************************************************************************************
	Convert
**************************************************************************************/
#define BIAS			(0x84)          /* Bias for linear code. */
#define CLIP			8159

#define SIGN_BIT        (0x80)          /* Sign bit for a A-law byte. */
#define QUANT_MASK      (0xf)           /* Quantization field mask. */
#define NSEGS           (8)             /* Number of A-law segments. */
#define SEG_SHIFT       (4)             /* Left shift for segment number. */
#define SEG_MASK        (0x70)          /* Segment field mask. */

static short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};
static short seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF};

guchar nf_audio_cvt_lpcm16_to_muraw(short  pcm_val)    /* 2's complement (16-bit range) */
{
	short mask=0, seg=0;
	guchar uval=0;

	/* Get the sign and the magnitude of the value. */
	pcm_val = pcm_val >> 2;
	if (pcm_val < 0) {
		pcm_val = -pcm_val;
		mask = 0x7F;
	} else {
		mask = 0xFF;
	}
		if ( pcm_val > CLIP ) pcm_val = CLIP;       /* clip the magnitude */
	pcm_val += (BIAS >> 2);

	/* Convert the scaled magnitude to segment number. */
	seg = nf_audio_cvt_search_seg(pcm_val, seg_uend, 8);

	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8)       /* out of range, return maximum value. */
		return (unsigned char) (0x7F ^ mask);
	else {
		uval = (unsigned char) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
		return (uval ^ mask);
	}
}

short nf_audio_cvt_search_seg(short val, short *table, short size)
{
	short i=0;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

short nf_audio_cvt_muraw_to_lpcm16(guchar u_val)
{
	short t=0;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

void nf_audio_convert(guchar *stream_src, gchar *stream_dest, guint len)
{
	gint i=0, j=0;
	gshort pcm_val=0;

	for(i=0, j=0; i<(gint)len; i++, j+=2)
	{
		memcpy(&pcm_val, stream_src+j, sizeof(gshort));
		*stream_dest=(gchar)nf_audio_cvt_lpcm16_to_muraw(pcm_val);

		*stream_dest++;
	}
}

#if 0
#include "jbshell.h"
static char nf_hi_aud_jbshell_cmd_help[] = "hi_aud ai [stop or start]\n";
static int nf_hi_aud_jbshell_cmd(int argc, char **argv)
{
	gint is_hdmi=0;

	if(argc < 4)
		goto nf_hi_aud_help_cmd;

	if(strcmp(argv[1], "ai") == 0)
	{
		is_hdmi=(gint)strtoul(argv[3], NULL, 10);

		if(strcmp(argv[2], "start") == 0)
			nf_audio_reset(is_hdmi, FALSE, TRUE);
		else if(strcmp(argv[2], "stop") == 0)
			nf_audio_reset(is_hdmi, FALSE, FALSE);
	}
	else if(strcmp(argv[1], "ao") == 0)
	{
		is_hdmi=(gint)strtoul(argv[3], NULL, 10);

		if(strcmp(argv[2] , "start") == 0)
			nf_audio_reset(is_hdmi, TRUE, TRUE);
		else if(strcmp(argv[2] , "stop") == 0)
			nf_audio_reset(is_hdmi, TRUE, FALSE);
	}
	else if(strcmp(argv[1], "enable_rd") == 0)
	{
		gboolean val;
		val=(gboolean)strtoul(argv[2], NULL, 10);

		nf_audio_setRdEnable(val);
	}
	else if(strcmp(argv[1], "live") == 0)
	{
		guint ch=0;

		if(argc < 3)
			goto nf_hi_aud_help_cmd;

		ch=(guint)strtoul(argv[2], NULL, 10);
		nf_audio_set_live_audio_ch(ch);
	}
	else if(strcmp(argv[1], "vol_adc") == 0)
	{
		guint is_mute=0, volume=0 ;

		is_mute=(guint)strtoul(argv[2], NULL, 10);
		volume=(guint)strtoul(argv[3], NULL, 10);

		g_message("%s line%d is_mute[%d] volume[%d]", __FUNCTION__, __LINE__, is_mute, volume);

		nf_HI_ctrl_vol_adc(is_mute, volume);
	}
	else if(strcmp(argv[1], "vol_dac") == 0)
	{
		guint is_mute=0, volume=0 ;

		is_mute=(guint)strtoul(argv[2], NULL, 10);
		volume=(guint)strtoul(argv[3], NULL, 10);

		g_message("%s line%d is_mute[%d] volume[%d]", __FUNCTION__, __LINE__, is_mute, volume);

		nf_HI_ctrl_vol_dac(is_mute, volume);
	}

	return 0;

nf_hi_aud_help_cmd:
	printf("Invalid arguments\n%s\n", nf_hi_aud_jbshell_cmd_help);
	
	return -1;
}
__commandlist(nf_hi_aud_jbshell_cmd, "hi_aud", nf_hi_aud_jbshell_cmd_help, nf_hi_aud_jbshell_cmd_help);
#endif

