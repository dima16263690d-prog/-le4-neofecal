#include "stdafx.h"
#include "ScmFunction.h"
#include "CCustomScript.h"
#include "CScriptEngine.h"

namespace CLEO
{
    ScmFunction* ScmFunction::Store[store_size] = { /* default initializer - nullptr */ };
    size_t ScmFunction::allocationPlace = 0;

    void ResetScmFunctionStore()
    {
        for (ScmFunction *scmFunc : ScmFunction::Store)
        {
            if (scmFunc) delete scmFunc;
        }
        ScmFunction::allocationPlace = 0;
    }
}
