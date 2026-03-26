#include "Mode.hpp"
#include <stdexcept>

Mode::Mode() : score(0) {
	std::fill(data1, data1 + sizeof(data1), cl_uchar(0));
	std::fill(data2, data2 + sizeof(data2), cl_uchar(0));
}

Mode Mode::benchmark() {
	Mode r;
	r.name = "benchmark";
	r.kernel = "profanity_score_benchmark";
	return r;
}

Mode Mode::zeros() {
	Mode r = range(0, 0);
	r.name = "zeros";
	return r;
}

static std::string::size_type hexValueNoException(char c) {
	if (c >= 'A' && c <= 'F') {
		c -= 'A' - 'a';
	}

	const std::string hex = "0123456789abcdef";
	const std::string::size_type ret = hex.find(c);
	return ret;
}

static std::string::size_type hexValue(char c) {
	const std::string::size_type ret = hexValueNoException(c);
	if(ret == std::string::npos) {
		throw std::runtime_error("bad hex value");
	}

	return ret;
}

// Fuzzy matching: pattern format per nibble position:
//   0-9,a-f = exact hex (one bit set in bitmask)
//   X = wildcard (bitmask = 0, skip)
//   [chars] = fuzzy set, e.g. [8b6] = accept 8, b, or 6
// data1[i] = low byte of 16-bit bitmask for nibble i (bits 0-7 = hex 0-7)
// data2[i] = high byte of 16-bit bitmask for nibble i (bits 0-7 = hex 8-f)
Mode Mode::fuzzy(const std::string strPattern) {
	Mode r;
	r.name = "fuzzy";
	r.kernel = "profanity_score_fuzzy";

	std::fill(r.data1, r.data1 + sizeof(r.data1), cl_uchar(0));
	std::fill(r.data2, r.data2 + sizeof(r.data2), cl_uchar(0));

	int nibbleIdx = 0;
	size_t i = 0;
	while (i < strPattern.size() && nibbleIdx < 40) {
		if (strPattern[i] == '[') {
			// Parse bracket group: [8b6] etc
			++i;
			cl_ushort mask = 0;
			while (i < strPattern.size() && strPattern[i] != ']') {
				auto val = hexValueNoException(strPattern[i]);
				if (val != std::string::npos) {
					mask |= (1 << val);
				}
				++i;
			}
			if (i < strPattern.size()) ++i; // skip ']'
			r.data1[nibbleIdx] = mask & 0xFF;
			r.data2[nibbleIdx] = (mask >> 8) & 0xFF;
			++nibbleIdx;
		} else if (strPattern[i] == 'X' || strPattern[i] == 'x') {
			// Wildcard - bitmask stays 0
			++nibbleIdx;
			++i;
		} else {
			auto val = hexValueNoException(strPattern[i]);
			if (val != std::string::npos) {
				// Exact match - single bit set
				cl_ushort mask = (1 << val);
				r.data1[nibbleIdx] = mask & 0xFF;
				r.data2[nibbleIdx] = (mask >> 8) & 0xFF;
				++nibbleIdx;
			}
			++i;
		}
	}

	r.score = nibbleIdx; // max possible score
	return r;
}

Mode Mode::matching(const std::string strHex) {
	Mode r;
	r.name = "matching";
	r.kernel = "profanity_score_matching";

	std::fill( r.data1, r.data1 + sizeof(r.data1), cl_uchar(0) );
	std::fill( r.data2, r.data2 + sizeof(r.data2), cl_uchar(0) );

	auto index = 0;
	
	for( size_t i = 0; i < strHex.size(); i += 2 ) {
		const auto indexHi = hexValueNoException(strHex[i]);
		const auto indexLo = i + 1 < strHex.size() ? hexValueNoException(strHex[i+1]) : std::string::npos;

		const auto valHi = (indexHi == std::string::npos) ? 0 : indexHi << 4;
		const auto valLo = (indexLo == std::string::npos) ? 0 : indexLo;

		const auto maskHi = (indexHi == std::string::npos) ? 0 : 0xF << 4;
		const auto maskLo = (indexLo == std::string::npos) ? 0 : 0xF;

		r.data1[index] = maskHi | maskLo;
		r.data2[index] = valHi | valLo;

		++index;
	}

	return r;
}

Mode Mode::leading(const char charLeading) {

	Mode r;
	r.name = "leading";
	r.kernel = "profanity_score_leading";
	r.data1[0] = static_cast<cl_uchar>(hexValue(charLeading));
	return r;
}

Mode Mode::range(const cl_uchar min, const cl_uchar max) {
	Mode r;
	r.name = "range";
	r.kernel = "profanity_score_range";
	r.data1[0] = min;
	r.data2[0] = max;
	return r;
}

Mode Mode::zeroBytes() {
	Mode r;
	r.name = "zeroBytes";
	r.kernel = "profanity_score_zerobytes";
	return r;
}

Mode Mode::letters() {
	Mode r = range(10, 15);
	r.name = "letters";
	return r;
}

Mode Mode::numbers() {
	Mode r = range(0, 9);
	r.name = "numbers";
	return r;
}

std::string Mode::transformKernel() const {
	switch (this->target) {
		case ADDRESS:
			return "";
		case CONTRACT:
			return "profanity_transform_contract";
		default:
			throw "No kernel for target";
	}
}

std::string Mode::transformName() const {
	switch (this->target) {
		case ADDRESS:
			return "Address";
		case CONTRACT:
			return "Contract";
		default:
			throw "No name for target";
	}
}

Mode Mode::leadingRange(const cl_uchar min, const cl_uchar max) {
	Mode r;
	r.name = "leadingrange";
	r.kernel = "profanity_score_leadingrange";
	r.data1[0] = min;
	r.data2[0] = max;
	return r;
}

Mode Mode::mirror() {
	Mode r;
	r.name = "mirror";
	r.kernel = "profanity_score_mirror";
	return r;
}

Mode Mode::doubles() {
	Mode r;
	r.name = "doubles";
	r.kernel = "profanity_score_doubles";
	return r;
}
