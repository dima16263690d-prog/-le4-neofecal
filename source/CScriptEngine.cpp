#include "stdafx.h"
#include "cleo.h"

namespace CLEO
{
    DWORD FUNC_AddScriptToQueue;
    DWORD FUNC_RemoveScriptFromQueue;
    DWORD FUNC_StopScript;
    DWORD FUNC_ScriptOpcodeHandler00;
    DWORD FUNC_GetScriptParams;
    DWORD FUNC_TransmitScriptParams;
    DWORD FUNC_SetScriptParams;
    DWORD FUNC_SetScriptCondResult;
    DWORD FUNC_GetScriptParamPointer1;
    DWORD FUNC_GetScriptStringParam;
    DWORD FUNC_GetScriptParamPointer2;

    void(__thiscall * AddScriptToQueue)(CRunningScript *, CRunningScript **queue);
    void(__thiscall * RemoveScriptFromQueue)(CRunningScript *, CRunningScript **queue);
    void(__thiscall * StopScript)(CRunningScript *);
    char(__thiscall * ScriptOpcodeHandler00)(CRunningScript *, WORD opcode);
    void(__thiscall * GetScriptParams)(CRunningScript *, int count);
    void(__thiscall * TransmitScriptParams)(CRunningScript *, CRunningScript *);
    void(__thiscall * SetScriptParams)(CRunningScript *, int count);
    void(__thiscall * SetScriptCondResult)(CRunningScript *, bool);
    SCRIPT_VAR *	(__thiscall * GetScriptParamPointer1)(CRunningScript *);
    void(__thiscall * GetScriptStringParam)(CRunningScript *, char* buf, BYTE len);
    SCRIPT_VAR *	(__thiscall * GetScriptParamPointer2)(CRunningScript *, int __unused__);

	void RunScriptDeleteDelegate(CRunningScript *script);

    void __fastcall _AddScriptToQueue(CRunningScript *pScript, int dummy, CRunningScript **queue)
    {
        _asm
        {
            push queue
            mov ecx, pScript
            call FUNC_AddScriptToQueue
        }
    }

    void __fastcall _RemoveScriptFromQueue(CRunningScript *pScript, int dummy, CRunningScript **queue)
    {
        _asm
        {
            push queue
            mov ecx, pScript
            call FUNC_RemoveScriptFromQueue
        }
    }

    void __fastcall _StopScript(CRunningScript *pScript)
    {
        _asm
        {
            mov ecx, pScript
            call FUNC_StopScript
        }
    }

    char __fastcall _ScriptOpcodeHandler00(CRunningScript *pScript, int dummy, WORD opcode)
    {
        int result;
        _asm
        {
            push opcode
            mov ecx, pScript
            call FUNC_ScriptOpcodeHandler00
            mov result, eax
        }
        return result;
    }

    void __fastcall _GetScriptParams(CRunningScript *pScript, int dummy, int count)
    {
        _asm
        {
            mov ecx, pScript
            push count
            call FUNC_GetScriptParams
        }
    }

    void __fastcall _TransmitScriptParams(CRunningScript *pScript, int dummy, CRunningScript *pScriptB)
    {
        _asm
        {
            mov ecx, pScript
            push pScriptB
            call FUNC_TransmitScriptParams
        }
    }

    void __fastcall _SetScriptParams(CRunningScript *pScript, int dummy, int count)
    {
        _asm
        {
            mov ecx, pScript
            push count
            call FUNC_SetScriptParams
        }
    }

    void __fastcall _SetScriptCondResult(CRunningScript *pScript, int dummy, int val)
    {
        _asm
        {
            mov ecx, pScript
            push val
            call FUNC_SetScriptCondResult
        }
    }

    SCRIPT_VAR * __fastcall _GetScriptParamPointer1(CRunningScript *pScript)
    {
        SCRIPT_VAR *result;
        _asm
        {
            mov ecx, pScript
            call FUNC_GetScriptParamPointer1
            mov result, eax
        }
        return (SCRIPT_VAR*)((size_t)result + pScript->GetBasePointer());
    }

    void __fastcall _GetScriptStringParam(CRunningScript *pScript, int dummy, char *buf, int len)
    {
        _asm
        {
            mov ecx, pScript
            push len
            push buf
            call FUNC_GetScriptStringParam
        }
    }

    SCRIPT_VAR * __fastcall _GetScriptParamPointer2(CRunningScript *pScript, int dummy, int unused)
    {
        _asm
        {
            mov ecx, pScript
            push unused
            call FUNC_GetScriptParamPointer2
        }
    }

    void(__cdecl * InitScm)();
    void(__cdecl * SaveScmData)();
    void(__cdecl * LoadScmData)();
    void(__cdecl * DrawScriptStuff)(char bBeforeFade);
    void(__cdecl * DrawScriptStuff_H)(char bBeforeFade);

    DWORD* GameTimer;
    extern "C" {
        SCRIPT_VAR *opcodeParams;
        SCRIPT_VAR *missionLocals;
        CRunningScript *staticThreads;
    }

    BYTE *scmBlock;
    BYTE *MissionLoaded;
    BYTE *missionBlock;
    BOOL *onMissionFlag;
    CTexture *scriptSprites;
    BYTE *scriptDraws;
    WORD *numScriptDraws;
    WORD *numScriptTexts;
    BYTE *useTextCommands;
    BYTE *scriptTexts;

    CRunningScript **inactiveThreadQueue, **activeThreadQueue;
	CCustomScript *lastScriptCreated = nullptr;

    // called to initialise the scripts (after the main.scm has actually had a chance to set up)
    void OnInitScm1(void)
    {
        TRACE("Scripts initialized");
        GetInstance().ScriptEngine.RemoveAllCustomScripts();
        InitScm();
        GetInstance().TextManager.ClearDynamicFxts();
        GetInstance().OpcodeSystem.FinalizeScriptObjects();
        GetInstance().SoundSystem.UnloadAllStreams();
        GetInstance().ScriptEngine.LoadCustomScripts(false);
    }

    // called on first load before the others
    void OnInitScm2(void)
    {
        TRACE("Scripts exclusively initialized");
        GetInstance().ScriptEngine.RemoveAllCustomScripts();
        InitScm();
        GetInstance().TextManager.ClearDynamicFxts();
        GetInstance().OpcodeSystem.FinalizeScriptObjects();
        GetInstance().SoundSystem.UnloadAllStreams();
        GetInstance().ScriptEngine.LoadCustomScripts();
    }

    // called to load the scripts
    void OnInitScm3(void)
    {
        TRACE("Scripts loaded");
        GetInstance().ScriptEngine.RemoveAllCustomScripts();
        InitScm();
        GetInstance().TextManager.ClearDynamicFxts();
        GetInstance().OpcodeSystem.FinalizeScriptObjects();
        GetInstance().SoundSystem.UnloadAllStreams();
        GetInstance().ScriptEngine.LoadCustomScripts(true);
    }

    extern "C" void __stdcall opcode_004E(CCustomScript *pScript)
    {
        if (pScript->IsCustom())
        {
            if (!pScript->IsMission())
            {
                TRACE("[004E] Incorrect usage of opcode in script '%s'.", pScript->GetName());
            }
            else *MissionLoaded = false;
            GetInstance().ScriptEngine.RemoveCustomScript(pScript);
        }
        else
        {
            if (!pScript->IsMission()) *MissionLoaded = false;
            RemoveScriptFromQueue(pScript, activeThreadQueue);
            AddScriptToQueue(pScript, inactiveThreadQueue);
            StopScript(pScript);
        }
    }

    extern "C" void __declspec(naked) opcode_004E_hook(void)
    {
        __asm
        {
            push esi
            call opcode_004E
            pop edi
            mov al, 1
            pop esi
            mov ecx, [esp + 0x14]
            mov fs : 0, ecx
            add esp, 32
            ret 0x4
        }
    }

    void OnNewGame(void)
    {
        static struct CGangWeapons {
            BYTE _f0;
            BYTE _f1; // -
            DWORD weapon1;
            DWORD weapon2;
            DWORD weapon3;
        } *gangWeapons((CGangWeapons *)0xC0B870);	// 1.01 eu specific
        TRACE("New game started");
        gangWeapons[0].weapon1 = 22;
        gangWeapons[0].weapon2 = 28;
        gangWeapons[0].weapon3 = 0;

        gangWeapons[1].weapon1 = 22;
        gangWeapons[1].weapon2 = 0;
        gangWeapons[1].weapon3 = 0;

        gangWeapons[2].weapon1 = 22;
        gangWeapons[2].weapon2 = 0;
        gangWeapons[2].weapon3 = 0;

        gangWeapons[4].weapon1 = 24;
        gangWeapons[4].weapon2 = 28;
        gangWeapons[4].weapon3 = 0;

        gangWeapons[5].weapon1 = 24;
        gangWeapons[5].weapon2 = 0;
        gangWeapons[5].weapon3 = 0;

        gangWeapons[6].weapon1 = 22;
        gangWeapons[6].weapon2 = 30;
        gangWeapons[6].weapon3 = 0;

        gangWeapons[7].weapon1 = 22;
        gangWeapons[7].weapon2 = 28;
        gangWeapons[7].weapon3 = 0;
        GetInstance().TextManager.ClearDynamicFxts();
        GetInstance().OpcodeSystem.FinalizeScriptObjects();
        GetInstance().ScriptEngine.RemoveAllCustomScripts();
        GetInstance().SoundSystem.UnloadAllStreams();
        GetInstance().ScriptEngine.LoadCustomScripts();
    }

    void OnLoadScmData(void)
    {
        TRACE(__FUNCSIG__);
        LoadScmData();
    }

    void OnSaveScmData(void)
    {
        TRACE(__FUNCSIG__);
        GetInstance().ScriptEngine.SaveState();
        GetInstance().ScriptEngine.UnregisterAllScripts();
        SaveScmData();
        GetInstance().ScriptEngine.ReregisterAllScripts();
    }

    struct CleoSafeHeader
    {
        const static unsigned sign;
        unsigned signature;
        unsigned n_saved_threads;
        unsigned n_stopped_threads;
    };

    const unsigned CleoSafeHeader::sign = 0x31345653;

    struct ThreadSavingInfo
    {
        unsigned long hash;
        SCRIPT_VAR tls[32];
        unsigned timers[2];
        bool condResult;
        unsigned sleepTime;
        eLogicalOperation logicalOp;
        bool notFlag;
        ptrdiff_t ip_diff;
        char threadName[8];

        ThreadSavingInfo(CCustomScript *cs) :
            hash(cs->dwChecksum), condResult(cs->bCondResult),
            logicalOp(cs->LogicalOp), notFlag(cs->NotFlag != false), ip_diff(cs->CurrentIP - reinterpret_cast<BYTE*>(cs->BaseIP))
        {
            sleepTime = cs->WakeTime >= *GameTimer ? 0 : cs->WakeTime - *GameTimer;
            std::copy(cs->LocalVar, cs->LocalVar + 32, tls);
            std::copy(cs->Timers, cs->Timers + 2, timers);
            std::copy(cs->Name, cs->Name + 8, threadName);
        }

        void Apply(CCustomScript *cs)
        {
            cs->dwChecksum = hash;
            std::copy(tls, tls + 32, cs->LocalVar);
            std::copy(timers, timers + 2, cs->Timers);
            cs->bCondResult = condResult;
            cs->WakeTime = *GameTimer + sleepTime;
            cs->LogicalOp = logicalOp;
            cs->NotFlag = notFlag;
            cs->CurrentIP = reinterpret_cast<BYTE*>(cs->BaseIP) + ip_diff;
            std::copy(threadName, threadName + 8, cs->Name);
            cs->bSaveEnabled = true;
        }

        ThreadSavingInfo() { }
    };


    // Stage 3: child custom-script state is stored in a sidecar file so the
    // legacy cs*.sav binary layout remains byte-for-byte compatible.
    struct ChildSaveHeader
    {
        const static unsigned sign;
        const static unsigned format_version;
        unsigned signature;
        unsigned version;
        unsigned n_children;
    };

    const unsigned ChildSaveHeader::sign = 0x31484343; // CCH1
    const unsigned ChildSaveHeader::format_version = 1;

    struct ChildThreadSavingInfo
    {
        unsigned node_id;
        unsigned parent_node_id;
        int label;
        unsigned ordinal;
        ThreadSavingInfo state;

        ChildThreadSavingInfo() : node_id(0), parent_node_id(0), label(0), ordinal(0) {}
        ChildThreadSavingInfo(CCustomScript *cs, unsigned parentId, unsigned nodeId, unsigned childOrdinal)
            : node_id(nodeId), parent_node_id(parentId), label(cs->GetChildLabel()), ordinal(childOrdinal), state(cs)
        {
        }

        void Apply(CCustomScript *cs)
        {
            state.Apply(cs);
            // Child streams are represented by the sidecar, not by the legacy
            // hash-only saved-thread list.
            cs->enable_saving(false);
        }
    };

    SCRIPT_VAR CScriptEngine::CleoVariables[0x400];

    template<typename T>
    void inline ReadBinary(std::istream& s, T& buf)
    {
        s.read(reinterpret_cast<char *>(&buf), sizeof(T));
    }

    template<typename T>
    void inline ReadBinary(std::istream& s, T *buf, size_t size)
    {
        s.read(reinterpret_cast<char *>(buf), sizeof(T) * size);
    }

    template<typename T>
    void inline WriteBinary(std::ostream& s, const T& data)
    {
        s.write(reinterpret_cast<const char *>(&data), sizeof(T));
    }

    template<typename T>
    void inline WriteBinary(std::ostream& s, const T*data, size_t size)
    {
        s.write(reinterpret_cast<const char *>(data), sizeof(T) * size);
    }

    void __fastcall HOOK_ProcessScript(CCustomScript * pScript, int)
    {
        if (pScript->IsCustom()) pScript->Process();
        else ProcessScript(pScript);
    }

    void HOOK_DrawScriptStuff(char bBeforeFade)
    {
        GetInstance().ScriptEngine.DrawScriptStuff(bBeforeFade);

        // restore SCM textures and return to the overwritten func (which may != DrawScriptSprites)
        return bBeforeFade ? DrawScriptStuff_H(bBeforeFade) : DrawScriptStuff(bBeforeFade);
    }

#define NUM_STORED_SPRITES 128
#define NUM_STORED_DRAWS 128
#define NUM_STORED_TEXTS 96
#define DRAW_DATA_SIZE 60
#define TEXT_DATA_SIZE 68
#define DRAW_ARRAY_SIZE NUM_STORED_DRAWS*DRAW_DATA_SIZE
#define TEXT_ARRAY_SIZE NUM_STORED_TEXTS*TEXT_DATA_SIZE
    CTexture storedSprites[NUM_STORED_SPRITES];
    BYTE storedDraws[DRAW_ARRAY_SIZE];
    BYTE storedTexts[TEXT_ARRAY_SIZE];
    BYTE storedUseTextCommands = 0;
    WORD numStoredDraws = 0;
    WORD numStoredTexts = 0;

    static void RestoreTextDrawDefaults()
    {
        for (int i = 0; i<NUM_STORED_TEXTS; ++i)
        {
            CTextDrawer * pText = (CTextDrawer*)&scriptTexts[i*TEXT_DATA_SIZE];
            pText->m_fScaleX = 0.48f;
            pText->m_fScaleY = 1.12f;
            pText->m_Colour = CRGBA(0xE1, 0xE1, 0xE1, 0xFF);
            pText->m_bJustify = false;
            pText->m_bAlignRight = false;
            pText->m_bCenter = false;
            pText->m_bBackground = false;
            pText->m_bUnk1 = false;
            pText->m_fLineHeight = 182.0f;
            pText->m_fLineWidth = 640.0f;
            pText->m_BackgroundColour = CRGBA(0x80, 0x80, 0x80, 0x80);
            pText->m_bProportional = true;
            pText->m_EffectColour = CRGBA(0, 0, 0, 0xFF);
            strncpy(pText->m_szGXT, "", 8);
            pText->m_ucShadow = 2;
            pText->m_ucOutline = 0;
            pText->m_bDrawBeforeFade = false;
            pText->m_nFont = 1;
            pText->m_fPosX = 0.0;
            pText->m_fPosY = 0.0;
            pText->m_nParam1 = -1;
            pText->m_nParam2 = -1;
        }
    }

    void CScriptEngine::DrawScriptStuff(char bBeforeFade)
    {
        for (auto i = CustomScripts.begin(); i != CustomScripts.end(); ++i)
        {
            auto script = *i;
            script->Draw(bBeforeFade);
        }
        if (auto script = GetCustomMission())
            script->Draw(bBeforeFade);
    }
    void CCustomScript::Process()
    {
        RestoreScriptSpecifics();

        bool bNeedDefaults = false;
        if (*useTextCommands)
        {
            RestoreTextDrawDefaults();
            *numScriptTexts = 0;
            std::fill(scriptDraws, scriptDraws + DRAW_ARRAY_SIZE, 0);
            *numScriptDraws = 0;
            if (*useTextCommands == 1)
                *useTextCommands = 0;
        }
		
		ProcessScript(this);

        StoreScriptSpecifics();
    }
    void CCustomScript::Draw(char bBeforeFade)
    {
        // no point if this script doesn't draw
        if (script_draws.size() || script_texts.size())
        {
            static CCustomScript * last;
            last = this;
            RestoreScriptDraws();
            RestoreScriptTextures();
            if (bBeforeFade) DrawScriptStuff_H(bBeforeFade);
            else DrawScriptStuff(bBeforeFade);
            StoreScriptDraws();
            StoreScriptTextures();
        }
    }
    void CCustomScript::StoreScriptDraws()
    {
        // store this scripts draws + texts
        if (*numScriptDraws)
            script_draws.assign(scriptDraws, scriptDraws + (*numScriptDraws * DRAW_DATA_SIZE));
        else if (script_draws.size())
            script_draws.clear();
        if (*numScriptTexts)
            script_texts.assign(scriptTexts, scriptTexts + (*numScriptTexts * TEXT_DATA_SIZE));
        else if (script_texts.size())
            script_texts.clear();

        UseTextCommands = *useTextCommands;
        NumDraws = *numScriptDraws;
        NumTexts = *numScriptTexts;

        // restore SCM draws + texts
        if (numStoredDraws) std::copy(storedDraws, storedDraws + (numStoredDraws * DRAW_DATA_SIZE), scriptDraws);
        else std::fill(scriptDraws, scriptDraws + DRAW_ARRAY_SIZE, 0);
        if (numStoredTexts) std::copy(storedTexts, storedTexts + (numStoredTexts * TEXT_DATA_SIZE), scriptTexts);
        else RestoreTextDrawDefaults();
        *numScriptDraws = numStoredDraws;
        *numScriptTexts = numStoredTexts;
        *useTextCommands = storedUseTextCommands;
    }
    void CCustomScript::RestoreScriptDraws()
    {
        // store SCM draws + texts
        storedUseTextCommands = *useTextCommands;
        numStoredDraws = *numScriptDraws;
        numStoredTexts = *numScriptTexts;
        if (numStoredDraws)
            std::copy(scriptDraws, scriptDraws + (numStoredDraws *  DRAW_DATA_SIZE), storedDraws);
        if (numStoredTexts)
            std::copy(scriptTexts, scriptTexts + (numStoredTexts * TEXT_DATA_SIZE), storedTexts);

        // restore script draws + texts
        if (!script_draws.size()) *numScriptDraws = 0;
        else
        {
            std::copy(script_draws.begin(), script_draws.end(), scriptDraws);
            *numScriptDraws = NumDraws;
        }
        if (!script_texts.size()) *numScriptTexts = 0;
        else
        {
            std::copy(script_texts.begin(), script_texts.end(), scriptTexts);
            *numScriptTexts = NumTexts;
        }
        *useTextCommands = UseTextCommands;
    }
    void CCustomScript::StoreScriptTextures()
    {
        // store this scripts textures + restore SCM textures + make sure this scripts textures arent cleared by another
        if (script_textures.size())
            script_textures.clear();
        for (int i = 0; i<NUM_STORED_SPRITES; ++i)
        {
            script_textures.push_back(*(RwTexture**)&scriptSprites[i]);
            scriptSprites[i] = storedSprites[i];
        }

        //std::copy(scriptSprites, scriptSprites + NUM_STORED_SPRITES, storedSprites);
    }
    void CCustomScript::RestoreScriptTextures()
    {
        int n = 0;

        // store SCM textures
        for (int i = 0; i<NUM_STORED_SPRITES; ++i)
        {
            storedSprites[i] = scriptSprites[i];
        }
        //std::copy(scriptSprites, scriptSprites + NUM_STORED_SPRITES, storedSprites);

        // ensure SCM textures arent cleared - except by the SCM
        if (!script_textures.size())
            std::fill((RwTexture**)scriptSprites, (RwTexture**)scriptSprites + NUM_STORED_SPRITES, nullptr);
        else
        {
            // restore textures for this script
            for (auto i = script_textures.begin(); i != script_textures.end(); ++i, ++n)
            {
                if (n >= NUM_STORED_SPRITES) break;
                *(RwTexture**)(&scriptSprites[n]) = *i;
            }
        }
    }
    void CCustomScript::StoreScriptSpecifics()
    {
        StoreScriptDraws();
        StoreScriptTextures();
    }
    void CCustomScript::RestoreScriptSpecifics()
    {
        RestoreScriptDraws();
        RestoreScriptTextures();
    }

    void CScriptEngine::Inject(CCodeInjector& inj)
    {
        TRACE("Injecting ScriptEngine...");
        CGameVersionManager& gvm = GetInstance().VersionManager;

        // Global Events crashfix
        //inj.MemoryWrite(0xA9AF6C, 0, 4);

        // Dirty hacks to keep compatibility with plugins + overcome VS thiscall restrictions
        FUNC_AddScriptToQueue = gvm.TranslateMemoryAddress(MA_ADD_SCRIPT_TO_QUEUE_FUNCTION);
        FUNC_RemoveScriptFromQueue = gvm.TranslateMemoryAddress(MA_REMOVE_SCRIPT_FROM_QUEUE_FUNCTION);
        FUNC_StopScript = gvm.TranslateMemoryAddress(MA_STOP_SCRIPT_FUNCTION);
        FUNC_ScriptOpcodeHandler00 = gvm.TranslateMemoryAddress(MA_SCRIPT_OPCODE_HANDLER0_FUNCTION);
        FUNC_GetScriptParams = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_PARAMS_FUNCTION);
        FUNC_TransmitScriptParams = gvm.TranslateMemoryAddress(MA_TRANSMIT_SCRIPT_PARAMS_FUNCTION);
        FUNC_SetScriptParams = gvm.TranslateMemoryAddress(MA_SET_SCRIPT_PARAMS_FUNCTION);
        FUNC_SetScriptCondResult = gvm.TranslateMemoryAddress(MA_SET_SCRIPT_COND_RESULT_FUNCTION);
        FUNC_GetScriptParamPointer1 = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_PARAM_POINTER1_FUNCTION);
        FUNC_GetScriptStringParam = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_STRING_PARAM_FUNCTION);
        FUNC_GetScriptParamPointer2 = gvm.TranslateMemoryAddress(MA_GET_SCRIPT_PARAM_POINTER2_FUNCTION);

        AddScriptToQueue = reinterpret_cast<void(__thiscall*)(CRunningScript*, CRunningScript**)>(_AddScriptToQueue);
        RemoveScriptFromQueue = reinterpret_cast<void(__thiscall*)(CRunningScript*, CRunningScript**)>(_RemoveScriptFromQueue);
        StopScript = reinterpret_cast<void(__thiscall*)(CRunningScript*)>(_StopScript);
        ScriptOpcodeHandler00 = reinterpret_cast<char(__thiscall*)(CRunningScript*, WORD)>(_ScriptOpcodeHandler00);
        GetScriptParams = reinterpret_cast<void(__thiscall*)(CRunningScript*, int)>(_GetScriptParams);
        TransmitScriptParams = reinterpret_cast<void(__thiscall*)(CRunningScript*, CRunningScript*)>(_TransmitScriptParams);
        SetScriptParams = reinterpret_cast<void(__thiscall*)(CRunningScript*, int)>(_SetScriptParams);
        SetScriptCondResult = reinterpret_cast<void(__thiscall*)(CRunningScript*, bool)>(_SetScriptCondResult);
        GetScriptParamPointer1 = reinterpret_cast<SCRIPT_VAR * (__thiscall*)(CRunningScript*)>(_GetScriptParamPointer1);
        GetScriptStringParam = reinterpret_cast<void(__thiscall*)(CRunningScript*, char*, BYTE)>(_GetScriptStringParam);
        GetScriptParamPointer2 = reinterpret_cast<SCRIPT_VAR * (__thiscall*)(CRunningScript*, int)>(_GetScriptParamPointer2);

        InitScm = gvm.TranslateMemoryAddress(MA_INIT_SCM_FUNCTION);
        SaveScmData = gvm.TranslateMemoryAddress(MA_SAVE_SCM_DATA_FUNCTION);
        LoadScmData = gvm.TranslateMemoryAddress(MA_LOAD_SCM_DATA_FUNCTION);

        GameTimer = gvm.TranslateMemoryAddress(MA_GAME_TIMER);
        opcodeParams = gvm.TranslateMemoryAddress(MA_OPCODE_PARAMS);
        missionLocals = gvm.TranslateMemoryAddress(MA_MISSION_LOCALS);
        scmBlock = gvm.TranslateMemoryAddress(MA_SCM_BLOCK);
        MissionLoaded = gvm.TranslateMemoryAddress(MA_MISSION_LOADED);
        missionBlock = gvm.TranslateMemoryAddress(MA_MISSION_BLOCK);
        onMissionFlag = gvm.TranslateMemoryAddress(MA_ON_MISSION_FLAG);

        // Protect script dependencies
        auto addr = gvm.TranslateMemoryAddress(MA_CALL_PROCESS_SCRIPT);
        inj.MemoryReadOffset(addr.address + 1, ProcessScript);
        inj.ReplaceFunction(HOOK_ProcessScript, addr);

        scriptSprites = gvm.TranslateMemoryAddress(MA_SCRIPT_SPRITE_ARRAY);
        scriptDraws = gvm.TranslateMemoryAddress(MA_SCRIPT_DRAW_ARRAY);
        scriptTexts = gvm.TranslateMemoryAddress(MA_SCRIPT_TEXT_ARRAY);
        numScriptDraws = gvm.TranslateMemoryAddress(MA_NUM_SCRIPT_DRAWS);
        numScriptTexts = gvm.TranslateMemoryAddress(MA_NUM_SCRIPT_TEXTS);
        useTextCommands = gvm.TranslateMemoryAddress(MA_USE_TEXT_COMMANDS);

        inj.MemoryReadOffset(gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE).address + 1, CLEO::DrawScriptStuff);
        inj.MemoryReadOffset(gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE).address + 1, DrawScriptStuff_H);
        inj.ReplaceFunction(HOOK_DrawScriptStuff, gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_AFTER_FADE));
        inj.ReplaceFunction(HOOK_DrawScriptStuff, gvm.TranslateMemoryAddress(MA_CALL_DRAW_SCRIPT_TEXTS_BEFORE_FADE));
        inj.MemoryWrite(gvm.TranslateMemoryAddress(MA_CODE_JUMP_FOR_TXD_STORE), OP_RET);

        inactiveThreadQueue = gvm.TranslateMemoryAddress(MA_INACTIVE_THREAD_QUEUE);
        activeThreadQueue = gvm.TranslateMemoryAddress(MA_ACTIVE_THREAD_QUEUE);
        staticThreads = gvm.TranslateMemoryAddress(MA_STATIC_THREADS);

        if (gvm.GetGameVersion() == GV_EU11)
        {
            inj.ReplaceFunction(OnInitScm3, gvm.TranslateMemoryAddress(MA_CALL_INIT_SCM3));
            inj.InjectFunction(OnNewGame, 0x5DEEA0);	// GV_EU11 specific
        }
        else
        {
            inj.ReplaceFunction(OnInitScm1, gvm.TranslateMemoryAddress(MA_CALL_INIT_SCM1));
            inj.ReplaceFunction(OnInitScm2, gvm.TranslateMemoryAddress(MA_CALL_INIT_SCM2));
            inj.ReplaceFunction(OnInitScm3, gvm.TranslateMemoryAddress(MA_CALL_INIT_SCM3));
        }

        inj.ReplaceFunction(OnLoadScmData, gvm.TranslateMemoryAddress(MA_CALL_LOAD_SCM_DATA));
        inj.ReplaceFunction(OnSaveScmData, gvm.TranslateMemoryAddress(MA_CALL_SAVE_SCM_DATA));
        inj.InjectFunction(&opcode_004E_hook, gvm.TranslateMemoryAddress(MA_OPCODE_004E));
    }

    CleoSafeHeader safe_header;
    ThreadSavingInfo *safe_info;
    unsigned long *stopped_info;
    std::unique_ptr<ThreadSavingInfo[]> safe_info_utilizer;
    std::unique_ptr<unsigned long[]> stopped_info_utilizer;
    std::vector<ChildThreadSavingInfo> pendingChildSaves;
    std::vector<bool> safeInfoUsed;

    void CScriptEngine::RestorePendingChildScript(CCustomScript *parent, CCustomScript *child, int label)
    {
        if (!parent || !child || parent->savedNodeId == 0)
            return;

        unsigned ordinal = 0;
        for (auto sibling : parent->childThreads)
        {
            if (sibling->childLabel != label)
                continue;

            if (sibling == child)
                break;

            ++ordinal;
        }

        for (auto it = pendingChildSaves.begin(); it != pendingChildSaves.end(); ++it)
        {
            if (it->parent_node_id != parent->savedNodeId ||
                it->label != label ||
                it->ordinal != ordinal)
            {
                continue;
            }

            const unsigned nodeId = it->node_id;
            it->Apply(child);
            child->savedNodeId = nodeId;

            TRACE("Restored custom child script '%s' from sidecar parent=%08X node=%u label=%d ordinal=%u",
                child->Name, parent->savedNodeId, nodeId, label, ordinal);

            pendingChildSaves.erase(it);
            return;
        }
    }

    void CScriptEngine::RestorePendingChildTree(CCustomScript *parent)
    {
        if (!parent || parent->savedNodeId == 0)
            return;

        for (;;)
        {
            size_t found = pendingChildSaves.size();

            // Restore children in ordinal order for each label. This keeps
            // duplicate labels (for example two 0E6F streams at the same
            // label) deterministic.
            for (size_t i = 0; i < pendingChildSaves.size(); ++i)
            {
                if (pendingChildSaves[i].parent_node_id != parent->savedNodeId)
                    continue;

                bool lowerOrdinalPending = false;
                for (size_t j = 0; j < pendingChildSaves.size(); ++j)
                {
                    if (pendingChildSaves[j].parent_node_id == parent->savedNodeId &&
                        pendingChildSaves[j].label == pendingChildSaves[i].label &&
                        pendingChildSaves[j].ordinal < pendingChildSaves[i].ordinal)
                    {
                        lowerOrdinalPending = true;
                        break;
                    }
                }

                if (!lowerOrdinalPending)
                {
                    found = i;
                    break;
                }
            }

            if (found == pendingChildSaves.size())
                break;

            ChildThreadSavingInfo saved = pendingChildSaves[found];
            pendingChildSaves.erase(pendingChildSaves.begin() + found);

            auto child = new CCustomScript(parent->Name, false, parent, saved.label);
            if (!child || !child->IsOK())
            {
                if (child)
                    delete child;

                DIAG("[CLEO][ERROR][CUSTOM] restore failed parent_node=%08X node=%u label=%d ordinal=%u",
                    parent->savedNodeId, saved.node_id, saved.label, saved.ordinal);
                continue;
            }

            AddCustomScript(child);
            saved.Apply(child);
            child->savedNodeId = saved.node_id;

            DIAG("[CLEO][CUSTOM][RESTORE] parent=%.*s parent_node=%08X node=%u label=%d ordinal=%u",
                8, parent->Name, parent->savedNodeId, saved.node_id, saved.label, saved.ordinal);

            RestorePendingChildTree(child);
        }
    }

    void CScriptEngine::LoadCustomScripts(bool load_mode)
    {
        char safe_name[MAX_PATH];

        // steam offset is different, so get it manually for now
        CGameVersionManager& gvm = GetInstance().VersionManager;
        int nSlot = gvm.GetGameVersion() != GV_STEAM ? *(BYTE*)&MenuManager->m_nSelectedSaveGame : *((BYTE*)MenuManager + 0x15B);

        sprintf(safe_name, "./cleo/cleo_saves/cs%d.sav", nSlot);

        safe_info = nullptr;
        stopped_info = nullptr;
        safe_header.n_saved_threads = safe_header.n_stopped_threads = 0;
        pendingChildSaves.clear();
        safeInfoUsed.clear();

        if (load_mode)
        {
            // load cleo saving file
            try
            {
                TRACE("Loading cleo safe %s", safe_name);
                std::ifstream ss(safe_name, std::ios::binary);
                if (ss.is_open())
                {
                    ss.exceptions(std::ios::eofbit | std::ios::badbit | std::ios::failbit);
                    ReadBinary(ss, safe_header);
                    if (safe_header.signature != CleoSafeHeader::sign)
                        throw std::runtime_error("Invalid file format");
                    safe_info = new ThreadSavingInfo[safe_header.n_saved_threads];
                    safe_info_utilizer.reset(safe_info);
                    stopped_info = new unsigned long[safe_header.n_stopped_threads];
                    stopped_info_utilizer.reset(stopped_info);
                    ReadBinary(ss, CleoVariables, 0x400);
                    ReadBinary(ss, safe_info, safe_header.n_saved_threads);
                    ReadBinary(ss, stopped_info, safe_header.n_stopped_threads);
                    safeInfoUsed.assign(safe_header.n_saved_threads, false);
                    for (size_t i = 0; i < safe_header.n_stopped_threads; ++i)
                        InactiveScriptHashes.insert(stopped_info[i]);
                    TRACE("Finished. Loaded %u cleo variables, %u saved threads info, %u stopped threads info",
                        0x400, safe_header.n_saved_threads, safe_header.n_stopped_threads);
                }
                else
                {
                    memset(CleoVariables, 0, sizeof(CleoVariables));
                }
            }
            catch (std::exception& ex)
            {
                TRACE("Loading of cleo safe %s failed: %s", safe_name, ex.what());
                safe_header.n_saved_threads = safe_header.n_stopped_threads = 0;
                memset(CleoVariables, 0, sizeof(CleoVariables));
            }
        }
        else
        {
            memset(CleoVariables, 0, sizeof(CleoVariables));
        }

        if (load_mode)
        {
            try
            {
                int nSlot = gvm.GetGameVersion() != GV_STEAM ? *(BYTE*)&MenuManager->m_nSelectedSaveGame : *((BYTE*)MenuManager + 0x15B);
                char child_safe_name[MAX_PATH];
                _snprintf_s(child_safe_name, sizeof(child_safe_name), _TRUNCATE,
                    "./cleo/cleo_saves/cs%d.children.sav", nSlot);

                std::ifstream cs(child_safe_name, std::ios::binary);
                if (cs.is_open())
                {
                    cs.exceptions(std::ios::eofbit | std::ios::badbit | std::ios::failbit);

                    ChildSaveHeader header{};
                    ReadBinary(cs, header);
                    if (header.signature != ChildSaveHeader::sign || header.version != ChildSaveHeader::format_version)
                        throw std::runtime_error("Invalid child save format");

                    pendingChildSaves.resize(header.n_children);
                    if (header.n_children)
                        ReadBinary(cs, pendingChildSaves.data(), header.n_children);

                    DIAG("[CLEO][LOAD][CUSTOM] loaded child states=%u file=%s", header.n_children, child_safe_name);
                }
            }
            catch (std::exception& ex)
            {
                pendingChildSaves.clear();
                DIAG("[CLEO][ERROR][LOAD] child state load failed: %s", ex.what());
            }
        }

        char cwd[MAX_PATH];
        _getcwd(cwd, sizeof(cwd));
        _chdir(cleo_dir);

        TRACE("Searching for cleo scripts");

        FilesWalk(cs_mask, [this](const char *filename) {
            auto cs = LoadScript(filename);
            if (cs)
                RestorePendingChildTree(cs);
        });
        FilesWalk(cs4_mask, [this](const char *filename) {
            auto cs = LoadScript(filename);
            if (cs)
            {
                cs->SetCompatibility(CLEO_VER_4);
                RestorePendingChildTree(cs);
            }
        });
        FilesWalk(cs3_mask, [this](const char *filename) {
            auto cs = LoadScript(filename);
            if (cs)
            {
                cs->SetCompatibility(CLEO_VER_3);
                RestorePendingChildTree(cs);
            }
        });

        _chdir(cwd);
    }

    CCustomScript * CScriptEngine::LoadScript(const char * szFilePath)
    {
        auto cs = new CCustomScript(szFilePath);

        if (!cs || !cs->bOK)
        {
            TRACE("Loading of custom script %s failed", szFilePath);
            if (cs) delete cs;
            return nullptr;
        }

        // check whether the script is in stop-list
        if (stopped_info)
        {
            for (size_t i = 0; i < safe_header.n_stopped_threads; ++i)
            {
                if (stopped_info[i] == cs->dwChecksum)
                {
                    TRACE("Custom script %s found in the stop-list", szFilePath);
                    InactiveScriptHashes.insert(stopped_info[i]);
                    delete cs;
                    return nullptr;
                }
            }
        }

        // check whether the script is in safe-list
        if (safe_info)
        {
            for (size_t i = 0; i < safe_header.n_saved_threads; ++i)
            {
                if (safeInfoUsed.size() > i && safeInfoUsed[i])
                    continue;

                if (safe_info[i].hash == cs->dwChecksum)
                {
                    TRACE("Custom script %s found in the safe-list", szFilePath);
                    safe_info[i].Apply(cs);

                    if (safeInfoUsed.size() > i)
                    {
                        safeInfoUsed[i] = true;
                        cs->savedNodeId = 0x80000000u | static_cast<unsigned>(i + 1);
                    }
                    break;
                }
            }
        }

        AddCustomScript(cs);
        return cs;
    }

    void CScriptEngine::SaveState()
    {
        try
        {
            std::list<CCustomScript *> savedThreads;
            std::for_each(CustomScripts.begin(), CustomScripts.end(), [this, &savedThreads](CCustomScript *cs) {
                if ((cs->bSaveEnabled || !cs->childThreads.empty()) && cs->parentThread == nullptr)
                    savedThreads.push_back(cs);
            });

            CleoSafeHeader header = { CleoSafeHeader::sign, savedThreads.size(), InactiveScriptHashes.size() };

            // Assign stable node ids for this save operation. Root scripts use
            // the legacy saved-thread index; child scripts use a separate id
            // space stored only in the sidecar file.
            unsigned rootIndex = 0;
            for (auto cs : savedThreads)
                cs->savedNodeId = 0x80000000u | (++rootIndex);

            std::vector<ChildThreadSavingInfo> childSaves;
            unsigned nextChildNodeId = 1;

            auto collectChildren = [&](auto&& self, CCustomScript *parent, unsigned parentNodeId) -> void
            {
                for (auto child : parent->childThreads)
                {
                    unsigned ordinal = 0;
                    for (auto sibling : parent->childThreads)
                    {
                        if (sibling->childLabel != child->childLabel)
                            continue;
                        if (sibling == child)
                            break;
                        ++ordinal;
                    }

                    const unsigned nodeId = nextChildNodeId++;
                    child->savedNodeId = nodeId;
                    childSaves.emplace_back(child, parentNodeId, nodeId, ordinal);
                    self(self, child, nodeId);
                }
            };

            for (auto root : savedThreads)
                collectChildren(collectChildren, root, root->savedNodeId);

            // steam offset is different, so get it manually for now
            CGameVersionManager& gvm = GetInstance().VersionManager;
            int nSlot = gvm.GetGameVersion() != GV_STEAM ? *(BYTE*)&MenuManager->m_nSelectedSaveGame : *((BYTE*)MenuManager + 0x15B);

            char safe_name[MAX_PATH];
            char child_safe_name[MAX_PATH];
            sprintf(safe_name, "./cleo/cleo_saves/cs%d.sav", nSlot);
            _snprintf_s(child_safe_name, sizeof(child_safe_name), _TRUNCATE,
                "./cleo/cleo_saves/cs%d.children.sav", nSlot);
            TRACE("Saving script engine state to the file %s", safe_name);

            CreateDirectory("cleo", NULL);
            CreateDirectory("cleo/cleo_saves", NULL);
            std::ofstream ss(safe_name, std::ios::binary);
            if (ss.is_open())
            {
                ss.exceptions(std::ios::failbit | std::ios::badbit);

                WriteBinary(ss, header);
                WriteBinary(ss, CleoVariables, 0x400);

                std::for_each(savedThreads.begin(), savedThreads.end(), [&savedThreads, &ss](CCustomScript *cs)
                {
                    ThreadSavingInfo savingInfo(cs);
                    WriteBinary(ss, savingInfo);
                });

                std::for_each(InactiveScriptHashes.begin(), InactiveScriptHashes.end(), [&ss](unsigned long hash) {
                    WriteBinary(ss, hash);
                });

                TRACE("Done. Saved %u cleo variables, %u saved threads, %u stopped threads",
                    0x400, header.n_saved_threads, header.n_stopped_threads);
            }
            else
            {
                TRACE("Failed to write save file '%s'!", safe_name);
            }

            try
            {
                std::ofstream childFile(child_safe_name, std::ios::binary);
                if (childFile.is_open())
                {
                    childFile.exceptions(std::ios::failbit | std::ios::badbit);

                    ChildSaveHeader childHeader = {
                        ChildSaveHeader::sign,
                        ChildSaveHeader::format_version,
                        childSaves.size()
                    };

                    WriteBinary(childFile, childHeader);
                    if (!childSaves.empty())
                        WriteBinary(childFile, childSaves.data(), childSaves.size());

                    DIAG("[CLEO][SAVE][CUSTOM] saved child states=%u file=%s",
                        childHeader.n_children, child_safe_name);
                }
                else
                {
                    DIAG("[CLEO][ERROR][SAVE] child state file write failed file=%s", child_safe_name);
                }
            }
            catch (std::exception& ex)
            {
                DIAG("[CLEO][ERROR][SAVE] child state save failed: %s", ex.what());
            }
        }
        catch (std::exception& ex)
        {
            TRACE("Saving failed. %s", ex.what());
        }
    }

    CRunningScript *CScriptEngine::FindScriptNamed(const char *name)
    {
        for (auto script = *activeThreadQueue; script; script = script->GetNext())
        {
            if (_stricmp(name, script->GetName()) == 0)
                return script;
        }
        return nullptr;
    }
    CCustomScript *CScriptEngine::FindCustomScriptNamed(const char *name)
    {
        if (CustomMission)
        {
            if (_stricmp(name, CustomMission->Name) == 0) return CustomMission;
        }

        for (auto it = CustomScripts.begin(); it != CustomScripts.end(); ++it)
        {
            auto cs = *it;
            if (_stricmp(name, cs->Name) == 0)
                return cs;
        }

        return nullptr;
    }

    void CScriptEngine::AddCustomScript(CCustomScript *cs)
    {
        if (cs->IsMission())
        {
            TRACE("Registering custom mission named %.*s", 8, cs->Name);
            DIAG("[CLEO][CUSTOM][REGISTER] mission name=%.*s", 8, cs->Name);
            CustomMission = cs;
        }
        else
        {
            TRACE("Registering custom script named %.*s", 8, cs->Name);
            DIAG("[CLEO][CUSTOM][REGISTER] name=%.*s parent=%p label=%d", 8, cs->Name, cs->parentThread, cs->childLabel);
            CustomScripts.push_back(cs);
        }
        AddScriptToQueue(cs, activeThreadQueue);
        cs->SetActive(true);
    }

    void CScriptEngine::RemoveCustomScript(CCustomScript *cs)
    {
        const bool wasChild = cs->parentThread != nullptr;

        // Detach this script from its parent first so removing a child cannot
        // leave a stale pointer in the parent's child list.
        if (cs->parentThread)
        {
            cs->parentThread->childThreads.remove(cs);
            cs->parentThread = nullptr;
        }

        // Remove children one by one. RemoveCustomScript(child) detaches the
        // child from this list, so iterating with an explicit front() is safe.
        while (!cs->childThreads.empty())
        {
            CCustomScript *childThread = cs->childThreads.front();
            CScriptEngine::RemoveCustomScript(childThread);
        }

        if (cs == CustomMission)
        {
            TRACE("Unregistering custom mission named %.*s", 8, cs->Name);
            DIAG("[CLEO][CUSTOM][END] mission name=%.*s", 8, cs->Name);
            RemoveScriptFromQueue(CustomMission, activeThreadQueue);
            ScriptsWaitingForDelete.push_back(cs);
            CustomMission->SetActive(false);
            CustomMission = nullptr;
            *MissionLoaded = false;
        }
        else
        {
            if (cs->bSaveEnabled && !wasChild)
            {
                InactiveScriptHashes.insert(cs->dwChecksum);
                TRACE("Stopping custom script named %.*s", 8, cs->Name);
                DIAG("[CLEO][CUSTOM][STOP] name=%.*s parent=%p label=%d", 8, cs->Name, cs->parentThread, cs->childLabel);
            }
            else
            {
                TRACE("Unregistering custom script named %.*s", 8, cs->Name);
                DIAG("[CLEO][CUSTOM][END] name=%.*s parent=%p label=%d", 8, cs->Name, cs->parentThread, cs->childLabel);
                ScriptsWaitingForDelete.push_back(cs);
            }

            CustomScripts.remove(cs);
            RemoveScriptFromQueue(cs, activeThreadQueue);
            cs->SetActive(false);
        }
    }

    void CScriptEngine::RemoveAllCustomScripts(void)
    {
        InactiveScriptHashes.clear();

        if (CustomMission)
            RemoveCustomScript(CustomMission);

        while (!CustomScripts.empty())
            RemoveCustomScript(CustomScripts.back());

        for (auto cs : ScriptsWaitingForDelete)
        {
            TRACE("Deleting inactive script named %.*s", 8, cs->Name);
            DIAG("[CLEO][CUSTOM][DELETE] name=%.*s", 8, cs->Name);
            delete cs;
        }
        ScriptsWaitingForDelete.clear();
    }

    void CScriptEngine::UnregisterAllScripts()
    {
        TRACE("Unregistering all custom scripts");
        DIAG("[CLEO][CUSTOM][UNREGISTER_ALL]");
        std::for_each(CustomScripts.begin(), CustomScripts.end(), [this](CCustomScript *cs) {
            RemoveScriptFromQueue(cs, activeThreadQueue);
            cs->SetActive(false);
        });
    }

    void CScriptEngine::ReregisterAllScripts()
    {
        TRACE("Reregistering all custom scripts");
        DIAG("[CLEO][CUSTOM][REREGISTER_ALL]");
        std::for_each(CustomScripts.begin(), CustomScripts.end(), [this](CCustomScript *cs) {
            AddScriptToQueue(cs, activeThreadQueue);
            cs->SetActive(true);
        });
    }

	// TODO: Consider split into 2 classes: CCustomExternalScript, CCustomChildScript
    CCustomScript::CCustomScript(const char *szFileName, bool bIsMiss, CCustomScript *parent, int label)
        : CRunningScript(), ownedBuffer(nullptr), bSaveEnabled(false), bOK(false),
        LastSearchPed(0), LastSearchCar(0), LastSearchObj(0),
        CompatVer(CLEO_VERSION), parentThread(nullptr), childLabel(label), savedNodeId(0)
    {
        IsCustom(1);
        bIsMission = bUseMissionCleanup = bIsMiss;
        UseTextCommands = 0;
        NumDraws = 0;
        NumTexts = 0;

        TRACE("Loading custom script %s...", szFileName);
        DIAG("[CLEO][CUSTOM][LOAD] file=%s parent=%p label=%d", szFileName, parent, label);

        try
        {
			std::ifstream is;
			if (label != 0) // Create external from label.
			{
				if (!parent)
					throw std::logic_error("Trying to create external thread from label without parent thread");
				// Child custom scripts may only be created from another custom script.
				// This keeps 0E6F/CLEO_CreateCustomScript from treating a native SCM
				// thread as a CCustomScript and using an incompatible code buffer.
				if (!parent->IsCustom())
					throw std::logic_error("Trying to create external thread from non-custom parent thread");
				// Child scripts inherit the parent's CLEO compatibility mode.
				CompatVer = parent->GetCompatibility();
				BaseIP = parent->GetBasePointer();
				CurrentIP = parent->GetBasePointer() - label;
				memcpy(Name, parent->Name, sizeof(Name));
				dwChecksum = parent->dwChecksum;
				parentThread = parent;
				parent->childThreads.push_back(this);
			}
			else
			{
				using std::ios;
				std::ifstream is(szFileName, std::ios::binary);
				is.exceptions(std::ios::badbit | std::ios::failbit);
				std::size_t length;
				is.seekg(0, std::ios::end);
				length = static_cast<std::size_t>(is.tellg());
				is.seekg(0, std::ios::beg);

				if (bIsMiss)
				{
					if (*MissionLoaded)
						throw std::logic_error("Starting of custom mission when other mission loaded");
					*MissionLoaded = 1;
					BaseIP = CurrentIP = missionBlock;
				}
				else {
					ownedBuffer = new BYTE[length];
					BaseIP = CurrentIP = ownedBuffer;
				}
				is.read(reinterpret_cast<char *>(BaseIP), length);

				const char *fname = strrchr(szFileName, '\\');
				const char *slash = strrchr(szFileName, '/');
				if (slash && (!fname || slash > fname)) fname = slash;
				if (fname) ++fname;
				else fname = szFileName;
				memcpy(Name, fname, sizeof(Name));
				Name[7] = '\0';
				dwChecksum = crc32(reinterpret_cast<BYTE *>(BaseIP), length);
			}
			lastScriptCreated = this;
            bOK = true;
            if (parent)
            {
                DIAG("[CLEO][CUSTOM][CREATE] name=%.*s parent=%.*s label=%d", 8, Name, 8, parent->Name, label);
            }
            else
            {
                DIAG("[CLEO][CUSTOM][CREATE] name=%.*s root=1", 8, Name);
            }
        }
        catch (std::exception& e)
        {
            TRACE("Error during loading of custom script %s occured.\nError message: %s", szFileName, e.what());
        }
        catch (...)
        {
            TRACE("Unknown error during loading of custom script %s occured.", szFileName);
        }
    }

    CCustomScript::~CCustomScript()
    {
        if (parentThread)
        {
            parentThread->childThreads.remove(this);
            parentThread = nullptr;
        }

        if (ownedBuffer)
            delete[] ownedBuffer;

		RunScriptDeleteDelegate(reinterpret_cast<CRunningScript*>(this));
		if (lastScriptCreated == this) lastScriptCreated = nullptr;
    }


	float VectorSqrMagnitude(CVector vector) { return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z; }
}
