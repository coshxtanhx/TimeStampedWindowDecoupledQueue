#ifndef SUBJECT_TYPE_H
#define SUBJECT_TYPE_H

#include <array>
#include <string>

namespace benchmark {
	enum class Subject : uint8_t {
		kNone, kTSCAS, kTSStutter, kTSAtomic, kTSInterval, kCBO, k2Dd, kTSWD
	};

	inline std::string GetSubjectName(Subject subject)
	{
		constexpr std::array<const char*, 8> names{
			"None", "TS-CAS", "TS-stutter", "TS-atomic", "TS-interval",
			"d-CBO", "2Dd", "TSWD"
		};

		return names[static_cast<int>(subject)];
	}
}

#endif
