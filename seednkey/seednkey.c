#include <stdio.h>
#include <stdint.h>
#include "seednkey.h"

KEYGENALGO_API VKeyGenResultExOpt GenerateKeyEx(
    const unsigned char* ipSeedArray,            // Array for the seed [in]
    unsigned int         iSeedArraySize,         // Length of the array for the seed [in]
    const unsigned int   iSecurityLevel,         // Security level [in]
    const char*          ipVariant,              // Name of the active variant [in]
    unsigned char*       iopKeyArray,            // Array for the key [in, out]
    unsigned int         iMaxKeyArraySize,       // Maximum length of the array for the key [in]
    unsigned int*        oActualKeyArraySize)    // Length of the key [out]
{
    const char c = 0;
    return GenerateKeyExOpt(
        ipSeedArray,
        iSeedArraySize,
        iSecurityLevel,
        ipVariant,
        &c,
        iopKeyArray,
        iMaxKeyArraySize,
        oActualKeyArraySize
    );
}

KEYGENALGO_API VKeyGenResultExOpt GenerateKeyExOpt(
    const unsigned char* ipSeedArray,            // Array for the seed [in]
    unsigned int         iSeedArraySize,         // Length of the array for the seed [in]
    const unsigned int   iSecurityLevel,         // Security level [in]
    const char*          ipVariant,              // Name of the active variant [in]
    const char*          ipOptions,              // Optional parameter which might be used for OEM specific information [in]
    unsigned char*       iopKeyArray,            // Array for the key [in, out]
    unsigned int         iMaxKeyArraySize,       // Maximum length of the array for the key [in]
    unsigned int*        oActualKeyArraySize     // Length of the key [out]
) {
    for(int i = 0; i < iSeedArraySize; i++) {
        int j = (iSeedArraySize - 1) - i;
        iopKeyArray[j] = ipSeedArray[i] + iSecurityLevel;
    }
    *oActualKeyArraySize = iSeedArraySize;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}