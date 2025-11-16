CXX = g++
CXXFLAGS = -Iinclude -std=c++11 -pthread

SRC = source/core/ofs_core.cpp \
      source/core/server.cpp \
      source/core/json_util.cpp \
      source/core/main.cpp \
      source/data_structures/my_queue.cpp \
      source/data_structures/my_hash_table.cpp \
      source/data_structures/my_tree.cpp \
      source/data_structures/my_bitmap.cpp

OUT = ofs_core

all:
	$(CXX) $(SRC) $(CXXFLAGS) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)