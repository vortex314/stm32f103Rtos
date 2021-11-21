/*
 * CborWriter.h
 *
 *  Created on: Nov 11, 2021
 *      Author: lieven
 */

#ifndef SRC_CBORWRITER_H_
#define SRC_CBORWRITER_H_

#include <cbor.h>
#include <vector>
#include <stdint.h>
#include <string>

#include "Sys.h"
class CborWriter {
	std::vector<CborEncoder*> _encoders;
	CborEncoder *_encoder;
	uint8_t* _data;
	uint32_t _capacity;
	CborError _error;
	bool _withCrc;
public:
	CborWriter(size_t sz) ;
	~CborWriter();
	CborWriter& reset();
	CborWriter& array();
	CborWriter& map() ;
	CborWriter& close() ;
	Bytes bytes();
	CborError error();
	CborWriter& add(const char *v) ;
	CborWriter& add(const std::string&);
	CborWriter& add(int);
	CborWriter& add(uint32_t);
	CborWriter& add(uint64_t);
	CborWriter& add(int64_t);
	CborWriter& add(double d);
	CborWriter& add(bool);
	CborWriter& addCrc();
	bool ok() ;
};



#endif /* SRC_CBORWRITER_H_ */
