#pragma once

#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <vector>

namespace cimbar {

// currently only used in tests.
cv::Mat gl_to_mat(GLFWwindow* w)
{
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);

	// 3 is the number of channels.
	// TODO: should stride be "rounded up" to a multiple of 4 to match PACK_ALIGNMENT?
	GLsizei stride = 3 * width;
	std::vector<char> buff(stride * height);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, buff.data());

	// TODO: maybe have glReadPixels operate on the cv::Mat buffer directly?
	cv::Mat res = cv::Mat(width, height, CV_8UC3, buff.data()).clone();
	cv::flip(res, res, 0); // vertical flip
	return res;
}

}
