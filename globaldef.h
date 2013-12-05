/* 
 * File:   globaldef.h
 * Author: od
 *
 * Created on June 14, 2013, 11:05 AM
 */

#ifndef GLOBALDEF_H
#define	GLOBALDEF_H

#define TRUE 1
#define FALSE 0

typedef unsigned char byte;
typedef unsigned short word;

#ifdef __GNUC__
typedef char int8;
typedef short int16;
typedef int int32;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#else // Assume this is XC8
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

typedef char int8;
typedef short int16;
typedef long int32;
#endif

#ifdef __GNUC__
#define PACKED_STRUCT __attribute__((packed))
#else
#define PACKED_STRUCT
#endif
#endif	/* GLOBALDEF_H */

