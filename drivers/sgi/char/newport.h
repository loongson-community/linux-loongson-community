/* $Id: newport.h,v 1.2 1997/07/02 06:20:19 miguel Exp $
 * newport.h: Defines and register layout for NEWPORT graphics
 *            hardware.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */

#ifndef _SGI_NEWPORT_H
#define _SGI_NEWPORT_H


typedef volatile unsigned long npireg_t;

union npfloat {
	volatile float f;
	npireg_t       i;
};

typedef union npfloat npfreg_t;

union np_dcb {
	npireg_t all;
	struct { volatile unsigned short s0, s1; } hwords;
	struct { volatile unsigned char b0, b1, b2, b3; } bytes;
};

struct newport_rexregs {
	npireg_t dmode1;      /* GL extra mode bits */
#define NPORT_DMODE1_PMASK       0x00000007
#define NPORT_DMODE1_NOPLANES    0x00000000
#define NPORT_DMODE1_RGBPLANES   0x00000001
#define NPORT_DMODE1_RGBAPLANES  0x00000002
#define NPORT_DMODE1_OLPLANES    0x00000004
#define NPORT_DMODE1_PUPPLANES   0x00000005
#define NPORT_DMODE1_CIDPLANES   0x00000006
#define NPORT_DMODE1_DDMASK      0x00000018
#define NPORT_DMODE1_DD4         0x00000000
#define NPORT_DMODE1_DD8         0x00000008
#define NPORT_DMODE1_DD12        0x00000010
#define NPORT_DMODE1_DD24        0x00000018
#define NPORT_DMODE1_DSRC        0x00000020
#define NPORT_DMODE1_YFLIP       0x00000040
#define NPORT_DMODE1_RWPCKD      0x00000080
#define NPORT_DMODE1_HDMASK      0x00000300
#define NPORT_DMODE1_HD4         0x00000000
#define NPORT_DMODE1_HD8         0x00000100
#define NPORT_DMODE1_HD12        0x00000200
#define NPORT_DMODE1_HD32        0x00000300
#define NPORT_DMODE1_RWDBL       0x00000400
#define NPORT_DMODE1_ESWAP       0x00000800 /* Endian swap */
#define NPORT_DMODE1_CCMASK      0x00007000
#define NPORT_DMODE1_CCLT        0x00001000
#define NPORT_DMODE1_CCEQ        0x00002000
#define NPORT_DMODE1_CCGT        0x00004000
#define NPORT_DMODE1_RGBMD       0x00008000
#define NPORT_DMODE1_DENAB       0x00010000 /* Dither enable */
#define NPORT_DMODE1_FCLR        0x00020000 /* Fast clear */
#define NPORT_DMODE1_BENAB       0x00040000 /* Blend enable */
#define NPORT_DMODE1_SFMASK      0x00380000
#define NPORT_DMODE1_SF0         0x00000000
#define NPORT_DMODE1_SF1         0x00080000
#define NPORT_DMODE1_SFDC        0x00100000
#define NPORT_DMODE1_SFMDC       0x00180000
#define NPORT_DMODE1_SFSA        0x00200000
#define NPORT_DMODE1_SFMSA       0x00280000
#define NPORT_DMODE1_DFMASK      0x01c00000
#define NPORT_DMODE1_DF0         0x00000000
#define NPORT_DMODE1_DF1         0x00400000
#define NPORT_DMODE1_DFSC        0x00800000
#define NPORT_DMODE1_DFMSC       0x00c00000
#define NPORT_DMODE1_DFSA        0x01000000
#define NPORT_DMODE1_DFMSA       0x01400000
#define NPORT_DMODE1_BBENAB      0x02000000 /* Back blend enable */
#define NPORT_DMODE1_PFENAB      0x04000000 /* Pre-fetch enable */
#define NPORT_DMODE1_ABLEND      0x08000000 /* Alpha blend */
#define NPORT_DMODE1_LOMASK      0xf0000000
#define NPORT_DMODE1_LOZERO      0x00000000
#define NPORT_DMODE1_LOAND       0x10000000
#define NPORT_DMODE1_LOANDR      0x20000000
#define NPORT_DMODE1_LOSRC       0x30000000
#define NPORT_DMODE1_LOANDI      0x40000000
#define NPORT_DMODE1_LODST       0x50000000
#define NPORT_DMODE1_LOXOR       0x60000000
#define NPORT_DMODE1_LOOR        0x70000000
#define NPORT_DMODE1_LONOR       0x80000000
#define NPORT_DMODE1_LOXNOR      0x90000000
#define NPORT_DMODE1_LONDST      0xa0000000
#define NPORT_DMODE1_LOORR       0xb0000000
#define NPORT_DMODE1_LONSRC      0xc0000000
#define NPORT_DMODE1_LOORI       0xd0000000
#define NPORT_DMODE1_LONAND      0xe0000000
#define NPORT_DMODE1_LOONE       0xf0000000

	npireg_t dmode0;      /* REX command register */

	/* These bits define the graphics opcode being performed. */
#define NPORT_DMODE0_OPMASK   0x00000003 /* Opcode mask */
#define NPORT_DMODE0_NOP      0x00000000 /* No operation */
#define NPORT_DMODE0_RD       0x00000001 /* Read operation */
#define NPORT_DMODE0_DRAW     0x00000002 /* Draw operation */
#define NPORT_DMODE0_S2S      0x00000003 /* Screen to screen operation */

	/* The following decide what addressing mode(s) are to be used */
#define NPORT_DMODE0_AMMASK   0x0000001c /* Address mode mask */
#define NPORT_DMODE0_SPAN     0x00000000 /* Spanning address mode */
#define NPORT_DMODE0_BLOCK    0x00000004 /* Block address mode */
#define NPORT_DMODE0_ILINE    0x00000008 /* Iline address mode */
#define NPORT_DMODE0_FLINE    0x0000000c /* Fline address mode */
#define NPORT_DMODE0_ALINE    0x00000010 /* Aline address mode */
#define NPORT_DMODE0_TLINE    0x00000014 /* Tline address mode */
#define NPORT_DMODE0_BLINE    0x00000018 /* Bline address mode */

	/* And now some misc. operation control bits. */
#define NPORT_DMODE0_DOSETUP  0x00000020
#define NPORT_DMODE0_CHOST    0x00000040
#define NPORT_DMODE0_AHOST    0x00000080
#define NPORT_DMODE0_STOPX    0x00000100
#define NPORT_DMODE0_STOPY    0x00000200
#define NPORT_DMODE0_SK1ST    0x00000400
#define NPORT_DMODE0_SKLST    0x00000800
#define NPORT_DMODE0_ZPENAB   0x00001000
#define NPORT_DMODE0_LISPENAB 0x00002000
#define NPORT_DMODE0_LISLST   0x00004000
#define NPORT_DMODE0_L32      0x00008000
#define NPORT_DMODE0_ZOPQ     0x00010000
#define NPORT_DMODE0_LISOPQ   0x00020000
#define NPORT_DMODE0_SHADE    0x00040000
#define NPORT_DMODE0_LRONLY   0x00080000
#define NPORT_DMODE0_XYOFF    0x00100000
#define NPORT_DMODE0_CLAMP    0x00200000
#define NPORT_DMODE0_ENDPF    0x00400000
#define NPORT_DMODE0_YSTR     0x00800000

	npireg_t lismode;     /* Mode for line stipple ops */
	npireg_t lispat;      /* Pattern for line stipple ops */
	npireg_t lispsave;    /* Backup save pattern */
	npireg_t zpat;        /* Pixel zpattern */
	npireg_t colbk;       /* Background color */
	npireg_t colvram;     /* Clear color for fast vram */
	npireg_t aref;        /* Reference value for afunctions */
	unsigned long _unused0;
	npireg_t scmskx0;     /* Window GL relative screen mask 0 */
	npireg_t scmsky0;     /* Window GL relative screen mask 0 */
	npireg_t xsetup;
	npireg_t xzpenab;
	npireg_t xlisrestore;
	npireg_t xlissave;

	unsigned long _unused1[0x30];

	npfreg_t fxstart;
	npfreg_t fystart;
	npfreg_t fxend;
	npfreg_t fyend;
	npireg_t xsv;
	npireg_t xymv;
	npfreg_t brd;
	npfreg_t brs1;
	npireg_t broinc1;
	volatile int brinc2;
	npireg_t bre1;
	npireg_t bre2;
	npireg_t aw0;
	npireg_t aw1;
	npfreg_t xstf;
	npfreg_t ystf;
	npfreg_t xef;
	npfreg_t yef;
	npireg_t xsti;
	npfreg_t xef1;
	npireg_t xysti;
	npireg_t xyei;
	npireg_t xstei;

	unsigned long _unused2[0x29];

	npfreg_t cred;
	npfreg_t calpha;
	npfreg_t cgreen;
	npfreg_t cblue;
	npfreg_t slred;
	npfreg_t slalpha;
	npfreg_t slgreen;
	npfreg_t slblue;
	npireg_t wmask;
	npireg_t ci;
	npfreg_t cx;
	npfreg_t sl1;
	npireg_t hrw0;
	npireg_t hrw1;
	npireg_t dmode;
#define NPORT_DMODE_WMASK   0x00000003
#define NPORT_DMODE_W4      0x00000000
#define NPORT_DMODE_W1      0x00000001
#define NPORT_DMODE_W2      0x00000002
#define NPORT_DMODE_W3      0x00000003
#define NPORT_DMODE_EDPACK  0x00000004
#define NPORT_DMODE_ECINC   0x00000008
#define NPORT_DMODE_CMASK   0x00000070
#define NPORT_DMODE_AMASK   0x00000780
#define NPORT_DMODE_AVC2    0x00000000
#define NPORT_DMODE_ACMALL  0x00000080
#define NPORT_DMODE_ACM0    0x00000100
#define NPORT_DMODE_ACM1    0x00000180
#define NPORT_DMODE_AXMALL  0x00000200
#define NPORT_DMODE_AXM0    0x00000280
#define NPORT_DMODE_AXM1    0x00000300
#define NPORT_DMODE_ABT     0x00000380
#define NPORT_DMODE_AVCC1   0x00000400
#define NPORT_DMODE_AVAB1   0x00000480
#define NPORT_DMODE_ALG3V0  0x00000500
#define NPORT_DMODE_A1562   0x00000580
#define NPORT_DMODE_ESACK   0x00000800
#define NPORT_DMODE_EASACK  0x00001000
#define NPORT_DMODE_CWMASK  0x0003e000
#define NPORT_DMODE_CHMASK  0x007c0000
#define NPORT_DMODE_CSMASK  0x0f800000
#define NPORT_DMODE_SENDIAN 0x10000000

	unsigned long _unused3;

	union np_dcb ddata0;
	npireg_t ddata1;
};

struct newport_cregs {
	npireg_t smskx1;
	npireg_t smsky1;
	npireg_t smskx2;
	npireg_t smsky2;
	npireg_t smskx3;
	npireg_t smsky3;
	npireg_t smskx4;
	npireg_t smsky4;
	npireg_t tscan;
	npireg_t win;
	npireg_t cmode;
#define NPORT_CMODE_SM0   0x00000001
#define NPORT_CMODE_SM1   0x00000002
#define NPORT_CMODE_SM2   0x00000004
#define NPORT_CMODE_SM3   0x00000008
#define NPORT_CMODE_SM4   0x00000010
#define NPORT_CMODE_CMSK  0x00001e00

	unsigned long _unused0;
	unsigned long cfg;
#define NPORT_CFG_G32MD   0x00000001
#define NPORT_CFG_BWIDTH  0x00000002
#define NPORT_CFG_ERCVR   0x00000004
#define NPORT_CFG_BDMSK   0x00000078
#define NPORT_CFG_GDMSK   0x00000f80
#define NPORT_CFG_GD0     0x00000080
#define NPORT_CFG_GD1     0x00000100
#define NPORT_CFG_GD2     0x00000200
#define NPORT_CFG_GD3     0x00000400
#define NPORT_CFG_GD4     0x00000800
#define NPORT_CFG_GFAINT  0x00001000
#define NPORT_CFG_TOMSK   0x0000e000
#define NPORT_CFG_VRMSK   0x00070000
#define NPORT_CFG_FBTYP   0x00080000

	npireg_t _unused1;
	npireg_t stat;
#define NPORT_STAT_VERS   0x00000007
#define NPORT_STAT_GBUSY  0x00000008
#define NPORT_STAT_BBUSY  0x00000010
#define NPORT_STAT_VRINT  0x00000020
#define NPORT_STAT_VIDINT 0x00000040
#define NPORT_STAT_GLMSK  0x00001f80
#define NPORT_STAT_BLMSK  0x0007e000
#define NPORT_STAT_BFIRQ  0x00080000
#define NPORT_STAT_GFIRQ  0x00100000

	npireg_t ustat;
	npireg_t dreset;
};

struct newport_regs {
	struct newport_rexregs set;
	unsigned long _unused0[0x16e];
	struct newport_rexregs go;
	unsigned long _unused1[0x22e];
	struct newport_cregs cset;
	unsigned long _unused2[0x1ef];
	struct newport_cregs cgo;
};
extern struct newport_regs *npregs;

/* Reading/writing VC2 registers. */
#define VC2_REGADDR_INDEX      0x00000000
#define VC2_REGADDR_IREG       0x00000010
#define VC2_REGADDR_RAM        0x00000030
#define VC2_PROTOCOL           (NPORT_DMODE_EASACK | 0x00800000 | 0x00040000)

#define VC2_VLINET_ADDR        0x000
#define VC2_VFRAMET_ADDR       0x400
#define VC2_CGLYPH_ADDR        0x500

/* Now the Indexed registers of the VC2. */
#define VC2_IREG_VENTRY        0x00
#define VC2_IREG_CENTRY        0x01
#define VC2_IREG_CURSX         0x02
#define VC2_IREG_CURSY         0x03
#define VC2_IREG_CCURSX        0x04
#define VC2_IREG_DENTRY        0x05
#define VC2_IREG_SLEN          0x06
#define VC2_IREG_RADDR         0x07
#define VC2_IREG_VFPTR         0x08
#define VC2_IREG_VLSPTR        0x09
#define VC2_IREG_VLIR          0x0a
#define VC2_IREG_VLCTR         0x0b
#define VC2_IREG_CTPTR         0x0c
#define VC2_IREG_WCURSY        0x0d
#define VC2_IREG_DFPTR         0x0e
#define VC2_IREG_DLTPTR        0x0f
#define VC2_IREG_CONTROL       0x10
#define VC2_IREG_CONFIG        0x20

extern inline void newport_vc2_set(struct newport_regs *regs, unsigned char vc2ireg,
				   unsigned short val)
{
	regs->set.dmode = (NPORT_DMODE_AVC2 | VC2_REGADDR_INDEX | NPORT_DMODE_W3 |
			   NPORT_DMODE_ECINC | VC2_PROTOCOL);
	regs->set.ddata0.all = (vc2ireg << 24) | (val << 8);
}

extern inline unsigned short newport_vc2_get(struct newport_regs *regs,
					     unsigned char vc2ireg)
{
	regs->set.dmode = (NPORT_DMODE_AVC2 | VC2_REGADDR_INDEX | NPORT_DMODE_W1 |
			   NPORT_DMODE_ECINC | VC2_PROTOCOL);
	regs->set.ddata0.bytes.b3 = vc2ireg;
	regs->set.dmode = (NPORT_DMODE_AVC2 | VC2_REGADDR_IREG | NPORT_DMODE_W2 |
			   NPORT_DMODE_ECINC | VC2_PROTOCOL);
	return regs->set.ddata0.hwords.s1;
}

/* VC2 Control register bits */
#define VC2_CTRL_EVIRQ     0x0001
#define VC2_CTRL_EDISP     0x0002
#define VC2_CTRL_EVIDEO    0x0004
#define VC2_CTRL_EDIDS     0x0008
#define VC2_CTRL_ECURS     0x0010
#define VC2_CTRL_EGSYNC    0x0020
#define VC2_CTRL_EILACE    0x0040
#define VC2_CTRL_ECDISP    0x0080
#define VC2_CTRL_ECCURS    0x0100
#define VC2_CTRL_ECG64     0x0200
#define VC2_CTRL_GLSEL     0x0400

/* Controlling the color map on NEWPORT. */
#define NCMAP_REGADDR_AREG   0x00000000
#define NCMAP_REGADDR_ALO    0x00000000
#define NCMAP_REGADDR_AHI    0x00000010
#define NCMAP_REGADDR_PBUF   0x00000020
#define NCMAP_REGADDR_CREG   0x00000030
#define NCMAP_REGADDR_SREG   0x00000040
#define NCMAP_REGADDR_RREG   0x00000060
#define NCMAP_PROTOCOL       (0x00008000 | 0x00040000 | 0x00800000)

static inline void newport_cmap_setaddr(struct newport_regs *regs,
					unsigned short addr)
{
	regs->set.dmode = (NPORT_DMODE_ACMALL | NCMAP_PROTOCOL |
			   NPORT_DMODE_SENDIAN | NPORT_DMODE_ECINC |
			   NCMAP_REGADDR_AREG | NPORT_DMODE_W2);
	regs->set.ddata0.hwords.s1 = addr;
	regs->set.dmode = (NPORT_DMODE_ACMALL | NCMAP_PROTOCOL |
			   NCMAP_REGADDR_PBUF | NPORT_DMODE_W3);
}

static inline void newport_cmap_setrgb(struct newport_regs *regs,
				       unsigned char red,
				       unsigned char green,
				       unsigned char blue)
{
	regs->set.ddata0.all =
		(red << 24) |
		(green << 16) |
		(blue << 8);
}

/* Miscellaneous NEWPORT routines. */
#define BUSY_TIMEOUT 100000
static inline int newport_wait(void)
{
	int i = 0;

	while(i < BUSY_TIMEOUT)
		if(!(npregs->cset.stat & NPORT_STAT_GBUSY))
			break;
	if(i == BUSY_TIMEOUT)
		return 1;
	return 0;
}

static inline int newport_bfwait(void)
{
	int i = 0;

	while(i < BUSY_TIMEOUT)
		if(!(npregs->cset.stat & NPORT_STAT_BBUSY))
			break;
	if(i == BUSY_TIMEOUT)
		return 1;
	return 0;
}

extern struct graphics_ops *newport_probe (int, const char **);

#endif /* !(_SGI_NEWPORT_H) */
