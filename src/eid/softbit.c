/*                                                        V.3.1 - 05.feb.2010
  ===========================================================================
   The file containing an encoded speech bitstream can be in a compact
   binary format, in the G.192 serial bitstream format (which uses
   16-bit softbits), or in the byte-oriented G.192 format.

   The file containing the error pattern will be in one of three
   possible formats: G.192 16-bit softbit format (without synchronism
   header for bit errors), byte-oriented version of the G.192 format,
   and compact, hard-bit binary (bit) mode. These are described in the
   following.

   The headerless G.192 serial bitstream format is as described in
   G.192, with the exceptions listed below. The main feature is that
   the softbits and frame erasure indicators are right-aligned at
   16-bit word boundaries (unsigned short): 
   '0'=0x007F and '1'=0x0081, and good/bad frame = 0x6B21/0x6B20

   In the byte-oriented softbit serial bitstream, only the lower byte
   of the softbits defined in G.192 are used. Hence:
   '0'=0x7F and '1'=0x81, and good/bad frame = 0x21/0x20

   In the compact (bit) mode, only hard bits are saved. Each byte will
   have information about eight bits or frames. The LBbs will refer to
   bits or frames that occur first in time. Here, '1' means that a bit
   is in error or that a frame should be erased, and a '0', otherwise.

   Conventions:
   ~~~~~~~~~~~~

   Bitstreams can be disturbed in two ways: by bit errors, or by frame
   erasures. The STL EID supports three basic modes: bit errors using
   Gilbert's model (labeled BER), frame erasures using Gilbert's model
   (labeled FER), and Bellcore model burst frame erasure (labeled
   BFER). Here are some conventions that apply to the particular
   formats for each of these three EID operating modes.

   BER: bitstream generated by this program are composed of bits 1/0,
        *without* synchronism headers or any other frame delimitation
        (i.e., only bits affecting the payload are present). Frame
        boundaries are defined by the user's application only. The
        following applies:
        G.192 mode: file will contain either 0x007F (no disturbance) or
	            0x0081 (bit error)
        Byte mode:  file will contain either 0x7F (no disturbance) or
	            0x81 (bit error)
        Compact mode: each bit in the file will indicate whether a
	            disturbance occurred (bit 1) or not (bit 0).
		    Lower order bits apply to bits occurring first
	            in time.

   FER/BFER: bitstream generate by this program is composed only by
        the indication of whether a frame should be erased or not. No
        payload is present. The following applies:
        G.192 mode: file will contain either 0x6B21 (no disturbance) or
	            0x6B20 (frame erasure)
        Byte mode:  file will contain either 0x21 (no disturbance) or
	            0x20 (frame erasure)
        Compact mode: each bit in the file will indicate whether a
	            frame erasure occurred (bit 1) or not (bit 0).
		    Lower order bits apply to bits occurring first
	            in time.

  ===========================================================================
*/
/* ..... Generic include files ..... */
#include "ugstdemo.h"           /* general UGST definitions */
#include <stdio.h>              /* Standard I/O Definitions */
#include <math.h>
#include <stdlib.h>
#include <string.h>             /* memset */
#include <ctype.h>              /* toupper */

/* ..... OS-specific include files ..... */
#if defined (unix) && !defined(MSDOS)
/*                 ^^^^^^^^^^^^^^^^^^ This strange construction is necessary
                                      for DJGPP, because "unix" is defined,
				      even it being MSDOS! */
#if defined(__ALPHA)
#include <unistd.h>             /* for SEEK_... definitions used by fseek() */
#else
#include <sys/unistd.h>         /* for SEEK_... definitions used by fseek() */
#endif
#endif

/* Specific includes */
#include "softbit.h"


/* 
   -------------------------------------------------------------------------
   long read_g192 (short *patt, long n, FILE *F);
   ~~~~~~~~~~~~~~

   Read a G.192-compliant 16-bit serial bitstream error pattern.

   Parameter:
   ~~~~~~~~~~
   patt .... G.192 array with the softbits representing 
             the bit error/frame erasure pattern
   n ....... number of softbits in the pattern
   F ....... pointer to FILE where the pattern should be readd

   Return value: 
   ~~~~~~~~~~~~~
   Returns a long with the number of shorts softbits from file. On error, 
   returns -1.

   Original author: <simao.campos@comsat.com>
   ~~~~~~~~~~~~~~~~

   History:
   ~~~~~~~~
   13.Aug.97  v.1.0  Created.
   -------------------------------------------------------------------------
 */
long read_g192 (patt, n, F)
     short *patt;
     long n;
     FILE *F;
{
  long i;

  /* Read words from file */
  i = fread (patt, sizeof (short), n, F);

  /* Return no.of samples read or error */
  return (ferror (F) ? -1l : i);
}

/* ....................... End of read_g192() ....................... */


/*
  -------------------------------------------------------------------------
  Read bit error pattern from a file in compressed binary (bit)
  format. This is a front-end function for the more generic read_bit()
  function.
  -------------------------------------------------------------------------
 */
long read_bit_ber (patt, n, F)
     short *patt;
     long n;
     FILE *F;
{
  return (read_bit (patt, n, F, BER));
}

/* ....................... End of read_bit_ber() ....................... */


/*
  -------------------------------------------------------------------------
  Read frame erasure pattern from a file in compressed binary (bit)
  format. This is a front-end function for the more generic read_bit()
  function.
  -------------------------------------------------------------------------
 */
long read_bit_fer (patt, n, F)
     short *patt;
     long n;
     FILE *F;
{
  return (read_bit (patt, n, F, FER));
}

/* ....................... End of read_bit_fer() ....................... */


/* 
   -------------------------------------------------------------------------
   long read_bit (short *patt, long n, FILE *F, char type);
   ~~~~~~~~~~~~~

   Read a bit-oriented file where the LSb corresponds to the bit that
   occurs first in time and saves into a headerless G.192 array.

   Parameter:
   ~~~~~~~~~~
   patt .... returned G.192 array with the softbits representing
             the bit error/frame erasure pattern
   n ....... number of softbits in the pattern
   F ....... pointer to FILE where the pattern should be read
   type .... bitstream type (FER or BER). The user need to provide
             this infoprmation, since it can not be inferred from
	     a compact binary bitstream file

   Return value: 
   ~~~~~~~~~~~~~
   Returns a long with the number of softbits read from a file. On error, 
   returns -1.

   Original author: <simao.campos@comsat.com>
   ~~~~~~~~~~~~~~~~

   History:
   ~~~~~~~~
   15.Aug.97  v.1.0  Created.
   ------------------------------------------------------------------------- 
*/
long read_bit (patt, n, F, type)
     short *patt;
     long n;
     FILE *F;
     char type;
{
  char *bits;
  short *p = patt;
  long bitno, j, k, nbytes, rbytes, ret_val;

  /* Skip function if no samples are to be read */
  if (n == 0)
    return (0);

  /* Calculate number of bytes necessary in the compact bitstream */
  if (n % 8) {
    fprintf (stderr, "The number of errors is not byte-aligned. \n");
    fprintf (stderr, "Zero insertion is supposed!\n");
  }
  nbytes = (long) (ceil (n / 8.0));

  /* Allocate memory */
  if ((bits = (char *) calloc (nbytes, sizeof (char))) == NULL)
    HARAKIRI ("Cannot allocate memory to read compact binary bitstream\n", 6);

  /* Reset memory to zero */
  memset (patt, 0, sizeof (short) * n);

  /* Read words from file; return on error */
  rbytes = fread (bits, sizeof (char), nbytes, F);

  /* Perform action according to returned no. of items */
  if (ferror (F))
    ret_val = -1l;
  /* 
     else if (feof(F)) ret_val = 0; */
  else {
    /* Convert compact bit oriented data to byte-oriented data */
    for (p = patt, bitno = j = 0; j < rbytes; j++) {
      /* Get first bit */
      *p++ = bits[j] & 0x0001;
      bitno++;                  /* One bit less to go! */

      /* "Expand" bits into hard bit words ... */
      for (k = 1; k < 8 && bitno < n; k++, bitno++)
        *p++ = (bits[j] >> k) & 0x0001;
    }

    /* Convert hard bits to soft bits, frame sync or frame erasure */
    switch (type) {
    case BER:
      for (p = patt, j = 0; j < bitno; j++, p++)
        *p = (*p) ? 0x0081 : 0x007F;
      break;
    case FER:
      for (p = patt, j = 0; j < bitno; j++, p++)
        *p = (*p) ? 0x6B20 : 0x6B21;
      break;
    }
    ret_val = bitno;
  }

  /* Free memory and quit */
  free (bits);
  return (ret_val);
}

/* ....................... End of read_bit() ....................... */


/* 
   -------------------------------------------------------------------------
   long read_byte (short *patt, long n, FILE *F);
   ~~~~~~~~~~~~~~~

   Read a G.192 error pattern from a file containing byte-oriented
   serial bitstream error pattern. The follwoing map is used:

   0x007F <- 0x7F ('0' softbit)
   0x0081 <- 0x81 ('1' softbit)
   0x0021 <- 0x21 (Frame OK)
   0x0020 <- 0x20 (Frame erasure)

   NOTE: The user is responsible for having only these four values in
         the input file. The code will NOT check for compliance.

   Parameter:
   ~~~~~~~~~~
   patt .... char array with the softbits representing the 16-bit 
             bit error softbit/frame erasure pattern
   n ....... number of softbits to be read
   F ....... pointer to FILE where the pattern should be read

   Return value: 
   ~~~~~~~~~~~~~
   Returns a long with the number of softbits read from a file. On error, 
   returns -1.

   Original author: <simao.campos@comsat.com>
   ~~~~~~~~~~~~~~~~

   History:
   ~~~~~~~~
   15.Aug.97  v.1.0  Created.
   ------------------------------------------------------------------------- 
*/
long read_byte (patt, n, F)
     short *patt;
     long n;
     FILE *F;
{
  char *byte;
  long i;
  unsigned char register tmp;

  /* Skip function if no samples are to be read */
  if (n == 0)
    return (0);

  /* Allocate memory */
  if ((byte = (char *) calloc (n, sizeof (char))) == NULL)
    HARAKIRI ("Cannot allocate memory to read data as byte bitstream\n", 6);

  /* Read words from file */
  i = fread (byte, sizeof (char), n, F);
  if (i < n) {
    /* the read operation returned less samples than expected */
    if (i <= 0)
      return (i);               /* Error or EOF */
    else
      n = i;                    /* Frame is shorter than expected */
  }

  /* Convert byte-oriented data to word16-oriented data */
  for (i = 0; i < n; i++) {
    tmp = byte[i];
    if (tmp == 0x20 || tmp == 0x21)     /* It is a frame sync/erasure word */
      patt[i] = 0x6B00 | tmp;
    else
      patt[i] = tmp;
  }

  /* Free memory and quit */
  free (byte);
  return (n);
}

/* ....................... End of read_byte() ....................... */


/* 
  -------------------------------------------------------------------------
  long save_g192 (short *patt, long n, FILE *F);
  ~~~~~~~~~~~~~~

  Save a G.192-compliant 16-bit serial bitstream error pattern.

  Parameter:
  ~~~~~~~~~~
  patt .... headerless G.192 array with the softbits representing 
            the bit error/frame erasure pattern
  n ....... number of softbits in the pattern
  F ....... pointer to FILE where the pattern should be saved

  Return value: 
  ~~~~~~~~~~~~~
  Returns a long with the number of shorts saved to a file. On error, 
  returns -1.

  Original author: <simao.campos@comsat.com>
  ~~~~~~~~~~~~~~~~

  History:
  ~~~~~~~~
  13.Aug.97  v.1.0  Created.
  5.Feb.2010 v.1.1  Corrected explicit return  type for  fwrite <visual> 

  -------------------------------------------------------------------------
*/
long save_g192 (patt, n, F)
     short *patt;
     long n;
     FILE *F;
{
  long i;
  i = fwrite (patt, sizeof (short), n, F);
  return (i < n ? -1l : n);
}

/* ....................... End of save_g192() ....................... */


/* 
  -------------------------------------------------------------------------
  long save_bit (short *patt, long n, FILE *F);
  ~~~~~~~~~~~~~

  Save a headerless G.192 error pattern as a bit-oriented file where the
  LSb corresponds to the bit that occurs first in time.

  Parameter:
  ~~~~~~~~~~
  patt .... headerless G.192 array with the softbits representing
            the bit error/frame erasure pattern
  n ....... number of softbits in the pattern
  F ....... pointer to FILE where the pattern should be saved

  Return value: 
  ~~~~~~~~~~~~~
  Returns a long with the number of shorts saved to a file. On error, 
  returns -1.

  Original author: <simao.campos@comsat.com>
  ~~~~~~~~~~~~~~~~

  History:
  ~~~~~~~~
  15.Aug.97  v.1.0  Created.
  -------------------------------------------------------------------------
*/
#define IS_ONE(x)  ((x) && G192_ONE)
long save_bit (patt, n, F)
     short *patt;
     long n;
     FILE *F;
{
  char *bits;
  short one, *p = patt;
  long i, j, k, nbytes;
  char register tmp;

  /* Skip function if no samples are to be read */
  if (n == 0)
    return (0);

  /* Calculate number of bytes necessary in the compact bitstream */
  if (n % 8) {
    fprintf (stderr, "The number of errors is not byte-aligned. \n");
    fprintf (stderr, "Zero insertion will be used and need to be \n");
    fprintf (stderr, "accounted for by the error-insertion program!\n");
  }
  nbytes = (long) (ceil (n / 8.0));

  /* Allocate memory */
  if ((bits = (char *) calloc (nbytes, sizeof (char))) == NULL)
    HARAKIRI ("Cannot allocate memory to save compact binary bitstream\n", 6);

  /* Reset memory to zero */
  memset (bits, 0, nbytes);

  /* Scan to determine whether it is a bit error or a frame erasure array */
  switch (*p) {
  case G192_ZERO:
  case G192_ONE:               /* Bit error */
    one = G192_ONE;
    break;
  case G192_SYNC:
  case G192_FER:               /* Frame erasure */
    one = G192_FER;
    break;
  }

  /* Convert byte-oriented to compact bit oriented data */
  for (i = j = 0; j < nbytes; j++) {
    /* Get 1st bit ... */
    tmp = (*p++ == one) ? 1 : 0;

    /* Compact all the other bits ... */
    for (k = 1; k < 8 && i < n; k++, i++) {
      tmp += (unsigned char) (((*p++) == one ? 1 : 0) << k);
    }

    /* Save word as short */
    bits[j] = tmp;
  }

  /* Save words to file */
  i = fwrite (bits, sizeof (char), nbytes, F);

  /* Free memory and quit */
  free (bits);
  return (n < i ? -1l : n);
}

/* ....................... End of save_bit() ....................... */


/* 
  -------------------------------------------------------------------------
  long save_byte (short *patt, long n, FILE *F);
  ~~~~~~~~~~~~~~~

  Save a G.192 error pattern as a byte-oriented serial bitstream
  error pattern by writing to file only the lower byte of each 16-bit
  word. The follwoing map is used:
  0x007F -> 0x7F ('0' softbit)
  0x0081 -> 0x81 ('1' softbit)
  0x0021 -> 0x21 (Frame OK)
  0x0020 -> 0x20 (Frame erasure)
  NOTE: The user is responsible for having only these four values in
        the input array. The code will NOT check for compliance.

  Parameter:
  ~~~~~~~~~~
  patt .... char array with the softbits representing the bit
            error/frame erasure pattern
  n ....... number of softbits in the pattern
  F ....... pointer to FILE where the pattern should be saved

  Return value: 
  ~~~~~~~~~~~~~
  Returns a long with the number of shorts saved to a file. On error, 
  returns -1.

  Original author: <simao.campos@comsat.com>
  ~~~~~~~~~~~~~~~~

  History:
  ~~~~~~~~
  15.Aug.97  v.1.0  Created.
  ------------------------------------------------------------------------- 
*/
long save_byte (patt, n, F)
     short *patt;
     long n;
     FILE *F;
{
  char *byte;
  long i;

  /* Skip function if no samples are to be read */
  if (n == 0)
    return (0);

  /* Allocate memory */
  if ((byte = (char *) calloc (n, sizeof (char))) == NULL)
    HARAKIRI ("Cannot allocate memory to save data as byte bitstream\n", 6);

  /* Convert word16-oriented data to byte-oriented data */
  /* NO compliance verification is performed, for performance reasons */
  for (i = 0; i < n; i++)
    byte[i] = (unsigned char) (patt[i] & 0x00FF);

  /* Save words to file */
  i = fwrite (byte, sizeof (char), n, F);

  /* Free memory and quit */
  free (byte);
  return (n < i ? -1l : n);
}

/* ....................... End of save_byte() ....................... */


/*
  ---------------------------------------------------------------------------
  char *format_str (int fmt);
  ~~~~~~~~~~~~~~~~~

  Function to return a string with the description of the current
  bitstream format (g192, byte, or bit).

  Parameters:
  fmt ... integer with the bitstream format

  Returned value:
  ~~~~~~~~~~~~~~~
  Returns a pointer to the format string, or a NULL pointer
  if fmt is invalid. 

  Original author: <simao.campos@comsat.com>
  ~~~~~~~~~~~~~~~~

  History:
  ~~~~~~~~
  21.Aug.97  v1.00 created
  ---------------------------------------------------------------------------
*/
char *format_str (fmt)
     int fmt;
{
  switch (fmt) {
  case byte:
    return "byte";
    break;
  case g192:
    return "g192";
    break;
  case compact:
    return "bit";
    break;
  }
  return "";
}

/* ....................... End of format_str() ....................... */


/*
  ---------------------------------------------------------------------------
  char *type_str (int type);
  ~~~~~~~~~~~~~~

  Function to return a string with the description of the current
  bitstream format (g192, byte, or bit).

  Parameters:
  type ... integer with the bitstream format

  Returned value:
  ~~~~~~~~~~~~~~~
  Returns a pointer to the format string, or a NULL pointer
  if type is invalid. 

  Original author: <simao.campos@comsat.com>
  ~~~~~~~~~~~~~~~~

  History:
  ~~~~~~~~
  21.Aug.97  v1.00 created
  ---------------------------------------------------------------------------
*/
char *type_str (type)
     int type;
{
  switch (type) {
  case BER:
    return "BER";
    break;
  case FER:
    return "FER";
    break;
  }
  return "";
}

/* ....................... End of type_str() ....................... */


/*
  --------------------------------------------------------------------------
  char check_eid_format (FILE *F, char *file, char *type);
  ~~~~~~~~~~~~~~~~~~~~~

  Function that checks the format (g192, byte, bit) in a given
  bitstream, and tries to guess the type of data (bit stream or frame
  erasure pattern)

  Parameter:
  ~~~~~~~~~~
  F ...... FILE * structure to file to be checked
  file ... name of file to be checked
  type ... pointer to guessed data type (FER or BER) in file

  Returned value:
  ~~~~~~~~~~~~~~~
  Returns the data format (g192, byte, bit) found in file.

  Original author: <simao.campos@comsat.com>
  ~~~~~~~~~~~~~~~~

  History:
  ~~~~~~~~
  15.Aug.97  v.1.0  Created.
  01.Jun.05  v.1.1  Bug correction: switch is made on the "unsigned short" value
					(v.1.0: "unsigned" only). <Cyril Guillaume & Stephane Ragot -- stephane.ragot@rd.francetelecom.com>
  -------------------------------------------------------------------------- 
*/
char check_eid_format (F, file, type)
     FILE *F;
     char *file;
     char *type;
{
  short word;
  char ret_val;
  unsigned long tmp = 0x41424344;       /* Hex version of the string ABCD */
  int little_endian;

  /* Find whether the OS is big- or little-endian */
  little_endian = strncmp ("ABCD", (char *) &tmp, 4);

  /* Get a 16-bit word from the file */
  fread (&word, sizeof (short), 1, F);

  /* Use some heuristics to determine what type of file is this */
  switch ((unsigned short) word) {
  case 0x7F7F:
  case 0x7F81:
  case 0x8181:
  case 0x817F:
    /* Byte-oriented G.192 bitstream */
    *type = BER;
    ret_val = byte;
    break;

  case 0x2020:
  case 0x2021:
  case 0x2120:
  case 0x2121:
    /* Byte-oriented G.192 syncs */
    *type = FER;
    ret_val = byte;
    break;

  case 0x007F:
  case 0x0081:
    /* G.192 bitstream in natural order */
    *type = BER;
    ret_val = g192;
    break;

  case 0x6B21:
  case 0x6B20:
    /* G.192 sync header in natural order */
    *type = FER;
    ret_val = g192;
    break;

  case 0x7F00:
  case 0x8100:
  case 0x216B:
  case 0x206B:
    /* G.192 format that needs to be byte-swapped */
    fprintf (stderr, "File %s needs to be byte-swapped! Aborted.\n", file);
    exit (8);

  default:
    /* Assuming it is compact bit mode */
    *type = nil;                /* Not possible to infer type for binary format! */
    ret_val = compact;
  }

  /* Final check to see if the input bitstream is a byte-oriented G.192 bitstream. In this case, the first byte is 0x2n (n=0..F) and the second byte must be the frame length in a BIG ENDIAN system, and vice-versa in a little-endian system */
  if (little_endian)
    word = ((word >> 8) & 0xFF) | ((word << 8));
/*
  if ((little_endian && (((unsigned)word & 0xF0)==0x20)) ||
      (((unsigned)word>>8) & 0xF0) == 0x20)
*/
  if (*type == nil && (((unsigned) word >> 8) & 0xF0) == 0x20) {
    *type = FER;
    ret_val = byte;
  }

  /* Rewind file & and return format identifier */
  fseek (F, 0l, SEEK_SET);
  return (ret_val);
}

/* ...................... End of check_eid_format() ...................... */


/* 
  ---------------------------------------------------------------------------
  long soft2hard (short *soft, short *hard, long n, char type);
  ~~~~~~~~~~~~~~ 

  Converts soft bits or frame sync words to hard bits. Type provides
  information on whether the buffer contains bit errors (soft bits) or
  frame erasures (frame sync). Returns the number of unexpected value
  found.
  
  ---------------------------------------------------------------------------
*/
long soft2hard (soft, hard, n, type)
     short *soft, *hard;
     long n;
     char type;
{
  long i, unexpected = 0;
  short register tmp;

  switch (type) {
  case BER:
    for (i = 0; i < n; i++) {
      tmp = *soft++;
      if (tmp == G192_ONE)
        *hard++ = 1;
      else if (tmp == G192_ZERO)
        *hard++ = 0;
      else
        unexpected++;
    }
    break;
  case FER:
    for (i = 0; i < n; i++) {
      tmp = *soft++;
      if (tmp == G192_FER)
        *hard++ = 1;
      else if ((tmp >> 4) == 0x06B2)
        *hard++ = 0;
      else
        unexpected++;
    }
    break;
  }

  return (unexpected);
}

/* ...................... End of soft2hard() ...................... */
