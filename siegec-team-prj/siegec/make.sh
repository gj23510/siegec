#! /bin/sh
gcc -Wall -o siegec siegeC.c analyzer.c lib.c interpreter.c
gcc -Wall popen.c -o gui
