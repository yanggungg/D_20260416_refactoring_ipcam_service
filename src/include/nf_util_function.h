#ifndef __NF_UTIL_FUNCTION_H__
#define __NF_UTIL_FUNCTION_H__



int base64_encode_len(int numBytes);
int base64_encode(char *text, int numBytes, char *encodedText);
int base64_decode(char *text, int numBytes, char *decodedText);

#endif
