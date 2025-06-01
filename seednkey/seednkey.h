#ifndef SEED_N_KEY_H
#define SEED_N_KEY_H

#include <windows.h>

typedef enum
{
	KGREO_Ok = 0,
	KGREO_BufferToSmall = 1,
	KGREO_SecurityLevelInvalid = 2,
	KGREO_VariantInvalid = 3,
	KGREO_UnspecifiedError = 4
} VKeyGenResultExOpt;

#define KEYGENALGO_API __declspec(dllexport)

KEYGENALGO_API VKeyGenResultExOpt GenerateKeyExOpt(
	const unsigned char* ipSeedArray,
	unsigned int iSeedArraySize,
	const unsigned int iSecurityLevel,
	const char* ipVariant,
	const char* ipOptions,
	unsigned char* iopKeyArray,
	unsigned int iMaxKeyArraySize,
	unsigned int* oActualKeyArraySize
);


#endif // SEED_N_KEY_H