#pragma once

/**
 * Represent a taken.
 * Can be a fact [A-Z] or an effect (|+^=)
 */
class TokenEffect
{
private:
public:
	char type;
	bool effect = false;
	TokenEffect(char type);
	~TokenEffect();
};
