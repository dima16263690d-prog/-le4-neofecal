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

    void NormalizeLogText(char *message)
    {
        struct Tag
        {
            const char *from;
            const char *to;
        };

        static const Tag tags[] =
        {
            { "[CLEO]", "[Cleo]" },
            { "[ERROR]", "[Error]" },
            { "[WARNING]", "[Warning]" },
            { "[INFO]", "[Info]" },
            { "[CUSTOM]", "[Custom]" },
            { "[RESTORE]", "[Restore]" },
            { "[LOAD]", "[Load]" },
            { "[SAVE]", "[Save]" },
            { "[END]", "[End]" },
            { "[STOP]", "[Stop]" },
            { "[DELETE]", "[Delete]" },
            { "[CREATE]", "[Create]" },
            { "[REGISTER]", "[Register]" }
        };

        for (const auto &tag : tags)
        {
            char *pos = nullptr;
            while ((pos = strstr(message, tag.from)) != nullptr)
            {
                const size_t fromLen = strlen(tag.from);
                const size_t toLen = strlen(tag.to);

                if (toLen <= fromLen)
                {
                    memcpy(pos, tag.to, toLen);
                    if (toLen < fromLen)
                        memmove(pos + toLen, pos + fromLen, strlen(pos + fromLen) + 1);
                }
            }
        }
    }
}

CDebug::CDebug()
    : m_hFile(szLogFileName)
{
    Write("Info", "Log started.");
}

CDebug::~CDebug()
{
    Write("Info", "Log finished.");
}

void CDebug::Write(const char *level, const char *message)
{
    const std::string key = std::string(level) + "|" + message;

    // Write each identical diagnostic only once per game session.
    if (!m_writtenMessages.insert(key).second)
        return;

    char normalized[2048];
    strncpy_s(normalized, sizeof(normalized), message, _TRUNCATE);
    NormalizeLogText(normalized);

    SYSTEMTIME t;
    char szBuf[2048];

    GetLocalTime(&t);

    sprintf_s(
        szBuf,
        sizeof(szBuf),
        "%02d/%02d/%04d %02d:%02d:%02d.%03d [%-7s] %s",
        t.wDay,
        t.wMonth,
        t.wYear,
        t.wHour,
        t.wMinute,
        t.wSecond,
        t.wMilliseconds,
        level,
        normalized
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
    WriteFormatted("Info", format, args);
    va_end(args);
}

void CDebug::TraceWarning(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    WriteFormatted("Warning", format, args);
    va_end(args);
}

void CDebug::TraceError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    WriteFormatted("Error", format, args);
    va_end(args);
}

void CDebug::TraceDiagnostic(const char *format, ...)
{
    char message[2048];

    va_list args;
    va_start(args, format);
    FormatMessage(message, sizeof(message), format, args);
    va_end(args);

    const char *level = "Info";

    if (strstr(message, "[ERROR]") || strstr(message, "[Error]") ||
        strstr(message, "error") || strstr(message, "Error") ||
        strstr(message, "failed") || strstr(message, "Failed"))
    {
        level = "Error";
    }
    else if (strstr(message, "[WARNING]") || strstr(message, "[Warning]") ||
             strstr(message, "warning") || strstr(message, "Warning"))
    {
        level = "Warning";
    }

    Write(level, message);
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
