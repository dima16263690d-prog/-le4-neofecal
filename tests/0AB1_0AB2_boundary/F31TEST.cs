{$CLEO .cs}

0000:
thread 'F31TEST'

:MAIN
wait 0

// Boundary test: 31 input parameters + 1 return value.
3@ = 1

0AB1: call_scm_func @ADD_VALUES 31 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 2@

if
    0039:   2@ == 31
then
else
    3@ = 0
end

if
    0039:   3@ == 1
then
    0AD1: show_formatted_text_highpriority "0AB1/0AB2 31 PARAMS PASS RESULT=%d" time 3000 2@
else
    0AD1: show_formatted_text_highpriority "0AB1/0AB2 31 PARAMS FAIL RESULT=%d" time 5000 2@
end

wait 3000
0A93: end_custom_thread

:ADD_VALUES
0AB2: ret 1 30@
