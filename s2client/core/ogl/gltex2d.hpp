#pragma once

#include <core/prerequisites.hpp>
#include <ext/glew/glew.h>
#include <core/io/logger.hpp>

namespace core {
	class gltex2d {
	public:
		GLuint id;
		GLsizei width, height;
		GLint internalformat;
		GLint filter;
		gltex2d(GLsizei width, GLsizei height, GLint internalformat, GLint filter = GL_LINEAR)
			: width(width), height(height), internalformat(internalformat), filter(filter), id(0) {
			init();
		}
		~gltex2d() {
			reset();
		}

		void init() {
			glGenTextures(1, &id);
			bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
			unbind();

		}
		void reset() {
			glDeleteTextures(1, &id);
			id = 0;
			init();
		}

		void bind()const {
			glBindTexture(GL_TEXTURE_2D, id);
		}

		void unbind()const {
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		bool generate(GLenum format, GLenum type, const void* pixels, GLint lod=0) {
			bind();
			glTexImage2D(GL_TEXTURE_2D, lod, internalformat, width, height, 0, format, type, pixels);
			auto err = glGetError();
			if (err != GL_NO_ERROR)
				core::error("GL error %Xh", err);
			return true;
		}
	};
}
