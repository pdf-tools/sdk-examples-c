/****************************************************************************
 *
 * File:            pdftoolsocrdocument.c
 *
 * Usage:           pdftoolsocrdocument <ocrEngineName> <language> <inputPath> <outputPath>
 *                  
 * Title:           OCR a PDF document
 *                  
 * Description:     Apply OCR to a PDF document to make scanned content
 *                  searchable. Text is recognized from images, existing text
 *                  is updated with correct Unicode, and tagging is added for
 *                  accessibility.
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
    printf("Usage: pdftoolsocrdocument <ocrEngineName> <language> <inputPath> <outputPath>.\n");
    printf("return codes:\n");
    printf("    0: Successful completion\n");
    printf("    1: Execution failed\n");

    return 1;
}

void PDFTOOLS_CALL WarningHandler(void* pContext, const TCHAR* szMessage, TPdfToolsOcr_WarningCategory iCategory,
                                  int iPageNo, const TCHAR* szContext)
{
    if (iPageNo > 0)
        _tprintf(_T("- %d: %s (%s page %d)\n"), iCategory, szMessage, szContext, iPageNo);
    else
        _tprintf(_T("- %d: %s (%s)\n"), iCategory, szMessage, szContext);
}

int _tmain(int argc, TCHAR* argv[])
{
    int    iRet = 0;
    size_t nBufSize;
    TCHAR  szErrorBuff[1024];

    setlocale(LC_CTYPE, "");


    // Check command line parameters
    if (argc < 5 || argc > 5)
    {
        return Usage();
    }

    TCHAR*                     szOcrEngineName = argv[1];
    TCHAR*                     szLanguage      = argv[2];
    TCHAR*                     szInPath        = argv[3];
    TCHAR*                     szOutPath       = argv[4];
    FILE*                      pInStream       = NULL;
    FILE*                      pOutStream      = NULL;
    TPdfToolsPdf_Document*     pInDoc          = NULL;
    TPdfToolsPdf_Document*     pOutDoc         = NULL;
    TPdfToolsOcr_Engine*       pEngine         = NULL;
    TPdfToolsOcr_OcrOptions*   pOptions        = NULL;
    TPdfToolsOcr_ImageOptions* pImageOptions   = NULL;
    TPdfToolsOcr_TextOptions*  pTextOptions    = NULL;
    TPdfToolsOcr_PageOptions*  pPageOptions    = NULL;
    TPdfToolsOcr_Processor*    pProcessor      = NULL;

    // Initialize library
    PdfTools_Initialize();

    // By default, a test license key is active. In this case, a watermark is added to the output. 
    // If you have a license key, please uncomment the following call and set the license key.
    // GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(PdfTools_Sdk_Initialize(_T("<-- insert license key -->"), NULL),
    //                                     _T("Failed to set the license key. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
    //                                     PdfTools_GetLastError());

    // Create the OCR engine
    pEngine = PdfToolsOcr_Engine_Create(szOcrEngineName);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pEngine, _T("Failed to create OCR engine. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
                                     PdfTools_GetLastError());

    // Set the language(s) for OCR recognition (e.g. "German,English")
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(PdfToolsOcr_Engine_SetLanguages(pEngine, szLanguage),
                                      _T("Failed to set OCR languages. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
                                      PdfTools_GetLastError());

    // Open input document
    pInStream = _tfopen(szInPath, _T("rb"));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pInStream, _T("Failed to open the input file \"%s\" for reading.\n"), szInPath);
    TPdfToolsSys_StreamDescriptor inDesc;
    PdfToolsSysCreateFILEStreamDescriptor(&inDesc, pInStream, 0);
    pInDoc = PdfToolsPdf_Document_Open(&inDesc, _T(""));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(
        pInDoc, _T("Failed to create a document from the input file \"%s\". %s (ErrorCode: 0x%08x).\n"), szInPath,
        szErrorBuff, PdfTools_GetLastError());

    // Configure OCR options
    pOptions = PdfToolsOcr_OcrOptions_New();
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pOptions, _T("Failed to create OCR options. %s (ErrorCode: 0x%08x).\n"),
                                     szErrorBuff, PdfTools_GetLastError());

    // Configure image OCR: recognize text from scanned images
    pImageOptions = PdfToolsOcr_OcrOptions_GetImageOptions(pOptions);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pImageOptions, _T("Failed to get image options. %s (ErrorCode: 0x%08x).\n"),
                                     szErrorBuff, PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(
        PdfToolsOcr_ImageOptions_SetMode(pImageOptions, ePdfToolsOcr_ImageProcessingMode_UpdateText),
        _T("Failed to set image processing mode. %s (ErrorCode: 0x%08x).\n"), szErrorBuff, PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(PdfToolsOcr_ImageOptions_SetRemoveOnlyInvisibleOcrText(pImageOptions, TRUE),
                                      _T("Failed to set RemoveOnlyInvisibleOcrText. %s (ErrorCode: 0x%08x).\n"),
                                      szErrorBuff, PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(PdfToolsOcr_ImageOptions_SetDeskewScan(pImageOptions, TRUE),
                                      _T("Failed to set DeskewScan. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
                                      PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(PdfToolsOcr_ImageOptions_SetRotateScan(pImageOptions, TRUE),
                                      _T("Failed to set RotateScan. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
                                      PdfTools_GetLastError());

    // Configure text OCR: update non-extractable text with correct Unicode
    pTextOptions = PdfToolsOcr_OcrOptions_GetTextOptions(pOptions);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pTextOptions, _T("Failed to get text options. %s (ErrorCode: 0x%08x).\n"),
                                     szErrorBuff, PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(
        PdfToolsOcr_TextOptions_SetMode(pTextOptions, ePdfToolsOcr_TextProcessingMode_Update),
        _T("Failed to set text processing mode. %s (ErrorCode: 0x%08x).\n"), szErrorBuff, PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(
        PdfToolsOcr_TextOptions_SetSkipMode(pTextOptions, ePdfToolsOcr_TextSkipMode_KnownSymbolic),
        _T("Failed to set text skip mode. %s (ErrorCode: 0x%08x).\n"), szErrorBuff, PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(
        PdfToolsOcr_TextOptions_SetUnicodeSource(pTextOptions, ePdfToolsOcr_UnicodeSource_InstalledFont),
        _T("Failed to set unicode source. %s (ErrorCode: 0x%08x).\n"), szErrorBuff, PdfTools_GetLastError());

    // Configure page OCR: process all pages and add tagging for accessibility
    pPageOptions = PdfToolsOcr_OcrOptions_GetPageOptions(pOptions);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pPageOptions, _T("Failed to get page options. %s (ErrorCode: 0x%08x).\n"),
                                     szErrorBuff, PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(
        PdfToolsOcr_PageOptions_SetMode(pPageOptions, ePdfToolsOcr_PageProcessingMode_All),
        _T("Failed to set page processing mode. %s (ErrorCode: 0x%08x).\n"), szErrorBuff, PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(PdfToolsOcr_PageOptions_SetTagging(pPageOptions, ePdfToolsOcr_TaggingMode_Auto),
                                      _T("Failed to set tagging mode. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
                                      PdfTools_GetLastError());

    // Create the OCR processor and add a warning handler
    pProcessor = PdfToolsOcr_Processor_New();
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pProcessor, _T("Failed to create OCR processor. %s (ErrorCode: 0x%08x).\n"),
                                     szErrorBuff, PdfTools_GetLastError());
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(PdfToolsOcr_Processor_AddWarningHandler(pProcessor, NULL, WarningHandler),
                                      _T("Failed to add warning handler. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
                                      PdfTools_GetLastError());

    // Create stream for output file
    pOutStream = _tfopen(szOutPath, _T("wb+"));
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pOutStream, _T("Failed to create output file \"%s\" for writing.\n"), szOutPath);
    TPdfToolsSys_StreamDescriptor outDesc;
    PdfToolsSysCreateFILEStreamDescriptor(&outDesc, pOutStream, 0);

    // Process the document with OCR
    pOutDoc = PdfToolsOcr_Processor_Process(pProcessor, pInDoc, pEngine, &outDesc, pOptions, NULL);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(pOutDoc, _T("The processing has failed. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
                                     PdfTools_GetLastError());


    _tprintf(_T("Execution successful.\n"));

cleanup:
    if (pOutDoc)
        PdfToolsPdf_Document_Close(pOutDoc);
    PdfTools_Release(pProcessor);
    PdfTools_Release(pOptions);
    if (pEngine)
        PdfToolsOcr_Engine_Close(pEngine);
    if (pOutStream)
        fclose(pOutStream);
    if (pInDoc)
        PdfToolsPdf_Document_Close(pInDoc);
    if (pInStream)
        fclose(pInStream);

    PdfTools_Uninitialize();

    return iRet;
}
