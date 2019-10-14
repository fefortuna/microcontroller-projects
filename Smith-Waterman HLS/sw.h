//===========================================================================
// bnn.h
//===========================================================================
// @brief: This header file defines the interface for the core functions.

#ifndef SWH
#define SWH
#include "typedefs.h"
#include <hls_stream.h>

//const int MAX_FMAP = 5184; //18*18*16
#define SEQMAX 63
#define SEQMAX2 3654
const char mu = 1;   // mismatch
const char match = 3; // match
const char gap_penalty = 4;

// Output struct
typedef struct {
	hsize_t H;
	ap_int<2> origin;
} outstruct;

typedef struct{
	hsize_t H;
	int j;
} maxfind;

typedef struct{
	hsize_t H;
	int j;
	int i;
} maxfind2;

typedef struct {
	int i;
	int j;
} coord;

// Top function for bnn accelerator
#pragma SDS data access_pattern(seq_a:SEQUENTIAL, seq_b:SEQUENTIAL, origins:SEQUENTIAL, max:SEQUENTIAL)
// #pragma SDS data sys_port(seq_a:AFI, seq_b:AFI, origins:AFI, max:AFI)
// #pragma SDS data sys_port(outp:AFI)
void sw_xcel(char seq_a[SEQMAX2], char seq_b[SEQMAX2], ap_uint<128> origins[(SEQMAX2/SEQMAX) * (SEQMAX2+1) * 2], int max[3]);//hls::stream<float> &strm_out);

#endif
