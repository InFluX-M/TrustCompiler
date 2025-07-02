CXX = g++
CXXFLAGS = -g -O0 -Wall -std=c++17

SRC = main.cpp \
      LexicalAnalyzer/lexical_analyzer.cpp \
      SyntaxAnalyzer/syntax_analyzer.cpp

INC = -ILexicalAnalyzer -ISyntaxAnalyzer

OUT = main

all: $(SRC)
	$(CXX) $(CXXFLAGS) $(INC) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
