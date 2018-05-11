На линуксе компилировать так:

    gcc -std=gnu11 -lbsd ls_alsr.c -o ls_alsr

На маках так:

    clang ls_alsr.c -o ls_alsr # gcc это alias для clang в маках
