#pragma once

enum Style {
	DEFAULT,
	NORMAL,
	HL_NORMAL,

	NOTE,
	MACRO,
	HL_NOTE,
	HL_MACRO,
	CS_NOTE,
	CS_MACRO,
	PL_NOTE,
	PL_MACRO,
	MK_NOTE,
	MK_MACRO,

	FRAME,

	EDIT,

	LEVEL,
};

static const struct { int fg, bg, attr; } styles[] {
	{},
	{ COLOR_WHITE,	COLOR_BLACK,	0		},
	{ COLOR_WHITE,	233, 			0		},

	{ COLOR_WHITE,	232, 			A_BOLD	},
	{ COLOR_WHITE,	232, 			0		},
	{ COLOR_WHITE,	235, 			A_BOLD	},
	{ COLOR_WHITE,	235, 			0		},
	{ COLOR_WHITE,	18, 			A_BOLD	},
	{ COLOR_WHITE,	18, 			0		},
	{ COLOR_WHITE,	234, 			A_BOLD	},
	{ COLOR_WHITE,	234, 			0		},
	{ COLOR_WHITE,	58, 			A_BOLD	},
	{ COLOR_WHITE,	58, 			0		},

	{ 17,			COLOR_BLACK,	A_BOLD	},

	{ COLOR_WHITE,	19,			 	0		},

	{ COLOR_WHITE,	COLOR_GREEN, 	0		},

};

inline void init_styles() {
	int i = 0;
	for (auto& s: styles) init_pair(i++, s.fg, s.bg);
}

inline void set_style(int i) { attrset(COLOR_PAIR(i) | styles[i].attr); }
inline void addchs(chtype c, int n) { while (n--) addch(c); }

