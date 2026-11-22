chat "goose"
{
	#include "teamplay.h"

	type "game_enter"
	{
		"Honk.";
		"It is a lovely day in the arena, and I am a horrible goose.";
		"Nice armor. Shame if someone took it.";
	}

	type "game_exit"
	{
		"Honk. I have a picnic to ruin.";
		"Taking this with me.";
	}

	type "level_start"
	{
		"Honk.";
		"To do: steal the quad. Honk at everyone.";
	}

	type "level_end"
	{
		"Honk.";
	}

	type "level_end_victory"
	{
		"HONK!";
		"Everything here is mine now.";
	}

	type "level_end_lose"
	{
		"hjonk.";
		"I let you have that one, ", 2, ".";
	}

	type "hit_talking"
	{
		"Rude. I was honking.";
	}

	type "hit_nodeath"
	{
		"Honk?";
		"You will have to do better than that, ", 0, ".";
	}

	type "hit_nokill"
	{
		"Hold still, ", 0, ".";
	}

	type "death_insult"
	{
		"Honk. That was my good armor.";
		"I will be back for your hat, ", 0, ".";
	}

	type "death_praise"
	{
		"Honk. Well aimed, ", 0, ".";
	}

	type "death_rail"
	{
		"A long way to shoot a goose from, ", 0, ".";
	}

	type "death_gauntlet"
	{
		"Do not touch the goose, ", 0, ".";
	}

	type "death_suicide"
	{
		"Honk. Nobody saw that.";
	}

	type "death_cratered"
	{
		"I forgot I had wings.";
	}

	type "death_lava"
	{
		"Roast goose.";
	}

	type "death_slime"
	{
		"Not my pond.";
	}

	type "death_drown"
	{
		"A goose. Drowned. Honk.";
	}

	type "death_telefrag"
	{
		"I was standing there, ", 0, ".";
	}

	type "death_bfg"
	{
		"Excessive, ", 0, ". Honk.";
	}

	type "kill_insult"
	{
		"Honk.";
		"Sorry, ", 0, ". Not sorry.";
		"I will be taking that, ", 0, ".";
	}

	type "kill_praise"
	{
		"Good chase, ", 0, ". Honk.";
	}

	type "kill_rail"
	{
		"Honk from afar.";
	}

	type "kill_gauntlet"
	{
		"Peck.";
	}

	type "kill_telefrag"
	{
		"Excuse me, ", 0, ".";
	}

	type "enemy_suicide"
	{
		"I did not even honk yet, ", 0, ".";
	}

	type "random_insult"
	{
		"Honk.";
		"Honk honk.";
		"Has anyone seen ", 1, "'s keys? I have.";
	}

	type "random_misc"
	{
		"Honk.";
		"Somebody rake this arena.";
	}

}
