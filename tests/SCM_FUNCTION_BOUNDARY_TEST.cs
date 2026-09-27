{$CLEO .cs}

// ============================================================================
// CLEO 4.4.4 unofficial patch - 0AB1 boundary test
// GTA SA 1.0 US
// Sanny Builder 3.8.5 syntax
//
// TESTS:
//   0 parameters
//   1 parameter
//   32 parameters (maximum supported)
//   33 parameters (invalid; must be rejected before copying arguments)
//
// EXPECTED:
//   TEST0 RESULT=0
//   TEST1 RESULT=111
//   TEST32 RESULT=33
//   TEST33 START
//   Then the 33-parameter call must be rejected by 0AB1.
//   The function @TEST_33 must NOT execute.
//
// No {$USE CLEO+} is required.
// ============================================================================

thread "FBOUND444"

wait 2000

4@ = -1

0AD1: show_formatted_text_highpriority "BOUNDARY TEST START" time 2000
wait 1000

// ---------------------------------------------------------------------------
// TEST 1: 0 parameters
// ---------------------------------------------------------------------------
0AB1: cleo_call @TEST_ZERO 0 4@

0AD1: show_formatted_text_highpriority "TEST0 RESULT=%d" time 3000 4@
wait 1000

// ---------------------------------------------------------------------------
// TEST 2: 1 parameter
// ---------------------------------------------------------------------------
0AB1: cleo_call @TEST_ONE 1 111 4@

0AD1: show_formatted_text_highpriority "TEST1 RESULT=%d" time 3000 4@
wait 1000

// ---------------------------------------------------------------------------
// TEST 3: 32 parameters - maximum supported
// ---------------------------------------------------------------------------
0AB1: cleo_call @TEST_32 32 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 4@

0AD1: show_formatted_text_highpriority "TEST32 RESULT=%d" time 3000 4@
wait 1000

// ---------------------------------------------------------------------------
// TEST 4: 33 parameters - intentionally invalid
// ---------------------------------------------------------------------------
0AD1: show_formatted_text_highpriority "TEST33 START" time 3000
wait 1000

0AB1: cleo_call @TEST_33 33 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 4@

// This line should NOT be reached.
// If it appears, 0AB1 failed to reject 33 parameters.
0AD1: show_formatted_text_highpriority "TEST33 UNEXPECTED PASS" time 4000

:LOOP
wait 1000
jump @LOOP

// ---------------------------------------------------------------------------
// 0 parameters -> return 0
// ---------------------------------------------------------------------------
:TEST_ZERO
4@ = 0
0AB2: cleo_return 1 4@

// ---------------------------------------------------------------------------
// 1 parameter -> return argument
// ---------------------------------------------------------------------------
:TEST_ONE
0AB2: cleo_return 1 0@

// ---------------------------------------------------------------------------
// 32 parameters -> 1 + 32 = 33
// 0@ receives first parameter, 31@ receives last parameter.
// ---------------------------------------------------------------------------
:TEST_32
0@ += 31@
0AB2: cleo_return 1 0@

// ---------------------------------------------------------------------------
// This label must never execute.
// ---------------------------------------------------------------------------
:TEST_33
4@ = 999
0AB2: cleo_return 1 4@
