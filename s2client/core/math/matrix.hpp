#pragma once
#pragma once

#include <core/prerequisites.hpp>
#include <core/math/vector3.hpp>

namespace core {
#pragma pack(push,1)
	class matrix {
	private:
		struct mrowwise {
		private:
			float* v;
			size_t rows, cols;
		public:
			mrowwise(matrix& parent) : v(parent.v), rows(parent.rows), cols(parent.cols) { }
			const mrowwise& operator+=(const matrix& o) {
				assert(o.cols == cols && o.rows == 1);
				for (size_t x = 0; x < cols; x++) {
					const float ov = o.get(x, 0);
					for (size_t y = 0; y < rows; y++) {
						v[y * cols + x] += ov;
					}
				}
				return *this;
			}
			matrix mean()const {
				matrix r(rows, 1);
				for (size_t y = 0; y < rows; y++) {
					float rtot = 0.f;
					for (size_t x = 0; x < cols; x++) {
						rtot += v[y * cols + x];
					}
					r.set(0, y, rtot / float(cols));
				}
				return r;
			}
		};
		struct mcolwise {
		private:
			float* v;
			size_t rows, cols;
		public:
			mcolwise(matrix& parent) : v(parent.v), rows(parent.rows), cols(parent.cols) { }
			const mcolwise& operator+=(const matrix& o) {
				assert(o.cols == 1 && o.rows == rows);
				for (size_t y = 0; y < rows; y++) {
					const float ov = o.get(0, y);
					for (size_t x = 0; x < cols; x++) {
						v[y * cols + x] += ov;
					}
				}
				return *this;
			}
			matrix mean()const {
				matrix r(1, cols);
				for (size_t x = 0; x < cols; x++) {
					float ctot = 0.f;
					for (size_t y = 0; y < rows; y++) {
						ctot += v[y * cols + x];
					}
					r.set(x, 0, ctot / float(rows));
				}
				return r;
			}
		};

		float* v;
		size_t rows, cols;
	public:
		const size_t Rows()const {
			return rows;
		}
		const size_t Cols()const {
			return cols;
		}
		
		std::string str()const {
			std::string r;
			for (size_t y = 0; y < rows; y++) {
				r += "[ ";
				for (size_t x = 0; x < cols; x++) {
					r += core::format("%.2f ", get(x,y));
				}
				r += "]\n";
			}
			return r;
		}

		matrix(size_t rows, size_t cols) : rows(rows), cols(cols), v(new float[rows * cols]{ 0.f }) { }
		matrix() : matrix(1, 1) { }
		matrix(matrix&& o)noexcept {
			rows = o.rows; cols = o.cols;
			v = o.v;
			o.v = nullptr; o.rows = o.cols = 0;
		}
		matrix(const matrix& o)
			: matrix(o.rows, o.cols) {
			memcpy(v, o.v, sizeof(float) * rows * cols);
		}
		matrix(std::initializer_list<std::initializer_list<float>> l)
			: matrix(l.size(), l.begin()->size()) {
			auto bigit = l.begin();
			for (size_t y = 0; y < rows; y++) {
				auto it = bigit++->begin();
				for (size_t x = 0; x < cols; x++)
					v[cols * y + x] = *it++;
			}
		}
		~matrix() {
			delete[] v;
			v = nullptr;
			rows = cols = 0;
		}
		void resize(size_t Rows, size_t Cols) {
			if (rows == Rows && cols == Cols)
				return;
			else {
				if (v)
					delete[] v;
				v = new float[Rows * Cols]{ 0.f };
				rows = Rows; cols = Cols;
			}
		}
		matrix& operator=(const matrix& o) {
			resize(o.rows, o.cols);
			memcpy(v, o.v, sizeof(float) * rows * cols);
			return *this;
		}
		static matrix identity(size_t sz) {
			auto m = matrix(sz, sz);
			for (size_t idx = 0; idx < sz; idx++)
				m.v[idx * sz + idx] = 1.f;
			return m;
		}

		constexpr float get(size_t x, size_t y)const {
			return v[cols * y + x];
		}
		constexpr float& get(size_t x, size_t y) {
			return v[cols * y + x];
		}
		constexpr void set(size_t x, size_t y, float v) {
			this->v[cols * y + x] = v;
		}
		matrix apply(float(*fn)(float))const {
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++)
					r.set(x, y, fn(get(x, y)));
			return r;
		}
		matrix abs()const {
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++)
					r.set(x, y, fabs(get(x, y)));
			return r;
		}
		matrix sqrtElems()const {
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++)
					r.set(x, y, sqrtf(get(x, y)));
			return r;
		}
		matrix sgn()const {
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++) {
					const float epsilon = 1e-7f;
					float v = get(x, y);
					if (fabs(v) < epsilon)
						v = 0.f;
					else if (v < 0.f)
						v = -1.f;
					else
						v = 1.f;
					r.set(x, y, v);
				}
			return r;
		}

		matrix squareElems()const {
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++)
					r.set(x, y, get(x, y) * get(x, y));
			return r;
		}

		matrix transpose() {
			matrix r(cols, rows);
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++)
					r.set(y, x, get(x, y));
			return r;
		}

		matrix hadamard(const matrix& o)const {
			assert(cols == o.cols && rows == o.rows);
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++) {
				for (size_t x = 0; x < cols; x++) {
					r.set(x, y, get(x, y) * o.get(x, y));
				}
			}
			return r;
		}

		matrix hadamardDiv(const matrix& o)const {
			assert(cols == o.cols && rows == o.rows);
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++) {
				for (size_t x = 0; x < cols; x++) {
					r.set(x, y, get(x, y) / o.get(x, y));
				}
			}
			return r;
		}

		const matrix& operator-=(const matrix& o) {
			assert(cols == o.cols && rows == o.rows);
			for (size_t y = 0; y < rows; y++) {
				for (size_t x = 0; x < cols; x++) {
					set(x, y, get(x, y) - o.get(x, y));
				}
			}
			return *this;
		}

		matrix operator-(const matrix& o)const {
			assert(cols == o.cols && rows == o.rows);
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++) {
				for (size_t x = 0; x < cols; x++) {
					r.set(x, y, get(x, y) - o.get(x, y));
				}
			}
			return r;
		}
		matrix operator+(const matrix& o)const {
			assert(cols == o.cols && rows == o.rows);
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++) {
				for (size_t x = 0; x < cols; x++) {
					r.set(x, y, get(x, y) + o.get(x, y));
				}
			}
			return r;
		}
		matrix& operator+=(const matrix& o) {
			assert(cols == o.cols && rows == o.rows);
			for (size_t y = 0; y < rows; y++) {
				for (size_t x = 0; x < cols; x++) {
					set(x, y, get(x, y) + o.get(x, y));
				}
			}
			return *this;
		}
		matrix operator+(float v)const {
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++) {
				for (size_t x = 0; x < cols; x++) {
					r.set(x, y, get(x, y) + v);
				}
			}
			return r;
		}

		matrix operator*(const matrix& o)const {
			assert(cols == o.rows);
			auto outrows = rows;
			auto outcols = o.cols;
			auto nprods = cols;
			matrix r(outrows, outcols);
			for (size_t yo = 0; yo < outrows; yo++) {
				for (size_t xo = 0; xo < outcols; xo++) {
					float p = 0.f;
					for (size_t i = 0; i < nprods; i++) {
						p += get(i, yo) * o.get(xo, i);
					}
					r.set(xo, yo, p);
				}
			}
			return r;
		}
		matrix operator*=(const matrix& o) {
			return (*this = *this * o);
		}

		matrix operator*(float v)const {
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++)
					r.set(x, y, get(x, y) * v);
			return r;
		}
		matrix operator/(float v)const {
			matrix r(rows, cols);
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++)
					r.set(x, y, get(x, y) / v);
			return r;
		}
		matrix& operator*=(float v) {
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++)
					set(x, y, get(x, y) * v);
			return *this;
		}
		matrix& operator/=(float v) {
			for (size_t y = 0; y < rows; y++)
				for (size_t x = 0; x < cols; x++)
					set(x, y, get(x, y) / v);
			return *this;
		}

		mrowwise rowwise() {
			return mrowwise(*this);
		}
		mcolwise colwise() {
			return mcolwise(*this);
		}
	};
	static matrix operator*(float v, const matrix& m) {
		auto rows = m.Rows();
		auto cols = m.Cols();
		matrix r(rows, cols);
		for (size_t y = 0; y < rows; y++)
			for (size_t x = 0; x < cols; x++)
				r.set(x, y, m.get(x, y) * v);
		return r;
	}
	static matrix operator/(float v, const matrix& m) {
		auto rows = m.Rows();
		auto cols = m.Cols();
		matrix r(rows, cols);
		for (size_t y = 0; y < rows; y++)
			for (size_t x = 0; x < cols; x++)
				r.set(x, y, v / m.get(x, y));
		return r;
	}
#pragma pack(pop)
}
