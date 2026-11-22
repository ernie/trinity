chat "gander"
{
	#include "teamplay.h"

	type "game_enter"
	{
		"Honk. We are live, chat.";
		"Chat, look who showed up: ", 1, ".";
		"Stream is up. Honk.";
	}

	type "game_exit"
	{
		"Ending stream. Honk.";
		"Raiding ", 1, "'s stream. Honk.";
	}

	type "level_start"
	{
		"Chat, take a gander at this map.";
		"Honk. We are live.";
	}

	type "level_end"
	{
		"Honk. GG, chat.";
	}

	type "level_end_victory"
	{
		"Like and subscribe. Honk.";
		"Chat, put that in the highlights.";
	}

	type "level_end_lose"
	{
		"Chat, we are switching games.";
		"Chat, go follow ", 2, ". Not really.";
	}

	type "hit_talking"
	{
		"I was reading chat, ", 0, ".";
	}

	type "hit_nodeath"
	{
		"Chat, you saw that, right?";
		"That goes in the blooper reel, ", 0, ".";
	}

	type "hit_nokill"
	{
		"Chat, watch me get ", 0, ".";
	}

	type "death_insult"
	{
		"Chat, that was lag.";
		"Chat, ", 0, " is stream sniping.";
	}

	type "death_praise"
	{
		"Clip it, ", 0, ". That was clean.";
	}

	type "death_rail"
	{
		"Stream sniper detected: ", 0, ".";
	}

	type "death_gauntlet"
	{
		"Chat, they bonked the streamer.";
	}

	type "death_suicide"
	{
		"Chat, that did not happen.";
	}

	type "death_cratered"
	{
		"I was reading donations.";
	}

	type "death_lava"
	{
		"Spicy stream today.";
	}

	type "death_slime"
	{
		"Chat, do not clip that.";
	}

	type "death_drown"
	{
		"Chat, my camera is underwater.";
	}

	type "death_telefrag"
	{
		"Rude, ", 0, ". I was mid-sentence.";
	}

	type "death_bfg"
	{
		"Chat, ", 0, " brought the noob cannon.";
	}

	type "kill_insult"
	{
		"Clip it. Clip that, ", 0, ".";
		"Chat, say hi to ", 0, ".";
		"Honk. Highlight reel.";
	}

	type "kill_praise"
	{
		"GG, ", 0, ". Go follow them, chat.";
	}

	type "kill_rail"
	{
		"Chat, did you see that shot?";
	}

	type "kill_gauntlet"
	{
		"Peck for the stream.";
	}

	type "kill_telefrag"
	{
		"Chat, rate my entrance. Sorry, ", 0, ".";
	}

	type "enemy_suicide"
	{
		"Chat, ", 0, " did that all by themselves.";
	}

	type "random_insult"
	{
		"Chat, ", 0, " is camping again.";
		"Honk. Who is watching ", 0, "?";
	}

	type "random_misc"
	{
		"Thanks for the follow. Honk.";
		"Hydrate, chat.";
		"Chat, is my mic on?";
	}

}
