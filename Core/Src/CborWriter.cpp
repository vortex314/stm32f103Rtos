/*
 * CborWriter.cpp
 *
 *  Created on: Nov 11, 2021
 *      Author: lieven
 */
#include "CborWriter.h"
#include "Crc32.h"

CborWriter::CborWriter(size_t sz) {
	_capacity = sz;
	_data = (uint8_t*) (new uint32_t[(sz / 4) + 1]);
	_encoder = new CborEncoder;
	_error = CborNoError;
	_withCrc = false;
}
CborWriter::~CborWriter() {
	delete (uint32_t*) _data;
}
CborWriter& CborWriter::reset() {
	while (_encoders.size()) {
		delete _encoders.back();
		_encoders.pop_back();
	}
	_error = CborNoError;
	_withCrc = false;
	cbor_encoder_init(_encoder, _data, _capacity, 0);
	return *this;
}
CborWriter& CborWriter::array() {
	CborEncoder *arrayEncoder = new CborEncoder;
	_error =
			cbor_encoder_create_array(_encoder, arrayEncoder, CborIndefiniteLength);
	_encoders.push_back(_encoder);
	_encoder = arrayEncoder;
	return *this;
}
CborWriter& CborWriter::map() {
	CborEncoder *mapEncoder = new CborEncoder;
	_error =
			cbor_encoder_create_map(_encoder, mapEncoder, CborIndefiniteLength);
	_encoders.push_back(_encoder);
	_encoder = mapEncoder;
	return *this;
}
CborWriter& CborWriter::close() {
	if (_error == CborNoError) {
		if (_encoders.size() > 0) {
			CborEncoder *rootEncoder = _encoders.back();
			_error = cbor_encoder_close_container(rootEncoder, _encoder);
			delete _encoder;
			_encoder = rootEncoder;
			_encoders.pop_back();
		} else {
			_error = CborUnknownError;
		}
	}
	return *this;
}
Bytes CborWriter::bytes() {
	if (_error == CborNoError && _encoders.size() == 0) {
		size_t sz = cbor_encoder_get_buffer_size(_encoder, _data);
		return Bytes(_data, _data + sz + (_withCrc ? 2 : 0));
	}
	return Bytes(0, 0);
}
CborError CborWriter::error() {
	return _error;
}
CborWriter& CborWriter::add(uint64_t v) {
	if (_error == CborNoError) {
		_error = cbor_encode_uint(_encoder, v);
	}
	return *this;
}

CborWriter& CborWriter::add(int v) {
	if (_error == CborNoError) {
		_error = cbor_encode_int(_encoder, v);
	}
	return *this;
}

CborWriter& CborWriter::add(const char *v) {
	if (_error == CborNoError) {
		_error = cbor_encode_text_string(_encoder, v, strlen(v));
	}
	return *this;
}

CborWriter& CborWriter::addCrc() {
	size_t origSize = cbor_encoder_get_buffer_size(_encoder, _data);
	size_t sz = origSize;
	// add 0's to have multiple 4 bytes
	while ((sz % 4) != 0) {
		_data[sz++] = 0;
	}
	uint32_t crc = Crc32::bufferCrc((uint32_t*) _data, sz / 4);
	_data[origSize++] = crc & 0xFF;
	_data[origSize++] = (crc & 0xFF00) >> 8;
	_withCrc = true;
	return *this;
}
bool CborWriter::ok() {
	return _error == CborNoError;
}

