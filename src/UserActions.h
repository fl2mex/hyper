#pragma once

namespace hyper
{
	struct KeyH
	{
		bool KeyState : 1;	// I wonder if I could bitpack all 348 keys into 78 bytes
		bool KeyHeld : 1;	// Challenge for another day
	};

	bool KeyCheck(KeyH key);
	bool KeyClick(KeyH& key); // Remember, this modifies the KeyH
	
	static struct UserActions // Having an extra header is dumb but I need it in two places
	{
		KeyH Keys[348]{ 0, 0 }; // I need to figure out how to get all of this as pointers
		bool MouseButtons[8]{ false }; // so i'm not passing around ~0.4 KILOBYTES every time
		double MousePos[2]{ 0.0 };	   // I move the mouse/press keys
	};

	extern UserActions userActions; // Global for static GLFW callback functions with unchangeable parameters
}