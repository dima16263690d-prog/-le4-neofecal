#include "stdafx.h"
#include "CDebug.h"

CDebug Debug;

#ifdef DEBUGIT

namespace
{
    void FormatMessage(char *buffer, size_t bufferSize, const char *format, va_list args)
    {
        vsnprintf_s(buffer, bufferSize, _TRUNCATE, format, args);
    }
}

CDebug::CDebug()
    : m_hFile(szLogFileName)
{
    Write("INFO", "Log started.");
}

CDebug::~CDebug()
{
    Write("INFO", "Log finished.");
}

void CDebug::Write(const char *level, const char *message)
{
    const std::string key = std::string(level) + "|" + message;

    // Write each identical diagnostic only once per game session.
    if (!m_writtenMessages.insert(key).second)
        return;

    SYSTEMTIME t;
    char szBuf[2048];

    GetLocalTime(&t);

    sprintf_s(
        szBuf,
        sizeof(szBuf),
        "%02d/%02d/%04d %02d:%02d:%02d.%03d [%s] %s",
        t.wDay,
        t.wMonth,
        t.wYear,
        t.wHour,
        t.wMinute,
        t.wSecond,
        t.wMilliseconds,
        level,
        message
    );

    m_hFile << szBuf << std::endl;
    m_hFile.flush();

    OutputDebugStringA(szBuf);
    OutputDebugStringA("\n");
}

void CDebug::WriteFormatted(const char *level, const char *format, va_list args)
{
    char message[2048];
    FormatMessage(message, sizeof(message), format, args);
    Write(level, message);
}

void CDebug::Trace(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    WriteFormatted("INFO", format, args);
    va_end(args);
}

void CDebug::TraceWarning(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    WriteFormatted("WARNING", format, args);
    va_end(args);
}

void CDebug::TraceError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    WriteFormatted("ERROR", format, args);
    va_end(args);
}

#endif

void Error(const char *szStr)
{
    MessageBox(nullptr, szStr, "CLEO error", MB_ICONERROR | MB_OK);
#ifdef DEBUGIT
    Debug.TraceError("%s", szStr);
#endif
    //exit(1);
}

void Warning(const char *szStr)
{
    MessageBox(nullptr, szStr, "CLEO warning", MB_ICONWARNING | MB_OK);
#ifdef DEBUGIT
    Debug.TraceWarning("%s", szStr);
#endif
    //exit(1);
}
