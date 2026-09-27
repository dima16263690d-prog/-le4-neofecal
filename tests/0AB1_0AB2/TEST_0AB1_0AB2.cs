{$CLEO .cs}

0000:
thread 'FTEST'

:MAIN
wait 0

0@ = 100
1@ = 7
3@ = 1

0AB1: call_scm_func @ADD_VALUES 2 10 20 2@

if
    0039:   2@ == 30
then
else
    3@ = 0
end

if
    0039:   0@ == 100
then
else
    3@ = 0
end

if
    0039:   1@ == 7
then
else
    3@ = 0
end

if
    0039:   3@ == 1
then
    0AD1: show_formatted_text_highpriority "0AB1/0AB2 PASS RESULT=%d" time 5000 2@
else
    0AD1: show_formatted_text_highpriority "0AB1/0AB2 FAIL RESULT=%d A=%d B=%d" time 5000 2@ 0@ 1@
end

wait 6000
0A93: end_custom_thread

:ADD_VALUES
005A: 0@ += 1@
0AB2: ret 1 0@
