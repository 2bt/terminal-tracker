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
	Envelope(std::vector<Node> n, int l) : nodes(n), loop(l) {}
	Envelope(std::vector<Node> n) : nodes(n), loop(-1) {}
	Envelope(float v) : nodes({v}), loop(-1) {}
	Envelope() : nodes(), loop(-1) {}
};

struct Macro {
	std::vector<std::string> 			parents;
	std::map<std::string,Envelope>		envs;
};

typedef std::vector<Row>						Pattern;
typedef std::map<std::string,Pattern>			PatternMap;
typedef std::array<std::string,CHANNEL_COUNT>	TableLine;


inline int get_max_rows(const TableLine& line, const PatternMap& patterns) {
	int m = 0;
	for (auto& pn : line) {
		if (patterns.count(pn) > 0) m = std::max(m, (int) patterns.at(pn).size());
	}
	return m;
}


struct Tune {
	std::vector<TableLine>		table;
	PatternMap					patterns;
	std::map<std::string,Macro>	macros;
	int							ticks_per_row;
	int							frames_per_tick;
};


bool save_tune(const Tune& tune, const char* name);
bool load_tune(Tune& tune, const char* name);
