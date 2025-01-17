#
# Makefile
# Copyright (C) Tony Givargis, 2019-2020
#
# This file is part of The Gravity Compiler.
#
# The Gravity Compiler is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version. The Gravity Compiler is distributed in
# the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
# the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE. See the GNU General Public License for more details. You should
# have received a copy of the GNU General Public License along with Foobar.
# If not, see <https://www.gnu.org/licenses/>.
#

#Automatic Compilation
#CC    = gcc
#FLAGS = -ansi -pedantic -Wshadow -Wall -Wextra -Werror -Wfatal-errors -fPIC -O3
#LIBS  = ../src/libgravity.a -ldl -lm
#DEST  = mnist
#OBJS  = mnist.o
#
#all: $(OBJS)
#	$(CC) -o $(DEST) $(OBJS) $(LIBS)
#
#%.o: %.c
#	$(CC) $(FLAGS) -c $<
#	$(CC) $(FLAGS) -MM $< > $*.d
#
#clean:
#	rm -f $(DEST) $(OBJS) *.o *.d *~ *#
#
#-include $(OBJS:.o=.d)


#Manual Compilation
#CC    = gcc
#FLAGS = -ansi -pedantic -Wshadow -Wall -Wextra -Werror -Wfatal-errors -fPIC -O3 -I../src
#LIBS  = ../src/libgravity.a -ldl -lm
#DEST  = mnist_manual
#OBJS  = mnist_manual.o test.o
#
#all: $(OBJS)
#	$(CC) -o $(DEST) $(OBJS) $(LIBS)
#
#%.o: %.c
#	$(CC) $(FLAGS) -c $<
#	$(CC) $(FLAGS) -MM $< > $*.d
#
#test.o: ../src/test.c ../src/test.h
#	$(CC) $(FLAGS) -c ../src/test.c -o test.o
#
#clean:
#	rm -f $(DEST) $(OBJS) *.o *.d *~ *#
#
#-include $(OBJS:.o=.d)

#CUDA Compilation (Manual)
CC    = gcc
NVCC  = nvcc
FLAGS = -ansi -pedantic -Wshadow -Wall -Wextra -Werror -Wfatal-errors -fPIC -O3 -I../src
NVFLAGS = -O3 -I../src
LIBS  = ../src/libgravity.a -ldl -lm -lcudart
DEST  = mnist_manual
OBJS  = mnist_manual.o test.o

all: $(OBJS)
	$(CC) -o $(DEST) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(NVFLAGS) -c $<
	$(CC) $(NVFLAGS) -MM $< > $*.d

%.o: %.cu
	$(NVCC) $(NVFLAGS) -c $<
	$(NVCC) $(NVFLAGS) -M $< > $*.d

test.o: ../src/test.cu ../src/test.h
	$(NVCC) $(NVFLAGS) -c ../src/test.cu -o test.o

clean:
	rm -f $(DEST) $(OBJS) *.o *.d *~ *#

-include $(OBJS:.o=.d)
