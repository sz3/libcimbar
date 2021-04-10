#include "raylib.h"

namespace cimbar {

class texture
{
public:
	texture(const Image& img, Color tint)
		: _tx(LoadTextureFromImage(img))
		, _tint(tint)
	{
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
