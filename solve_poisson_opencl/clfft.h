#pragma once
#include "util.h"
#include "precompiled.h"

namespace clfft {
	struct CLErrorCheck {
		void operator=(cl_int res) {
			if (res != CL_SUCCESS) {
				cout << "res = " << res << endl;
				throw std::runtime_error("foo");
			}
		}
	};

	class Plan {
	public:
		Plan(ivec2 sz, cl::CommandQueue& queue, cl::Context& context) {
			size_t clLengths[2] = { sz.x, sz.y };

			CLErrorCheck err;
			err = clfftCreateDefaultPlan(&id, context(), CLFFT_2D, clLengths);
			err = clfftSetPlanPrecision(id, CLFFT_SINGLE);
			err = clfftSetLayout(id, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);
			err = clfftSetResultLocation(id, CLFFT_INPLACE);
			err = clfftBakePlan(id, 1, &queue(), NULL, NULL);
		}

		clfftPlanHandle operator()() {
			return id;
		}
	private:
		clfftPlanHandle id;
	};
}