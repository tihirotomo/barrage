#pragma once
#include <array>

namespace Sound {
	enum class SoundName {
		bgm,
		run_1,
		run_2,
		run_3,
		run_4,
		run_5,
		attack,
		attack_hit,
		jump,
		avoid,
		damage,
		text_1,
		text_2,
		text_3,
		text_all_draw,
		select,
		select_cursor,
		boss_nearAttack_sword,
		bullet1,
		bullet1_start,
		sword_fly,
		bullet2_start,
		screen_break1,
		screen_break2,
		half_attack,
		near_attack,
		boss_slash,
		onstage,
		size,
	};

	static std::array<std::string, int(SoundName::size)> SoundNameStr = {
		"bgm",
		"run_1",
		"run_2",
		"run_3",
		"run_4",
		"run_5",
		"attack",
		"attack_hit",
		"jump",
		"avoid",
		"damage",
		"text_1",
		"text_2",
		"text_3",
		"text_all_draw",
		"select",
		"select_cursor",
		"boss_nearAttack_sword",
		"bullet1",
		"bullet1_start",
		"sword_fly",
		"bullet2_start",
		"screen_break1",
		"screen_break2",
		"half_attack",
		"near_attack",
		"boss_slash",
		"onstage"
	};
}