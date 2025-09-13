#pragma once

class TokenEffect
{
private:
public:
	char type;
	bool effect = false;
	TokenEffect(char type);
	~TokenEffect();
};
