/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <GLES3/gl3.h>
#include <opencv2/opencv.hpp>

namespace cimbar {
namespace mat_to_gl {

	inline void load_gl_texture(GLuint texid, const cv::Mat& mat)
	{
		glBindTexture(GL_TEXTURE_2D, texid);

		// not sure whether we need this or not
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, mat.cols, mat.rows, 0, format, GL_UNSIGNED_BYTE, mat.data);
		glBindTexture(GL_TEXTURE_2D, 0); // clean up
	}

}
}
