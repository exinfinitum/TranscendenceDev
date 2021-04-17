//	Utilities.cpp
//
//	Miscellaneous utility functions
//	Copyright (c) 2019 Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"
#include <cstring>

DWORD Kernel::sysGetTicksElapsed (DWORD dwTick, DWORD *retdwNow)

//	sysGetTicksElapsed
//
//	Returns the number of milliseconds since the given tick count

	{
	DWORD dwNow = ::GetTickCount();
	if (retdwNow)
		*retdwNow = dwNow;

	if (dwNow < dwTick)
		return (0xffffffff - dwTick) + dwNow + 1;
	else
		return dwNow - dwTick;
	}

int Kernel::sysGetProcessorCount (void)

//	sysGetProcessorCount
//
//	Returns the number of processors on the machine

	{
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
	}

CString Kernel::sysGetUserName (void)

//	sysGetUserName
//
//	Returns the name of the currently logged-on user

	{
	CString sName;
	DWORD dwLen = 256;
	char *pPos = sName.GetWritePointer(dwLen);

	::GetUserName(pPos, &dwLen);
	sName.Truncate(lstrlen(pPos));

	return sName;
	}

bool Kernel::sysIsBigEndian (void)

//	sysIsBigEndian
//
//	Returns TRUE if we're running on a big-endian architecture

	{
	return ((*(unsigned short *) ("#S") >> 8) == '#');
	}

bool Kernel::sysOpenURL (const CString &sURL)

//	sysOpenURL
//
//	Launches the default browser to the given URL.

	{
	HINSTANCE hResult = ::ShellExecute(NULL, "open", sURL.GetASCIIZPointer(), NULL, NULL, SW_SHOWNORMAL);
	if ((DWORD)hResult <= 32)
		{
		//	Errors are <= 32.
		return false;
		}

	//	Done

	return true;
	}

#define mix(a,b,c) \
	{ \
	a -= b; a -= c; a ^= (c>>13); \
	b -= c; b -= a; b ^= (a<<8); \
	c -= a; c -= b; c ^= (b>>13); \
	a -= b; a -= c; a ^= (c>>12); \
	b -= c; b -= a; b ^= (a<<16); \
	c -= a; c -= b; c ^= (b>>5); \
	a -= b; a -= c; a ^= (c>>3); \
	b -= c; b -= a; b ^= (a<<10); \
	c -= a; c -= b; c ^= (b>>15); \
	}

DWORD Kernel::utlHashFunctionCase (BYTE *pKey, int iKeyLen)

//	utlHashFunction
//
//	Hash the data
//
//	Source: Bob Jenkins
//	http://burtleburtle.net/bob/hash/evahash.html

	{
	DWORD initval = 1013;
	BYTE *k = pKey;
	DWORD a,b,c,len;

	//	Set up the internal state
	len = iKeyLen;
	a = b = 0x9e3779b9;		//	the golden ratio; an arbitrary value
	c = initval;			//	the previous hash value (arbitrary)

	//	---------------------------------------- handle most of the key

	while (len >= 12)
		{
		a += (k[0] +((DWORD)k[1]<<8) +((DWORD)k[2]<<16) +((DWORD)k[3]<<24));
		b += (k[4] +((DWORD)k[5]<<8) +((DWORD)k[6]<<16) +((DWORD)k[7]<<24));
		c += (k[8] +((DWORD)k[9]<<8) +((DWORD)k[10]<<16)+((DWORD)k[11]<<24));
		mix(a,b,c);
		k += 12; len -= 12;
		}

	//	------------------------------------- handle the last 11 bytes
	c += iKeyLen;
	switch(len)				//	all the case statements fall through
		{
		case 11: c+=((DWORD)k[10]<<24); [[fallthrough]];
		case 10: c+=((DWORD)k[9]<<16); [[fallthrough]];
		case 9 : c+=((DWORD)k[8]<<8); [[fallthrough]];
		//	the first byte of c is reserved for the length */
		case 8 : b+=((DWORD)k[7]<<24); [[fallthrough]];
		case 7 : b+=((DWORD)k[6]<<16); [[fallthrough]];
		case 6 : b+=((DWORD)k[5]<<8); [[fallthrough]];
		case 5 : b+=k[4]; [[fallthrough]];
		case 4 : a+=((DWORD)k[3]<<24); [[fallthrough]];
		case 3 : a+=((DWORD)k[2]<<16); [[fallthrough]];
		case 2 : a+=((DWORD)k[1]<<8); [[fallthrough]];
		case 1 : a+=k[0];
		//	case 0: nothing left to add */
		}

	mix(a,b,c);

	//	-------------------------------------------- report the result
	return c;
	}

#undef mix

void Kernel::utlMemSet (LPVOID pDest, DWORD Count, BYTE Value)

//	utlMemSet
//
//	Initializes a block of memory to the given value.
//	
//	Inputs:
//		pDest: Pointer to block of memory to initialize
//		Count: Length of block in bytes
//		Value: Value to initialize to

	{
	std::memset(pDest, Value, Count);
	}

void Kernel::utlMemCopy (const char *pSource, char *pDest, DWORD dwCount)

//	utlMemCopy
//
//	Copies a block of memory of Count bytes from pSource to pDest.
//	
//	Inputs:
//		pSource: Pointer to source memory block
//		pDest: Pointer to destination memory block
//		dwCount: Number of bytes to copy

	{
	std::memcpy(pDest, pSource, dwCount);
	}

BOOL Kernel::utlMemCompare (char *pSource, char *pDest, DWORD dwCount)

//	utlMemCompare
//
//	Compare two blocks of memory for equality

	{
	for (; dwCount > 0; dwCount--)
		if (*pDest++ != *pSource++)
			return FALSE;

	return TRUE;
	}
