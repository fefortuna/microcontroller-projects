#include <stddef.h>
#include "codec.h"
#include <stdlib.h>

// --------------------------------------------------------------------
// Decoder (morse -> ascii)
// --------------------------------------------------------------------

/* hash: form hash value for short s */
unsigned dec_hash(short s)
{
    unsigned hashval = s % HASHSIZE + (s >> 5);
    return hashval % HASHSIZE;
}

/* dec_get: look for morse in decoder_table */
struct dec_list *dec_get(short morse)
{
    struct dec_list *np;
    for (np = decoder_table[dec_hash(morse)]; np != 0; np = np->next)
        if (morse == np->morse)
          return np; /* found */
    return 0; /* not found */
}

/* dec_put: put (name, defn) in decoder_table */
struct dec_list *dec_put(char ascii, short morse)
{
    struct dec_list *np;
    unsigned hashval;
    if ((np = dec_get(morse)) == 0) { /* not found */
        // Attempt to malloc
        np = (struct dec_list *) malloc(sizeof(struct dec_list));
        if (np == 0) // || (np->morse = morse) == NULL)
          return 0;
        
        // Fill it in
        np->morse = morse;
        // add to chain
        hashval = dec_hash(morse);
        np->next = decoder_table[hashval];
        decoder_table[hashval] = np;
    }
    // Assign the new value
    np->ascii = ascii;

    return np;
}

// ---------------------------------------------------------------------
// Encoder (ascii -> morse)
// ---------------------------------------------------------------------

/* hash: form hash value for char c */
unsigned enc_hash(char c)
{
    unsigned hashval = c;
    return hashval % HASHSIZE;
}

short encode (char ascii){
    struct dec_list* temp= enc_get(ascii);
    return temp->morse;
}

struct dec_list *enc_get(char ascii)
{
    struct dec_list *np;
    for (int i = 0; i < HASHSIZE; i++){
        for (np = decoder_table[i]; np != NULL; np = np->next){
            if (ascii == np->ascii) return np; // found
        }
    }
    /*
    struct enc_list *np;
    for (np = encoder_table[enc_hash(ascii)]; np != NULL; np = np->next)
        if (ascii == np->ascii)
            return np; // found
    */
    return NULL; // not found
}

// ------------------------------------------------------------------------
// Common methods
// ------------------------------------------------------------------------

void codec_init() {
    // Initialize both encoder and decoder
    
	dec_put('A', 0x180);
	dec_put('B', 0x254);
	dec_put('C', 0x264);
	dec_put('D', 0x250);
	dec_put('E', 0x100);
	dec_put('F', 0x164);
	dec_put('G', 0x290);
	dec_put('H', 0x154);
	dec_put('I', 0x140);
	dec_put('J', 0x1A8);
	dec_put('K', 0x260);
	dec_put('L', 0x194);
	dec_put('M', 0x280);
	dec_put('N', 0x240);
	dec_put('O', 0x2A0);
	dec_put('P', 0x1A4);
	dec_put('Q', 0x298);
	dec_put('R', 0x190);
	dec_put('S', 0x150);
	dec_put('T', 0x200);
	dec_put('U', 0x160);
	dec_put('V', 0x158);
	dec_put('W', 0x1A0);
	dec_put('X', 0x258);
	dec_put('Y', 0x268);
	dec_put('Z', 0x294);
    dec_put(' ', 0x000);
	//dec_put('0', 0x2AA);
	//dec_put('1', 0x1AA);
	//dec_put('2', 0x16A);
	//dec_put('3', 0x15A);
	//dec_put('4', 0x156);
	//dec_put('5', 0x155);
	//dec_put('6', 0x255);
	//dec_put('7', 0x295);
	//dec_put('8', 0x2A5);
	//dec_put('9', 0x2A9);
	
}
