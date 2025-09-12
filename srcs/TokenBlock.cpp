#include "TokenBlock.hpp"

TokenBlock::TokenBlock(unsigned int priority): priority(priority)
{
}

TokenBlock::~TokenBlock()
{
}

bool TokenBlock::execute()
{
	if (this->empty())
		return false;

	// Helper to read the next value possibly prefixed by NOTs
	auto nextValue = [this](size_t &idx) -> bool {
		int not_count = 0;
		while (idx < this->size() && (*this)[idx].name == '!')
		{
			++not_count;
			++idx;
		}
		if (idx >= this->size())
			return false;
		bool val = (*this)[idx].get();
		++idx;
		if (not_count % 2)
			val = !val;
		return val;
	};

	// Evaluate left-to-right with simple binary ops
	size_t i = 0;
	bool cur = nextValue(i);
	while (i < this->size())
	{
		char op = (*this)[i].name;
		// Not is unary; if encountered here, treat as postfix not on current
		if (op == '!')
		{
			cur = !cur;
			++i;
			continue;
		}
		++i; // consume operator
		bool rhs = nextValue(i);
		switch (op)
		{
			case '+': cur = (cur && rhs); break;
			case '|': cur = (cur || rhs); break;
			case '^': cur = (cur ^ rhs); break;
			case '>': cur = ((!cur) || rhs); break; // implies
			case '=': cur = ((cur && rhs) || (!cur && !rhs)); break; // iff
			default: /* unknown token, ignore */ break;
		}
	}
	return cur;
}
