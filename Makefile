# Compiler and flags
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -g3 -Isrcs

# Project name
TARGET   := expert

# Source files (inside srcs/)
SRCS := srcs/main.cpp \
        srcs/App.cpp \
        srcs/Parser.cpp \
        srcs/TokenBlock.cpp \
        srcs/TokenEffect.cpp \
        srcs/Resolver.cpp \
		srcs/LogicRule.cpp \
		srcs/TruthTable.cpp \

# Object files (replace .cpp with .o)
OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test:
	python scripts/run_tests.py

clean:
	rm -f $(OBJS) $(TARGET)

re:	clean all


run: $(TARGET)
	./$(TARGET) input.txt
