#pragma once
#include <stdexcept>

// -------- base & effect ----------
class Booleans
{
protected:
    bool value;

public:
    explicit Booleans(bool v = false) : value(v) {}
    operator bool() const { return value; }
};

class TokenEffect : public Booleans
{
public:
    char name;
    TokenEffect() : Booleans(false), name(0) {}
    TokenEffect(char name) : Booleans(false), name(name) {}
    ~TokenEffect() {}

    bool get() const { return Booleans::operator bool(); }
    void set(bool v) { value = v; }
    explicit operator bool() const { return Booleans::operator bool(); }
};

// ---------------- operator tags (classes per operator) ----------------
class TokenAnd : public TokenEffect
{
public:
    TokenAnd() : TokenEffect('+') {}
};
class TokenOr : public TokenEffect
{
public:
    TokenOr() : TokenEffect('|') {}
};
class TokenXor : public TokenEffect
{
public:
    TokenXor() : TokenEffect('^') {}
};
class TokenImplies : public TokenEffect
{
public:
    TokenImplies() : TokenEffect('>') {}
};
class TokenIff : public TokenEffect
{
public:
    TokenIff() : TokenEffect('=') {}
};

// ---------------- binder structs (hold left value) ----------------
struct _AndBind
{
    bool left;
    explicit _AndBind(bool v) : left(v) {}
};
struct _OrBind
{
    bool left;
    explicit _OrBind(bool v) : left(v) {}
};
struct _XorBind
{
    bool left;
    explicit _XorBind(bool v) : left(v) {}
};
struct _ImpliesBind
{
    bool left;
    explicit _ImpliesBind(bool v) : left(v) {}
};
struct _IffBind
{
    bool left;
    explicit _IffBind(bool v) : left(v) {}
};

// --------------- infix: left << Operator()  ----------------
inline _AndBind operator<<(const TokenEffect &left, const TokenAnd &)
{
    return _AndBind(static_cast<bool>(left));
}
inline _OrBind operator<<(const TokenEffect &left, const TokenOr &)
{
    return _OrBind(static_cast<bool>(left));
}
inline _XorBind operator<<(const TokenEffect &left, const TokenXor &)
{
    return _XorBind(static_cast<bool>(left));
}
inline _ImpliesBind operator<<(const TokenEffect &left, const TokenImplies &)
{
    return _ImpliesBind(static_cast<bool>(left));
}
inline _IffBind operator<<(const TokenEffect &left, const TokenIff &)
{
    return _IffBind(static_cast<bool>(left));
}

// --------------- finish: binder << right --------------------
inline TokenEffect operator<<(const _AndBind &lhs, const TokenEffect &right)
{
    return TokenEffect(lhs.left && static_cast<bool>(right));
}
inline TokenEffect operator<<(const _OrBind &lhs, const TokenEffect &right)
{
    return TokenEffect(lhs.left || static_cast<bool>(right));
}
inline TokenEffect operator<<(const _XorBind &lhs, const TokenEffect &right)
{
    return TokenEffect(lhs.left ^ static_cast<bool>(right));
}
inline TokenEffect operator<<(const _ImpliesBind &lhs, const TokenEffect &right)
{
    bool p = lhs.left, q = static_cast<bool>(right);
    return TokenEffect((!p) || q); // p ⇒ q == !p || q
}
inline TokenEffect operator<<(const _IffBind &lhs, const TokenEffect &right)
{
    bool p = lhs.left, q = static_cast<bool>(right);
    return TokenEffect((p && q) || (!p && !q)); // p ⇔ q
}

// ---------------- unary NOT (both styles supported) ----------------
class TokenNot : public TokenEffect
{
public:
    TokenNot() : TokenEffect('!') {}
    ~TokenNot() = default;
};

// prefix-ish: TokenNot() << A
inline TokenEffect operator<<(const TokenNot &, const TokenEffect &right)
{
    return TokenEffect(!static_cast<bool>(right));
}

// postfix-ish: A << TokenNot()   (optional symmetry)
inline TokenEffect operator<<(const TokenEffect &left, const TokenNot &)
{
    return TokenEffect(!static_cast<bool>(left));
}
