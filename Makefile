.DEFAULT_GOAL=build

TARGET := "composer"

build:
	clang++ -O2 -Wall --std=c++20 $(TARGET).cpp -o $(TARGET)
