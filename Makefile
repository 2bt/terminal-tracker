CF = -Wall --std=c++11 -I PEGTL/ -O2
LF = -Wall --std=c++11 -lncurses -lSDL -lportmidi -lsndfile

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)
TRG = tt

all: $(TRG)

obj/%.o: src/%.cpp Makefile
	@mkdir -p obj/
	g++ $(CF) $< -c -o $@

$(TRG): $(OBJ) Makefile
	g++ $(OBJ) $(LF) -o $@

ogg:
	ffmpeg -i log.wav -c:a libvorbis -qscale:a 7 log.ogg

clean:
	rm -rf $(TRG) obj/ log.wav log.ogg
