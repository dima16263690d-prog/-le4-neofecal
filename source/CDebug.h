#pragma once

#include <set>
#include <string>

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
    std::set<std::string> m_writtenMessages;

    void Write(const char *level, const char *message);
    void WriteFormatted(const char *level, const char *format, va_list args);
#endif

public:
#ifdef DEBUGIT

    CDebug();
    ~CDebug();

    void Trace(const char *format, ...);
    void TraceWarning(const char *format, ...);
    void TraceError(const char *format, ...);

#endif
};

extern CDebug Debug;
void Warning(const char *);
void Error(const char *);
