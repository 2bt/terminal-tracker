#include <string.h>

#include "tune.h"
#include "patternwin.h"




static std::vector<std::string> split(const char* s) {
	std::vector<std::string> res;

	for (;;) {
		s += strspn(s, " \n\t");
		int l = strcspn(s, " \n\t");
		if (l == 0) break;
		res.emplace_back(s, s + l);
		s += l;
	}

	return res;
}

static std::string& strip_dots(std::string& w) {
	while (!w.empty() && w.back() == '.') w.pop_back();
	return w;
}




bool load_tune(Tune& tune, const char* name) {
	FILE* f = fopen(name, "r");
	if (!f) return false;

	Pattern* pat = nullptr;
	char s[512];
	int mode = 0;

	while (fgets(s, sizeof(s), f)) {
		if (s[0] == '#') continue;

		auto words = split(s);

		if (mode == 't') {
			if (s[0] != ' ') mode = 0;
			else {
				tune.table.push_back({});
				for (int i = 0; i < std::min<int>(CHANNEL_COUNT, words.size()); i++) {
					tune.table.back()[i] = strip_dots(words[i]);
				}
				continue;
			}
		}

		if (mode == 'p') {
			if (s[0] != ' ') mode = 0;
			else {
				pat->push_back({});
				auto& row = pat->back();
				for (int i = 0; i < std::min<int>(MACROS_PER_ROW + 1, words.size()); i++) {
					auto& w = words[i];

					if (i == 0) {
						if (w == "===") row.note = -1;
						else if (w == "...") row.note = 0;
						else {
							if (w.size() != 3 ||
								w[0] < 'A' || w[0] > 'G' ||
								(w[1] != '-' && w[1] != '#') ||
								w[2] < '0' || w[2] > '9') {
								return false;
							}
							row.note = std::string("CCDDEFFGGAAB").find(w[0]) + 1;
							row.note += w[1] == '#';
							row.note += (w[2] - '0') * 12;
						}
					}
					else row.macros[i - 1] = strip_dots(w);
				}
				continue;
			}
		}


		if (words.size() == 1 && words[0] == "TABLE") {
			mode = 't';
			tune.table.clear();
			continue;
		}

		if (words.size() == 2 && words[0] == "PATTERN") {
			pat = &tune.patterns[words[1]];
			pat->clear();
			mode = 'p';
			continue;
		}

		return false;
	}
	fclose(f);
	return true;
}










bool save_tune(const Tune& tune, const char* name) {
	FILE* f = fopen(name, "w");
	if (!f) return false;

	fprintf(f, "TABLE\n");
	for (auto& line : tune.table) {
		int limit = line.size();
		while (limit > 1 && line[limit - 1] == "") limit--;
		for (int c = 0; c < limit; c++) {
			fprintf(f, " ");
			fprintf(f, "%s", line[c].c_str());
			for (int i = line[c].size(); i < PatternWin::CHAN_CHAR_WIDTH; i++) fprintf(f, ".");
		}
		fprintf(f, "\n");
	}
	for (auto& p : tune.patterns) {
		fprintf(f, "PATTERN %s\n", p.first.c_str());
		for (auto& row : p.second) {
			fprintf(f, " ");
			if (row.note > 0) {
				fprintf(f, "%c%c%X",
					"CCDDEFFGGAAB"[(row.note - 1) % 12],
					"-#-#--#-#-#-"[(row.note - 1) % 12],
					(row.note - 1) / 12);
			}
			else if (row.note == -1) fprintf(f, "===");
			else fprintf(f, "...");

			for (auto& m : row.macros) {
				fprintf(f, " %s", m.c_str());
				for (int i = m.size(); i < PatternWin::MACRO_CHAR_WIDTH; i++) fprintf(f, ".");
			}
			fprintf(f, "\n");
		}
	}
	fclose(f);
	return true;
}




