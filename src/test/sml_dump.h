// sml_dump.h
#pragma once

#include <iostream>
#include <type_traits>
#include <boost/sml.hpp>

#include <iostream>
#include <string>
#include <regex>
#include <boost/sml.hpp>

namespace sml_util {
namespace sml = boost::sml;

// どんなプロジェクト・バージョンでも使い回せる汎用フィルター
inline std::string clean_type_name(const char* raw_name) {
	std::string name = raw_name;
	
	// 1. struct や class を削除
	name = std::regex_replace(name, std::regex("struct |class "), "");
	
	// ★修正箇所： [a-zA-Z0-9_] の代わりに \w を使用して regex_error(error_range) を完全回避
	// \w は「任意の英数字およびアンダースコア」を意味するため、効果は全く同じです
	name = std::regex_replace(name, std::regex(R"(\w+::)"), "");
	
	// 3. zero_wrapper を削除
	name = std::regex_replace(name, std::regex(R"(zero_wrapper<)"), "");
	name = std::regex_replace(name, std::regex(R"(, ?void>)"), "");

	// 4. on_entry や on_exit の <_,_> などをスッキリさせる
	name = std::regex_replace(name, std::regex(R"(on_entry<.*?>)"), "on_entry");
	name = std::regex_replace(name, std::regex(R"(on_exit<.*?>)"), "on_exit");

	// 5. MSVCのラムダ式のカオスな名前を "lambda" という文字に変換（妥協用）
	name = std::regex_replace(name, std::regex(R"(`.*?<(lambda_\d+)>)"), "$1");

	// ★追加6. サブステートマシンの型 (Composite State) を綺麗にする
	// "sm<sm_policy<MySm> >" や "sm<MySm>" の中身だけを取り出す
	name = std::regex_replace(name, std::regex(R"(sm<sm_policy<(\w+)>\s*>)"), "$1");
	name = std::regex_replace(name, std::regex(R"(sm<(\w+)>)"), "$1");

	return name;
}
// テンプレート関数側も clean_type_name を通すように修正
template <class T>
void dump_guard() {
	std::cout << " [" << clean_type_name(sml::aux::get_type_name<T>()) << "]";
}
template <> void dump_guard<sml::front::always>() {}

template <class T>
void dump_action() {
	std::cout << " / " << clean_type_name(sml::aux::get_type_name<T>());
}
template <> void dump_action<sml::front::none>() {}

// ==========================================
// 各遷移を出力する関数（オプションフラグを追加）
// ==========================================
template <class Tr>
void dump_transition(bool show_internal) {
	std::string src = clean_type_name(sml::aux::get_type_name<typename Tr::src_state>());
	std::string dst = clean_type_name(sml::aux::get_type_name<typename Tr::dst_state>());
	std::string evt = clean_type_name(sml::aux::get_type_name<typename Tr::event>());

	// ★ オプションがfalseの場合、on_entry / on_exit に関連する内部処理をスキップする
	if (!show_internal) {
		if (evt.find("on_entry") != std::string::npos || 
			evt.find("on_exit") != std::string::npos) {
			return; // 何も出力せずに終了
		}
	}

	std::cout << src << " --> " << dst << " : " << evt;
	dump_guard<typename Tr::guard>();
	dump_action<typename Tr::action>();
	std::cout << std::endl;
}

// ==========================================
// 遷移リスト展開メタ関数（引数を渡せるように変更）
// ==========================================
template <class TList> struct dump_transitions_impl;
template <template <class...> class TList, class... Ts>
struct dump_transitions_impl<TList<Ts...>> {
	static void dump(bool show_internal) {
		// パラメータパック展開時にフラグを渡す
		int dummy[] = { 0, (dump_transition<Ts>(show_internal), 0)... };
		(void)dummy;
	}
};

// ==========================================
// ★ 階層化ダンプ関数（第2引数で表示/非表示を切り替えられるように変更）
// ==========================================
template <class... SubSMs, class MainSM_T>
void dump_with_composite(const MainSM_T& sm, bool show_internal = false) {
	std::cout << "@startuml\n" << std::endl;

	// サブステートマシンの展開
	int dummy[] = { 0, (
		[&] { // ラムダ式にフラグをキャプチャさせるため [&] に変更
			std::cout << "state " << clean_type_name(sml::aux::get_type_name<SubSMs>()) << " {\n";
			dump_transitions_impl<typename sml::sm<SubSMs>::transitions>::dump(show_internal);
			std::cout << "}\n\n";
		}(), 
		0
	)... };
	(void)dummy;

	// メインステートマシンの展開
	dump_transitions_impl<typename MainSM_T::transitions>::dump(show_internal);

	std::cout << "\n@enduml" << std::endl;
}

template <class SM>
void dump(const SM&, bool show_internal = false) {
	std::cout << "@startuml\n" << std::endl;
	dump_transitions_impl<typename SM::transitions>::dump(show_internal);
	std::cout << "\n@enduml" << std::endl;
}


} // namespace sml_util

