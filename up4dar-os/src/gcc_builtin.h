/*
 * gcc_builtin.h
 *
 * Created: 05.02.2012 15:11:11
 *  Author: mdirska
 */ 


#ifndef GCC_BUILTIN_H_
#define GCC_BUILTIN_H_


extern void * memcpy(void *, const void *, size_t );
extern int memcmp(const void *, const void *, size_t );
extern void * memset(void *, int, size_t );

extern void * strncpy(char *, const char *, size_t );

extern int strlen ( const char * );
extern const char * strstr (const char *, const char * );

#endif /* GCC_BUILTIN_H_ */