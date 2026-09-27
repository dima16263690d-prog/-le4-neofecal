{$CLEO .cs}

// ============================================================================
// CLEO 4.4.4 unofficial patch - minimal 0AB1 / 0AB2 function test
// GTA SA 1.0 US
//
// EXPECTED:
//   RESULT=30
//   C0=12345 C1=67890
//
// The caller locals 0@ and 1@ are deliberately initialized before 0AB1.
// The function overwrites them with its arguments (10 and 20), calculates
// 30, returns it through 4@, and ScmFunction::Return() must restore the
// caller's original 0@ / 1@ values.
//
// This test intentionally uses the legacy CLEO 4 function opcodes only.
// ============================================================================

thread "FTEST444"

wait 2000

0@ = 12345
1@ = 67890
4@ = -1

0AD1: show_formatted_text_highpriority "FUNCTEST START" time 2000
wait 500

0AB1: call_scm_func @ADD_VALUES 2 10 20 4@

0AD1: show_formatted_text_highpriority "RESULT=%d C0=%d C1=%d" time 5000 4@ 0@ 1@
wait 6000

:LOOP
wait 1000
jump @LOOP

:ADD_VALUES
005A: 0@ += 1@
0AB2: ret 1 0@
