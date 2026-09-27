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
#endif

public:
#ifdef DEBUGIT

    CDebug() : m_hFile(szLogFileName)
    {
        TraceAlways("Log started.");
    }

    ~CDebug()
    {
        TraceAlways("Log finished.");
    }

    void Trace(const char *format, ...);
    void TraceAlways(const char *format, ...);

#endif
};

extern CDebug Debug;
void Warning(const char *);
void Error(const char *);
