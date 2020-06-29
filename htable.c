/*
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2001,2002  Brian Stafford  <brian@stafford.uklinux.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>

#include <assert.h>

/* A simplistic hash table implementation using hashing and chaining.
   The table is always 256 entries long although this is an arbitrary
   choice and there is nothing in the code to prevent other table sizes
   or hash functions from working.

   Case insensitive searching is performed.
*/

#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>

#include <missing.h> /* declarations for missing library functions */

#include "htable.h"

struct h_node
  {
    struct h_node *next;	/* Next node in chain for this hash value */
    char *name;			/* Node name */
  };

#define HASHSIZE 256

static const unsigned char shuffle[HASHSIZE] =
  {
    215, 207, 188,  72,  82, 194,  89, 230,
     17,  49, 127, 179, 139, 200, 104, 114,
    233,  52, 138,  42, 175, 159, 142,  77,
    247,   3, 185,  54, 157,  19, 153,  14,
    112, 184,  32, 220,  20, 148, 251, 141,
     66, 195, 174, 150, 246,  76, 242, 227,
    145,  84,   7,   5, 144, 211,  31,  71,
    123, 217, 134, 243, 152, 137,  67, 213,
     83, 223, 203, 119, 110, 113,  99, 158,
    156,  61,  85, 187, 151,  90,   6, 237,
    177,  45, 133,  87,  27, 106,  15,  68,
     50,  80, 239, 250, 108, 253, 199, 124,
      2, 210, 205,  21, 209, 252,  29, 196,
    219,  78,  86, 178,  22,  53,  74,   9,
    155,  91, 122, 235,  65, 129,  64, 206,
     41,  46, 245, 125, 198, 189,  94,  79,
    101, 160, 193,  43, 216, 128,  44,  70,
    147, 229, 167, 186,  96, 166, 255, 146,
    204, 224, 171, 149,  97, 102,   1, 165,
     39, 222,  56,  12, 191, 202, 111, 103,
    120,  24,  69, 100,  34, 164, 135, 197,
    225,  18,  40, 236, 131, 231, 140,  63,
    181, 170,  73, 244,  58,  25,  98, 183,
     75,  57, 176, 118,  30, 226,  37,  36,
    130,  33,  55,  26,  10, 161, 107,  38,
    221, 234, 201, 121, 249, 116, 143,  62,
    190,  59, 115,  93,  92, 228, 192, 109,
     51,   8,  47,  13, 117, 173, 214,  81,
    169, 241, 182, 162,   0,  95, 218,  23,
    248, 132,  48, 232, 136, 240,  28, 154,
    126, 208,  60,  11,  16, 105,   4, 163,
    172, 238, 254,  88, 180, 168, 212,  35,
  };

static unsigned int
hashi (const char *string, int length)
{
  unsigned char c, h1;
  
  assert (string != NULL);

  for (h1 = 0; length-- > 0; h1 = shuffle[h1 ^ c])
    {
      c = *string++;
      if (isupper (c))
	c = tolower (c);
    }
  return h1;
}

/* Insert a new node into the table.  It is not an error for an entry with
   the same name to be already present in the table.  The new entry will
   be found when searching the table.  When removed, the former entry
   will be found on a subsequent search */
void *
h_insert (struct h_node **table, const char *name, int namelen, size_t size)
{
  unsigned int hv;
  struct h_node *node;

  assert (table != NULL && name != NULL);

  if (namelen < 0)
    namelen = strlen (name);
  if (namelen == 0)
    return NULL;
  size += sizeof (struct h_node);
  if ((node = malloc (size)) == NULL)
    return NULL;
  memset (node, 0, size);
  if ((node->name = malloc (namelen)) == NULL)
    {
      free (node);
      return NULL;
    }
  memcpy (node->name, name, namelen);
  hv = hashi (node->name, namelen);
  node->next = table[hv];
  table[hv] = node;
  return node + 1;
}

/* Remove the node from the table.
 */
void 
h_remove (struct h_node **table, void *data)
{
  struct h_node *node = (struct h_node *) data - 1;
  unsigned int hv;
  struct h_node *p;

  assert (table != NULL && node != NULL);

  hv = hashi (node->name, strlen (node->name));
  if (table[hv] == node)
    table[hv] = node->next;
  else
    for (p = table[hv]; p != NULL; p = p->next)
      if (p->next == node)
        {
	  p->next = node->next;
	  node->next = NULL;
	  break;
	}
  free (node->name);
  free (node);
}

/* Search for a node in the table.
 */
void *
h_search (struct h_node **table, const char *name, int namelen)
{
  struct h_node *p;

  assert (table != NULL && name != NULL);

  if (namelen < 0)
    namelen = strlen (name);
  for (p = table[hashi (name, namelen)]; p != NULL; p = p->next)
    if (strncasecmp (name, p->name, namelen) == 0)
      return p + 1;
  return NULL;
}

/* For each entry in the hash table, call the specified callback.
   Entries are located in no particular order. */
void
h_enumerate (struct h_node **table,
	     void (*cb) (const char *name, void *data, void *arg), void *arg)
{
  struct h_node *p;
  int i;

  assert (table != NULL && cb != NULL);

  for (i = 0; i < HASHSIZE; i++)
    for (p = table[i]; p != NULL; p = p->next)
      (*cb) (p->name, p + 1, arg);
}

/* Create a new hash table.
 */
struct h_node **
h_create (void)
{
  return calloc (HASHSIZE, sizeof (struct h_node *));
}

/* Destroy the hash table.  This frees all memory allocated to the table,
   nodes and names.  It also calls the callback function for each item in the
   table just before freeing its other resources.
 */
void
h_destroy (struct h_node **table,
	   void (*cb) (const char *name, void *data, void *arg), void *arg)
{
  struct h_node *p, *next;
  int i;

  assert (table != NULL);

  for (i = 0; i < HASHSIZE; i++)
    for (p = table[i]; p != NULL; p = next)
      {
	next = p->next;
	if (cb != NULL)
	  (*cb) (p->name, p + 1, arg);
	free (p->name);
	free (p);
      }
  free (table);
}

