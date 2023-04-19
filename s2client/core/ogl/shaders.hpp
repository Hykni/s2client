#pragma once

#include <core/prerequisites.hpp>
#include <ext/glew/glew.h>
#include <core/math/vector3.hpp>
#include <core/math/mat4.hpp>
#include <core/utils/color.hpp>

namespace core {
	class vertexarray {
		GLuint mVAO;
		vector<GLuint> mBuffers;
        struct AttributeBuffer {
            GLuint id;
            size_t size;
        };
		unordered_map<GLuint, AttributeBuffer> mAttribBuffers;
		GLuint mIndexBuffer;

		GLuint newbuffer() {
			GLuint bo;
			glGenBuffers(1, &bo);
			mBuffers.push_back(bo);
			return bo;
		}
        AttributeBuffer& getbuffer(int attrib) {
            auto it = mAttribBuffers.find(attrib);
			if (it == mAttribBuffers.end()) {
				auto bfid = newbuffer();
				return (mAttribBuffers[attrib] = AttributeBuffer{ .id = bfid, .size = 0 });
			}
			else
				return it->second;
        }
		vertexarray(const vertexarray& o) = delete;
	public:
		vertexarray() {
			mIndexBuffer = 0;
			glGenVertexArrays(1, &mVAO);
		}
		~vertexarray() {
            glDeleteBuffers((GLsizei)mBuffers.size(), mBuffers.data());
			mBuffers.clear();
			glDeleteVertexArrays(1, &mVAO);
		}
		
		template<int N>
		GLuint indexdata(int attrib, const unsigned int(&indices)[N], GLenum usage = GL_STREAM_DRAW) {
			if(mIndexBuffer == 0)
				mIndexBuffer = newbuffer();
			bind();
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, usage);
			unbind();
			return mIndexBuffer;
		}

		GLuint vertexdata(int attrib, const vector3f* buf, int count, GLenum usage = GL_DYNAMIC_DRAW) {
			bind();
			auto& attrbf = getbuffer(attrib);
			auto size = sizeof(vector3f) * count;
			
            glBindBuffer(GL_ARRAY_BUFFER, attrbf.id);
            if(size > attrbf.size) {
                glBufferData(GL_ARRAY_BUFFER, size, buf, usage);
                attrbf.size = size;
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, 0, size, buf);
            }
            glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray(attrib);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			unbind();
			return attrbf.id;
		}

		template<int N, int M>
		GLuint vertexdata(int attrib, const float(&buf)[N][M], GLenum usage = GL_DYNAMIC_DRAW) {
			bind();
			auto& attrbf = getbuffer(attrib);
            auto size = sizeof(float) * N * M;

			glBindBuffer(GL_ARRAY_BUFFER, attrbf.id);
			if(size > attrbf.size) {
                glBufferData(GL_ARRAY_BUFFER, size, buf, usage);
                attrbf.size = size;
            } else {
				glBufferSubData(GL_ARRAY_BUFFER, 0, size, buf);
            }
            glVertexAttribPointer(attrib, M, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray(attrib);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			unbind();
			return attrbf.id;
		}

		void bind()const {
			glBindVertexArray(mVAO);
		}
		void unbind()const {
			glBindVertexArray(0);
		}

	};
	class glprogram {
		GLuint mId;

		static GLuint CompileShader(GLenum type, string_view src);
	public:
		glprogram();
		~glprogram();

		void attachVertexShader(string_view src);
		void attachFragmentShader(string_view src);
		bool link();

		void use();

		GLint uniformloc(string_view name)const {
			return glGetUniformLocation(mId, name.data());
		}
		GLint attribloc(string_view name)const {
			return glGetAttribLocation(mId, name.data());
		}
		template<typename T, typename t = std::remove_cvref_t<T>>
		void set(string_view name, const T& val) {
			if constexpr (std::is_integral_v<t>)
				glUniform1i(uniformloc(name), val);
			else if constexpr (std::is_floating_point_v<t>)
				glUniform1f(uniformloc(name), val);
			else if constexpr (std::is_same_v<t, vector3f>)
				glUniform3f(uniformloc(name), val.x, val.y, val.z);
			else if constexpr (std::is_same_v<t, color>)
				glUniform4f(uniformloc(name), val.R, val.G, val.B, val.A);
			else if constexpr (std::is_same_v<t, mat4f>)
				glUniformMatrix4fv(uniformloc(name), 1, GL_TRUE, &val._11);
			else
				static_assert(false, "Unknown uniform type");
		}
		template<typename T, typename t = std::remove_cvref_t<T>>
		T get(string_view name) {
			T out{};
			if constexpr (std::is_integral_v<t>)
				glGetUniformiv(mId, uniformloc(name), &out);
			else if constexpr (std::is_floating_point_v<t>)
				glGetUniformfv(mId, uniformloc(name), &out);
			else if constexpr (std::is_same_v<t, vector3f>)
				glGetUniformfv(mId, uniformloc(name), &out);
			else if constexpr (std::is_same_v<t, color>)
				glGetUniformfv(mId, uniformloc(name), &out);
			else if constexpr (std::is_same_v<t, mat4f>)
				glGetUniformfv(mId, uniformloc(name), &out);
			else
				static_assert(false, "Unknown uniform type");
			return out;
		}
	};
}
