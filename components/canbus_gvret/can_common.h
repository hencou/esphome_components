#ifndef _CAN_COMMON_
#define _CAN_COMMON_

#include <Arduino.h>

namespace esphome {
namespace canbus_gvret {

/** Define the typical baudrate for CAN communication. */
#ifdef CAN_BPS_500K
#undef CAN_BPS_1000K
#undef CAN_BPS_800K
#undef CAN_BPS_500K
#undef CAN_BPS_250K
#undef CAN_BPS_125K
#undef CAN_BPS_50K
#undef CAN_BPS_33333
#undef CAN_BPS_25K
#endif

#define CAN_BPS_1000K	1000000
#define CAN_BPS_800K	800000
#define CAN_BPS_500K	500000
#define CAN_BPS_250K	250000
#define CAN_BPS_125K	125000
#define CAN_BPS_50K		50000
#define CAN_BPS_33333	33333
#define CAN_BPS_25K		25000


#define SIZE_LISTENERS	4 //number of classes that can register as listeners with this class
#define CAN_DEFAULT_BAUD	500000
#define CAN_DEFAULT_FD_RATE 4000000

class BitRef
{
public:
    BitRef& operator=( bool x )
    {
        *byteRef = (*byteRef & ~(1 << bitPos));
        if (x) *byteRef = *byteRef | (1 << bitPos);
        return *this;
    }
    //BitRef& operator=( const BitRef& x );

    operator bool() const 
    {
        if (*byteRef & (1 << bitPos)) return true;
        return false;
    }
public:
    BitRef(uint8_t *ref, int pos)
    {
        byteRef = ref;
        bitPos = pos;
    }
private:
    uint8_t *byteRef;
    int bitPos;
};

typedef union {
    uint64_t uint64;
    uint32_t uint32[2]; 
    uint16_t uint16[4];
    uint8_t  uint8[8];
    int64_t int64;
    int32_t int32[2]; 
    int16_t int16[4];
    int8_t  int8[8];

    //deprecated names used by older code
    uint64_t value;
    struct {
        uint32_t low;
        uint32_t high;
    };
    struct {
        uint16_t s0;
        uint16_t s1;
        uint16_t s2;
        uint16_t s3;
    };
    uint8_t bytes[8];
    uint8_t byte[8]; //alternate name so you can omit the s if you feel it makes more sense
    struct {
        uint8_t bitField[8];
        const bool operator[]( int pos ) const
        {
            if (pos < 0 || pos > 63) return 0;
            int bitFieldIdx = pos / 8;
            return (bitField[bitFieldIdx] >> pos) & 1;
        }
        BitRef operator[]( int pos )
        {
            if (pos < 0 || pos > 63) return BitRef((uint8_t *)&bitField[0], 0);
            uint8_t *ptr = (uint8_t *)&bitField[0]; 
            return BitRef(ptr + (pos / 8), pos & 7);
        }
    } bit;
} BytesUnion;

typedef union {
    uint64_t uint64[8];
    uint32_t uint32[16]; 
    uint16_t uint16[32];
    uint8_t  uint8[64];
    int64_t int64[8];
    int32_t int32[16]; 
    int16_t int16[32];
    int8_t  int8[64];

    struct {
        uint8_t bitField[64];
        const bool operator[]( int pos ) const
        {
            if (pos < 0 || pos > 511) return 0; //64 8 bit bytes is 512 bits, we start counting bits at 0
            int bitfieldIdx = pos / 8;
            return (bitField[bitfieldIdx] >> pos) & 1;
        }
        BitRef operator[]( int pos )
        {
            if (pos < 0 || pos > 511) return BitRef((uint8_t *)&bitField[0], 0);
            uint8_t *ptr = (uint8_t *)&bitField[0]; 
            return BitRef(ptr + (pos / 8), pos & 7);
        }
    } bit;
} BytesUnion_FD;

class CAN_FRAME
{
public:
    CAN_FRAME();

    BytesUnion data;    // 64 bits - lots of ways to access it.
    uint32_t id;        // 29 bit if ide set, 11 bit otherwise
    uint32_t fid;       // family ID - used internally to library
    uint32_t timestamp; // CAN timer value when mailbox message was received.
    uint8_t rtr;        // Remote Transmission Request (1 = RTR, 0 = data frame)
    uint8_t priority;   // Priority but only important for TX frames and then only for special uses (0-31)
    uint8_t extended;   // Extended ID flag
    uint8_t length;     // Number of data bytes
    
};

class CAN_FRAME_FD
{
public:
    CAN_FRAME_FD();

    BytesUnion_FD data;   // 64 bytes - lots of ways to access it.
    uint32_t id;          // EID if ide set, SID otherwise
    uint32_t fid;         // family ID
    uint32_t timestamp;   // CAN timer value when mailbox message was received.
    uint8_t rrs;          // RRS for CAN-FD (optional 12th standard ID bit)
    uint8_t priority;     // Priority but only important for TX frames and then only for special uses. (0-31)
    uint8_t extended;     // Extended ID flag
    uint8_t fdMode;       // 0 = normal CAN frame, 1 = CAN-FD frame
    uint8_t length;       // Number of data bytes
};

}
}

#endif

