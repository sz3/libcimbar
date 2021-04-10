#include "raylib.h"

namespace cimbar {

class TextureMat
{
public:
	TextureMat(const Image& img, Color tint)
	    : _tx(LoadTextureFromImage(img))
	    , _tint(tint)
	{
	}

	const Texture2D& texture() const
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
