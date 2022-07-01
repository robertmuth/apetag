# ======================================================================
# Makefile
#
#   Copyright (C) 2003 and onward Robert Muth <robert at muth dot org>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, version 3 of the License.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
# ======================================================================

PROGRAMS = apetag

SOURCES = basic.C switch.C main.C

OBJECTS = $(SOURCES:.C=.o)

HEADERS = basic.H switch.H
CXXDEBUG = -g
CXXOPT = -O3
CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++0x $(CXXOPT)   $(CXXDEBUG)

all:	$(PROGRAMS)

# ======================================================================

apetag: $(OBJECTS)
	$(CXX) $(CXXDEBUG) -o apetag $(OBJECTS)

apetag.static: $(OBJECTS)
	$(CXX) $(CXXDEBUG) -static -o apetag.static  $(OBJECTS)

check_cpp:
	cppcheck *.C

check_py:
	pylint  --rcfile=pylintrc *.py

clean:
	rm -f *.o *.pyc *~ \#* $(PROGRAMS) test.out

dep:
	makedepend -Y $(SOURCES)

format:
	clang-format -i $(SOURCES) $(HEADERS)

test: apetag
	./test.sh > test.out
	diff test.out TestData/golden.out
	@echo test OK

# ======================================================================
# DO NOT DELETE

