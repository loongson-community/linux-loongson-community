/*
 * newport.c: context switching the newport graphics card
 *
 * Author: Miguel de Icaza
 */

#include "newport.h"

/* Kernel routines for supporting graphics context switching */

void newport_save (void *y)
{
	newport_ctx *x = y;
	newport_wait ();

#define LOAD(val) x->val = npregs->set.val;
#define LOADI(val) x->val = npregs->set.val.i;
#define LOADC(val) x->val = npregs->cset.val;
	
	LOAD(drawmode1);
	LOAD(drawmode0);
	LOAD(lsmode);
	LOAD(lspattern);
	LOAD(lspatsave);
	LOAD(zpattern);
	LOAD(colorback);
	LOAD(colorvram);
	LOAD(alpharef);
	LOAD(smask0x);
	LOAD(smask0y);
	LOADI(_xstart);
	LOADI(_ystart);
	LOADI(_xend);
	LOADI(_yend);
	LOAD(xsave);
	LOAD(xymove);
	LOADI(bresd);
	LOADI(bress1);
	LOAD(bresoctinc1);
	LOAD(bresrndinc2);
	LOAD(brese1);
	LOAD(bress2);
	LOAD(aweight0);
	LOAD(aweight1);
	LOADI(colorred);
	LOADI(coloralpha);
	LOADI(colorgrn);
	LOADI(colorblue);
	LOADI(slopered);
	LOADI(slopealpha);
	LOADI(slopegrn);
	LOADI(slopeblue);
	LOAD(wrmask);
	LOAD(hostrw0);
	LOAD(hostrw1);
	
        /* configregs */
	
	LOADC(smask1x);
	LOADC(smask1y);
	LOADC(smask2x);
	LOADC(smask2y);
	LOADC(smask3x);
	LOADC(smask3y);
	LOADC(smask4x);
	LOADC(smask4y);
	LOADC(topscan);
	LOADC(xywin);
	LOADC(clipmode);
	LOADC(config);

	/* Mhm, maybe I am missing something, but it seems that
	 * saving/restoring the DCB is only a matter of saving these
	 * registers
	 */

	newport_bfwait ();
	LOAD (dcbmode);
	newport_bfwait ();
	x->dcbdata0 = npregs->set.dcbdata0.all;
	newport_bfwait ();
	LOAD(dcbdata1);
}

/*
 * Importat things to keep in mind when restoring the newport context:
 *
 * 1. slopered register is stored as a 2's complete (s12.11);
 *    needs to be converted to a signed magnitude (s(8)12.11).
 *
 * 2. xsave should be stored after xstart.
 *
 * 3. None of the registers should be written with the GO address.
 *    (read the docs for more details on this).
 */
void newport_restore (void *y)
{
	newport_ctx *x = y;
#define STORE(val) npregs->set.val = x->val
#define STOREI(val) npregs->set.val.i = x->val
#define STOREC(val) npregs->cset.val = x->val
	newport_wait ();
	
	STORE(drawmode1);
	STORE(drawmode0);
	STORE(lsmode);
	STORE(lspattern);
	STORE(lspatsave);
	STORE(zpattern);
	STORE(colorback);
	STORE(colorvram);
	STORE(alpharef);
	STORE(smask0x);
	STORE(smask0y);
	STOREI(_xstart);
	STOREI(_ystart);
	STOREI(_xend);
	STOREI(_yend);
	STORE(xsave);
	STORE(xymove);
	STOREI(bresd);
	STOREI(bress1);
	STORE(bresoctinc1);
	STORE(bresrndinc2);
	STORE(brese1);
	STORE(bress2);
	STORE(aweight0);
	STORE(aweight1);
	STOREI(colorred);
	STOREI(coloralpha);
	STOREI(colorgrn);
	STOREI(colorblue);
	STOREI(slopered);
	STOREI(slopealpha);
	STOREI(slopegrn);
	STOREI(slopeblue);
	STORE(wrmask);
	STORE(hostrw0);
	STORE(hostrw1);
	
        /* configregs */
	
	STOREC(smask1x);
	STOREC(smask1y);
	STOREC(smask2x);
	STOREC(smask2y);
	STOREC(smask3x);
	STOREC(smask3y);
	STOREC(smask4x);
	STOREC(smask4y);
	STOREC(topscan);
	STOREC(xywin);
	STOREC(clipmode);
	STOREC(config);

	/* FIXME: restore dcb thingies */
}
