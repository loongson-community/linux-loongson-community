/*
 * linux/include/asm-arm/arch-shark/param.h
 *
 * by Alexander.Schulz@stud.uni-karlsruhe.de
 */

/* This must be a power of 2 because the RTC
 * can't use anything else.
 */
#define HZ 64
#ifdef __KERNEL__
/* Conceptually

     #define hz_to_std(a) ((a) * 100 / HZ)

   is what has to be done, it just has overflow problems with the
   intermediate result of the multiply after a bit more than 7 days.
   See include/asm-mips/param.h for a optized sample implementation
   used on DECstations.
 */
#error Provide a definiton for hz_to_std
#endif
