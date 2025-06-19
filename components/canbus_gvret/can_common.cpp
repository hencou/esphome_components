#include "can_common.h"

namespace esphome {
namespace canbus_gvret {

//CAN FD only allows discrete frame lengths after the normal CAN 8 byte limit. They are encoded below
//as a FLASH based look up table for ease of use
const uint8_t fdLengthEncoding[65] = {0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,9 ,9 ,9 ,10,10,10,
								  10,11,11,11,11,12,12,12,12,13,13,13,13,13,13,13,
								  13,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
								  14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};

CAN_FRAME::CAN_FRAME()
{
	id = 0;
	fid = 0;
	rtr = 0;
	priority = 15;
	extended = false;
	timestamp = 0;
	length = 0;
	data.value = 0;
}

CAN_FRAME_FD::CAN_FRAME_FD()
{
	id = 0;
	fid = 0;
	rrs = 0;
	fdMode = 0;
	priority = 15;
	extended = false;
	timestamp = 0;
	length = 0;
	for (int i = 0; i < 8; i++) data.uint64[i] = 0;
}
}
}