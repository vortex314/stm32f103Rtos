/*
 * CborReader.cpp
 *
 *  Created on: Nov 11, 2021
 *      Author: lieven
 */

#include "CborReader.h"

CborReader::CborReader(size_t sz) :
		_sz(sz) {
	_data = new uint8_t[sz];
	_value = new CborValue;
}

CborReader::~CborReader() {
	delete _data;
}

CborReader& CborReader::parse(uint8_t *buffer, size_t length) {
	while (_containers.size()) {
		delete _value;
		_value = _containers.back();
		_containers.pop_back();
	}
	_error = cbor_parser_init(buffer, length, 0, &_parser, _value);
	return *this;
}
CborReader& CborReader::parse(Bytes &bs) {

	return parse(bs.data(), bs.size());
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
	if (_error == CborNoError)
		_error = cbor_value_get_uint64(_value, &v);
	if (!_error)
		_error = cbor_value_advance(_value);
	return *this;
}

CborReader& CborReader::get(int64_t &v) {
	if (_error == CborNoError)
		_error = cbor_value_get_int64(_value, &v);
	if (!_error)
		_error = cbor_value_advance_fixed(_value);
	return *this;
}

CborReader& CborReader::get(int &v) {
	if (_error == CborNoError)
		_error = cbor_value_get_int(_value, &v);
	if (!_error)
		_error = cbor_value_advance_fixed(_value);
	return *this;
}

CborReader& CborReader::get(std::string &s) {
	if (_error == CborNoError) {
		if (cbor_value_is_text_string(_value)) {
//			size_t length;
//			_error = cbor_value_calculate_string_length(_value, &length);
			char *temp;
			size_t size;
			_error = cbor_value_dup_text_string(_value, &temp, &size, 0);
			if (_error == CborNoError) {
				s = temp;
			}
			::free(temp);
			if (!_error)
				_error = cbor_value_advance(_value);
		} else {
			_error = CborErrorIllegalType;
		}
	}
	return *this;
}
bool CborReader::ok() {
	return _error == CborNoError;
}

