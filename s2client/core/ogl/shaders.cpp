#include "shaders.hpp"

#include <core/io/logger.hpp>

namespace core {
	GLuint glprogram::CompileShader(GLenum type, string_view src) {
		GLint success = 0;
		const GLchar* srcdata = src.data();
		GLuint shaderid = glCreateShader(type);
		glShaderSource(shaderid, 1, &srcdata, NULL);
		glCompileShader(shaderid);
		glGetShaderiv(shaderid, GL_COMPILE_STATUS, &success);
		if (!success) {
			char log[512];
			glGetShaderInfoLog(shaderid, 512, NULL, log);
			core::error("Shader compilation failed.\n\t%s\n", log);
			glDeleteShader(shaderid);
			return 0;
		}
		return shaderid;
	}
	glprogram::glprogram() {
		mId = glCreateProgram();
	}
	glprogram::~glprogram() {
		glDeleteProgram(mId);
	}
	void glprogram::attachVertexShader(string_view src) {
		auto id = CompileShader(GL_VERTEX_SHADER, src);
		assert(id != 0);
		glAttachShader(mId, id);
		glDeleteShader(id);
	}
	void glprogram::attachFragmentShader(string_view src) {
		auto id = CompileShader(GL_FRAGMENT_SHADER, src);
		assert(id != 0);
		glAttachShader(mId, id);
		glDeleteShader(id);
	}
	bool glprogram::link() {
		GLint success = 0;
		glLinkProgram(mId);
		glGetProgramiv(mId, GL_LINK_STATUS, &success);
		if (!success) {
			char log[512];
			glGetProgramInfoLog(mId, 512, NULL, log);
			core::error("Shader link failed.\n\t%s\n", log);
			return false;
		}
		return true;
	}
	void glprogram::use() {
		glUseProgram(mId);
	}
}
