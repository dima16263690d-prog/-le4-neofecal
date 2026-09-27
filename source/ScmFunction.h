#pragma once
#include "CTheScripts.h"
#include <list>
#include <string>

namespace CLEO
{
    class CCustomScript;

    struct ScmFunction
    {
		unsigned short prevScmFunctionId, thisScmFunctionId;
		BYTE callArgCount;
		BYTE *callIP;
		BYTE *retnAddress;
		void *savedBaseIP;
		size_t savedCodeSize;
		BYTE *savedStack[8];
		WORD savedSP;
		SCRIPT_VAR savedTls[32];
		std::list<std::string> stringParams; // texts with this scope lifetime
		bool savedCondResult;
		eLogicalOperation savedLogicalOp;
		bool savedNotFlag;
		std::string savedScriptFileDir;
		std::string savedScriptFileName;

		static const size_t store_size = 0x400;
		static ScmFunction *Store[store_size];
		static size_t allocationPlace;

		void *operator new(size_t size);

		void operator delete(void *mem);

		ScmFunction(CRunningScript *thread);

		void Return(CRunningScript *thread);
    };

    void ResetScmFunctionStore();
}
