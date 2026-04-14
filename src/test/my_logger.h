// my_logger.h
#pragma once

#include <vector>
#include <boost/sml.hpp>
#include <fmt/printf.h>

namespace sml_util {
namespace sml = boost::sml;

struct MyLogger {
	bool short_name = true;
	std::vector<std::string>* p_buffer = nullptr;

	const char* short_or_full(const char* name) const
	{
		if (!short_name || !name) {
			return name;
		}

		const char* last = name;
		for (const char* p = name; *p; ++p) {
			if (p[0] == ':' && p[1] == ':') {
				last = p + 2;
			}
		}
		return last;
	}

	void p(std::string&& s)
	{
		if (p_buffer)
			p_buffer->push_back(std::move(s));
		else
			puts(s.c_str());
	}

	// ① イベントが処理されようとした時に呼ばれる
	template <class SM, class TEvent>
	void log_process_event(const TEvent&) {
		p(fmt::format("[process_event] {}",
			short_or_full(sml::aux::get_type_name<TEvent>())));
	}

	// ② ガード条件が評価された結果をログに出す
	template <class SM, class TGuard, class TEvent>
	void log_guard(const TGuard&, const TEvent&, bool result) {
		p(fmt::format("[guard] {} {} {}", 
			short_or_full(sml::aux::get_type_name<TGuard>()),
			short_or_full(sml::aux::get_type_name<TEvent>()), 
			(result ? "[OK]" : "[Reject]")));
	}

	// ③ アクションが実行された時に呼ばれる
	template <class SM, class TAction, class TEvent>
	void log_action(const TAction&, const TEvent&) {
		p(fmt::format("[action] {} {}", 
			short_or_full(sml::aux::get_type_name<TAction>()),
			short_or_full(sml::aux::get_type_name<TEvent>())));
	}

	// ④ 状態が実際に遷移した時に呼ばれる
	template <class SM, class TSrcState, class TDstState>
	void log_state_change(const TSrcState& src, const TDstState& dst) {
		// src と dst には状態名を文字列として取得できる c_str() が用意されています
		p(fmt::format("[transition] {} -> {}", 
			short_or_full(src.c_str()), 
			short_or_full(dst.c_str())));
	}
};

inline std::string join(const std::vector<std::string>& v, char ch)
{
	std::string r;
	for (auto it = v.begin(),e = v.end(); it != e; ++it) {
		if (it != v.begin())
			r += ch;
		r += *it;
	}
	return r;
}

inline std::string extract(std::vector<std::string>& buf)
{
	auto result = join(buf, '\n');
	buf.clear();
	return result;
}

} // namespace sml_util

