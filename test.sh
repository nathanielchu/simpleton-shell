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

echo
echo "Passed all test cases."
