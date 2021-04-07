#pragma once

#include <core/prerequisites.hpp>
#include <core/math/matrix.hpp>
#include <core/utils/random.hpp>

// simple power series approx. of e^x around x=0
#define PS_expf(x) std::expf(x)//(1.f + x + (x*x)/2.0f + (x*x*x)/6.0f + (x*x*x*x)/24.0f + (x*x*x*x*x)/120.0f + (x*x*x*x*x*x)/(120.0f*6.0f))

namespace nn {
	using core::matrix;
	class baselayer_t {
	public:
		virtual void forward(const matrix& input) = 0;
		virtual const matrix& output()const = 0;
		virtual const size_t numOutputs() = 0;
		virtual void backpropagate(const matrix& input, const matrix& dLdA) = 0;
		virtual const matrix& dLdin()const = 0;
		virtual matrix& Weights() = 0;
		virtual const matrix& dLdW()const = 0;
		virtual matrix& Bias() = 0;
		virtual const matrix& dLdB()const = 0;
		virtual const char* actname()const = 0;
	};

	namespace optimizers {
		class baseoptimizer_t {
		public:
			virtual void update(matrix& Ws, const matrix& dLdW, matrix& Bs, const matrix& dLdB) = 0;
		};
		class sgd : public baseoptimizer_t {
			float rate = 0.1f;
			float decay = 0.0f;
		public:
			void update(matrix& Ws, const matrix& dLdW, matrix& Bs, const matrix& dLdB) {
				Ws -= rate * (dLdW + Ws * decay);
				Bs -= rate * (dLdB + Bs * decay);
			}
		};
		class adagrad : public baseoptimizer_t {
			float rate = 0.1f;
			map<const matrix*, matrix> history;
		public:
			adagrad(float lrate = 0.1f) : rate(lrate) { }
			void update(matrix& Ws, const matrix& dLdW, matrix& Bs, const matrix& dLdB) {
				const float epsilon = 1e-8f;
				matrix& dLdWsqsum = history[&dLdW];
				matrix& dLdBsqsum = history[&dLdB];
				dLdWsqsum.resize(dLdW.Rows(), dLdW.Cols());
				dLdBsqsum.resize(dLdB.Rows(), dLdB.Cols());
				dLdWsqsum += dLdW.squareElems();
				dLdBsqsum += dLdB.squareElems();
				Ws -= (rate / (dLdWsqsum.sqrtElems() + epsilon)).hadamard(dLdW);
				Bs -= (rate / (dLdBsqsum.sqrtElems() + epsilon)).hadamard(dLdB);
			}
		};
		class RMSprop : public baseoptimizer_t {
			float rate = 0.1f;
			float gamma;
			map<const matrix*, matrix> history;
		public:
			RMSprop(float lrate = 0.001f, float Gamma = 0.9f) : rate(lrate), gamma(Gamma) { }
			void update(matrix& Ws, const matrix& dLdW, matrix& Bs, const matrix& dLdB) {
				const float epsilon = 1e-8f;
				matrix& dLdWsqsum = history[&dLdW];
				matrix& dLdBsqsum = history[&dLdB];
				dLdWsqsum.resize(dLdW.Rows(), dLdW.Cols());
				dLdBsqsum.resize(dLdB.Rows(), dLdB.Cols());
				dLdWsqsum = (gamma * dLdWsqsum) + (1.f - gamma + epsilon) * dLdW.squareElems();
				dLdBsqsum = (gamma * dLdBsqsum) + (1.f - gamma + epsilon) * dLdB.squareElems();
				Ws -= (rate / (dLdWsqsum.sqrtElems() + epsilon)).hadamard(dLdW);
				Bs -= (rate / (dLdBsqsum.sqrtElems() + epsilon)).hadamard(dLdB);
			}
		};
		class adam : public baseoptimizer_t {
			float rate = 0.1f;
			float beta1;
			float beta2;
			
			struct Record {
				matrix first;
				matrix second;
				float beta1t;
				float beta2t;
			};
			map<const matrix*, Record> history;

			Record& remember(const matrix& m) {
				auto it = history.find(&m);
				if (it == history.end()) {
					it = history.emplace(&m, Record{
						.first = matrix(m.Rows(), m.Cols()),
						.second = matrix(m.Rows(), m.Cols()),
						.beta1t = beta1,
						.beta2t = beta2
					}).first;
				}
				return it->second;
			}
		public:
			adam(float lrate = 0.001f, float Beta1 = 0.9f, float Beta2 = 0.999f)
				: rate(lrate), beta1(Beta1), beta2(Beta2) {
			}

			void update(matrix& Ws, const matrix& dLdW, matrix& Bs, const matrix& dLdB) {
				const float epsilon = 1e-8f;
				auto& Wmoments = remember(Ws);
				auto& Bmoments = remember(Bs);
				float beta1t = Wmoments.beta1t;
				float beta2t = Wmoments.beta2t;

				Wmoments.first = beta1 * Wmoments.first + (1.f - beta1) * dLdW;
				Wmoments.second = beta2 * Wmoments.second + (1.f - beta2) * dLdW.squareElems();
				Bmoments.first = beta1 * Bmoments.first + (1.f - beta1) * dLdB;
				Bmoments.second = beta2 * Bmoments.second + (1.f - beta2) * dLdB.squareElems();

				const float cc1 = 1.f / (1.f - beta1t);
				const float cc2 = 1.f / (1.f - beta2t);
				Ws -= ((rate * cc1) * Wmoments.first).hadamardDiv(cc2 * Wmoments.second.sqrtElems() + epsilon);
				Bs -= ((rate * cc1) * Bmoments.first).hadamardDiv(cc2 * Bmoments.second.sqrtElems() + epsilon);
				
				beta1t *= beta1;
				beta2t *= beta2;
				Wmoments.beta1t = beta1t;
				Wmoments.beta2t = beta2t;
			}
		};
	}
	namespace curves {
		class identity {
		public:
			static constexpr float f(float x) {
				return x;
			}
			static constexpr float dfdx(float x) {
				return 1.f;
			}
		};
		class logistic {
		public:
			static float f(float x) {
				return 1.f / (1.f + PS_expf(-x));
			}
			static float dfdx(float x) {
				// e^-x / ((1+e^-x)^2)
				const float v = PS_expf(-x);
				return v / ((1 + v) * (1 + v));
			}
		};
		class relu {
		public:
			static constexpr float f(float x) {
				return max(0.f, x);
			}
			static constexpr float dfdx(float x) {
				return x < 0.f ? 0.f : 1.f;
			}
		};
		class swish {
		public:
			static float f(float x) {
				return x / (1.f + PS_expf(-x));
			}
			static float dfdx(float x) {
				//const float ex = PS_expf(-x);
				float sf = f(x);
				float lf = logistic::f(x);
				return sf + lf * (1.f - sf);
			}
		};
	}

	template<class Activation=curves::logistic>
	class fclayer : public baselayer_t {
		matrix mA;
		matrix mWeights;
		matrix mBias;
		matrix mZ;     // values before activation
		matrix mdLdW;  // d(L) / d(Weights)
		matrix mdLdB;  // d(L) / d(Bias)
		matrix mdLdin; // d(L) / d(input)
	public:
		fclayer() {
			resize(1, 1);
		}
		fclayer(size_t inputsize, size_t outputsize) {
			resize(inputsize, outputsize);
		}
		void resize(size_t inputsize, size_t outputsize) {
			mWeights.resize(inputsize, outputsize);
			mBias.resize(outputsize, 1);
			mdLdW.resize(inputsize, outputsize);
			mdLdB.resize(outputsize, 1);
			randomize();
		}
		void randomize() {
			const auto rng_gd = [](float v) -> float {
				float x = (core::random::int32(100) - 50) / 50.f; // [-0.5, 0.5]
				constexpr float c = 0.3989422804f; //1.f / (sqrtf(2 * M_PI));
				float r =  c * PS_expf(x); // ignore -(1/2) factor for x because we already apply it before range generation
				return r;
			};
			mWeights = mWeights.apply(rng_gd);
			mBias = mBias.apply(rng_gd);
		}
		void forward(const matrix& input) {
			mZ = (mWeights.transpose() * input);
			mZ.colwise() += mBias;
			mA = mZ.apply(&Activation::f);
		}
		const matrix& output()const {
			return mA;
		}
		const size_t numOutputs() {
			return mBias.Rows();
		}
		void backpropagate(const matrix& input, const matrix& dLdA) {
			const size_t nobs = input.Cols();
			// calc mdLdW, mdLdB
			
			// d(L)/d(Z) = d(A)/d(Z) . d(L)/d(A)
			// d(L)/d(W) = d(L)/d(Z) . d(Z)/d(W) ; dZdW = input
			// dLdZ = dLdA . dAdZ
			matrix dAdZ = mZ.apply(Activation::dfdx);
			matrix dLdZ = dAdZ.hadamard(dLdA);
			//core::info("input:\n%s", input.str());
			//core::info("mZ:\n%s", mZ.str());
			//core::info("dLdA:\n%s", dLdA.str());
			//core::info("dAdZ:\n%s", dAdZ.str());
			//core::info("dLdZ:\n%s\n", dLdZ.str());
			
			mdLdW = input;
			mdLdW *= dLdZ.transpose();
			mdLdW /= float(nobs);
			//core::info("dLdW:\n%s\n", mdLdW.str());

			// dLdB = dLdZ . dZdB ; z = W.in + B; dZdB = 1
			mdLdB = dLdZ.rowwise().mean();

			// dLdin = dLdZ . dZdin; dZdin = W
			mdLdin = mWeights * dLdZ;
		}
		const matrix& dLdin()const {
			return mdLdin;
		}
		matrix& Weights() {
			return mWeights;
		}
		const matrix& dLdW()const {
			return mdLdW;
		}
		const matrix& dLdB()const {
			return mdLdB;
		}
		matrix& Bias() {
			return mBias;
		}
		const char* actname()const {
			return typeid(Activation).name();
		}
	};

	//class softmaxlayer : public baselayer_t {
	//	matrix mA;
	//	matrix mWeights;
	//	matrix mBias;
	//	matrix mZ;     // values before activation
	//	matrix mdLdW;  // d(L) / d(Weights)
	//	matrix mdLdB;  // d(L) / d(Bias)
	//	matrix mdLdin; // d(L) / d(input)
	//public:
	//	softmaxlayer() {
	//		resize(1, 1);
	//	}
	//	softmaxlayer(size_t inputsize, size_t outputsize) {
	//		resize(inputsize, outputsize);
	//	}
	//	void resize(size_t inputsize, size_t outputsize) {
	//		mWeights.resize(inputsize, outputsize);
	//		mBias.resize(outputsize, 1);
	//		mdLdW.resize(inputsize, outputsize);
	//		mdLdB.resize(outputsize, 1);
	//		randomize();
	//	}
	//	void randomize() {
	//		const auto rng_gd = [](float v) -> float {
	//			float x = (core::random::int32(100) - 50) / 50.f; // [-0.5, 0.5]
	//			constexpr float c = 0.3989422804f; //1.f / (sqrtf(2 * M_PI));
	//			float r = c * PS_expf(x); // ignore -(1/2) factor for x because we already apply it before range generation
	//			return r;
	//		};
	//		mWeights = mWeights.apply(rng_gd);
	//		mBias = mBias.apply(rng_gd);
	//	}
	//	void forward(const matrix& input) {
	//		mZ = (mWeights.transpose() * input);
	//		mZ.colwise() += mBias;
	//		mA = mZ.apply(&Activation::f);
	//	}
	//	const matrix& output()const {
	//		return mA;
	//	}
	//	const size_t numOutputs() {
	//		return mBias.Rows();
	//	}
	//	void backpropagate(const matrix& input, const matrix& dLdA) {
	//		const size_t nobs = input.Cols();
	//		// calc mdLdW, mdLdB

	//		// d(L)/d(Z) = d(A)/d(Z) . d(L)/d(A)
	//		// d(L)/d(W) = d(L)/d(Z) . d(Z)/d(W) ; dZdW = input
	//		// dLdZ = dLdA . dAdZ
	//		matrix dAdZ = mZ.apply(Activation::dfdx);
	//		matrix dLdZ = dAdZ.hadamard(dLdA);
	//		//core::info("input:\n%s", input.str());
	//		//core::info("mZ:\n%s", mZ.str());
	//		//core::info("dLdA:\n%s", dLdA.str());
	//		//core::info("dAdZ:\n%s", dAdZ.str());
	//		//core::info("dLdZ:\n%s\n", dLdZ.str());

	//		mdLdW = input;
	//		mdLdW *= dLdZ.transpose();
	//		mdLdW /= float(nobs);
	//		//core::info("dLdW:\n%s\n", mdLdW.str());

	//		// dLdB = dLdZ . dZdB ; z = W.in + B; dZdB = 1
	//		mdLdB = dLdZ.rowwise().mean();

	//		// dLdin = dLdZ . dZdin; dZdin = W
	//		mdLdin = mWeights * dLdZ;
	//	}
	//	const matrix& dLdin()const {
	//		return mdLdin;
	//	}
	//	matrix& Weights() {
	//		return mWeights;
	//	}
	//	const matrix& dLdW()const {
	//		return mdLdW;
	//	}
	//	const matrix& dLdB()const {
	//		return mdLdB;
	//	}
	//	matrix& Bias() {
	//		return mBias;
	//	}
	//	const char* actname()const {
	//		return typeid(Activation).name();
	//	}
	//};


	template<int nInputs, int nOutputs>
	class neuralnetwork {
		vector<baselayer_t*> mLayers;
		matrix mdLdA;
		matrix mLoss;
		float mLearningRate = 0.1f;
	public:
		neuralnetwork() { }
		~neuralnetwork() {
			for (auto L : mLayers)
				delete L;
			mLayers.clear();
		}

		template<class ActFn=curves::logistic>
		void layer(int nodes) {
			if (mLayers.empty()) {
				mLayers.push_back(new nn::fclayer<ActFn>(nInputs, nodes));
			}
			else {
				auto& prevLayer = mLayers.back();
				mLayers.push_back(new nn::fclayer<ActFn>(prevLayer->numOutputs(), nodes));
			}
		}

		array<float, nOutputs> forward(const matrix& input) {
			matrix t = input;
			for (auto L : mLayers) {
				L->forward(t);
				t = L->output();
			}
			array<float, nOutputs> result;
			for (int i = 0; i < nOutputs; i++) {
				result[i] = t.get(i, 0);
			}
			return result;
		}

		const matrix& loss()const {
			return mLoss;
		}

		void calculate_loss(const matrix& output, const matrix& target) {
			// mse L = (1/2)|y^-y|^2 ; y^ = last activations
			// => dL/dA = y^ - y
			mLoss = 0.5f * (output - target).squareElems();
			mdLdA = output - target;

			// abs L = |y^-y|
			// dLdA = (y^-y)/|y^-y|
			// mLoss = (output - target).abs();
			// mdLdA = (output - target).sgn();
		}

		void backpropagate(const matrix& input, const matrix& target) {
			auto first = mLayers.front();
			auto last = mLayers.back();
			calculate_loss(last->output(), target);
			//core::info("Loss:\n%s\n", mLoss.str());
			if (mLayers.size() == 1) {
				first->backpropagate(input, mdLdA);
				return;
			}

			auto dLdin = mdLdA;
			auto it = mLayers.rbegin();
			while (it != mLayers.rend()) {
				auto nxt = std::next(it);
				auto& prev_out = (nxt == mLayers.rend()) ? input : (*nxt)->output();
				//core::info("dLdin:\n%s\n", dLdin.str());
				(*it)->backpropagate(prev_out, dLdin);
				dLdin = (*it)->dLdin();
				++it;
			}
		}

		void update(optimizers::baseoptimizer_t* optimizer) {
			for (auto L : mLayers)
				optimizer->update(L->Weights(), L->dLdW(), L->Bias(), L->dLdB());
		}
	};
}
