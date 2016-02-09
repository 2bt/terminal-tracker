CF = -Wall --std=c++11 -I PEGTL/ -O2
LF = -Wall --std=c++11 -lncurses -lSDL -lportmidi -lsndfile -luv

CC = clang++

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)
TRG = tt

all: $(TRG)

obj/%.o: src/%.cpp Makefile
	@mkdir -p obj/
	$(CC) $(CF) $< -c -o $@

$(TRG): $(OBJ) Makefile
	$(CC) $(OBJ) $(LF) -o $@

ogg:
	ffmpeg -i log.wav -c:a libvorbis -qscale:a 7 log.ogg

clean:
	rm -rf $(TRG) obj/ log.wav log.ogg
