/*
 * Spine.cpp
 *
 *  Created on: Nov 21, 2021
 *      Author: lieven
 */

#include "Spine.h"

Spine::Spine(Thread &thread)
		:
		Actor(thread), ticker(thread, 100, true, "ticker"), _framesRxd(5), _cborWriter(FRAME_MAX), _loopbackTimer(thread, 1000, true, "loopbackTimer"), _connectTimer(thread, 3000, true, "connectTimer") {
	_errorCount = 0;
	setNode(Sys::hostname());
	rxdFrame >> _framesRxd;
	_framesRxd.async(thread);
	_framesRxd >> [&](const Bytes &bs) {
		int msgType;
		_cborReader.fill(bs);
		if (_cborReader.checkCrc()) {
			if (_cborReader.parse().array().get(msgType).ok()) {
				switch (msgType) {
					case B_PUBLISH: {
						cborIn.on(_cborReader);
						break;
					}
					case B_SUBSCRIBE:
					case B_NODE: {
						WARN("I am not handling this");
						break;
					}
				}
			} else {
				INFO("get msgType failed");
			}
			_cborReader.close();
		} else {
			INFO("crc failed [%d] : %s", bs.size(), hexDump(bs).c_str());
		}
	};
}

void Spine::init() {
	_uptimePublisher = &publisher<uint64_t>("system/uptime");
	_latencyPublisher = &publisher<uint64_t>("system/latency");
	_loopbackSubscriber = &subscriber<uint64_t>("system/loopback");
	_loopbackPublisher = &publisher<uint64_t>(_loopbackTopic);
	_loopbackTimer >> [&](const TimerMsg &tm) {
		if (!connected()) sendNode(node.c_str());
		_loopbackPublisher->on(Sys::millis());
		_uptimePublisher->on(Sys::millis());

	};
	_connectTimer >> [&](const TimerMsg&) {
		INFO("disconnected");
		connected = false;
	};
	subscriber<uint64_t>(_loopbackTopic) >> [&](const uint64_t &in) {
		_latencyPublisher->on((Sys::millis() - in) * 1000);
		connected = true;
		INFO("connected");
		_connectTimer.reset();
	};
}

void Spine::setNode(const char *n) {
	node = n;
	srcPrefix = "src/" + node + "/";
	dstPrefix = "dst/" + node + "/";
	_loopbackTopic = dstPrefix + "system/loopback";
	connected = false;
}
/*
 void Spine::init() {

 ticker
 >> [&](const TimerMsg&) {
 count++;
 switch (count % 6) {
 case 0: {
 sendNode("stm32f103");
 break;
 }
 case 1: {
 publish("src/stm32f103/system/uptime", Sys::millis());
 break;
 }
 case 2: {
 publish("dst/stm32f103/system/loopback", Sys::millis());
 break;
 }
 case 3: {
 publish("src/stm32f103/system/highWaterMark", uxTaskGetStackHighWaterMark(NULL));
 break;
 }
 case 4: {
 publish("src/stm32f103/system/build", __DATE__ " " __TIME__);
 break;
 }
 case 5: {
 publish("src/stm32f103/system/pi", 3.141592653);
 break;
 }
 }
 };
 }*/
//================================= BROKER COMMANDS =========================================
void Spine::sendNode(const char *topic) {
	if (_cborWriter.reset().array().add(B_NODE).add(topic).close().addCrc().ok())
		txdFrame.on(_cborWriter.bytes());
}

