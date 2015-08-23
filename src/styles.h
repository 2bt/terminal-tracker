#pragma once

enum Style {
	S_DEFAULT,
	S_NORMAL,
	S_HL_NORMAL,

	S_NOTE,
	S_MACRO,
	S_HL_NOTE,
	S_HL_MACRO,
	S_CS_NOTE,
	S_CS_MACRO,
	S_PL_NOTE,
	S_PL_MACRO,
	S_MK_NOTE,
	S_MK_MACRO,
	S_ET_NOTE,
	S_ET_MACRO,
	S_RC_NOTE,
	S_RC_MACRO,

	S_FRAME,

	S_LEVEL,
};

static const struct { int fg, bg, attr; } styles[] {
	{},
	{ COLOR_WHITE,	COLOR_BLACK,	0		},
	{ COLOR_WHITE,	233, 			0		},

	{ COLOR_WHITE,	232, 			A_BOLD	},
	{ 11,			232, 			0		},

	{ COLOR_WHITE,	235, 			A_BOLD	},
	{ 11,			235, 			0		},
	{ COLOR_WHITE,	23, 			A_BOLD	},
	{ 11,			23, 			0		},
	{ COLOR_WHITE,	234, 			A_BOLD	},
	{ 11,			234, 			0		},
	{ COLOR_WHITE,	22, 			A_BOLD	},
	{ 11,			22, 			0		},
	{ COLOR_WHITE,	22,			 	A_BOLD	},
	{ 11,			22,			 	0		},
	{ COLOR_WHITE,	88,			 	A_BOLD	},
	{ 11,			88,			 	0		},

	{ 235,			COLOR_BLACK,	A_BOLD	},


	{ COLOR_WHITE,	COLOR_GREEN, 	0		},

};

inline void init_styles() {
	int i = 0;
	for (auto& s: styles) init_pair(i++, s.fg, s.bg);
}

inline void set_style(int i) { attrset(COLOR_PAIR(i) | styles[i].attr); }
inline void addchs(chtype c, int n) { while (n-- > 0) addch(c); }

