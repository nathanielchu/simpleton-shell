#!/bin/bash
# Based on Piazza post @8.

testname=""

function run() {
	eval "./simpsh $1 >testout.txt 2>testerr.txt"
}

function should_fail() {
	if [ $? -lt 1 ]; then
		echo "FAILED test: $testname."
		clean
		exit 1
	fi
	echo -n "."
}

function should_succeed() {
	if [ $? -gt 0 ]; then
		echo "FAILED test: $testname."
		clean
		exit 1
	fi
	echo -n "."
}

function expect_error() {
	grep -q -- "$1" testerr.txt
	should_succeed
}

function clean() {
	rm -f test*.txt
	echo
}

testname="opening nonexistent file"
run "--rdonly cantpossiblyexist"
should_fail
expect_error "Error opening file"

clean

testname="opening existing file"
run "--rdonly Makefile"
should_succeed "opening existing file"

clean

testname="invalid file descriptors for --command"
run "--command 0 0 0 echo hi"
should_fail
expect_error "refers to undefined file"

clean

testname="no arguments for --command"
run "--command"
should_fail
expect_error "requires an argument"

clean

testname="<4 arguments for --command"
run "--command 1 2 3"
should_fail
expect_error "--command option requires at least 4 arguments"

clean

testname="--command working"
echo "The Quick Brown Fox Jumps Over The Lazy Dog" > testfile1.txt
touch testfile2.txt
run "--rdonly testfile1.txt --wronly testfile2.txt --command 0 1 1 tr [:lower:] [:upper:]"
should_succeed
grep -Fq "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG" testfile2.txt
should_succeed

clean

testname="--verbose working"
touch testfile1.txt
touch testfile2.txt
run "--rdonly testfile1.txt --verbose --wronly testfile2.txt --command 0 1 1 cat"
should_succeed
grep -Fq -- "--wronly testfile2.txt" testout.txt
should_succeed
grep -Fq -- "--command 0 1 1 cat" testout.txt
should_succeed

clean

testname="--commands after failing commands should succeed"
touch testfile1.txt
run "--wronly testfile1.txt --command 1 2 3 echo foo --command 0 0 0 echo foo"
should_fail
grep -q "foo" testfile1.txt
should_succeed

clean

testname="path command can write to write only file"
echo_path=$(which echo)
touch testfile1.txt
run "--wronly testfile1.txt --command 0 0 0 $echo_path foo"
should_succeed
grep -q "foo" testfile1.txt
should_succeed

clean

testname="Shouldn't be able to write to read only file"
touch testfile1.txt
run "--rdonly testfile1.txt --command 0 0 0 echo foo"
# The spec is ambiguous on whether or not simpsh considers this an error.
# We interpret it as not an error since programs may not have any output.
should_succeed
grep -q "foo" testfile1.txt
should_fail

clean



echo
echo "Passed all test cases."
