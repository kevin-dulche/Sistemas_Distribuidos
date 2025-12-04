/*****************************************************************************
 *                                                                           *
 *  Copyright (c) 1991-1994                                                  *
 *  by TreadMarks, L.L.C. (TREADMARKS), Houston, Texas	  		     *
 *                                                                           *
 *  This software is furnished under a license and may be used and  copied   *
 *  only  in  accordance  with  the  terms  of  such  license and with the   *
 *  inclusion of the above copyright notice.  This software or  any  other   *
 *  copies  thereof may not be provided or otherwise made available to any   *
 *  other person.  No title to or ownership of  the  software  is  hereby    *
 *  transferred.                                                             *
 *									     *
 *  The recipient of this software (RECIPIENT) acknowledges and agrees that  *
 *  the software contains information and trade secrets that are	     *
 *  confidential and proprietary to TREADMARKS.  RECIPIENT agrees to take    *
 *  all reasonable steps to safeguard the software, and to prevent its       *
 *  disclosure.								     * 
 *                                                                           *
 *  The information in this software is subject to change  without  notice   *
 *  and should  not  be  construed  as  a commitment by TREADMARKS.	     *
 *                                                                           *
 *  This software is furnished AS IS, without warranty of any kind, either   *
 *  express or implied (including, but not limited to, any implied warranty  *
 *  of merchantability or fitness), with regard to the software.  	     *
 *  TREADMARKS assumes no responsibility for the use or reliability of its   *
 *  software.  TREADMARKS shall not be liable for any special, incidental,   *
 *  or consequential damages, or any damages whatsoever due to causes beyond *
 *  the reasonable control of TREADMARKS, loss of use, data or profits, or   *
 *  from loss or destruction of materials provided to TREADMARKS by	     *
 *  RECIPIENT.								     *
 *									     *
 *  TREADMARKS's liability for damages arising out of or in connection with  *
 *  the use or performance of this software, whether in an action of	     *
 *  contract or tort including negligence, shall be limited to the purchase  *
 *  price, or the total amount paid by RECIPIENT, whichever is less.	     *
 *                                                                           *
 *****************************************************************************/

/*
 * $Id: sh_malloc.c,v 10.1.1.5 1998/01/09 22:39:47 alc Exp $
 *
 * Description:    
 *	shared heap management routines
 *
 * External Functions:
 *			Tmk_malloc,
 *			Tmk_free,
 *			Tmk_heap_initialize
 *
 * Facility:	TreadMarks Distributed Shared Memory System
 * History:
 *	10-May-1993	Alan L. Cox	Created
 *	28-Jun-1993	Alan L. Cox	Added curbrk to table
 *	 3-Aug-1993	Alan L. Cox	Corrected alignment for SPARC
 *	25-Aug-1993	Alan L. Cox	Added Tmk_sbrk
 *	31-Mar-1994	Povl T. Koch	Adapted for Alpha
 *
 *	Version 0.9.0
 *
 *	Version 0.9.1
 *
 *	22-Nov-1994	Alan L. Cox	Modified Tmk_free to check ptr
 *
 *	Version 0.9.2
 *
 *	 9-Apr-1995	Alan L. Cox	Added block sizes of (3/4)*x^2 bytes
 *
 *	Version 0.9.3
 */
#include "Tmk.h"

/*
 *
 */
#define TABLE_LOCK	0

/*
 * BUCKET_BEGIN is the first bucket's index.  BUCKET_END is the last bucket's
 * index.  We sacrifice the first few entries in the array of buckets, up to
 * BUCKET_BEGIN, to simplify the computation of BUCKET_SIZE.
 */
#define	BUCKET_END	59

#define	BUCKET_BEGIN	8

#define	BUCKET_SIZE(i)	((2 | (i & 1)) << (i >> 1))

/*
 * Warning:
 *
 * You must not change the order of the fields in struct Chain.
 */
typedef
struct	Chain  *chain_t;

struct	Chain	{
	unsigned long	index;
	chain_t		chain;		
};

/*
 * XOR the bucket # with C_xor_index to compute the index, and
 * XOR the index with C_xor_index to compute the bucket #.
 */
#define C_xor_index	((unsigned long) -16909321)

struct	Table	{
	caddr_t		curbrk;
	chain_t		nextfit[BUCKET_END];
};

/*
 * global is a private pointer to the shared data structures used by
 * the memory allocater.
 */
static
struct	Table  *table;

/*
 * Assumes that TABLE_LOCK is held.
 */
static
void	table_initialize( void )
{
	unsigned long	bucket = BUCKET_BEGIN;

	while (BUCKET_SIZE(bucket) < sizeof(struct Table))
		bucket++;

	table = (struct Table *) page_array_[0].vadr;
	table->curbrk = page_array_[0].vadr + BUCKET_SIZE(bucket);

	memset(table->nextfit, 0, sizeof(table->nextfit));

	Tmk_distribute(&table, sizeof(table));
}

/*
 * Tmk_sbrk
 *
 * Permanently allocates a chunk of shared memory.  Size is increased
 * to the next double-word boundary.  The chunk of shared memory is
 * double-word aligned.
 *
 * The smallest allocatable size is eight bytes.
 */
void   *Tmk_sbrk(size)
        unsigned        size;
{
	caddr_t		vadr;

	size = (size + 7) &~ 7;

        Tmk_lock_acquire(TABLE_LOCK);

	if (table == NULL)
		table_initialize();

	vadr = (caddr_t)((unsigned long)(table->curbrk + 7) &~ (unsigned long) 7);

	if ((vadr + size) > page_array_[Tmk_npages].vadr)
		vadr = 0;
	else
		table->curbrk = vadr + size;

        Tmk_lock_release(TABLE_LOCK);

	return (void *) vadr;
}

/*
 * sh_resize
 *
 * Searches for a free chunk of memory larger than the requested size.
 *
 * Recursively splits one half of the chunk into halves until (1) two
 * chunks of the requested size are created, (2) three chunks of the
 * requested size are created, or (3) two chunks of the requested size
 * and one chunk of the next smaller size, a power of two, are created.
 *
 * Returns 1 if a free chunk is found.  Otherwise, returns 0.
 *
 * Assumes that TABLE_LOCK is held.
 */
static	int		sh_resize(start)
	unsigned long	start;
{
	unsigned long	bucket;

        for (bucket = start + 2; bucket < BUCKET_END; bucket++)
                if (table->nextfit[bucket]) {
                        while (bucket > start + 1) {

				chain_t	chain = table->nextfit[bucket];

				table->nextfit[bucket] = chain->chain;

				assert(bucket == chain->index ^ C_xor_index);

				if ((bucket -= 2) == start + 1) {

					bucket--;   /* bucket == start */
					bucket &= ~ (unsigned long) 1;

					chain->index = bucket ^ C_xor_index;
					chain->chain = table->nextfit[bucket];

					table->nextfit[bucket] = chain;

					chain = (chain_t)((caddr_t) chain + BUCKET_SIZE(bucket));

					bucket = start;
				}
				chain->index = bucket ^ C_xor_index;
				chain->chain = table->nextfit[bucket];

				table->nextfit[bucket] = chain;

				chain = (chain_t)((caddr_t) chain + BUCKET_SIZE(bucket));
				chain->index = bucket ^ C_xor_index;
				chain->chain = table->nextfit[bucket];

				table->nextfit[bucket] = chain;
                        }
                        return 1;
                }
	return 0;
}

/*
 * Tmk_malloc
 *
 * Calls sh_resize if the bucket corresponding to the requested size
 * is empty.
 *
 * The smallest allocatable size is twenty-eight bytes.
 *
 * Leaks a small amount of memory if the request is larger than
 * available memory.
 */
void   *Tmk_malloc(size)
	unsigned	size;
{
	unsigned long	bucket = BUCKET_BEGIN;
	chain_t		chain;

	while (BUCKET_SIZE(bucket) < (size + sizeof(chain->index)))
		bucket++;

	Tmk_lock_acquire(TABLE_LOCK);

	if (table == NULL)
		table_initialize();

	while ((chain = table->nextfit[bucket]) == 0)
		if (0 == sh_resize(bucket)) {

			if (BUCKET_SIZE(bucket) >= Tmk_page_size)
				table->curbrk = (caddr_t)(((unsigned long)(table->curbrk + Tmk_page_size) &~ (unsigned long)(Tmk_page_size - 1)) - sizeof(chain->index));

			/*
			 * Enforce double-word alignment for &chain->chain
			 */
			chain = (chain_t)(table->curbrk = (caddr_t)((unsigned long) table->curbrk | sizeof(chain->index)));
			chain->index = bucket ^ C_xor_index;

			if ((table->curbrk += BUCKET_SIZE(bucket)) > page_array_[Tmk_npages].vadr) {

				table->curbrk = (caddr_t) chain;

				Tmk_lock_release(TABLE_LOCK);

				return 0;
			}
			goto unlock;
		}

        table->nextfit[bucket] = chain->chain;
 unlock:
	Tmk_lock_release(TABLE_LOCK);

	assert(bucket == chain->index ^ C_xor_index);

	return (void *)&chain->chain;
}

/*
 * Tmk_free
 *
 * Checks the pointer for validity: tests whether the XORed index is
 * a valid bucket #.
 */
void	Tmk_free(ptr)
	void	       *ptr;
{
	if (ptr) {

		chain_t		chain = (chain_t)((caddr_t) ptr - sizeof(chain->index));
		unsigned long	bucket = chain->index ^ C_xor_index;

		if (bucket < BUCKET_END) {

			Tmk_lock_acquire(TABLE_LOCK);

			if (table == NULL)
				table_initialize();

			chain->chain = table->nextfit[bucket];
			table->nextfit[bucket] = chain;

			Tmk_lock_release(TABLE_LOCK);
		}
		else
			Tmk_err("Tmk_free: bad ptr == %d\n", ptr);
	}
}
