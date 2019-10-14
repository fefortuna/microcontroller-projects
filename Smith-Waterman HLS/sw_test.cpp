//=========================================================================
// sw_test.cpp
//=========================================================================
// @brief: testbench for Smith-Waterman (sw)

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <string>
#include <string.h>
#include <cmath>
#include "sw.h"
#include "timer.h"
#include "sds_lib.h"
using namespace std;

// Number of test instances
const int TEST_SIZE = 100;

//------------------------------------------------------------------------
// Helper function for reading images and labels
//------------------------------------------------------------------------

int gen_index(int i, int j){
  if (i+j < SEQMAX+1) 
    return i + (((i+j) * (i+j+1)) >> 1);
  else {
    int ip = SEQMAX-i;
    int jp = SEQMAX-j;
    return ((SEQMAX+1)*(SEQMAX+1)-1) - (((ip+jp)*(ip+jp+1)) >> 1) - ip;
  }
}

void gen_prev(ap_uint<128>* origins,  int i, int j, coord* c){
  int temp2 = (i%SEQMAX) == 0 ? i/SEQMAX -1: i/SEQMAX;
  int temp = temp2*(2*SEQMAX2+1) + i+j-(SEQMAX*temp2);
  temp2 = (i%SEQMAX) == 0 ? SEQMAX : i%SEQMAX;
  if(temp < (SEQMAX2/SEQMAX) * (SEQMAX2+1) * 2){
    switch((origins[temp] >> (2*temp2)) & 3){
      case 0:   // was case 0
        c->i = i - 1;
        c->j = j - 1;
        break;
      case 1: // was case 1
        c->i = i - 1;
        c->j = j;
        break;
      case 2:  // actually 2
        c->i = i;
        c->j = j - 1;
        break;
      case 3:
        c->i = i;
        c->j = j;
        break;
    }
  }
}

void checkfile(int open, char filename[]) {

    if (open) {
        cout << "Error: Can't open the file "<<filename<<endl;
        exit(1);
    }
    else cout<<"Opened file \"" << filename << "\"\n";
}

string read_sequence(ifstream& f)
{
    string seq;
    char line[20000];
    while( f.good() )
    {
        f.getline(line,20000);
        if( line[0] == 0 || line[0]=='#' )
            continue;
        for(int i = 0; line[i] != 0; ++i)
        {
            int c = toupper(line[i]);
            if( c != 'A' && c != 'G' && c != 'C' && c != 'T' )
                continue;
            seq.push_back(char(c));
        }
    }
    return seq;
}

void read_test_images(int8_t test_images[TEST_SIZE][256]) {
  std::ifstream infile("data/test_b.dat");
  if (infile.is_open()) {
    for (int index = 0; index < TEST_SIZE; index++) {
      for (int pixel = 0; pixel < 256; pixel++) {
        int i;
        infile >> i;
        test_images[index][pixel] = i;
      }
    }
    infile.close();
  }
}

void read_test_labels(int test_labels[TEST_SIZE]) {
  std::ifstream infile("data/label.dat");
  if (infile.is_open()) {
    for (int index = 0; index < TEST_SIZE; index++) {
      infile >> test_labels[index];
    }
    infile.close();
  }
}

//------------------------------------------------------------------------
// Digitrec testbench
//------------------------------------------------------------------------

int main(){
  char nameof_seq_a[30];
  strcpy(nameof_seq_a, "data/LITMUS28i.gbk.txt");
  char nameof_seq_b[30];
  strcpy(nameof_seq_b, "data/LITMUS28i-mal.gbk.txt");
  string seq_a,seq_b;

  // read the sequences into two vectors, seq_a and seq_b
  ifstream stream_seq_a;
  stream_seq_a.open(nameof_seq_a);
  checkfile(! stream_seq_a,nameof_seq_a);
  cout << "Reading file \"" << nameof_seq_a << "\"\n";
  seq_a = read_sequence(stream_seq_a);
  cout << "File \"" << nameof_seq_a << "\" read\n\n";
  ifstream stream_seq_b;
  stream_seq_b.open(nameof_seq_b);
  checkfile(! stream_seq_b,nameof_seq_b);
  cout << "Reading file \"" << nameof_seq_b << "\"\n";
  seq_b = read_sequence(stream_seq_b);
  cout << "File \"" << nameof_seq_b << "\" read\n\n";

  // Record sequence length and place string into char array
  unsigned int N_a = seq_a.length();
  unsigned int N_b = seq_b.length();
  char* charseq_a = (char*) sds_alloc(SEQMAX2*sizeof(char));
  char* charseq_b = (char*) sds_alloc(SEQMAX2*sizeof(char));
  strncpy(charseq_a, seq_a.c_str(),SEQMAX2);
  strncpy(charseq_b, seq_b.c_str(),SEQMAX2);
  for(int x=N_a;x<SEQMAX2;x++){
    charseq_a[x] =  1;
  }
  for(int x=N_b;x<SEQMAX2;x++){
    charseq_b[x] =  2;
  }


  

  cout << "First sequence has length  : " << N_a <<endl;
  cout << "Second sequence has length : " << N_b << endl << endl;

  // Allocate and zero output matrix
  cout << "Allocating origins matrix\n";
  ap_uint<128>* origins = (ap_uint<128> *) sds_alloc((SEQMAX2/SEQMAX) * (SEQMAX2+1) * 2 * sizeof(ap_uint<128>));
  
  if (!origins) return -1;
  for(int i = 0; i < (SEQMAX2/SEQMAX) * (SEQMAX2+1) * 2; i++) 
    origins[i] = 0;
  cout << "Memory for output matrix allocated and zeroed\n";

  // Smith-Waterman Algorithm
  Timer timer("Smith-Waterman");
  int* maxi = (int*)sds_alloc(3*sizeof(int));
  timer.start();
  sw_xcel(charseq_a, charseq_b, origins, maxi);// === Hardware implemented ======================
  //sw_xcel(charseq_a, charseq_b, outp); 
  timer.stop();
  //cout<<"max score: "<<H_max<<endl;
  int i_max = maxi[1];
  int j_max =  maxi[2];
  cout<<"max location: ("<<i_max<<", "<<j_max<<")"<<endl;
  cout<<"max score:"<<maxi[0]<<endl;
  // Backtracking from H_max
  int curr_i = i_max;
  int curr_j = j_max;
  coord c;
  gen_prev(origins, i_max, j_max, &c);
  int next_i = c.i; 
  int next_j = c.j;
  // int next_i = 0;
  // int next_j =0;

  int tick = 0;
  char consensus_a[N_a+N_b+2], consensus_b[N_a+N_b+2];
  while(((curr_i!=next_i) || (curr_j!=next_j)) && (next_j!=0) && (next_i!=0)) {
      if(next_i==curr_i)     consensus_a[tick] = '-';                  // deletion in A
      else                   consensus_a[tick] = charseq_a[curr_i-1];   // match/mismatch in A

      if(next_j==curr_j)     consensus_b[tick] = '-';                  // deletion in B
      else                   consensus_b[tick] = charseq_b[curr_j-1];   // match/mismatch in B
      // cout<<curr_i<<"  "<<curr_j<<endl;
      // cout<<charseq_a[curr_j-1]<<"  "<<charseq_b[curr_i-1]<<endl;
      curr_i = next_i;
      curr_j = next_j;
      gen_prev(origins, curr_i, curr_j, &c);
      next_i = c.i;
      next_j = c.j;
      tick++;
      
      //cout<<next_i<<"  "<<next_j<<endl<<endl;
  }
  consensus_a[tick] = charseq_a[0];
  consensus_b[tick] = charseq_b[0];

  // Print output
  cout << endl << "***********************************************" << endl;
  cout << "The alignment of the sequences" << endl << endl;
  for ( unsigned int i = 0 ; i < N_a; i++ ) 
    cout << charseq_a[i];
  cout << "  and " << endl;
  for ( unsigned int i = 0; i < N_b; i++ ) {
    cout << charseq_b[i];
  };
  cout << endl << endl;
  cout << "is for the parameters  miss = " << static_cast<signed>(mu) << ", match = " << static_cast<signed>(match) << " and gap penalty = " << static_cast<signed>(gap_penalty) << " given by" << endl << endl;
  for ( int i = tick-1; i >= 0; i-- ) 
    cout << consensus_a[i];
  cout<<endl;
  for ( int j = tick-1; j >= 0; j-- ) 
    cout << consensus_b[j];

 //cout<<endl<<"tick: "<<tick<<endl;
 cout<<endl;

  return 0;

}

