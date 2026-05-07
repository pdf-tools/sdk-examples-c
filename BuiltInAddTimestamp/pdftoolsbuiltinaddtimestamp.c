/****************************************************************************
 *
 * File:            pdftoolsbuiltinaddtimestamp.c
 *
 * Usage:           pdftoolsbuiltinaddtimestamp <timeStampUrl> <inputPath> <outputPath>
 *                  
 * Title:           Add a document time-stamp to a PDF
 *                  
 * Description:     Add a trusted document time-stamp to a PDF
 *                  and confirm that the signed document has not been
 *                  altered. This type of signature proves that
 *                  the document existed at a specific time and ensures its
 *                  integrity.
 *                  
 * Author:          PDF Tools AG
 *
 * Copyright:       Copyright (C) 2026 PDF Tools AG, Switzerland
 *                  Permission to use, copy, modify, and distribute this
 *                  software and its documentation for any purpose and without
 *                  fee is hereby granted, provided that the above copyright
 *                  notice appear in all copies and that both that copyright
 *                  notice and this permission notice appear in supporting
 *                  documentation. This software is provided "as is" without
 *                  express or implied warranty.
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PdfTools.h"

#include <locale.h>
#include "compat.h"


#define MIN(a, b)     (((a) < (b) ? (a) : (b)))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a)[0])

#define GOTO_CLEANUP_IF_NULL_PRINT_ERROR(inVar, ...)                                           \
    do                                                                                         \
    {                                                                                          \
        if ((inVar) == NULL)                                                                   \
        {                                                                                      \
            nBufSize = PdfTools_GetLastErrorMessage(NULL, 0);                                  \
            PdfTools_GetLastErrorMessage(szErrorBuff, MIN(ARRAY_SIZE(szErrorBuff), nBufSize)); \
            _tprintf(__VA_ARGS__);                                                             \
            iRet = 1;                                                                          \
            goto cleanup;                                                                      \
        }                                                                                      \
    } while (0);

#define GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(outBool, ...)                                        \
    do                                                                                         \
    {                                                                                          \
        if ((outBool) == FALSE)                                                                \
        {                                                                                      \
            nBufSize = PdfTools_GetLastErrorMessage(NULL, 0);                                  \
            PdfTools_GetLastErrorMessage(szErrorBuff, MIN(ARRAY_SIZE(szErrorBuff), nBufSize)); \
            _tprintf(__VA_ARGS__);                                                             \
            iRet = 1;                                                                          \
            goto cleanup;                                                                      \
        }                                                                                      \
    } while (0);

int Usage()
{
    printf("Usage: pdftoolsbuiltinaddtimestamp <timeStampUrl> <inputPath> <outputPath>.\n");
    printf("return codes:\n");
    printf("    0: Successful completion\n");
    printf("    1: Execution failed\n");

    return 1;
}

int _tmain(int argc, TCHAR* argv[])
{
    int    iRet = 0;
    size_t nBufSize;
    TCHAR  szErrorBuff[1024];

    setlocale(LC_CTYPE, "");


    // Check command line parameters
    if (argc < 4 || argc > 4)
    {
        return Usage();
    }

    TCHAR*                                                  szTimestampUrl          = argv[1];
    TCHAR*                                                  szInPath                = argv[2];
    TCHAR*                                                  szOutPath               = argv[3];
    FILE*                                                   pInStream               = NULL;
    FILE*                                                   pOutStream              = NULL;
    TPdfToolsPdf_Document*                                  pInDoc                  = NULL;
    TPdfToolsCryptoProvidersBuiltIn_Provider*               pSession                = NULL;
    TPdfToolsCryptoProvidersBuiltIn_TimestampConfiguration* pTimestampConfiguration = NULL;
    TPdfToolsSign_Signer*                                   pSigner                 = NULL;
    TPdfToolsPdf_Document*                                  pOutDoc                 = NULL;

    // Initialize library
    PdfTools_Initialize();

    // By default, a test license key is active. In this case, a watermark is added to the output. 
    // If you have a license key, please uncomment the following call and set the license key.
    // GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(PdfTools_Sdk_Initialize(_T("<-- insert license key -->"), NULL),
    //                                     _T("Failed to set the license key. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
    //                                     PdfTools_GetLastError());

    // Create a session to the built-in cryptographic provider
    pSession = PdfToolsCryptoProvidersBuiltIn_Provider_New();

    // Set the time-stamp URL
    PdfToolsCryptoProvidersBuiltIn_Provider_SetTimestampUrl(pSession, szTimestampUrl);

    // Create time-stamp configuration
    pTimestampConfiguration = PdfToolsCryptoProvidersBuiltIn_Provider_CreateTimestamp(pSession);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pTimestampConfiguration,
                                     _T("Failed to create time-stamp configuration. %s (ErrorCode: 0x%08x).\n"),
                                     szErrorBuff, PdfTools_GetLastError());

    // Open input document
    pInStream = _tfopen(szInPath, _T("rb"));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pInStream, _T("Failed to open the input file \"%s\" for reading.\n"), szInPath);
    TPdfToolsSys_StreamDescriptor inDesc;
    PdfToolsSysCreateFILEStreamDescriptor(&inDesc, pInStream, 0);
    pInDoc = PdfToolsPdf_Document_Open(&inDesc, _T(""));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(
        pInDoc, _T("Failed to create a document from the input file \"%s\". %s (ErrorCode: 0x%08x).\n"), szInPath,
        szErrorBuff, PdfTools_GetLastError());

    // Create output stream for writing
    pOutStream = _tfopen(szOutPath, _T("wb+"));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pOutStream, _T("Failed to open the output file \"%s\" for writing.\n"), szOutPath);
    TPdfToolsSys_StreamDescriptor outDesc;
    PdfToolsSysCreateFILEStreamDescriptor(&outDesc, pOutStream, 0);

    // Add the document time-stamp
    pSigner = PdfToolsSign_Signer_New();
    pOutDoc = PdfToolsSign_Signer_AddTimestamp(
        pSigner, pInDoc, (TPdfToolsSign_TimestampConfiguration*)pTimestampConfiguration, &outDesc, NULL);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pOutDoc, _T("The processing has failed. (ErrorCode: 0x%08x).\n"),
                                     PdfTools_GetLastError());


    _tprintf(_T("Execution successful.\n"));

cleanup:
    if (pOutDoc)
        PdfToolsPdf_Document_Close(pOutDoc);
    PdfTools_Release(pSigner);
    PdfTools_Release(pTimestampConfiguration);
    PdfTools_Release(pSession);
    if (pOutStream)
        fclose(pOutStream);
    if (pInDoc)
        PdfToolsPdf_Document_Close(pInDoc);
    if (pInStream)
        fclose(pInStream);

    PdfTools_Uninitialize();

    return iRet;
} 