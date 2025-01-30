#pragma once

namespace hyper
{
	static struct UserActions // Having an extra header is dumb but I need it in two places
	{
		bool Keys[348]{ false }; // I need to figure out how to get all of this as pointers
		bool MouseButtons[8]{ false }; // so i'm not passing around ~0.4 KILOBYTES every time
		double MousePos[2]{ 0.0 };	   // I move the mouse/press keys
	} userActions; // Should declare this somewhere else more visible
}