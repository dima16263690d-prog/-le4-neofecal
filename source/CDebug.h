#pragma once

#define TRACE __noop

#ifdef DEBUGIT
#undef TRACE
#define TRACE(a,...) {Debug.Trace(a, __VA_ARGS__);}
#endif

const char szLogFileName[] = "cleo.log";

class CDebug
{
#ifdef DEBUGIT
    std::ofstream m_hFile;
    std::string m_lastMessage;
    unsigned int m_repeatCount;
#endif

public:
#ifdef DEBUGIT

    CDebug()
        : m_hFile(szLogFileName),
          m_repeatCount(0)
    {
        Trace("Log started.");
    }

    ~CDebug()
    {
        Trace("Log finished.");
    }

private:
    void WriteLine(const char *message)
    {
        SYSTEMTIME t;
        char szBuf[1024];

        GetLocalTime(&t);
        sprintf_s(
            szBuf,
            sizeof(szBuf),
            "%02d/%02d/%04d %02d:%02d:%02d.%03d %s",
            t.wDay,
            t.wMonth,
            t.wYear,
            t.wHour,
            t.wMinute,
            t.wSecond,
            t.wMilliseconds,
            message
        );

        m_hFile << szBuf << std::endl;
        OutputDebugStringA(szBuf);
        OutputDebugStringA("\n");
    }

    void FlushRepeated()
    {
        if (m_repeatCount <= 1)
            return;

        char summary[128];
        sprintf_s(
            summary,
            sizeof(summary),
            "[CLEO][LOG] Previous message repeated %u additional time%s",
            m_repeatCount - 1,
            (m_repeatCount - 1 == 1) ? "" : "s"
        );

        WriteLine(summary);
    }

public:
    void Trace(const char *format, ...)
    {
        char message[1024];

        va_list arg;
        va_start(arg, format);
        vsnprintf_s(message, sizeof(message), _TRUNCATE, format, arg);
        va_end(arg);

        if (m_repeatCount > 0 && m_lastMessage == message)
        {
            ++m_repeatCount;
            return;
        }

        FlushRepeated();

        m_lastMessage = message;
        m_repeatCount = 1;

        WriteLine(message);
    }
#endif
};

extern CDebug Debug;
void Warning(const char *);
void Error(const char *);
