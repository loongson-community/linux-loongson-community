/*
 * milo/bitags.c -- handles the tags passed to the kernel
 *
 * Copyright (C) 1995 by Stoned Elipot <Stoned.Elipot@fnet.fr>
 * written by Stoned Elipot from an original idea of
 * Ralf Baechle <ralf@waldorf-gmbh.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for more
 */
#include <stdio.h>
#include <asm/bootinfo.h>

static unsigned int debuglevel = 0;

extern unsigned long mach_mem_upper; /* from milo.c */
extern unsigned long mach_mem_lower; /* from milo.c */

static unsigned long next_tag = (unsigned long)NULL;

/*
 * Create a tag
 *
 * Parameters: tag - tag type to create
 *             size - tag's data size
 *             tagdata - pointer on tag's data
 * 
 * returns   : 0 - success
 *             1 - failure
 */
int
bi_TagAdd(enum bi_tag type, unsigned long size, void *data)
{
  tag t;
  unsigned long addr;
  
  t.tag = type;
  t.size = size;
  if (next_tag == (unsigned long)NULL) /* HuHo... first tag to create */
    {
      if (mach_mem_upper != (unsigned long)NULL) /* RAM detection code had run */
	{
	  next_tag = mach_mem_upper;
	}
      else	
	/* RAM dectection code had not run, let's hope the 
	 * tag we are creating is a memupper one, else fail 
	 * ...miserably, hopelessly, lonely 
         */
	{
	  if (type != tag_memupper)
	    {
	      return 1;
	    }
	  else
	    {
              /* 
               * saved, it's a memupper tag: put it's value in
               * mach_mem_upper so launch() can pass it to the kernel
               * in a0 and well we're going to create a tag anyway...
               */
	      next_tag = *(unsigned long*)data;
              memcpy((void*)&mach_mem_upper, data, size);
	    }
	}
    }
  addr = next_tag - (sizeof(tag));
  if (debuglevel >=2)
    {
      printk("bi_TagAdd: adding tag struct at %08x, tag: %d, size: %08x\r\n", addr, t.tag, t.size);
    }
  memcpy((void*)addr, (void*)&t, (size_t)(sizeof(tag)));
  if (size != 0)
    {
      addr = addr - size;
      if (debuglevel >=2)
	{
	  printk("bi_TagAdd: adding tag value at %08x\r\n", addr);
	}
      memcpy((void*)addr, data, (size_t)(t.size));
    }
  next_tag = addr;
  return 0;
}

/*
 * Create tags from a "null-terminated" array of tag
 * (tag type of the tag_def struct in array must be 'dummy')
 *
 * Parameter: taglist - tag array pointer
 * 
 * returns  : 0 - success
 *            1 - failure
 */
int 
bi_TagAddList(tag_def* taglist)
{
  int ret_val = 0;
  for(; (taglist->t.tag != tag_dummy) && (!ret_val); taglist++)
    {
      /*
       * we assume this tag is present in the default taglist
       * for the machine we're running on
       */
      if (taglist->t.tag == tag_memlower) 
	{
	  mach_mem_lower = (unsigned long)(*((unsigned long*)taglist->d));
/* ajouter detection de memupper pour simplifier le code de bi_TagAdd: soit mach_mem_upper
   a ete initialise par <machine_ident>() soit est initialise par le pre;ier tag de la list
   par default pour la machine */
	}
      ret_val = bi_TagAdd(taglist->t.tag, taglist->t.size, taglist->d);
    }
  return ret_val;
}

/*
 * Parse the tags present in upper memory to find out
 * a pecular one.
 *
 * Parameter: type - tag type to find
 * 
 * returns  : NULL  - failure
 *            !NULL - pointer on the tag structure found 
 *
 * Note: Just a 'prototype', the kernel's one is in 
 *       arch/mips/kernel/setup.c
 */
/* tag* */
/* bi_TagFind(enum bi_tag type) */
/* { */
/*   tag* t; */
/*   t = (tag*)(mach_mem_upper - sizeof(tag)); */
/*   while((t->tag != tag_dummy) && (t->tag != type)) */
/*       t = (tag*)(NEXTTAGPTR(t)); */
/*   if (t->tag == tag_dummy)	*/
/*     { */
/*       return (tag*)NULL; */
/*     } */
/*   return t; */
/* } */


/*
 * Make a listing of tags available in memory: debug helper.
 */
/* void */
/* bi_TagWalk(void) */
/* { */
/*   tag* t; */
/*   int i=0; */
/*   t = (tag*)(mach_mem_upper - sizeof(tag)); */
/*   while(t->tag != tag_dummy) */
/*     { */
/*       printk("tag #02%dm addr: %08x, type %d, size %u\n\r", i, (void*)t, t->tag, t->size); */
/*       t = (tag*)(NEXTTAGPTR(t)); */
/*       i++; */
/*     } */
/*   return; */
/* } */

