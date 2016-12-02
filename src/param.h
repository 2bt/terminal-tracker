#pragma once

#include <vector>

#include "tune.h"


class Param {
public:
	void init(const Envelope& env) {
		m_env = env;
		m_pos = 0;
		m_first_tick = true;
	}
	bool tick() {
		if (m_pos >= (int) m_env.nodes.size()) {
			if (m_env.loop < 0) return false;
			m_pos = m_env.loop;
		}
		float v = m_value;
		auto& n = m_env.nodes[m_pos];
		if (n.delta) m_value += n.value;
		else m_value = n.value;
		m_pos++;
		if (m_first_tick) {
			m_first_tick = false;
			return true;
		}
		return m_value != v;
	}
	float val() const { return m_value; }
private:
	Envelope	m_env;
	int			m_pos = 0;
	float		m_value = 0;
	bool		m_first_tick = true;
};


template <int size, const std::map<std::string,int>& mapping>
class ParamBatch {
public:
	void configure(const EnvelopeMap& envs) {
		for (auto& p : envs) {
			auto it = mapping.find(p.first);
			if (it != mapping.end()) m_params[it->second].init(p.second);
		}
	}
	template <class cb>
	void tick(cb change) {
		int i = 0;
		for (auto& p : m_params) {
			if (p.tick()) change(i, p.val());
			i++;
		}
	}
private:
	std::array<Param,size> m_params;
};


class SimpleParamBatch {
public:
	void configure(const EnvelopeMap& envs) {
		for (auto& p : envs) {
			m_param_map[p.first].init(p.second);
		}
	}
	template <class cb>
	void tick(cb change) {
		for (auto& p : m_param_map) {
			if (p.second.tick()) change(p.first, p.second.val());
		}
	}
private:
	std::map<std::string,Param> m_param_map;
};
