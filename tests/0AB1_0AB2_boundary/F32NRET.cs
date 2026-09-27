{$CLEO .cs}

0000:
thread 'F32NRET'

:MAIN
wait 0

// Isolation test: exactly 32 input parameters, no return values.
0AB1: call_scm_func @NO_RETURN 32 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32

0AD1: show_formatted_text_highpriority "0AB1 32 PARAMS NO-RETURN PASS" time 3000

wait 3000
0A93: end_custom_thread

:NO_RETURN
0AB2: ret 0
