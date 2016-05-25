#ifndef __NETCONFIG_H_
#define __NETCONFIG_H_

#include <string>
#include <iostream>
#include <sstream>

using namespace std;

typedef int                 INT;
typedef long long  QWORD;
typedef unsigned char       BYTE;

// 数据大小端转换
#define __SWAP64(A) ( ( ( ( QWORD )(A) & 0xff00000000000000 ) >> 56 ) | \
	(((QWORD)(A)& 0x00ff000000000000) >> 40) | \
	(((QWORD)(A)& 0x0000ff0000000000) >> 24) | \
	(((QWORD)(A)& 0x000000ff00000000) >> 8) | \
	(((QWORD)(A)& 0x00000000ff000000) << 8) | \
	(((QWORD)(A)& 0x0000000000ff0000) << 24) | \
	(((QWORD)(A)& 0x000000000000ff00) << 40) | \
	(((QWORD)(A)& 0x00000000000000ff) << 56))

#define __SWAP32(A) ( ( ( A & 0xff000000 ) >> 24 ) | \
	((A & 0x00ff0000) >> 8) | \
	((A & 0x0000ff00) << 8) | \
	((A & 0x000000ff) << 24))

#define __SWAP16(A) ( ( ( A & 0xff00 ) >> 8 ) | \
	((A & 0x00ff) << 8))

#endif

