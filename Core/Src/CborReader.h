/*
 * CborReader.h
 *
 *  Created on: Nov 11, 2021
 *      Author: lieven
 */

#ifndef SRC_CBORREADER_H_
#define SRC_CBORREADER_H_


#include <cbor.h>
#include <cbor.h>
#include <vector>
#include <string>
#include <stdint.h>

typedef std::vector<uint8_t> Bytes;

class CborReader {
	uint8_t *_data;
	size_t _sz;
	CborValue *_value;
	CborError _error;
	CborParser _parser;
	std::vector<CborValue*> _containers;
public:
	CborReader(size_t sz) ;
	~CborReader();
	CborReader& parse(uint8_t *buffer, size_t length) ;
	CborReader& parse(Bytes &bs) ;
	CborReader& array() ;
	CborReader& close() ;
	CborReader& get(uint64_t &v) ;
	CborReader& get(int64_t &v);
	CborReader& get(int &v) ;
	CborReader& get(std::string &s) ;
	bool ok() ;
};


#endif /* SRC_CBORREADER_H_ */
