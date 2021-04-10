#include "raylib.h"

namespace cimbar {

class texture
{
public:
	texture()
		: _tx{0, 0, 0, 0, 0}
		, _tint(WHITE)
	{
	}

	texture(const Image& img, Color tint)
		: _tx(LoadTextureFromImage(img))
		, _tint(tint)
	{
	}

	operator bool() const
	{
		return _tx.width > 0;
	}

	const Texture2D& tx() const
	{
		return _tx;
	}

	const Color& tint() const
	{
		return _tint;
	}

protected:
	Texture2D _tx;
	Color _tint;
};

}
