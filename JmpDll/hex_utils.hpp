#pragma once

#include <string>



/**
 * @brief  Gets a double value from the hex string
 *
 * @param  the hex string that is to be converted
 *
 * @return  a double value
 */
double hex_to_double(const std::string& hexstr) {
	double d;
	uint64_t int_bits = std::stoull(hexstr, nullptr, 16);
	std::memcpy(&d, &int_bits, sizeof(double));
	return d;
}
