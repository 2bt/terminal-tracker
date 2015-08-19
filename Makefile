
CF = -Wall --std=c++14 -I PEGTL/ -O2
LF = -Wall --std=c++14 -lncurses -lSDL -lportmidi -lsndfile

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)
TRG = tt


all: $(TRG)


obj/%.o: src/%.cpp Makefile
	@mkdir -p obj/
	g++ $(CF) $< -c -o $@

$(TRG): $(OBJ) Makefile
	g++ $(LF) $(OBJ) -o $@

clean:
	rm -rf $(TRG) obj/
