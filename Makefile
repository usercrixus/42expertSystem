# Compiler and flags
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -g3 -Isrcs

# Project name
TARGET   := expert

# Source files (inside srcs/)
SRCS := srcs/main.cpp \
        srcs/Parser.cpp \
        srcs/TokenBlock.cpp \
        srcs/TokenEffect.cpp \
        srcs/Resolver.cpp \

# Object files (replace .cpp with .o)
OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test:
	./expert tests/and_not.txt
	./expert tests/or_xor.txt
	./expert tests/simple.txt
	./expert tests/undefined_cycle.txt
	./expert tests/subject.txt
	./expert tests/undefined_xor.txt
	./expert tests/amb_conflict_simple.txt
	./expert tests/amb_conflict_multi_path.txt
	./expert tests/or_xor_skip_q.txt
	./expert tests/amb_chain.txt

clean:
	rm -f $(OBJS) $(TARGET)

re:	clean all


run: $(TARGET)
	./$(TARGET) input.txt
