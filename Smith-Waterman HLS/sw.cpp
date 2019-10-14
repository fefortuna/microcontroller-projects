//==========================================================================
// sw.cpp
//==========================================================================
// @brief: Smith-Waterman

#include "sw.h"
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;

//----------------------------------------------------------
// Global Vars
//----------------------------------------------------------

  //const float mu    = 0.33;
  //const float gap_penalty = 1.33;
//  const char mu = -2;   // mismatch
//  const char match = 3; // match
//  const char gap_penalty = 10;

/******************************************************************************/

void find_max(int x, maxfind find[SEQMAX+1], maxfind2* max) {

    hsize_t val = -1;            // start with max = first element
    for(int i = 0; i<=SEQMAX; i++) {
      #pragma HLS unroll
        if(find[i].H > val){            // prioritize the diagonal (temp[0])
            val = find[i].H;
            max->H = find[i].H;
            max->i = x+i;
            max->j = find[i].j;
        }
        find[i].H = 0;
        // cout<<find[i].H<<endl;
    }
}

void find_real_max(maxfind2 find[SEQMAX2/SEQMAX], maxfind2* max) {

    hsize_t val = -1;            // start with max = first element
    for(int i = 0; i<SEQMAX2/SEQMAX; i++) {
      #pragma HLS unroll
        if(find[i].H > val){            // prioritize the diagonal (temp[0])
            val = find[i].H;
            max->H = find[i].H;
            max->i = find[i].i;
            max->j = find[i].j;
        }
        //cout<<find[i].H<<endl;
    }
}


//----------------------------------------------------------
// SW Accelerator 
//----------------------------------------------------------
// @param[in] : 
// @return : 
void sw_xcel(char seq_a[SEQMAX2], char seq_b[SEQMAX2], ap_uint<128> origins[(SEQMAX2/SEQMAX) * (SEQMAX2+1) * 2], int max[3]){
  #pragma HLS stream variable=origins 
  char seq_a_[SEQMAX2];
  #pragma HLS array_partition variable=seq_a_ cyclic factor=63
  char seq_b_[SEQMAX];
  #pragma HLS array_partition variable=seq_b_
  hsize_t buffer[SEQMAX+1][2];  
  #pragma HLS array_partition variable=buffer dim=0
  hsize_t buffer2[2*SEQMAX2+1];
  maxfind find[SEQMAX+1];
  #pragma HLS array_partition variable=find

  for(int i=0;i<SEQMAX2;i++){
    // #pragma HLS unroll
    seq_a_[i]=seq_a[i];
  }

  for (int i = 0; i <= SEQMAX; i++){
    #pragma HLS unroll
    buffer[i][0] = 0;
    buffer[i][1] = 0;
    find[i].H = 0;
  }

  for(int i = 0;i<SEQMAX;i++){
    seq_b_[i] = 0;
  }

  for(int i = 0;i<2*SEQMAX2+1;i++){
    buffer2[i] = 0;
  }

  int ind = 0;
  maxfind2 max_;
  max_.H = 0;
  // max[0]=0;
  for(int x=0;x<SEQMAX2/SEQMAX;x++){

    for (int d = 0; d <= SEQMAX; d++){
      #pragma HLS unroll
      buffer[d][0] = 0;
      buffer[d][1] = 0;
    }

    for (int z = 0; z <= 2*SEQMAX2; z++){
      #pragma HLS pipeline II=1
      hsize_t top = 0;
      hsize_t bot = 0;
      ap_uint<128> idx = 0;
      ap_uint<128> send = 0;
      hsize_t temporary;

      for (int i = 0; i <= SEQMAX; i++){
        #pragma HLS unroll
        int j = z - i;
        hsize_t temp[4] = {0, 0, 0, 0};
        #pragma HLS array_partition variable=temp

        if (i >= 1) {
          temp[0] = ((seq_a_[x*SEQMAX+i-1] == seq_b_[i-1]) ? top + match : top - mu);
        }
        temp[1] = bot - gap_penalty;
        top = buffer[i][(z+0)%2];
        bot = buffer[i][(z+1)%2];
        temp[2] = bot - gap_penalty;

        int t1 = temp[1] > temp[0] ? 1:0;
        int t2 = temp[2] >= temp[3] ? 2:3;
        idx = temp[t1] >= temp[t2] ? t1 : t2;

        send = send | (idx<<(i*2));

        if (j >= 1 && j <= SEQMAX2 && i >= 1) {
          buffer[i][(z+0)%2] = temp[idx];
          if(find[i].H < temp[idx]){
            find[i].H = temp[idx];
            find[i].j = j;
          }
        }
        if(i==SEQMAX){
          temporary = temp[idx];
        }
      }

      buffer[0][(z)%2] = buffer2[z];
      if(z-SEQMAX >= 0){
        buffer2[z-SEQMAX] = temporary;
      }

      origins[ind] = send;

      for(int v = SEQMAX-1;v>0;v--){
        #pragma HLS unroll
        seq_b_[v] = seq_b_[v-1];
      }
      if(z>=1 && z<=SEQMAX2)
        seq_b_[0] = seq_b[z-1];
      ind++;
   }

   maxfind2 tem;
   find_max(x*SEQMAX, find, &tem);
   if(tem.H > max_.H){            // prioritize the diagonal (temp[0])
     max_.H=tem.H;
     max_.i=tem.i;
     max_.j=tem.j;
   } 
 }
  max[0] = max_.H;
  max[1] = max_.i;
  max[2] = max_.j;
}

