#!/bin/bash
# Based on Piazza post @8.

testname=""

function run() {
	eval "./simpsh $1 >testout.txt 2>testerr.txt"
}

function should_fail() {
	if [ $? -eq 0 ]; then
		echo "FAILED test: $testname."
		clean
		exit 1
	fi
	echo -n "."
}

function should_succeed() {
	if [ $? -ne 0 ]; then
		echo "FAILED test: $testname."
		clean
		exit 1
	fi
	echo -n "."
}

function file_should_contain() {
	grep -Fq -- "$1" "$2"
	should_succeed
}

function file_should_not_contain() {
	grep -Fq -- "$1" "$2"
	should_fail
}

function clean() {
	rm -f test*.txt
	echo
}

sample1="The Quick Brown Fox Jumps Over The Lazy Dog"
sample1a="THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG"
sample2="Lorem ipsum dolor sit amet."

testname="opening nonexistent file"
run "--rdonly cantpossiblyexist"
should_fail
file_should_contain "Error opening file" testerr.txt

clean

testname="opening existing file"
run "--rdonly Makefile"
should_succeed "opening existing file"

clean

testname="invalid file descriptors for --command"
run "--command 0 0 0 echo hi"
should_fail
file_should_contain "refers to undefined, closed, or problematic file" testerr.txt

clean

testname="no arguments for --command"
run "--command"
should_fail
file_should_contain "requires an argument" testerr.txt

clean

testname="<4 arguments for --command"
run "--command 1 2 3"
should_fail
file_should_contain "--command option requires at least 4 arguments" testerr.txt

clean

testname="--command working"
echo "$sample1" > testfile1.txt
touch testfile2.txt
run "--rdonly testfile1.txt --wronly testfile2.txt --command 0 1 1 tr [:lower:] [:upper:]"
should_succeed

clean

testname="--verbose working"
touch testfile1.txt
touch testfile2.txt
run "--rdonly testfile1.txt --verbose --wronly testfile2.txt --command 0 1 1 cat"
should_succeed
file_should_contain "--wronly testfile2.txt" testout.txt
file_should_contain "--command 0 1 1 cat" testout.txt

clean

testname="--commands after failing commands should succeed"
touch testfile1.txt
run "--wronly testfile1.txt --command 1 2 3 echo foo --command 0 0 0 echo foo"
should_fail

clean

testname="path command can write to write only file"
echo_path=$(which echo)
touch testfile1.txt
run "--wronly testfile1.txt --command 0 0 0 $echo_path foo"
should_succeed

clean

testname="Shouldn't be able to write to read only file"
touch testfile1.txt
run "--rdonly testfile1.txt --command 0 0 0 echo foo"
# The spec is ambiguous on whether or not simpsh considers this an error.
# We interpret it as not an error since programs may not have any output.
should_succeed
file_should_not_contain "foo" testfile1.txt

clean

testname="--creat and --append working"
echo "$sample1" > testfile1.txt
run "--rdonly testfile1.txt --creat --wronly testfile2.txt --command 0 1 1 cat"
should_succeed
file_should_contain "$sample1" testfile2.txt
echo "$sample2" > testfile3.txt
run "--rdonly testfile3.txt --append --wronly testfile2.txt --command 0 1 1 cat"
should_succeed

clean

testname="--rdwr working"
echo "$sample1" > testfile1.txt
echo "$sample2" > testfile2.txt
run "--rdwr testfile1.txt --rdwr testfile1.txt --rdonly testfile2.txt --creat --wronly testfile3.txt --command 0 3 3 cat --command 2 1 1 cat"
should_succeed

clean

testname="--pipe working"
echo "$sample1" > testfile1.txt
run "--rdonly testfile1.txt --creat --wronly testfile2.txt --pipe --command 0 3 3 cat --command 2 1 1 tr [:lower:] [:upper:]"
should_succeed

clean

testname="--wait working"
echo "$sample1" > testfile1.txt
run "--rdwr testfile1.txt --command 0 0 0 sleep 0.5s --command 0 0 0 exit 1 --wait"
should_fail
file_should_contain "Exit status 0: sleep 0.5s" testout.txt
file_should_contain "Exit status 1: exit 1" testout.txt

clean

testname="--wait with pipes and --close"
echo "$sample1" > testfile1.txt
run "--rdonly testfile1.txt --creat --wronly testfile2.txt --pipe --command 0 3 3 cat --command 2 1 1 tr [:lower:] [:upper:] --close 3 --wait"
should_succeed
file_should_contain "Exit status 0: cat" testout.txt
file_should_contain "Exit status 0: tr [:lower:] [:upper:]" testout.txt

clean

testname="--wait with signal (SIGPIPE)"
echo "$sample1" > testfile1.txt
run "--rdonly testfile1.txt --pipe --close 1 --command 0 2 2 cat --close 2 --wait"
should_fail
file_should_contain "Terminated by signal 13: cat" testout.txt

clean

testname="--abort working"
run "--abort"
should_fail

clean

testname="--catch and --abort working"
run "--catch 11 --abort"
should_fail
file_should_contain "11 caught" testerr.txt

clean

# Can't really test with --pause due to race conditions

testname="--default working"
run "--catch 11 --default 11 --abort"
should_fail
file_should_not_contain "11 caught" testerr.txt

clean



echo
echo "Passed all test cases."
