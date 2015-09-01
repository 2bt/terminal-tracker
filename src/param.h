#pragma once

#include "tune.h"


class Param {
public:
	void init(Envelope env) {
		_env = env;
		_pos = 0;
		_first_tick = true;
	}
	bool tick() {
		if (_pos >= (int) _env.nodes.size()) {
			if (_env.loop < 0) return false;
			_pos = _env.loop;
		}
		float v = _value;
		auto& n = _env.nodes[_pos];
		if (n.delta) _value += n.value;
		else _value = n.value;
		_pos++;
		if (_first_tick) {
			_first_tick = false;
			return true;
		}
		return _value != v;
	}
	float val() const { return _value; }
private:
	Envelope	_env;
	int			_pos = 0;
	float		_value = 0;
	bool		_first_tick = true;
};
