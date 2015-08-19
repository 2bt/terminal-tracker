#include <string.h>

#include "tune.h"
#include "patternwin.h"


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

bool load_tune(Tune& tune, const char* name) {
	FILE* f = fopen(name, "r");
	if (!f) return false;

	char s[512];
	char pn[512];
	Pattern* pat = nullptr;
	int mode = 0;

	while (fgets(s, sizeof(s), f)) {
		if (mode == 't') {
			if (s[0] != ' ') mode = 0;
			else {
				tune.table.push_back({});
				char *p = s;
				int i = 0;
				while ((p = strchr(p, ' ')) && i < CHANNEL_COUNT) {
					int len = strcspn(++p, ". \n");
					tune.table.back()[i] = std::string(p, p+len);
					i++;
				}
				continue;
			}
		}

		if (mode == 'p') {
			if (s[0] != ' ') mode = 0;
			else {
				pat->push_back({});
				auto& row = pat->back();
				char *p = s;
				int i = -1;
				while ((p = strchr(p, ' ')) && i < MACROS_PER_ROW) {
					int len = strcspn(++p, ". \n");
					std::string w(p, p+len);
					if (i == -1) {
						if (w == "===") row.note = -1;
						else if (w == "") row.note = 0;
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
					else row.macros[i] = w;
					i++;
				}
				continue;
			}
		}

		if (strcmp(s, "TABLE\n") == 0) {
			mode = 't';
			tune.table.clear();
			continue;
		}
		if (sscanf(s, "PATTERN %s", pn) == 1) {
			pat = &tune.patterns[std::string(pn)];
			pat->clear();
			mode = 'p';
			continue;
		}
		return false;
	}
	fclose(f);
	return true;
}




