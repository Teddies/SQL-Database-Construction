## For students: change this path to the root of your code
CODEROOT = ..

LDLIBS = -lreadline

#CC = gcc
## If you use OS X, then use CC = g++ , instead of CC = g++-4.8
CC = g++
#CC = g++-4.8
CXX = $(CC)

# Comment the following line to disable command line interface (CLI).
CPPFLAGS = -Wall -I$(CODEROOT) -std=c++11 -DDATABASE_FOLDER=\"$(CODEROOT)/cli/\" -g # with debugging info

# Uncomment the following line to compile the code without using CLI.
#CPPFLAGS = -Wall -I$(CODEROOT) -g -std=c++0x  # with debugging info and the C++11 feature

