#include "stdafx.h"
#include "cleo.h"
#include "CDebug.h"
#include "CCodeInjector.h"

namespace CLEO
{
    void CCodeInjector::OpenReadWriteAccess()
    {
        if (bAccessOpen) return;

        auto dwLoadOffset = static_cast<memory_pointer>(GetModuleHandle(nullptr));

        // Temporarily make the game's .text and .rdata sections writable.
        // CMemoryProtection keeps the original protection and restores it
        // automatically when CCodeInjector is destroyed.
        auto pImageBase = (BYTE *)dwLoadOffset;
        auto pDosHeader = (PIMAGE_DOS_HEADER)dwLoadOffset;
        auto pNtHeader = (PIMAGE_NT_HEADERS)(pImageBase + pDosHeader->e_lfanew);
        auto pSection = IMAGE_FIRST_SECTION(pNtHeader);

        for (int i = pNtHeader->FileHeader.NumberOfSections; i; i--, pSection++)
        {
            const bool isText = !strcmp((char*)pSection->Name, ".text");
            const bool isRdata = !strcmp((char*)pSection->Name, ".rdata");

            if (!isText && !isRdata)
                continue;

            DWORD dwPhysSize = (pSection->Misc.VirtualSize + 4095) & ~4095;
            DWORD newProtect = (pSection->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                ? PAGE_EXECUTE_READWRITE
                : PAGE_READWRITE;

            TRACE("Unprotecting memory region '%s': 0x%08X (size: 0x%08X)",
                pSection->Name,
                (DWORD)pSection->VirtualAddress,
                (DWORD)dwPhysSize
            );

            void* address = pImageBase + pSection->VirtualAddress;

            if (isText)
                m_textProtection = CMemoryProtection(address, dwPhysSize, newProtect);
            else
                m_rdataProtection = CMemoryProtection(address, dwPhysSize, newProtect);
        }

        bAccessOpen = true;
    }

    void CCodeInjector::CloseReadWriteAccess()
    {
        if (!bAccessOpen) return;

        // Restore the exact protections captured by OpenReadWriteAccess().
        // CCodeInjector normally stays alive for the lifetime of CLEO, so
        // plugins keep write access during initialization and normal runtime.
        if (m_textProtection.IsActive())
        {
            TRACE("Restoring memory protection for '.text'");
            m_textProtection.Reset();
        }

        if (m_rdataProtection.IsActive())
        {
            TRACE("Restoring memory protection for '.rdata'");
            m_rdataProtection.Reset();
        }

        bAccessOpen = false;
    }
}
