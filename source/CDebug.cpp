#include "stdafx.h"
#include "CDebug.h"

CDebug Debug;

#ifdef DEBUGIT

namespace
{
    bool IsImportantDiagnostic(const char *message)
    {
        // TRACE is intentionally quiet for normal/high-frequency runtime activity.
        // Errors, failed operations and invalid states remain visible.
        static const char *keywords[] =
        {
            "[Error]",
            "[Warning]",
            "Failed",
            "failed",
            "Incorrect",
            "incorrect",
            "Unknown",
            "unknown",
            "Unallowed",
            "unallowed",
            "exceeds",
            "without active",
            "not enough",
            "invalid",
            "Invalid",
            "exception",
            "Exception",
            "crash",
            "Crash",
            "cannot",
            "Cannot",
            "couldn't",
            "Couldn't"
        };

        for (const char *keyword : keywords)
        {
            if (strstr(message, keyword))
                return true;
        }

        return false;
    }
}

void CDebug::TraceAlways(const char *format, ...)
{
    SYSTEMTIME t;
    char szBuf[1024];

    GetLocalTime(&t);

    int offset = sprintf_s(
        szBuf,
        sizeof(szBuf),
        "%02d/%02d/%04d %02d:%02d:%02d.%03d ",
        t.wDay,
        t.wMonth,
        t.wYear,
        t.wHour,
        t.wMinute,
        t.wSecond,
        t.wMilliseconds
    );

    va_list arg;
    va_start(arg, format);
    vsnprintf_s(
        szBuf + offset,
        sizeof(szBuf) - offset,
        _TRUNCATE,
        format,
        arg
    );
    va_end(arg);

    m_hFile << szBuf << std::endl;
    m_hFile.flush();

    OutputDebugStringA(szBuf);
    OutputDebugStringA("\n");
}

void CDebug::Trace(const char *format, ...)
{
    char message[1024];

    va_list arg;
    va_start(arg, format);
    vsnprintf_s(message, sizeof(message), _TRUNCATE, format, arg);
    va_end(arg);

    if (!IsImportantDiagnostic(message))
        return;

    TraceAlways("%s", message);
}

#endif

void Error(const char *szStr)
{
    MessageBox(nullptr, szStr, "CLEO error", MB_ICONERROR | MB_OK);
#ifdef DEBUGIT
    Debug.TraceAlways("[Error] %s", szStr);
#endif
    //exit(1);
}

void Warning(const char *szStr)
{
    MessageBox(nullptr, szStr, "CLEO warning", MB_ICONWARNING | MB_OK);
#ifdef DEBUGIT
    Debug.TraceAlways("[Warning] %s", szStr);
#endif
    //exit(1);
}
