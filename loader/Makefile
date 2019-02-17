PROGRAM = loader

CC = gcc
CXX = g++

INCLUDE = -I../deps -I../include

CXXFLAGS = -Wall -Wextra -std=c++17 -g
CFLAGS = -Wall -Wextra -std=gnu99

##=====================================================

# The source file types (headers excluded).
# .c indicates C source files, and others C++ ones.
SRCEXTS = .c .cpp

# The header file types.
HDREXTS = .h .hpp

CXXFLAGS += $(INCLUDE)

ifeq ($(SRCDIRS),)
	SRCDIRS := $(shell find $(SRCDIRS) -type d)
endif

SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
HEADERS = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(HDREXTS))))
SRC_CXX = $(filter-out %.c,$(SOURCES))
OBJS    = $(addsuffix .o, $(basename $(SOURCES)))
DEPS    = $(foreach f, $(OBJS), $(addprefix $(dir $(f))., $(patsubst %.o, %.d, $(notdir $(f)))))

DEP_OPT = $(shell if `$(CC) --version | grep -i "GCC" >/dev/null`; then \
                  echo "-MM"; else echo "-M"; fi )
DEPEND.d    = $(CC)  $(DEP_OPT) $(CFLAGS) $(CXXFLAGS)
COMPILE.c   = $(CC)  $(CFLAGS) -c
COMPILE.cxx = $(CXX)  $(CXXFLAGS) -c


.PHONY: all objs clean

all: $(PROGRAM)

.%.d:%.c
	@echo -n $(dir $<) > $@
	@$(DEPEND.d) $< >> $@


.%.d:%.cpp
	@echo -n $(dir $<) > $@
	@$(DEPEND.d) $< >> $@

objs:$(OBJS)

%.o:%.c
	$(COMPILE.c) $< -o $@

%.o:%.cpp
	$(COMPILE.cxx) $< -o $@

$(PROGRAM):$(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(PROGRAM)
