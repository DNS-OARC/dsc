#ifndef __dsc_base64_h
#define __dsc_base64_h

int base64_encode(const void *data, int size, char **str);
int base64_decode(const char *str, void *data);

#endif /* __dsc_base64_h */
