/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "util/vec_xy.h"
#include <GLES3/gl3.h>
#include <opencv2/opencv.hpp>

namespace cimbar {
namespace mat_to_gl {

	inline void load_gl_texture(GLuint texid, const cv::Mat& mat, vec_xy texdims={})
	{
		glBindTexture(GL_TEXTURE_2D, texid);

		GLenum format = GL_RGB;
		switch (mat.channels())
		{
			case 1:
				format = GL_LUMINANCE;
				break;
			case 4:
				format = GL_RGBA;
				break;
			default:
				;
		}

		if (mat.cols != texdims.x or mat.rows != texdims.y) // need init
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, mat.cols, mat.rows, 0, format, GL_UNSIGNED_BYTE, mat.data);
		}
		else // reuse existing memory
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mat.cols, mat.rows, format, GL_UNSIGNED_BYTE, mat.data);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

}
}
