#ifndef __CODEC_H__
#define __CODEC_H__

#define HASHSIZE 32

// --------------------------------------------
// Decoder (morse -> ascii)
// --------------------------------------------
struct dec_list { /* table entry: */
    struct dec_list *next; /* next entry in chain */
    short morse; /* binary-encoded Morse character */
    char  ascii; /* ASCII character */
};

static struct dec_list *decoder_table[HASHSIZE]; /* pointer table */

struct dec_list *dec_get(short morse);
struct dec_list *dec_put(char ascii, short morse);


// --------------------------------------------
// Encoder (ascii -> morse)
// --------------------------------------------

short encode(char ascii);
struct dec_list *enc_get(char ascii);

// ----------------------------------------------
// Common methods
// ----------------------------------------------

void codec_init(void);

#endif
