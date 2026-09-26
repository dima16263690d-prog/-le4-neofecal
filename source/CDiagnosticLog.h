#pragma once

class CDiagnosticLog
{
#ifdef DEBUGIT
    std::ofstream m_hFile;
#endif

public:
#ifdef DEBUGIT
    CDiagnosticLog();
    ~CDiagnosticLog();

    void Trace(const char *format, ...);
    void Error(const char *format, ...);
#endif
};

extern CDiagnosticLog DiagnosticLog;

#ifdef DEBUGIT
#define DIAG(...) { DiagnosticLog.Trace(__VA_ARGS__); }
#define DIAG_ERROR(...) { DiagnosticLog.Error(__VA_ARGS__); }
#else
#define DIAG(...) __noop
#define DIAG_ERROR(...) __noop
#endif
