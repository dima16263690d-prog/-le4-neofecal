{$CLEO .cs}
{$USE CLEO+}

// ============================================================================
// CLEO 4.4.4 unofficial patch - ScmFunction / 0AB1 / 0AB2 test
// GTA SA 1.0.0.0 US
//
// PURPOSE
// -------
// Minimal test for:
//   0AB1 -> function label -> 0AB2 -> return value -> caller
//
// Expected result:
//   RESULT=30
//
// This test intentionally does NOT use:
//   0E6F child scripts
//   0E70
//   Save/Load
//   csN.children.sav
//
// HISTORY BEFORE TODAY
// --------------------
// 1. The project started from the CLEO 4.4.4 codebase and keeps the old CLEO
//    execution model, old scripts and Sanny Builder compatibility as the base.
//
// 2. Custom-script lifecycle was repaired/diagnosed for GTA SA 1.0 US:
//    LOAD -> CREATE -> REGISTER -> END/STOP -> DELETE.
//
// 3. 0E6F child custom scripts were implemented without replacing the old
//    custom-script core. Child relationships use:
//      parentThread
//      childThreads
//      childLabel
//      savedNodeId
//
// 4. Child state save/load was extended with the sidecar:
//      csN.children.sav
//    while legacy:
//      csN.sav
//    remains the root save format.
//
// 5. Nested child restoration was tested with:
//      root -> child -> nested child
//    and was confirmed to restore the child tree after load.
//
// 6. The 0AB1/0AB2 function work was started around ScmFunction, including
//    local-variable scope, condition state and GOSUB stack isolation.
//
// TODAY - 27.09.2026
// ------------------
// 1. Compared the current 0AB1/0AB2 problem with the CLEO 5 function model.
//
// 2. Extended CCustomScript with function-scope state:
//      CodeSize
//      ScriptFileDir
//      ScriptFileName
//
// 3. Made child custom scripts inherit the above script state from parent.
//
// 4. Extended ScmFunction to snapshot/restore:
//      BaseIP
//      CodeSize
//      Stack[8]
//      SP
//      LocalVar[32]
//      Condition
//      LogicalOperation
//      NotFlag
//      ScriptFileDir
//      ScriptFileName
//      return IP
//
// 5. 0AB2 was adapted to perform CLEO 5-style vararg validation while keeping
//    the existing CLEO 4 GTA SA parameter API for the actual write-back.
//
// 6. Focused runtime diagnostics were added around 0AB1/0AB2:
//      ENTER -> READY -> JUMP
//      ENTER -> RETURN -> AFTER_RETURN
//
// 7. The existing 0E6F / child-save system was intentionally left separate
//    and was not replaced by ScmFunction.
//
// CURRENT TEST STATUS
// -------------------
// Previous runtime test reached the custom scripts, but the minimal 0AB1/0AB2
// function test displayed no result. A later manual return-slot experiment
// caused a crash and was reverted to the GTA SA parameter writer. The current
// code now uses the full function-state snapshot described above.
//
// NEXT TEST
// ---------
// Build Release / GTASA Win32, install the resulting CLEO4 build, run this
// script alone, and check for:
//      RESULT=30
//
// Also inspect cleo_diagnostic.log for the 0AB1/0AB2 diagnostic sequence.
//
// ============================================================================

thread "FUNCTEST"

wait 2000

0AB1: call_scm_func @ADD_VALUES 2 10 20 4@

0AD1: show_formatted_text_highpriority "RESULT=%d" time 5000 4@
wait 6000

:LOOP
wait 1000
jump @LOOP

:ADD_VALUES
005A: 0@ += 1@
0AB2: ret 1 0@
