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

		void *operator new(size_t size)
		{
			size_t start_search = allocationPlace;
			while (Store[allocationPlace])
			{
				if (++allocationPlace >= store_size) allocationPlace = 0;
				if (allocationPlace == start_search) throw std::bad_alloc();
			}
			ScmFunction *obj = reinterpret_cast<ScmFunction *>(::operator new(size));
			Store[allocationPlace] = obj;
			return obj;
		}

		void operator delete(void *mem)
		{
			Store[reinterpret_cast<ScmFunction *>(mem)->thisScmFunctionId] = nullptr;
			::operator delete(mem);
		}

		ScmFunction(CRunningScript *thread)
			: prevScmFunctionId(reinterpret_cast<CCustomScript*>(thread)->GetScmFunction()),
			  callArgCount(0),
			  callIP(thread->GetBytePointer()),
			  retnAddress(nullptr),
			  savedBaseIP(nullptr),
			  savedCodeSize(0),
			  savedSP(0),
			  savedCondResult(false),
			  savedLogicalOp(eLogicalOperation::NONE),
			  savedNotFlag(false)
		{
			auto cs = reinterpret_cast<CCustomScript*>(thread);

			// Full execution-scope snapshot, following the CLEO 5 model while
			// keeping the existing CLEO 4 CRunningScript layout intact.
			savedBaseIP = cs->GetBasePointer();
			savedCodeSize = cs->GetCodeSize();
			savedScriptFileDir = cs->GetScriptFileDir();
			savedScriptFileName = cs->GetScriptFileName();

			auto scope = cs->IsMission() ? missionLocals : cs->LocalVar;
			std::copy(scope, scope + 32, savedTls);
			std::copy(cs->Stack, cs->Stack + 8, savedStack);
			savedSP = cs->SP;
			savedCondResult = cs->bCondResult;
			savedLogicalOp = cs->LogicalOp;
			savedNotFlag = cs->NotFlag;

			// Start a clean function scope. 0AB1 supplies its local arguments
			// after this snapshot has been taken.
			std::fill(cs->Stack, cs->Stack + 8, nullptr);
			cs->SP = 0;
			cs->bCondResult = false;
			cs->LogicalOp = eLogicalOperation::NONE;
			cs->NotFlag = false;

			cs->SetScmFunction(thisScmFunctionId = static_cast<unsigned short>(allocationPlace));
		}

		void Return(CRunningScript *thread)
		{
			auto cs = reinterpret_cast<CCustomScript*>(thread);

			// Restore the complete caller execution context.
			cs->SetBaseIp(savedBaseIP);
			cs->SetCodeSize(savedCodeSize);
			cs->SetScriptFileDir(savedScriptFileDir.c_str());
			cs->SetScriptFileName(savedScriptFileName.c_str());

			std::copy(savedStack, savedStack + 8, cs->Stack);
			cs->SP = savedSP;
			std::copy(savedTls, savedTls + 32, cs->IsMission() ? missionLocals : cs->LocalVar);

			// Restore the caller's conditional aggregation state.
			bool condResult = cs->bCondResult;
			if (savedNotFlag) condResult = !condResult;

			if (savedLogicalOp >= eLogicalOperation::AND_2 && savedLogicalOp < eLogicalOperation::AND_END)
			{
				cs->bCondResult = savedCondResult && condResult;
				cs->LogicalOp = --savedLogicalOp;
			}
			else if (savedLogicalOp >= eLogicalOperation::OR_2 && savedLogicalOp < eLogicalOperation::OR_END)
			{
				cs->bCondResult = savedCondResult || condResult;
				cs->LogicalOp = --savedLogicalOp;
			}
			else
			{
				cs->bCondResult = condResult;
				cs->LogicalOp = savedLogicalOp;
			}

			cs->SetIp(retnAddress);
			cs->SetScmFunction(prevScmFunctionId);
		}
    };

    void ResetScmFunctionStore();
}
