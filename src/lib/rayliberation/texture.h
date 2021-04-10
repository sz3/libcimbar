#include "raylib.h"

#include "render_texture.h"

namespace cimbar {

class texture
{
public:
	texture();
	texture(const Image& img, Color tint);

	operator bool() const;

	const Texture2D& tx() const;

	void set_tint(Color c);
	const Color& tint() const;

	void copyTo(render_texture::slice slice);

protected:
	Texture2D _tx;
	Color _tint;

public:
	int rows = 0; // matching cv::Mat interface for now..
	int cols = 0;
};

}
