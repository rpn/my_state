// sml_test_base.cpp
#include <gtest/gtest.h>

#include <iostream>
#include <type_traits>
#include <boost/sml.hpp>

#include <iostream>
#include <string>
#include <regex>
#include <boost/sml.hpp>

namespace sml_dump_sample {
namespace sml = boost::sml;

// ==========================================
// 1. 型名から不要な文字列を削除するユーティリティ
// ==========================================
//std::string clean_type_name(const char* raw_name) {
//    std::string name = raw_name;
//    // MSVC特有の "struct " や "class " を削除
//    name = std::regex_replace(name, std::regex("struct |class "), "");
//    // 必要に応じて自分の名前空間（例: sml_test_base::）も削除してスッキリさせる
//    name = std::regex_replace(name, std::regex("sml_test_base::"), "");
//    return name;
//}

// 型名から不要な文字列を削除するユーティリティ
//std::string clean_type_name(const char* raw_name) {
//    std::string name = raw_name;
//    
//    // 1. MSVC特有の "struct " や "class " を削除
//    name = std::regex_replace(name, std::regex("struct |class "), "");
//    
//    // 2. ユーザーの名前空間を削除（今回の出力に合わせて）
//    name = std::regex_replace(name, std::regex("sml_dump_sample::"), "");
//    
//    // 3. SML 1.1.13 の最適化ラッパー "boost::ext::sml::...::zero_wrapper<" を削除
//    name = std::regex_replace(name, std::regex(R"(boost::ext::sml::v1_1_13::aux::zero_wrapper<)"), "");
//    // ラッパーの終端にある ",void>" または ", void>" を削除
//    name = std::regex_replace(name, std::regex(R"(,void>|, void>)"), "");
//    
//    return name;
//}

// どんなプロジェクト・バージョンでも使い回せる汎用フィルター
std::string clean_type_name(const char* raw_name) {
    std::string name = raw_name;
    
    // 1. MSVC特有の "struct " や "class " を削除
    name = std::regex_replace(name, std::regex("struct |class "), "");
    
    // 2. 任意の名前空間（hoge::fuga::）をすべて削除する
    // これにより、ユーザー独自の "sml_dump_sample::" も、
    // SML内部の "boost::ext::sml::v1_1_13::aux::" もバージョン問わず一掃されます
    name = std::regex_replace(name, std::regex(R"([a-zA-Z0-9_]+::)"), "");
    
    // 3. 名前空間が消えてシンプルになった "zero_wrapper<Type, void>" からガワだけを削除
    // 前方と後方の文字列をピンポイントで消すため、Typeの中にカンマ等があっても壊れません
    name = std::regex_replace(name, std::regex(R"(zero_wrapper<)"), "");
    name = std::regex_replace(name, std::regex(R"(, ?void>)"), "");
    
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

template <class Tr>
void dump_transition() {
    std::cout << clean_type_name(sml::aux::get_type_name<typename Tr::src_state>()) << " --> "
              << clean_type_name(sml::aux::get_type_name<typename Tr::dst_state>())
              << " : " << clean_type_name(sml::aux::get_type_name<typename Tr::event>());
    dump_guard<typename Tr::guard>();
    dump_action<typename Tr::action>();
    std::cout << std::endl;
}

template <class TList> struct dump_transitions_impl;
template <template <class...> class TList, class... Ts>
struct dump_transitions_impl<TList<Ts...>> {
    static void dump() {
        int dummy[] = { 0, (dump_transition<Ts>(), 0)... };
        (void)dummy;
    }
};

template <class SM>
void dump(const SM&) {
    std::cout << "@startuml\n" << std::endl;
    dump_transitions_impl<typename SM::transitions>::dump();
    std::cout << "\n@enduml" << std::endl;
}

// ==========================================
// 2. テスト用のステートマシン（ラムダからStructへ変更）
// ==========================================
struct OnJump {};
struct OnTick { float dt; };
struct idle {};
struct jump {};

// アクションやガードは Struct (関数オブジェクト) にする
struct is_time_up {
    bool operator()() const { return true; }
};

struct play_sound {
    void operator()() const { /* 音を鳴らす処理 */ }
};

struct PlayerSm {
    auto operator()() const {
        using namespace sml;
        // ラムダ式ではなく、上記で定義した Struct の型をそのまま渡す
        return make_transition_table(
            *state<idle> + event<OnJump> / play_sound{} = state<jump>
            ,state<jump> + event<OnTick> [is_time_up{}] = state<idle>
        );
    }
};

TEST(SmlDumpSample, test1)
{
	sml::sm<PlayerSm> player_sm;
	dump(player_sm);
}

} // namespace sml_dump_sample
