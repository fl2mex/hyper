#include "UserActions.h"

namespace hyper
{
	UserActions userActions{}; // Define it only here and make sure this is the only place where this is ever defined

	bool KeyCheck(KeyH key) // Put these here to avoid redefinition
	{
		return key.KeyState;
	};

	bool KeyClick(KeyH& key) // Remember, this modifies the KeyH
	{
		if (!key.KeyHeld)
		{
			if (key.KeyState)
			{
				key.KeyHeld = true;
				return true;
			}
		}
		else if (!key.KeyState)
		{
			key.KeyHeld = false;
		}
		return false;
	};
}