#pragma once

namespace hyper
{
	static struct UserActions // Having an extra header is dumb but I need it in two places
	{
		bool Keys[348]{ false };
		bool MouseButtons[8]{ false };
		double MousePos[2]{ 0.0 };
	} userActions;
}