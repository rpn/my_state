// simple_enemy_state_1.cpp
#include <gtest/gtest.h>
#include <tuple>
#include <optional>
#include <mutex>
#include <thread>
#include <atomic>
#include "simple_player_state.h"

namespace simple_enemy_state_1 {


#if NOTE
enum class ShooterView {
	int post_address = 0;

	bool open_post_box();
		// 開設
	void close_post_box();
};
#endif

#if NOTE
- callする仕組みを作る
- viewにはハンドルが入っている
- ハンドルは単純なタイプである必要がある(intとか)
- しかしコールバックは引数付き
- ポストからコールバックを取り出す関数が必要(昔作ったような・・・)
- 受け取りがなかったらどうなる？キューにたまり続ける？

開設したら届くようになる
	- 開設以前のイベントが届かないのは困る場合は？
		- 強制開設があってもいいかも？
	- 複数の相手に同時に届けるとかできるの？
		- 便利関数としては用意するかも

- torelloにいっぱいタスクを作ろう
- 会社用のwebサービスって簡単に作れる？
#endif

template <typename... Args>
struct MyQue {
	using args_type = std::tuple<Args...>;
	std::vector<args_type> a;
	mutable std::mutex mtx_;

	bool empty() const
	{
		std::lock_guard<std::mutex> lock(mtx_);
		return a.empty();
	}

	size_t size() const
	{
		std::lock_guard<std::mutex> lock(mtx_);
		return a.size();
	}

	//void push(Args&&... args)
	//{
	//	a.emplace_back(std::forward<Args>(args)...);
	//}

	template <typename... U>
	void push(U&&... u) {
		static_assert(sizeof...(U) == sizeof...(Args), "argument count mismatch");
		std::lock_guard<std::mutex> lock(mtx_);
		a.emplace_back(std::forward<U>(u)...);
	}

	template <typename F>
	bool invoke(F f)
	{
		std::optional<args_type> request;
		{
			std::lock_guard<std::mutex> lock(mtx_);
			if (a.empty())
				return false;
			request = std::move(a.front());
			a.erase(a.begin());
		}
		std::apply(f, *request);
		return true;
	}

	std::optional<args_type> pop()
	{
		std::lock_guard<std::mutex> lock(mtx_);
		if (a.empty())
			return std::nullopt;
		auto result = std::move(a.front());
		a.erase(a.begin());
		return result;
	}
};

template <typename T>
struct MyQue<T> {
	using args_type = T;
	std::vector<args_type> a;
	mutable std::mutex mtx_;

	bool empty() const
	{
		std::lock_guard<std::mutex> lock(mtx_);
		return a.empty();
	}

	size_t size() const
	{
		std::lock_guard<std::mutex> lock(mtx_);
		return a.size();
	}

	template <typename U>
	void push(U&& u)
	{
		std::lock_guard<std::mutex> lock(mtx_);
		a.emplace_back(std::forward<U>(u));
	}

	template <typename F>
	bool invoke(F f)
	{
		std::optional<args_type> request;
		{
			std::lock_guard<std::mutex> lock(mtx_);
			if (a.empty())
				return false;
			request = std::move(a.front());
			a.erase(a.begin());
		}
		f(*request);
		return true;
	}

	std::optional<args_type> pop()
	{
		std::lock_guard<std::mutex> lock(mtx_);
		if (a.empty())
			return std::nullopt;
		auto result = std::move(a.front());
		a.erase(a.begin());
		return result;
	}
};

template <>
struct MyQue<void> {
	size_t count = 0;
	mutable std::mutex mtx_;

	bool empty() const
	{
		std::lock_guard<std::mutex> lock(mtx_);
		return count == 0;
	}

	size_t size() const
	{
		std::lock_guard<std::mutex> lock(mtx_);
		return count;
	}

	void push()
	{
		std::lock_guard<std::mutex> lock(mtx_);
		count++;
	}

	template <typename F>
	bool invoke(F f)
	{
		{
			std::lock_guard<std::mutex> lock(mtx_);
			if (count == 0)
				return false;
			--count;
		}
		f();
		return true;
	}

	bool pop()
	{
		std::lock_guard<std::mutex> lock(mtx_);
		if (count == 0)
			return false;
		--count;
		return true;
	}
};

TEST(SimpleEnemyState, tuple)
{
	std::tuple<int, int> t(1,2);
	ASSERT_EQ(1, std::get<0>(t));
	ASSERT_EQ(2, std::get<1>(t));

	std::apply([](int a, int b) {
		ASSERT_EQ(1, a);
		ASSERT_EQ(2, b);
	}, t);

	{
		MyQue<int, int> que;
		que.push(1, 2);
		ASSERT_EQ(1, que.a.size());
		ASSERT_EQ(1, std::get<0>(que.a[0]));
		ASSERT_EQ(2, std::get<1>(que.a[0]));
	}

	{
		MyQue<std::string> que;
		que.push("hello world");
		ASSERT_EQ("hello world", que.a.front());

		que.invoke([](std::string s) {
			ASSERT_EQ("hello world", s);
		});
		ASSERT_TRUE(que.empty());

		que.push("foo");
		que.push("bar");
		que.push("baz");
		que.invoke([](std::string s) {
			ASSERT_EQ("foo", s);
		});
		{
			auto o = que.pop();
			ASSERT_TRUE(o);
			ASSERT_EQ("bar", *o);
		}
		//que.invoke([](std::string s) {
		//	ASSERT_EQ("bar", s);
		//});
		que.invoke([](std::string s) {
			ASSERT_EQ("baz", s);
		});
	}
	{
		MyQue<void> q;
		q.push();
		ASSERT_EQ(1, q.size());
		ASSERT_TRUE(q.pop());
		ASSERT_FALSE(q.pop());
	}
}

struct MyView {
	int handle = 0;
	std::shared_ptr<MyQue<int, std::string>> p_que;

	//bool open()
	//{
	//	if (p_que != nullptr)
	//		return false;
	//	p_que = std::make_shared<MyQue<int, std::string>>();
	//	return true;
	//}

	template <typename F>
	bool pop(F f)
	{
		if (p_que)
			return p_que->invoke(std::forward<F>(f));
		return false;
	}
};

struct MyState {
	std::vector<std::weak_ptr<MyQue<int, std::string>>> p_ques;

	size_t push(int a, std::string s)
	{
		size_t n = 0;
		for (auto it = p_ques.begin(), e = p_ques.end(); it != e;) {
			if (auto p = it->lock()) {
				p->push(a, s);
				++it;
				++n;
			}
			else {
				it = p_ques.erase(it);
				e = p_ques.end();
			}
		}
		return n;
	}

	std::shared_ptr<MyView> create_view()
	{
		auto p = std::make_shared<MyView>();
		p->p_que = std::make_shared<MyQue<int, std::string>>();
		p_ques.emplace_back(p->p_que);
		return p;
	}
};

TEST(SimpleEnemyState, view_call_1)
{
	MyState s;
	auto p = s.create_view();
	ASSERT_TRUE(p);
	ASSERT_TRUE(p->p_que);

	ASSERT_EQ(1u, s.push(123, "foo"));

	ASSERT_TRUE(p->pop([](int a, std::string t) {
		ASSERT_EQ(123, a);
		ASSERT_EQ("foo", t);
	}));
}

TEST(SimpleEnemyState, queue_thread_safe_basic)
{
	MyQue<int> q;
	constexpr int kProducerCount = 4;
	constexpr int kPushPerProducer = 250;
	constexpr int kTotalPush = kProducerCount * kPushPerProducer;

	std::atomic<int> pop_count{ 0 };
	std::vector<std::thread> producers;
	producers.reserve(kProducerCount);

	for (int i = 0; i < kProducerCount; ++i) {
		producers.emplace_back([&q, kPushPerProducer]() {
			for (int j = 0; j < kPushPerProducer; ++j) {
				q.push(j);
			}
		});
	}

	std::thread consumer([&q, &pop_count, kTotalPush]() {
		while (pop_count.load() < kTotalPush) {
			auto item = q.pop();
			if (item) {
				++pop_count;
			}
			else {
				std::this_thread::yield();
			}
		}
	});

	for (auto& t : producers)
		t.join();
	consumer.join();

	ASSERT_EQ(kTotalPush, pop_count.load());
	ASSERT_TRUE(q.empty());
}


#if 0
TEST(SimpleEnemyState, test1)
{
	ASSERT_TRUE(false);

#if NOTE
	is_called(post_address);

	callback(post_address, []() {

	});	
#endif
}
#endif

enum class ShooterStateType {
	IDLE,
	SHOOT,
};

struct ShooterView {
	ShooterStateType state_;
	std::shared_ptr<MyQue<float>> p_shoot_que_;

	ShooterStateType state() const
	{
		return state_;
	}

	template <typename F>
	bool handle_shoot(F f)
	{
		if (p_shoot_que_)
			return p_shoot_que_->invoke(std::forward<F>(f));
		return false;
	}

	std::optional<float> handle_shoot()
	{
		if (p_shoot_que_)
			return p_shoot_que_->pop();
		return std::nullopt;
	}
};

struct ShooterState {
	ShooterStateType state_ {};
	float interval_ = 0;
	std::vector<std::weak_ptr<MyQue<float>>> shoots_;

	void tick(float delta_time)
	{
		interval_ += delta_time;
		if (interval_ < 1.0f) {
			state_ = ShooterStateType::IDLE;
			return;
		}
		interval_ = 0;
		state_ = ShooterStateType::SHOOT;
		shoot(1.0f);
	}

	size_t shoot(float speed)
	{
		size_t n = 0;
		for (auto it = shoots_.begin(),e = shoots_.end(); it != e;) {
			if (auto p = it->lock()) {
				p->push(speed);
				++it;
				++n;
			}
			else {
				it = shoots_.erase(it);
				e = shoots_.end();
			}
		}
		return n;
	}

	std::shared_ptr<ShooterView> create_view()
	{
		auto p = std::make_shared<ShooterView>();
		p->state_ = state_;
		p->p_shoot_que_ = std::make_shared<MyQue<float>>();
		shoots_.emplace_back(p->p_shoot_que_);
		return p;
	}
};

TEST(SimpleEnemyState, test_1)
{
	ShooterState ss;
	auto p_view = ss.create_view();
	ASSERT_TRUE(p_view);

	ASSERT_EQ(ShooterStateType::IDLE, p_view->state());
	ss.tick(1.0f);	
	//ASSERT_EQ(ShooterStateType::IDLE, p_view->state());

	ASSERT_TRUE(p_view->handle_shoot([](float speed) {
		ASSERT_EQ(1, speed);
	}));
	ASSERT_FALSE(p_view->handle_shoot());
}


} // namespace simple_enemy_state_1
