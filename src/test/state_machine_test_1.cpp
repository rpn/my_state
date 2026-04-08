// state_machine_test_1.cpp
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace state_machine_test_1 {

struct EnemyContext {
	bool enemyVisible = false;
	bool inAttackRange = false;
	int attackEvents = 0;
};

class IState {
public:
	virtual ~IState() = default;
	virtual void onEnter(EnemyContext&) {}
	virtual void onExit(EnemyContext&) {}
	virtual void update(EnemyContext&, float dt) = 0;
	virtual const char* name() const = 0;
	virtual const char* subStateName() const { return ""; }
};

class StateMachine {
public:
	void change(std::unique_ptr<IState> next, EnemyContext& ctx)
	{
		if (state_)
			state_->onExit(ctx);
		state_ = std::move(next);
		if (state_)
			state_->onEnter(ctx);
	}

	void update(EnemyContext& ctx, float dt)
	{
		if (state_)
			state_->update(ctx, dt);
	}

	const char* name() const
	{
		return state_ ? state_->name() : "None";
	}

	const char* subStateName() const
	{
		return state_ ? state_->subStateName() : "";
	}

private:
	std::unique_ptr<IState> state_;
};

class ICombatSubState {
public:
	virtual ~ICombatSubState() = default;
	virtual void onEnter(EnemyContext&) {}
	virtual void onExit(EnemyContext&) {}
	virtual void update(EnemyContext&, float dt) = 0;
	virtual const char* name() const = 0;
};

class CombatSubMachine {
public:
	void change(std::unique_ptr<ICombatSubState> next, EnemyContext& ctx)
	{
		if (state_)
			state_->onExit(ctx);
		state_ = std::move(next);
		if (state_)
			state_->onEnter(ctx);
	}

	void update(EnemyContext& ctx, float dt)
	{
		if (state_)
			state_->update(ctx, dt);
	}

	const char* name() const
	{
		return state_ ? state_->name() : "None";
	}

private:
	std::unique_ptr<ICombatSubState> state_;
};

class PatrolState final : public IState {
public:
	void update(EnemyContext&, float) override {}
	const char* name() const override { return "Patrol"; }
};

class CombatChaseState final : public ICombatSubState {
public:
	void update(EnemyContext&, float) override {}
	const char* name() const override { return "Chase"; }
};

class CombatAttackState final : public ICombatSubState {
public:
	void update(EnemyContext& ctx, float) override
	{
		++ctx.attackEvents;
	}

	const char* name() const override { return "Attack"; }
};

class CombatState final : public IState {
public:
	void onEnter(EnemyContext& ctx) override
	{
		sub_.change(std::make_unique<CombatChaseState>(), ctx);
	}

	void onExit(EnemyContext& ctx) override
	{
		sub_.change(nullptr, ctx);
	}

	void update(EnemyContext& ctx, float dt) override
	{
		if (ctx.inAttackRange) {
			if (std::string(sub_.name()) != "Attack")
				sub_.change(std::make_unique<CombatAttackState>(), ctx);
		} else {
			if (std::string(sub_.name()) != "Chase")
				sub_.change(std::make_unique<CombatChaseState>(), ctx);
		}
		sub_.update(ctx, dt);
	}

	const char* name() const override { return "Combat"; }
	const char* subStateName() const override { return sub_.name(); }

private:
	CombatSubMachine sub_;
};

class EnemyController {
public:
	EnemyController()
	{
		machine_.change(std::make_unique<PatrolState>(), ctx_);
	}

	void setEnemyVisible(bool v) { ctx_.enemyVisible = v; }
	void setInAttackRange(bool v) { ctx_.inAttackRange = v; }

	void update(float dt)
	{
		if (ctx_.enemyVisible) {
			if (std::string(machine_.name()) != "Combat")
				machine_.change(std::make_unique<CombatState>(), ctx_);
		} else {
			if (std::string(machine_.name()) != "Patrol")
				machine_.change(std::make_unique<PatrolState>(), ctx_);
		}
		machine_.update(ctx_, dt);
	}

	const char* stateName() const { return machine_.name(); }
	const char* combatSubStateName() const { return machine_.subStateName(); }
	int attackEvents() const { return ctx_.attackEvents; }

private:
	EnemyContext ctx_;
	StateMachine machine_;
};

TEST(StateMachineTest1, inherited_state_with_substate)
{
	EnemyController c;
	ASSERT_STREQ("Patrol", c.stateName());

	c.setEnemyVisible(true);
	c.setInAttackRange(false);
	c.update(0.016f);
	ASSERT_STREQ("Combat", c.stateName());
	ASSERT_STREQ("Chase", c.combatSubStateName());
	ASSERT_EQ(0, c.attackEvents());

	c.setInAttackRange(true);
	c.update(0.016f);
	ASSERT_STREQ("Attack", c.combatSubStateName());
	ASSERT_EQ(1, c.attackEvents());

	c.setEnemyVisible(false);
	c.update(0.016f);
	ASSERT_STREQ("Patrol", c.stateName());
	ASSERT_STREQ("", c.combatSubStateName());
}

} // namespace state_machine_test_1
