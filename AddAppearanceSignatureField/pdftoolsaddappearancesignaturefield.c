/****************************************************************************
 *
 * File:            pdftoolsaddappearancesignaturefield.c
 *
 * Usage:           pdftoolsaddappearancesignaturefield <certificateFile> <password> <appConfigFile> <inputPath> <outputPath>
 *                  
 * Title:           Sign a PDF and apply a visual signature appearance
 *                  
 * Description:     Sign a PDF document using a provided certificate and
 *                  apply a visual signature appearance. This process
 *                  requires an input PDF that already contains a signature
 *                  field. The provided certificate is used to sign the
 *                  document and attach the signature to the existing field.
 *                  The visual appearance of the signature is updated using
 *                  an XML or JSON file, allowing the addition of text,
 *                  images, or PDFs. This signature consists of both a
 *                  visible and a non-visible part. Only the non-visible part
 *                  is used by other applications to verify the integrity of
 *                  the signed part of the document and validate the signing
 *                  certificate. The signing certificate is retrieved from a
 *                  password-protected PKCS#12 file (.pfx or .p12).
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
    printf("Usage: pdftoolsaddappearancesignaturefield <certificateFile> <password> <appConfigFile> <inputPath> <outputPath>.\n");
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
    if (argc < 6 || argc > 6)
    {
        return Usage();
    }

    TCHAR*                                                  szCertificateFile       = argv[1];
    TCHAR*                                                  szPassword              = argv[2];
    TCHAR*                                                  szAppConfigFile         = argv[3];
    TCHAR*                                                  szInPath                = argv[4];
    TCHAR*                                                  szOutPath               = argv[5];
    FILE*                                                   pCertificateFileStream  = NULL;
    FILE*                                                   pInStream               = NULL;
    FILE*                                                   pOutStream              = NULL;
    FILE*                                                   pAppStream              = NULL;
    TPdfToolsPdf_Document*                                  pInDoc                  = NULL;
    TPdfToolsCryptoProvidersBuiltIn_Provider*               pSession                = NULL;
    TPdfToolsCryptoProvidersBuiltIn_SignatureConfiguration* pSignatureConfiguration = NULL;
    TPdfToolsSign_Appearance*                               pAppearance             = NULL;
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

    // Create signature configuration from PFX (or P12) file
    pCertificateFileStream = _tfopen(szCertificateFile, _T("rb"));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(
        pCertificateFileStream, _T("Failed to open the certificate file \"%s\" for reading.\n"), szCertificateFile);
    TPdfToolsSys_StreamDescriptor certificateFileDesc;
    PdfToolsSysCreateFILEStreamDescriptor(&certificateFileDesc, pCertificateFileStream, 0);
    pSignatureConfiguration = PdfToolsCryptoProvidersBuiltIn_Provider_CreateSignatureFromCertificate(
        pSession, &certificateFileDesc, szPassword);

    // Open input document
    pInStream = _tfopen(szInPath, _T("rb"));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pInStream, _T("Failed to open the input file \"%s\" for reading.\n"), szInPath);
    TPdfToolsSys_StreamDescriptor inDesc;
    PdfToolsSysCreateFILEStreamDescriptor(&inDesc, pInStream, 0);
    pInDoc = PdfToolsPdf_Document_Open(&inDesc, _T(""));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(
        pInDoc, _T("Failed to create a document from the input file \"%s\". %s (ErrorCode: 0x%08x).\n"), szInPath,
        szErrorBuff, PdfTools_GetLastError());

    // Choose first signature field
    TPdfToolsPdf_SignatureFieldList* pFieldList  = PdfToolsPdf_Document_GetSignatureFields(pInDoc);
    int                              nFieldCount = PdfToolsPdf_SignatureFieldList_GetCount(pFieldList);
    for (int i = 0; i < nFieldCount; i++)
    {
        TPdfToolsPdf_SignatureField* pField = PdfToolsPdf_SignatureFieldList_Get(pFieldList, i);
        if (pField != NULL)
        {
            TCHAR szFieldName[256];
            PdfToolsPdf_SignatureField_GetFieldName(pField, szFieldName, ARRAY_SIZE(szFieldName));
            PdfToolsSign_SignatureConfiguration_SetFieldName(
                (TPdfToolsSign_SignatureConfiguration*)pSignatureConfiguration, szFieldName);
            break;
        }
    }

    // Create appearance from either an XML or a JSON file
    pAppStream = _tfopen(szAppConfigFile, _T("rb"));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pAppStream, _T("Failed to open the appearance config file \"%s\" for reading.\n"),
                                     szAppConfigFile);
    TPdfToolsSys_StreamDescriptor appDesc;
    PdfToolsSysCreateFILEStreamDescriptor(&appDesc, pAppStream, 0);

    const TCHAR* szExt = _tcsrchr(szAppConfigFile, '.');
    if (szExt != NULL && _tcsicmp(szExt, _T(".xml")) == 0)
        pAppearance = PdfToolsSign_Appearance_CreateFromXml(&appDesc);
    else
        pAppearance = PdfToolsSign_Appearance_CreateFromJson(&appDesc);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pAppearance, _T("Failed to create appearance. %s (ErrorCode: 0x%08x).\n"),
                                     szErrorBuff, PdfTools_GetLastError());

    // Add custom text variable
    PdfToolsSign_CustomTextVariableMap_Set(PdfToolsSign_Appearance_GetCustomTextVariables(pAppearance), _T("company"),
                                           _T("Daily Planet"));

    // Set appearance on signature configuration
    PdfToolsSign_SignatureConfiguration_SetAppearance((TPdfToolsSign_SignatureConfiguration*)pSignatureConfiguration,
                                                      pAppearance);

    // Create output stream for writing
    pOutStream = _tfopen(szOutPath, _T("wb+"));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pOutStream, _T("Failed to open the output file \"%s\" for writing.\n"), szOutPath);
    TPdfToolsSys_StreamDescriptor outDesc;
    PdfToolsSysCreateFILEStreamDescriptor(&outDesc, pOutStream, 0);

    // Sign the input document
    pSigner = PdfToolsSign_Signer_New();
    pOutDoc = PdfToolsSign_Signer_Sign(pSigner, pInDoc, (TPdfToolsSign_SignatureConfiguration*)pSignatureConfiguration,
                                       &outDesc, NULL);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pOutDoc, _T("The processing has failed. (ErrorCode: 0x%08x).\n"),
                                     PdfTools_GetLastError());


    _tprintf(_T("Execution successful.\n"));

cleanup:
    if (pOutDoc)
        PdfToolsPdf_Document_Close(pOutDoc);
    PdfTools_Release(pSigner);
    PdfTools_Release(pAppearance);
    PdfTools_Release(pSignatureConfiguration);
    PdfTools_Release(pSession);
    if (pOutStream)
        fclose(pOutStream);
    if (pAppStream)
        fclose(pAppStream);
    if (pInDoc)
        PdfToolsPdf_Document_Close(pInDoc);
    if (pInStream)
        fclose(pInStream);
    if (pCertificateFileStream)
        fclose(pCertificateFileStream);

    PdfTools_Uninitialize();

    return iRet;
} 