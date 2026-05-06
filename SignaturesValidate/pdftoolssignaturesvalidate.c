/****************************************************************************
 *
 * File:            pdftoolssignaturesvalidate.c
 *
 * Usage:           pdftoolssignaturesvalidate <inputPath> [<certificateDirectory>]
 *                  
 * Title:           Validate the signatures contained in an input document
 *                  
 * Description:     Extract and validate signature information for all
 *                  digital signatures in the input document, then print the
 *                  results to the console.
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

#if defined(WIN32)
#include <windows.h>
#else
#include <dirent.h>
#endif

#include "PdfTools.h"

#include <locale.h>
#if !defined(WIN32)
#define TCHAR char
#define _tcslen strlen
#define _tcscat strcat
#define _tcscpy strcpy
#define _tcsrchr strrchr
#define _tcstok strtok
#define _tcslen strlen
#define _tcscmp strcmp
#define _tcsftime strftime
#define _tcsncpy strncpy
#define _tmain main
#define _tfopen fopen
#define _ftprintf fprintf
#define _stprintf sprintf
#define _tstof atof
#define _tremove remove
#define _tprintf printf
#define _T(str) str
#endif


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

// Convert SubIndication enum to string
const TCHAR* SubIndicationToString(TPdfToolsSignatureValidation_SubIndication iSubIndication)
{
    switch (iSubIndication)
    {
    case ePdfToolsSignatureValidation_SubIndication_Revoked:
        return _T("Revoked");
    case ePdfToolsSignatureValidation_SubIndication_HashFailure:
        return _T("HashFailure");
    case ePdfToolsSignatureValidation_SubIndication_SigCryptoFailure:
        return _T("SigCryptoFailure");
    case ePdfToolsSignatureValidation_SubIndication_SigConstraintsFailure:
        return _T("SigConstraintsFailure");
    case ePdfToolsSignatureValidation_SubIndication_ChainConstraintsFailure:
        return _T("ChainConstraintsFailure");
    case ePdfToolsSignatureValidation_SubIndication_CryptoConstraintsFailure:
        return _T("CryptoConstraintsFailure");
    case ePdfToolsSignatureValidation_SubIndication_Expired:
        return _T("Expired");
    case ePdfToolsSignatureValidation_SubIndication_NotYetValid:
        return _T("NotYetValid");
    case ePdfToolsSignatureValidation_SubIndication_FormatFailure:
        return _T("FormatFailure");
    case ePdfToolsSignatureValidation_SubIndication_PolicyProcessingError:
        return _T("PolicyProcessingError");
    case ePdfToolsSignatureValidation_SubIndication_UnknownCommitmentType:
        return _T("UnknownCommitmentType");
    case ePdfToolsSignatureValidation_SubIndication_TimestampOrderFailure:
        return _T("TimestampOrderFailure");
    case ePdfToolsSignatureValidation_SubIndication_NoSignerCertificateFound:
        return _T("NoSignerCertificateFound");
    case ePdfToolsSignatureValidation_SubIndication_NoCertificateChainFound:
        return _T("NoCertificateChainFound");
    case ePdfToolsSignatureValidation_SubIndication_RevokedNoPoe:
        return _T("RevokedNoPoe");
    case ePdfToolsSignatureValidation_SubIndication_RevokedCaNoPoe:
        return _T("RevokedCaNoPoe");
    case ePdfToolsSignatureValidation_SubIndication_OutOfBoundsNoPoe:
        return _T("OutOfBoundsNoPoe");
    case ePdfToolsSignatureValidation_SubIndication_CryptoConstraintsFailureNoPoe:
        return _T("CryptoConstraintsFailureNoPoe");
    case ePdfToolsSignatureValidation_SubIndication_NoPoe:
        return _T("NoPoe");
    case ePdfToolsSignatureValidation_SubIndication_TryLater:
        return _T("TryLater");
    case ePdfToolsSignatureValidation_SubIndication_NoPolicy:
        return _T("NoPolicy");
    case ePdfToolsSignatureValidation_SubIndication_SignedDataNotFound:
        return _T("SignedDataNotFound");
    case ePdfToolsSignatureValidation_SubIndication_IncompleteCertificateChain:
        return _T("IncompleteCertificateChain");
    case ePdfToolsSignatureValidation_SubIndication_CertificateNoRevocationInformation:
        return _T("CertificateNoRevocationInformation");
    case ePdfToolsSignatureValidation_SubIndication_MissingRevocationInformation:
        return _T("MissingRevocationInformation");
    case ePdfToolsSignatureValidation_SubIndication_ExpiredNoRevocationInformation:
        return _T("ExpiredNoRevocationInformation");
    case ePdfToolsSignatureValidation_SubIndication_Untrusted:
        return _T("Untrusted");
    case ePdfToolsSignatureValidation_SubIndication_Generic:
        return _T("Generic");
    default:
        return _T("Unknown");
    }
}

// Convert HashAlgorithm enum to string
const TCHAR* HashAlgorithmToString(TPdfToolsCrypto_HashAlgorithm iHashAlgorithm)
{
    switch (iHashAlgorithm)
    {
    case ePdfToolsCrypto_HashAlgorithm_Md5:
        return _T("Md5");
    case ePdfToolsCrypto_HashAlgorithm_RipeMd160:
        return _T("RipeMd160");
    case ePdfToolsCrypto_HashAlgorithm_Sha1:
        return _T("Sha1");
    case ePdfToolsCrypto_HashAlgorithm_Sha256:
        return _T("Sha256");
    case ePdfToolsCrypto_HashAlgorithm_Sha384:
        return _T("Sha384");
    case ePdfToolsCrypto_HashAlgorithm_Sha512:
        return _T("Sha512");
    case ePdfToolsCrypto_HashAlgorithm_Sha3_256:
        return _T("Sha3_256");
    case ePdfToolsCrypto_HashAlgorithm_Sha3_384:
        return _T("Sha3_384");
    case ePdfToolsCrypto_HashAlgorithm_Sha3_512:
        return _T("Sha3_512");
    default:
        return _T("Unknown");
    }
}

// Convert TimeSource enum to string
const TCHAR* TimeSourceToString(TPdfToolsSignatureValidation_TimeSource iTimeSource)
{
    switch (iTimeSource)
    {
    case ePdfToolsSignatureValidation_TimeSource_ProofOfExistence:
        return _T("ProofOfExistence");
    case ePdfToolsSignatureValidation_TimeSource_ExpiredTimeStamp:
        return _T("ExpiredTimeStamp");
    case ePdfToolsSignatureValidation_TimeSource_SignatureTime:
        return _T("SignatureTime");
    default:
        return _T("Unknown");
    }
}

// Convert DataSource flags enum to string (comma-separated for combined flags)
void DataSourceToString(TPdfToolsSignatureValidation_DataSource iDataSource, TCHAR* szBuffer, size_t nBufferSize)
{
    szBuffer[0] = _T('\0');
    struct
    {
        TPdfToolsSignatureValidation_DataSource flag;
        const TCHAR*                            name;
    } flags[] = {
        {ePdfToolsSignatureValidation_DataSource_EmbedInSignature, _T("EmbedInSignature")},
        {ePdfToolsSignatureValidation_DataSource_EmbedInDocument, _T("EmbedInDocument")},
        {ePdfToolsSignatureValidation_DataSource_Download, _T("Download")},
        {ePdfToolsSignatureValidation_DataSource_System, _T("System")},
        {ePdfToolsSignatureValidation_DataSource_Aatl, _T("Aatl")},
        {ePdfToolsSignatureValidation_DataSource_Eutl, _T("Eutl")},
        {ePdfToolsSignatureValidation_DataSource_CustomTrustList, _T("CustomTrustList")},
    };
    int bFirst = 1;
    for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i++)
    {
        if (iDataSource & flags[i].flag)
        {
            if (!bFirst)
                _tcscat(szBuffer, _T(", "));
            _tcscat(szBuffer, flags[i].name);
            bFirst = 0;
        }
    }
    if (bFirst)
        _tcscat(szBuffer, _T("Unknown"));
}

// Format a constraint result to string matching C# ConstraintToString output
// Format: <prefix><SubIndication> <message>
// prefix: "" for Valid, "?" for Indeterminate, "!" for Invalid
void PrintConstraintResult(TPdfToolsSignatureValidation_ConstraintResult* pConstraint, int iIsFullRevisionCovered)
{
    if (pConstraint == NULL)
    {
        _tprintf(_T("null"));
        return;
    }

    TPdfToolsSignatureValidation_Indication iIndication =
        PdfToolsSignatureValidation_ConstraintResult_GetIndication(pConstraint);
    TPdfToolsSignatureValidation_SubIndication iSubIndication =
        PdfToolsSignatureValidation_ConstraintResult_GetSubIndication(pConstraint);

    TCHAR szMessage[1024] = {0};
    PdfToolsSignatureValidation_ConstraintResult_GetMessage(pConstraint, szMessage, ARRAY_SIZE(szMessage));

    if (iIsFullRevisionCovered == 0)
    {
        // Byte range invalid case
        _tprintf(_T("!Invalid signature byte range."));
        if (iIndication != ePdfToolsSignatureValidation_Indication_Valid)
            _tprintf(_T(" %s %s"), SubIndicationToString(iSubIndication), szMessage);
    }
    else
    {
        const TCHAR* szPrefix = _T("");
        if (iIndication == ePdfToolsSignatureValidation_Indication_Indeterminate)
            szPrefix = _T("?");
        else if (iIndication == ePdfToolsSignatureValidation_Indication_Invalid)
            szPrefix = _T("!");

        _tprintf(_T("%s%s %s"), szPrefix, SubIndicationToString(iSubIndication), szMessage);
    }
}

// Format a date to string (DD/MM/YYYY HH:MM:SS format to match C# output)
void FormatDate(TPdfToolsSys_Date* pDate, TCHAR* szBuffer, size_t nBufferSize)
{
    if (pDate->iTZSign == 0)
        _stprintf(szBuffer, _T("%02d/%02d/%04d %02d:%02d:%02d"),
                  pDate->iDay, pDate->iMonth, pDate->iYear,
                  pDate->iHour, pDate->iMinute, pDate->iSecond);
    else
        _stprintf(szBuffer, _T("%02d/%02d/%04d %02d:%02d:%02d %c%02d:%02d"),
                  pDate->iDay, pDate->iMonth, pDate->iYear,
                  pDate->iHour, pDate->iMinute, pDate->iSecond,
                  pDate->iTZSign > 0 ? '+' : '-',
                  pDate->iTZHour, pDate->iTZMinute);
}

// Print certificate details
void PrintCertificate(TPdfToolsSignatureValidation_Certificate* pCert)
{
    if (pCert == NULL)
    {
        _tprintf(_T("    - null\n"));
        return;
    }

    TCHAR szSubjectName[512] = {0};
    TCHAR szIssuerName[512]  = {0};
    TCHAR szFingerprint[256] = {0};
    PdfToolsSignatureValidation_Certificate_GetSubjectName(pCert, szSubjectName, ARRAY_SIZE(szSubjectName));
    PdfToolsSignatureValidation_Certificate_GetIssuerName(pCert, szIssuerName, ARRAY_SIZE(szIssuerName));
    PdfToolsSignatureValidation_Certificate_GetFingerprint(pCert, szFingerprint, ARRAY_SIZE(szFingerprint));

    // Format fingerprint as uppercase with dash separators (e.g., "03-4E-67-AC-...")
    {
        TCHAR szFormattedFp[256] = {0};
        size_t nFpLen = _tcslen(szFingerprint);
        size_t j = 0;
        for (size_t k = 0; k < nFpLen; k++)
        {
            TCHAR c = szFingerprint[k];
            if (c >= _T('a') && c <= _T('f'))
                c = c - _T('a') + _T('A');
            szFormattedFp[j++] = c;
            if (k % 2 == 1 && k + 1 < nFpLen)
                szFormattedFp[j++] = _T('-');
        }
        szFormattedFp[j] = _T('\0');
        _tcscpy(szFingerprint, szFormattedFp);
    }

    TPdfToolsSys_Date notBefore, notAfter;
    TCHAR             szNotBefore[64] = {0};
    TCHAR             szNotAfter[64]  = {0};
    if (PdfToolsSignatureValidation_Certificate_GetNotBefore(pCert, &notBefore))
        FormatDate(&notBefore, szNotBefore, ARRAY_SIZE(szNotBefore));
    if (PdfToolsSignatureValidation_Certificate_GetNotAfter(pCert, &notAfter))
        FormatDate(&notAfter, szNotAfter, ARRAY_SIZE(szNotAfter));

    TPdfToolsSignatureValidation_DataSource iSource = PdfToolsSignatureValidation_Certificate_GetSource(pCert);
    TCHAR szSource[256] = {0};
    DataSourceToString(iSource, szSource, ARRAY_SIZE(szSource));

    _tprintf(_T("    - Subject    : %s\n"), szSubjectName);
    _tprintf(_T("    - Issuer     : %s\n"), szIssuerName);
    _tprintf(_T("    - Validity   : %s - %s\n"), szNotBefore, szNotAfter);
    _tprintf(_T("    - Fingerprint: %s\n"), szFingerprint);
    _tprintf(_T("    - Source     : %s\n"), szSource);

    TPdfToolsSignatureValidation_ConstraintResult* pValidity =
        PdfToolsSignatureValidation_Certificate_GetValidity(pCert);
    _tprintf(_T("    - Validity   : "));
    PrintConstraintResult(pValidity, 1);
    _tprintf(_T("\n"));
}

// Print signature content (CMS signature or timestamp)
void PrintSignatureContent(TPdfToolsSignatureValidation_SignatureContent* pContent, int iIsFullRevisionCovered)
{
    if (pContent == NULL)
    {
        _tprintf(_T("  - null\n"));
        return;
    }

    // Print validity
    TPdfToolsSignatureValidation_ConstraintResult* pValidity =
        PdfToolsSignatureValidation_SignatureContent_GetValidity(pContent);
    _tprintf(_T("  - Validity  : "));
    PrintConstraintResult(pValidity, iIsFullRevisionCovered);
    _tprintf(_T("\n"));

    TPdfToolsSignatureValidation_SignatureContentType iType =
        PdfToolsSignatureValidation_SignatureContent_GetType(pContent);

    if (iType == ePdfToolsSignatureValidation_SignatureContentType_CmsSignatureContent)
    {
        TPdfToolsSignatureValidation_CmsSignatureContent* pCms =
            (TPdfToolsSignatureValidation_CmsSignatureContent*)pContent;

        // Validation time
        TPdfToolsSys_Date validationTime;
        TCHAR             szValidationTime[64] = {0};
        if (PdfToolsSignatureValidation_CmsSignatureContent_GetValidationTime(pCms, &validationTime))
            FormatDate(&validationTime, szValidationTime, ARRAY_SIZE(szValidationTime));
        TPdfToolsSignatureValidation_TimeSource iTimeSource =
            PdfToolsSignatureValidation_CmsSignatureContent_GetValidationTimeSource(pCms);
        _tprintf(_T("  - Validation: %s from %s\n"), szValidationTime, TimeSourceToString(iTimeSource));

        // Hash algorithm
        TPdfToolsCrypto_HashAlgorithm iHashAlg =
            PdfToolsSignatureValidation_CmsSignatureContent_GetHashAlgorithm(pCms);
        _tprintf(_T("  - Hash      : %s\n"), HashAlgorithmToString(iHashAlg));

        // Signing certificate
        _tprintf(_T("  - Signing Cert\n"));
        TPdfToolsSignatureValidation_Certificate* pSigningCert =
            PdfToolsSignatureValidation_CmsSignatureContent_GetSigningCertificate(pCms);
        PrintCertificate(pSigningCert);

        // Certificate chain
        _tprintf(_T("  - Chain\n"));
        TPdfToolsSignatureValidation_CertificateChain* pChain =
            PdfToolsSignatureValidation_CmsSignatureContent_GetCertificateChain(pCms);
        if (pChain != NULL)
        {
            int nChainCount = PdfToolsSignatureValidation_CertificateChain_GetCount(pChain);
            for (int j = 0; j < nChainCount; j++)
            {
                _tprintf(_T("  - Issuer Cert %d\n"), j + 1);
                TPdfToolsSignatureValidation_Certificate* pChainCert =
                    PdfToolsSignatureValidation_CertificateChain_Get(pChain, j);
                PrintCertificate(pChainCert);
            }
            BOOL bIsComplete = PdfToolsSignatureValidation_CertificateChain_IsComplete(pChain);
            _tprintf(_T("  - Chain     : %s chain\n"), bIsComplete ? _T("complete") : _T("incomplete"));
        }

        // Timestamp
        _tprintf(_T("  Time-Stamp\n"));
        TPdfToolsSignatureValidation_TimeStampContent* pTimeStamp =
            PdfToolsSignatureValidation_CmsSignatureContent_GetTimeStamp(pCms);
        PrintSignatureContent((TPdfToolsSignatureValidation_SignatureContent*)pTimeStamp, 1);
    }
    else if (iType == ePdfToolsSignatureValidation_SignatureContentType_TimeStampContent)
    {
        TPdfToolsSignatureValidation_TimeStampContent* pTs =
            (TPdfToolsSignatureValidation_TimeStampContent*)pContent;

        // Validation time
        TPdfToolsSys_Date validationTime;
        TCHAR             szValidationTime[64] = {0};
        if (PdfToolsSignatureValidation_TimeStampContent_GetValidationTime(pTs, &validationTime))
            FormatDate(&validationTime, szValidationTime, ARRAY_SIZE(szValidationTime));
        TPdfToolsSignatureValidation_TimeSource iTimeSource =
            PdfToolsSignatureValidation_TimeStampContent_GetValidationTimeSource(pTs);
        _tprintf(_T("  - Validation: %s from %s\n"), szValidationTime, TimeSourceToString(iTimeSource));

        // Hash algorithm
        TPdfToolsCrypto_HashAlgorithm iHashAlg =
            PdfToolsSignatureValidation_TimeStampContent_GetHashAlgorithm(pTs);
        _tprintf(_T("  - Hash      : %s\n"), HashAlgorithmToString(iHashAlg));

        // Timestamp date
        TPdfToolsSys_Date tsDate;
        TCHAR             szTsDate[64] = {0};
        if (PdfToolsSignatureValidation_TimeStampContent_GetDate(pTs, &tsDate))
            FormatDate(&tsDate, szTsDate, ARRAY_SIZE(szTsDate));
        _tprintf(_T("  - Time      : %s\n"), szTsDate);

        // Signing certificate
        _tprintf(_T("  - Signing Cert\n"));
        TPdfToolsSignatureValidation_Certificate* pSigningCert =
            PdfToolsSignatureValidation_TimeStampContent_GetSigningCertificate(pTs);
        PrintCertificate(pSigningCert);

        // Certificate chain
        _tprintf(_T("  - Chain\n"));
        TPdfToolsSignatureValidation_CertificateChain* pChain =
            PdfToolsSignatureValidation_TimeStampContent_GetCertificateChain(pTs);
        if (pChain != NULL)
        {
            int nChainCount = PdfToolsSignatureValidation_CertificateChain_GetCount(pChain);
            for (int j = 0; j < nChainCount; j++)
            {
                _tprintf(_T("  - Issuer Cert %d\n"), j + 1);
                TPdfToolsSignatureValidation_Certificate* pChainCert =
                    PdfToolsSignatureValidation_CertificateChain_Get(pChain, j);
                PrintCertificate(pChainCert);
            }
            BOOL bIsComplete = PdfToolsSignatureValidation_CertificateChain_IsComplete(pChain);
            _tprintf(_T("  - Chain      : %s chain\n"), bIsComplete ? _T("complete") : _T("incomplete"));
        }
    }
    else if (iType == ePdfToolsSignatureValidation_SignatureContentType_UnsupportedSignatureContent)
    {
        // Nothing additional to print for unsupported signatures
    }
    else
    {
        _tprintf(_T("Unsupported signature content type\n"));
    }
}

// Constraint event listener callback
void PDFTOOLS_CALL ConstraintListener(void* pContext, const TCHAR* szMessage,
                         TPdfToolsSignatureValidation_Indication iIndication,
                         TPdfToolsSignatureValidation_SubIndication iSubIndication,
                         TPdfToolsPdf_SignedSignatureField* pSignature,
                         const TCHAR* szDataPart)
{
    TCHAR szName[256] = {0};
    PdfToolsPdf_SignedSignatureField_GetName(pSignature, szName, ARRAY_SIZE(szName));

    const TCHAR* szPrefix = _T("");
    if (iIndication == ePdfToolsSignatureValidation_Indication_Indeterminate)
        szPrefix = _T("?");
    else if (iIndication == ePdfToolsSignatureValidation_Indication_Invalid)
        szPrefix = _T("!");

    _tprintf(_T("  - %s%s%s: %s%s %s\n"),
             szName,
             (szDataPart && _tcslen(szDataPart) > 0) ? _T(": ") : _T(""),
             (szDataPart && _tcslen(szDataPart) > 0) ? szDataPart : _T(""),
             szPrefix,
             SubIndicationToString(iSubIndication),
             szMessage);
}

// Check if a file name ends with a given suffix
int EndsWith(const TCHAR* szFileName, const TCHAR* szSuffix)
{
    size_t nFileLen   = _tcslen(szFileName);
    size_t nSuffixLen = _tcslen(szSuffix);
    if (nFileLen < nSuffixLen)
        return 0;
    return _tcscmp(szFileName + nFileLen - nSuffixLen, szSuffix) == 0;
}

int Usage()
{
    printf("Usage: pdftoolssignaturesvalidate <inputPath> [<certificateDirectory>].\n");
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
    if (argc < 2)
    {
        return Usage();
    }

    TCHAR*                                                   szInPath   = argv[1];
    TCHAR*                                                   szCertDir  = (argc == 3) ? argv[2] : NULL;
    FILE*                                                    pInStream  = NULL;
    TPdfToolsPdf_Document*                                   pInDoc     = NULL;
    TPdfToolsSignatureValidationProfiles_Default*             pProfile   = NULL;
    TPdfToolsSignatureValidation_CustomTrustList*             pCtl       = NULL;
    TPdfToolsSignatureValidation_Validator*                   pValidator = NULL;
    TPdfToolsSignatureValidation_ValidationResults*        pResults   = NULL;

    // Initialize library
    PdfTools_Initialize();

    // By default, a test license key is active. In this case, a watermark is added to the output. 
    // If you have a license key, please uncomment the following call and set the license key.
    // GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(PdfTools_Sdk_Initialize(_T("insert-license-key-here"), NULL),
    //                                     _T("Failed to set the license key. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
    //                                     PdfTools_GetLastError());

    // Create the default validation profile
    pProfile = PdfToolsSignatureValidationProfiles_Default_New();
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(
        pProfile,
        _T("Failed to create validation profile. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
        PdfTools_GetLastError());

    // For offline operation, build a custom trust list from the file system
    // and disable external revocation checks
    if (szCertDir != NULL)
    {
        _tprintf(_T("Using 'offline' validation mode with custom trust list.\n\n"));

        // Create a CustomTrustList to hold the certificates
        pCtl = PdfToolsSignatureValidation_CustomTrustList_New();
        GOTO_CLEANUP_IF_NULL_PRINT_ERROR(
            pCtl,
            _T("Failed to create custom trust list. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
            PdfTools_GetLastError());

        // Iterate through files in the certificate directory and add certificates
        // to the custom trust list
#if defined(WIN32)
        {
            WIN32_FIND_DATA findData;
            HANDLE          hFind;
            TCHAR           szSearchPath[512];
            TCHAR           szFilePath[512];

            _stprintf(szSearchPath, _T("%s\*"), szCertDir);
            hFind = FindFirstFile(szSearchPath, &findData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        continue;

                    _stprintf(szFilePath, _T("%s\%s"), szCertDir, findData.cFileName);

                    FILE* pCertStream = _tfopen(szFilePath, _T("rb"));
                    if (pCertStream == NULL)
                    {
                        _tprintf(_T("Could not open certificate file '%s'.\n"), szFilePath);
                        continue;
                    }

                    TPdfToolsSys_StreamDescriptor certDesc;
                    PdfToolsSysCreateFILEStreamDescriptor(&certDesc, pCertStream, 0);

                    if (EndsWith(findData.cFileName, _T(".cer")) || EndsWith(findData.cFileName, _T(".pem")))
                    {
                        if (!PdfToolsSignatureValidation_CustomTrustList_AddCertificates(pCtl, &certDesc))
                            _tprintf(_T("Could not add certificate '%s' to custom trust list.\n"), szFilePath);
                    }
                    else if (EndsWith(findData.cFileName, _T(".p12")) || EndsWith(findData.cFileName, _T(".pfx")))
                    {
                        if (!PdfToolsSignatureValidation_CustomTrustList_AddArchive(pCtl, &certDesc, NULL))
                            _tprintf(_T("Could not add archive '%s' to custom trust list.\n"), szFilePath);
                    }

                    fclose(pCertStream);
                } while (FindNextFile(hFind, &findData));
                FindClose(hFind);
            }
            else
            {
                _tprintf(_T("Directory %s is missing. No certificates were added to the custom trust list.\n"),
                         szCertDir);
            }
        }
#else
        {
            DIR*           pDir;
            struct dirent* pEntry;
            char           szFilePath[512];

            pDir = opendir(szCertDir);
            if (pDir != NULL)
            {
                while ((pEntry = readdir(pDir)) != NULL)
                {
                    if (pEntry->d_name[0] == '.')
                        continue;

                    _stprintf(szFilePath, _T("%s/%s"), szCertDir, pEntry->d_name);

                    FILE* pCertStream = _tfopen(szFilePath, _T("rb"));
                    if (pCertStream == NULL)
                    {
                        _tprintf(_T("Could not open certificate file '%s'.\n"), szFilePath);
                        continue;
                    }

                    TPdfToolsSys_StreamDescriptor certDesc;
                    PdfToolsSysCreateFILEStreamDescriptor(&certDesc, pCertStream, 0);

                    if (EndsWith(pEntry->d_name, _T(".cer")) || EndsWith(pEntry->d_name, _T(".pem")))
                    {
                        if (!PdfToolsSignatureValidation_CustomTrustList_AddCertificates(pCtl, &certDesc))
                            _tprintf(_T("Could not add certificate '%s' to custom trust list.\n"), szFilePath);
                    }
                    else if (EndsWith(pEntry->d_name, _T(".p12")) || EndsWith(pEntry->d_name, _T(".pfx")))
                    {
                        if (!PdfToolsSignatureValidation_CustomTrustList_AddArchive(pCtl, &certDesc, NULL))
                            _tprintf(_T("Could not add archive '%s' to custom trust list.\n"), szFilePath);
                    }

                    fclose(pCertStream);
                }
                closedir(pDir);
            }
            else
            {
                _tprintf(_T("Directory %s is missing. No certificates were added to the custom trust list.\n"),
                         szCertDir);
            }
        }
#endif
        _tprintf(_T("\n"));

        // Assign the custom trust list to the validation profile
        GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(
            PdfToolsSignatureValidationProfiles_Profile_SetCustomTrustList((TPdfToolsSignatureValidationProfiles_Profile*)pProfile, pCtl),
            _T("Failed to set custom trust list. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
            PdfTools_GetLastError());

        // Allow validation from embedded file sources and the custom trust list
        {
            TPdfToolsSignatureValidationProfiles_ValidationOptions* pValOpts =
                PdfToolsSignatureValidationProfiles_Profile_GetValidationOptions((TPdfToolsSignatureValidationProfiles_Profile*)pProfile);
            PdfToolsSignatureValidationProfiles_ValidationOptions_SetTimeSource(
                pValOpts, ePdfToolsSignatureValidation_TimeSource_ProofOfExistence |
                              ePdfToolsSignatureValidation_TimeSource_ExpiredTimeStamp |
                              ePdfToolsSignatureValidation_TimeSource_SignatureTime);
            PdfToolsSignatureValidationProfiles_ValidationOptions_SetCertificateSources(
                pValOpts, ePdfToolsSignatureValidation_DataSource_EmbedInSignature |
                              ePdfToolsSignatureValidation_DataSource_EmbedInDocument |
                              ePdfToolsSignatureValidation_DataSource_CustomTrustList);
        }

        // Disable revocation checks
        {
            TPdfToolsSignatureValidationProfiles_TrustConstraints* pSigningConstraints =
                PdfToolsSignatureValidationProfiles_Profile_GetSigningCertTrustConstraints((TPdfToolsSignatureValidationProfiles_Profile*)pProfile);
            PdfToolsSignatureValidationProfiles_TrustConstraints_SetRevocationCheckPolicy(
                pSigningConstraints, ePdfToolsSignatureValidationProfiles_RevocationCheckPolicy_NoCheck);

            TPdfToolsSignatureValidationProfiles_TrustConstraints* pTsConstraints =
                PdfToolsSignatureValidationProfiles_Profile_GetTimeStampTrustConstraints((TPdfToolsSignatureValidationProfiles_Profile*)pProfile);
            PdfToolsSignatureValidationProfiles_TrustConstraints_SetRevocationCheckPolicy(
                pTsConstraints, ePdfToolsSignatureValidationProfiles_RevocationCheckPolicy_NoCheck);
        }
    }

    // Create the validator object
    pValidator = PdfToolsSignatureValidation_Validator_New();
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(
        pValidator,
        _T("Failed to create validator. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
        PdfTools_GetLastError());

    // Add a constraint event listener
    GOTO_CLEANUP_IF_FALSE_PRINT_ERROR(
        PdfToolsSignatureValidation_Validator_AddConstraintHandler(pValidator, NULL, ConstraintListener),
        _T("Failed to add constraint handler. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
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

    // Validate ALL signatures in the document
    _tprintf(_T("Validation Constraints\n"));
    pResults = PdfToolsSignatureValidation_Validator_Validate(
        pValidator, pInDoc, (TPdfToolsSignatureValidationProfiles_Profile*)pProfile,
        ePdfToolsSignatureValidation_SignatureSelector_All);
    GOTO_CLEANUP_IF_NULL_PRINT_ERROR(
        pResults,
        _T("Failed to validate signatures. %s (ErrorCode: 0x%08x).\n"), szErrorBuff,
        PdfTools_GetLastError());

    // Print results
    {
        int nCount = PdfToolsSignatureValidation_ValidationResults_GetCount(pResults);

        _tprintf(_T("\nSignatures validated: %d\n\n"), nCount);

        for (int i = 0; i < nCount; i++)
        {
            TPdfToolsSignatureValidation_ValidationResult* pResult =
                PdfToolsSignatureValidation_ValidationResults_Get(pResults, i);
            if (pResult == NULL)
                continue;

            // Get the signature field
            TPdfToolsPdf_SignedSignatureField* pField =
                PdfToolsSignatureValidation_ValidationResult_GetSignatureField(pResult);

            // Get and print the field name
            TCHAR szFieldName[256] = {0};
            TCHAR szName[256]      = {0};
            PdfToolsPdf_SignatureField_GetFieldName((TPdfToolsPdf_SignatureField*)pField, szFieldName, ARRAY_SIZE(szFieldName));
            PdfToolsPdf_SignedSignatureField_GetName(pField, szName, ARRAY_SIZE(szName));
            _tprintf(_T("%s of %s\n"), szFieldName, szName);

            // Print revision info
            {
                TPdfToolsPdf_Revision* pRevision = PdfToolsPdf_SignedSignatureField_GetRevision(pField);
                if (pRevision != NULL)
                {
                    BOOL bIsLatest = PdfToolsPdf_Revision_IsLatest(pRevision);
                    _tprintf(_T("  - Revision  : %s\n"), bIsLatest ? _T("latest") : _T("intermediate"));
                }
                else
                {
                    _tprintf(_T("Unable to validate document Revision: "));
                    nBufSize = PdfTools_GetLastErrorMessage(NULL, 0);
                    PdfTools_GetLastErrorMessage(szErrorBuff, MIN(ARRAY_SIZE(szErrorBuff), nBufSize));
                    _tprintf(_T("%s\n"), szErrorBuff);
                }
            }

            // Determine if full revision is covered
            int iIsFullRevisionCovered = 1;
            {
                BOOL bResult = PdfToolsPdf_SignedSignatureField_IsFullRevisionCovered(pField);
                if (bResult == FALSE && PdfTools_GetLastError() == ePdfTools_Error_Success)
                    iIsFullRevisionCovered = 0;
            }

            // Print signature content details
            TPdfToolsSignatureValidation_SignatureContent* pContent =
                PdfToolsSignatureValidation_ValidationResult_GetSignatureContent(pResult);
            PrintSignatureContent(pContent, iIsFullRevisionCovered);

            _tprintf(_T("\n"));
        }
    }


cleanup:
    PdfTools_Release(pResults);
    PdfTools_Release(pValidator);
    PdfTools_Release(pCtl);
    PdfTools_Release(pProfile);
    if (pInDoc)
        PdfToolsPdf_Document_Close(pInDoc);
    if (pInStream)
        fclose(pInStream);

    PdfTools_Uninitialize();

    return iRet;
} 