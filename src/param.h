#pragma once

#include <functional>
#include <vector>

#include "tune.h"


class Param {
public:
	void init(const Envelope& env) {
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


template <int size, const std::map<std::string,int>& mapping>
class ParamBatch {
public:
	bool configure(const EnvelopeMap& envs) {
		bool res = true;
		for (auto& p : envs) {
			auto it = mapping.find(p.first);
			if (it == mapping.end()) res = false;
			else _params[it->second].init(p.second);
		}
		return res;
	}
	void tick(std::function<void(int, float)> change) {
		int i = 0;
		for (auto& p : _params) {
			if (p.tick()) change(i, p.val());
			i++;
		}
	}
private:
	std::array<Param,size> _params;
};


class SimpleParamBatch {
public:
	void configure(const EnvelopeMap& envs) {
		for (auto& p : envs) {
			_param_map[p.first].init(p.second);
		}
	}
	void tick(std::function<void(const std::string&, float)> change) {
		for (auto& p : _param_map) {
			if (p.second.tick()) change(p.first, p.second.val());
		}
	}
private:
	std::map<std::string,Param> _param_map;
};
