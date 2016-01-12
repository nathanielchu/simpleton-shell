#!/bin/bash

function should_fail() {
	result=$?;

	echo -n "==> $1 ";

	if [ $result -lt 1 ]; then
		echo "FAILURE";
		exit 1;
	fi
}

function should_succeed(){
	result=$?;

	echo -n "==> $1 ";

	if [ $result -gt 0 ]; then
		echo "FAILURE";
		exit 1;
	fi
}

testfile1=/test/test1
testfile2=/test/test2
testfile3=/test/test3
testfile4=/test/test4

#Error opening files that don't exist
./simpsh --rdonly cantpossiblyexist 2>&1 | grep "Error opening file cantpossiblyexist, proceeding to next option." > /dev/null
should_succeed "1"

#No error opening existing files
./simpsh --rdonly Makefile 2>&1 | grep "Error opening file Makefile, proceeding to next option." > /dev/null
should_fail "2"

#Error on invalid file descriptors
./simpsh --command 1 2 3 echo "hi" 2>&1 | grep "Standard input for command echo refers to invalid file, aborting command." > /dev/null
should_succeed "3"

#Option mush have required arguemnts
./simpsh --command 2>&1 | grep "./simpsh: option '--command' requires an argument" > /dev/null
should_succeed "4"

#--command requires at least 4 arguments
./simpsh --command 1 2 3 2>&1 | grep "command option requires at least 4 arguments." > /dev/null
should_succeed "5"












echo "Passed all test cases."
