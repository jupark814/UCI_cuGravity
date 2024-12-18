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

CC    = gcc
FLAGS = -ansi -pedantic -Wshadow -Wall -Wextra -Werror -Wfatal-errors -fPIC -O3
LIBS  = -ldl -lm
DEST  = gravity
OBJS  = g_common.o g_vcm.o g_ir.o g_ann.o g_emitc.o g.o y.tab.o lex.yy.o

all: lang $(OBJS) $(DEST).o
	$(CC) -o $(DEST) $(DEST).o $(OBJS) $(LIBS)
	$(CC) -shared -o lib$(DEST).so $(OBJS) $(LIBS)
	$(AR) rcs lib$(DEST).a $(OBJS)

%.o: %.c
	$(CC) $(FLAGS) -c $<
	$(CC) $(FLAGS) -MM $< > $*.d

lang: g_lang.y g_lang.l
	yacc -d g_lang.y
	lex g_lang.l
	$(CC) -fPIC -Wall -O3 -c y.tab.c
	$(CC) -fPIC -Wall -O3 -c lex.yy.c

clean:
	rm -f $(DEST) y.tab.* lex.yy.* *.so *.a *.o *.d *~ *#

-include $(OBJS:.o=.d)
