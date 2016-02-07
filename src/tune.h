#pragma once

#include <array>
#include <vector>
#include <string>
#include <map>



enum {
	MACROS_PER_ROW = 2,
	CHANNEL_COUNT = 16,
	MIXRATE = 44100,
};



struct Row {
	int note;
	std::array<std::string,MACROS_PER_ROW> macros;
};

struct Envelope {
	struct Node {
		float value;
		bool delta;
		Node(float v, bool d) : value(v), delta(d) {}
		Node(float v) : value(v), delta(false) {}
	};
	std::vector<Node> nodes;
	int loop;
	Envelope(std::vector<Node> n={}, int l=-1) : nodes(n), loop(l) {}
	Envelope(float v) : nodes({v}), loop(-1) {}
};

typedef	std::map<std::string,Envelope>			EnvelopeMap;

struct Macro {
	std::vector<std::string> 			parents;
	EnvelopeMap							envs;
};

typedef std::vector<Row>						Pattern;
typedef std::map<std::string,Pattern>			PatternMap;
typedef std::map<std::string,Macro>				MacroMap;
typedef std::array<std::string,CHANNEL_COUNT>	TableLine;




struct Tune {
	std::vector<TableLine>		table;
	PatternMap					patterns;
	MacroMap					macros;
	int							frames_per_tick;
	Envelope					ticks_per_row;

	EnvelopeMap					envs;
};


inline int get_max_rows(const Tune& tune, int block) {
	int m = 0;
	for (auto& pn : tune.table[block]) {
		auto it = tune.patterns.find(pn);
		if (it != tune.patterns.end()) m = std::max(m, (int) it->second.size());
	}
	return m;
}


bool save_tune(const Tune& tune, const char* name, bool all=true);
bool load_tune(Tune& tune, const char* name);
void strip_tune(Tune& tune);
