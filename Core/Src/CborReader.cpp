/*
 * CborReader.cpp
 *
 *  Created on: Nov 11, 2021
 *      Author: lieven
 */

#include "CborReader.h"
#include "Crc32.h"
#include "log.h"

CborReader::CborReader(size_t capacity) {
	_data = 0;
	_capacity=0;
	_data = allocate(capacity);
	_value = new CborValue;
	_error = CborNoError;
	_size = 0;
}

uint8_t*  CborReader::allocate(size_t capacity){
	if ( _capacity > capacity ) return _data;
	if ( _data ) delete _data;
	_capacity = capacity;
	return  (uint8_t*) (new uint32_t[(capacity / 4) + 1]);
}

CborReader::~CborReader() {
	delete _data;
}

CborReader& CborReader::fill(const uint8_t *data, size_t length) {
	_data = allocate(length);
	for (size_t i = 0; i < length; i++) {
		_data[i] = data[i];
	}
	_size = length;
	_error = CborNoError;
	return *this;
}

CborReader& CborReader::parse()  {
	while (_containers.size()) {
		delete _value;
		_value = _containers.back();
		_containers.pop_back();
	}
	_error = cbor_parser_init(_data, _size, 0, &_parser, _value);
	return *this;
}
CborReader& CborReader::fill(const Bytes &bs) {
	return fill((const uint8_t*) (bs.data()), bs.size());
}

CborReader& CborReader::array() {
	if (_error == CborNoError && cbor_value_is_container(_value)) {
		CborValue *arrayValue = new CborValue;
		_error = cbor_value_enter_container(_value, arrayValue);
		if (!_error) {
			_containers.push_back(_value);
			_value = arrayValue;
		} else
			delete arrayValue;
	} else {
		_error = CborErrorIllegalType;
	}
	return *this;
}
CborReader& CborReader::close() {
	if (_error == CborNoError && _containers.size() > 0) {
		CborValue *it = _containers.back();
		while (cbor_value_get_type(_value) != CborInvalidType
				&& !cbor_value_at_end(_value))
			cbor_value_advance(_value);
		if (cbor_value_is_container(it)) {
			_error = cbor_value_leave_container(it, _value);
			delete _value;
			_value = it;
			_containers.pop_back();
		} else
			_error = CborErrorIllegalType;
	}
	return *this;
}

CborReader& CborReader::get(uint64_t &v) {
	if (_error == CborNoError) _error = cbor_value_get_uint64(_value, &v);
	if (!_error) _error = cbor_value_advance(_value);
	return *this;
}

CborReader& CborReader::get(uint32_t &ui) {
	uint64_t v=0;
	if (_error == CborNoError) _error = cbor_value_get_uint64(_value, &v);
	if (!_error) {
		ui = v;
		_error = cbor_value_advance(_value);
	}
	return *this;
}

CborReader& CborReader::get(int64_t &v) {
	if (_error == CborNoError) _error = cbor_value_get_int64(_value, &v);
	if (!_error) _error = cbor_value_advance_fixed(_value);
	return *this;
}

CborReader& CborReader::get(int &v) {
	if (_error == CborNoError) _error = cbor_value_get_int(_value, &v);
	if (!_error) _error = cbor_value_advance_fixed(_value);
	return *this;
}

CborReader& CborReader::get(std::string &s) {
	if (_error == CborNoError) {
		if (cbor_value_is_text_string(_value)) {
			//			size_t length;
			//			_error =
			// cbor_value_calculate_string_length(_value, &length);
			char *temp;
			size_t size;
			_error = cbor_value_dup_text_string(_value, &temp, &size, 0);
			if (_error == CborNoError) {
				s = temp;
			}
			::free(temp);
			if (!_error) _error = cbor_value_advance(_value);
		} else {
			_error = CborErrorIllegalType;
		}
	}
	return *this;
}

bool CborReader::checkCrc() {
	if (_size < 3) {
		_error = CborErrorOutOfMemory;
		return false;
	}
	uint16_t crc16 = _data[_size - 1];
	crc16 <<= 8;
	crc16 += _data[_size - 2];
	size_t cursor = _size - 2;
	while ((cursor % 4) != 0) {
		_data[cursor] = 0;
		cursor++;
	}
	uint32_t crc = Crc32::bufferCrc((uint32_t*) _data, cursor / 4);
	if ((crc & 0xFFFF) == crc16) {
		_size -= 2;
		return true;
	} else {
		_data[_size - 2] = crc16 & 0xFF;
		_data[_size - 1] = (crc16 & 0xFF00) >> 8;
		return false;
	}
}

Bytes CborReader::toBytes() {
	return Bytes(_data, _data + _size);
}

#include <cborjson.h>
#include <stdarg.h>

CborError stringStreamFunction(void *arg, const char *fmt, ...) {
	std::string str;
	str.clear();
	int size = strlen(fmt) * 2 + 50; // Use a rubric appropriate for your code
	if (size > 10240) fprintf(stdout, " invalid log size\n");
	va_list ap;
	while (1) { // Maximum two passes on a POSIX system...
		assert(size < 10240);
		str.resize(size);
		va_start(ap, fmt);
		int n = vsnprintf((char*) str.data(), size, fmt, ap);
		va_end(ap);
		if (n > -1 && n < size) { // Everything worked
			str.resize(n);
			break;
		}
		if (n > -1)     // Needed size returned
			size = n + 1; // For null char
		else
			size *= 2; // Guess at a larger size (OS specific)
	}
	std::string *args = (std::string*) arg;
	*args += str;
	return CborNoError;
}

CborReader& CborReader::toJson(std::string &json) {
	_error =
			cbor_value_to_pretty_stream(stringStreamFunction, &json, _value, 0);
	return *this;
}

bool CborReader::ok() {
	return _error == CborNoError;
}
