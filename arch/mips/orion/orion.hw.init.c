# 1 "/root/root4/orion/hw/orion.hw.init.c"

 
 
 
 
 
 
 
 
 
# 1 "/root/root4/orion/hw/include/hw.boot.h" 1
 

 














 



 








	 
# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/compiler.h" 1
 

 








 



































 































# 32 "/root/root4/orion/hw/include/hw.boot.h" 2

	 
# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/struct.h" 1
 

 








 
















 








 






 






# 34 "/root/root4/orion/hw/include/hw.boot.h" 2


	 
# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/debug.h" 1
 

 
























extern	void	DataDump (char *banner, void *data, int size);
extern	void	intr_printf (char *mesg);
extern	void	polled_printf (char *format, ...);
extern	void	debug_printf (char *format, ...);
extern	void	StackDump (void);









 













 


























# 107 "/root/root4/orion/hw/include/../../cosine/common/inc/debug.h"


 







# 126 "/root/root4/orion/hw/include/../../cosine/common/inc/debug.h"

# 137 "/root/root4/orion/hw/include/../../cosine/common/inc/debug.h"















 
























# 37 "/root/root4/orion/hw/include/hw.boot.h" 2



	 
# 1 "/root/root4/orion/hw/include/../../cosine/hwdeps/inc/hw.types.h" 1
 

 








	 



 



 











typedef	unsigned char		uchar_t;	 
typedef	unsigned short		ushort_t;	 
typedef	unsigned int		uint_t;		 
typedef	unsigned long		ulong_t;	 

typedef	unsigned char		uint8_t;
typedef	  char			sint8_t;

typedef	unsigned short		uint16_t;
typedef	  short		sint16_t;

typedef	unsigned int		uint32_t;
typedef	  int			sint32_t;


typedef	unsigned long long	uint64_t;
typedef	  long long	sint64_t;


typedef sint8_t		S8;
typedef sint8_t		s8;

typedef uint8_t		U8;
typedef uint8_t		u8;

typedef sint16_t	S16;
typedef sint16_t	s16;

typedef uint16_t	U16;
typedef uint16_t	u16;

typedef sint32_t	S32;
typedef sint32_t	s32;

typedef uint32_t	U32;
typedef uint32_t	u32;


typedef sint64_t	S64;
typedef sint64_t	s64;

typedef uint64_t	U64;
typedef uint64_t	u64;


typedef uint32_t	BOOL;

 




 
 
 

    
    
    
    

# 110 "/root/root4/orion/hw/include/../../cosine/hwdeps/inc/hw.types.h"




# 41 "/root/root4/orion/hw/include/hw.boot.h" 2

# 1 "/root/root4/orion/hw/include/../../cosine/hwdeps/inc/hw.pci.h" 1
 

 








	 






	 









 




 
















 











 













	 
extern 	void	pci_cpuDevNum(int *bus, int *device);
extern 	int		pci_getMaxBusNo(void);
extern 	void	*pci_intrToVecNum (int	intrLine, int	bus,
	int	device);
extern 	int		pci_findDevice (int  deviceId, int  vendorId,
	int  index, uint32_t * bus, uint32_t * device);
extern 	void	pci_intrOn (int	intrLine);
extern 	void	pci_intrOff (int	intrLine);
	 
 
extern 	int		pci_read (
	 	int	bus,				 
	 	int	device,				 
	 	int	function,			 
	 	int	reg,				 
	 	long	*data			 
	);
 
extern 	int		pci_write (
	 	int	bus,				 
	 	int	device,				 
	 	int	function,			 
	 	int	reg,				 
	 	long	data			 
	);
 
extern 	long	pci_getByte (
	 	long	data,			 
	 	int		byte			 
);
extern 	long	pci_setByte (
	 	long	data,			 
	 	long	pciData,		 
	 	int		byte			 
);
extern 	long	pci_getWord (
	 	long	data,			 
	 	int		word			 
);
extern 	long	pci_setWord (
	 	long	data,			 
	 	long	pciData,		 
	 	int		word			 
);

 
extern 	void	pci_scan (void);
extern 	int		pci_findDevice (
	int  deviceId,		 
	int  vendorId,		 
	int  index,			 
	uint32_t * bus,		 
	uint32_t * device		 
	);



# 42 "/root/root4/orion/hw/include/hw.boot.h" 2

# 1 "/root/root4/orion/hw/include/../../cosine/hwdeps/inc/hw.vm.h" 1
 

 











	 

extern	void	*vm_VirtToPhy (void	*x);
extern	void	*vm_PhyToVirt (void	*x, int attributes);



	 
extern	int		vm_IsCacheReadCoherent (void);
extern	int		vm_IsCacheWriteCoherent (void);
	 










extern	void	vm_InvalidateInstructionCache (void *start, int size);
extern	void	vm_InvalidateDataCache (void *start, int size);
extern	void	vm_FlushDataCache (void *start, int size);
extern	void	vm_FlushAndInvalidateDataCache (void *start, int size);


	 









static inline 
void	*
vm_VirtToPhy (void	*x)
{
	 
	return	(void *)(((int)x) & ~0xE0000000);
}
static inline 
void	*
vm_PhyToVirt (void	*x, int attributes)
{
	int				bits = 0;
	void			*vaddr;

	{ { if (! (  attributes != 0  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "attributes != 0", "/root/root4/orion/hw/include/../../cosine/hwdeps/inc/hw.vm.h", 66); asm("	break "); } } ; } ;
	 
	{ { if (! (  (((int)x) & 0xE0000000) == 0  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "(((int)x) & 0xE0000000) == 0", "/root/root4/orion/hw/include/../../cosine/hwdeps/inc/hw.vm.h", 68); asm("	break "); } } ; } ;
	if (attributes & 0x01 )
		bits = 0x80000000;
	else if (attributes & 0x02 )
		bits = 0xA0000000;
	 


	vaddr = (void *)((((int)x) & ~0xE0000000) | bits);
	return	vaddr;
}
# 100 "/root/root4/orion/hw/include/../../cosine/hwdeps/inc/hw.vm.h"

static inline 
int
vm_IsCacheReadCoherent (void)
{
	return	1 ;							 
}
static inline 
int
vm_IsCacheWriteCoherent (void)
{
	return	1 ;							 
}

static inline 
void
vm_InvalidateInstructionCache (void *start, int size)
{
	extern	Sys_Clear_Cache (void *, int);
	Sys_Clear_Cache (start, size);
}
static inline 
void
vm_InvalidateDataCache (void *start, int size)
{
	extern	Sys_Inval_Dcache (void *, int);
	Sys_Inval_Dcache (start, size);
}
void
static inline 
vm_FlushDataCache (void *start, int size)
{
	extern	Sys_Flush_Dcache ();
	Sys_Flush_Dcache ();
}
void
static inline 
vm_FlushAndInvalidateDataCache (void *start, int size)
{
	extern	Sys_Clear_Dcache (void *, int);
	Sys_Clear_Dcache (start, size);
}








# 43 "/root/root4/orion/hw/include/hw.boot.h" 2


	 
# 1 "/root/root4/orion/hw/include/../../cosine/ipsx/inc/ipsx.includes.h" 1
 

 








# 1 "/root/root4/orion/hw/ipsx/inc/pe_id.h" 1
 

 








typedef	uint16_t				ipsx_pe_type_t;
typedef	uint16_t				ipsx_pe_id_t;
typedef	ipsx_pe_type_t			ipsx_blade_type_t;
typedef	ipsx_pe_id_t			ipsx_slot_id_t;

typedef	uint16_t				address_space_t;

typedef	uint16_t				ipsx_lqid_t;


	 




	 



	 




	 








extern	address_space_t		local_address_space_id;
extern	ipsx_slot_id_t		local_slot_id;
extern	ipsx_pe_id_t		local_pe_id;
extern	ipsx_pe_type_t		local_pe_type;
extern	ipsx_blade_type_t	local_blade_type;

	 


	 
extern	void	ipsx_pe_down (
	address_space_t			address_id
	);
extern	void	ipsx_pe_up (
	address_space_t			address_id
	);
extern	void	ipsx_blade_down (
	address_space_t			address_id
	);
extern	void	ipsx_blade_up (
	address_space_t			address_id
	);
extern void		ipsx_validate_slot_id (
	ipsx_slot_id_t  slot_id
	);

	 
extern	int		ipsx_is_pe_live (
	address_space_t			address_id
	);
extern	int		ipsx_is_blade_live (
	address_space_t			address_id
	);
extern	int		ipsx_is_address_space_functional (
	address_space_t			address_id
	);


	 


extern	ipsx_slot_id_t	ipsx_get_slot_id (
	address_space_t				address_id
	);
extern	ipsx_pe_id_t	ipsx_get_pe_id (
	address_space_t				address_id
	);

extern	address_space_t ipsx_construct_address_id (
	ipsx_slot_id_t				slot_id,
	ipsx_pe_id_t				pe_id
	);

extern  address_space_t ipsx_get_my_address_id ();
extern  ipsx_slot_id_t ipsx_get_my_slot_id ();
extern  ipsx_pe_id_t ipsx_get_my_pe_id ();


	 











extern	address_space_t ipsx_start_slot_address_id (
	ipsx_slot_id_t				slot_id
	);
extern	address_space_t ipsx_next_slot_address_id (
	address_space_t				address_id
	);
extern	address_space_t ipsx_end_slot_address_id (
	ipsx_slot_id_t				slot_id
	);







static inline 
address_space_t ipsx_get_my_address_id ()
{
	return local_address_space_id;
}

static inline 
ipsx_slot_id_t ipsx_get_my_slot_id ()
{
	return local_slot_id;
}

static inline 
ipsx_pe_id_t ipsx_get_my_pe_id ()
{
	return local_pe_id;
}

static inline 
address_space_t ipsx_construct_address_id (
	ipsx_slot_id_t				slot_id,
	ipsx_pe_id_t				pe_id
	)
{
	{ { if (! (  slot_id <= 127   )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "slot_id <= 127", "/root/root4/orion/hw/ipsx/inc/pe_id.h", 155); asm("	break "); } } ; } ;
	{ { if (! (  pe_id <= 7   )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "pe_id <= 7", "/root/root4/orion/hw/ipsx/inc/pe_id.h", 156); asm("	break "); } } ; } ;
	slot_id		&=  0x7F ;
	slot_id		<<= 8 ;
	pe_id		&=  0x0F ;
	pe_id		<<= 0 ;
	return	(0x80F0  | slot_id | pe_id);
}
static inline 
ipsx_slot_id_t ipsx_get_slot_id (
	address_space_t				address_id
	)
{
	return	((address_id >> 8 ) & 0x7F );
}
static inline 
ipsx_pe_id_t ipsx_get_pe_id (
	address_space_t				address_id
	)
{
	return	((address_id >> 0 ) & 0x0F );
}

static inline 
address_space_t ipsx_start_slot_address_id (
	ipsx_slot_id_t				slot_id
	)
{
	{ { if (! (  slot_id < 127   )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "slot_id < 127", "/root/root4/orion/hw/ipsx/inc/pe_id.h", 183); asm("	break "); } } ; } ;
	return	ipsx_construct_address_id (slot_id, 0 );
}
static inline 
address_space_t ipsx_next_slot_address_id (
	address_space_t				address_id
	)
{
	ipsx_pe_id_t				pe_id = ipsx_get_pe_id (address_id);
	pe_id ++;
	return	ipsx_construct_address_id (
		ipsx_get_slot_id (address_id), pe_id);
}
static inline 
address_space_t ipsx_end_slot_address_id (
	ipsx_slot_id_t				slot_id
	)
{
	{ { if (! (  slot_id < 127   )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "slot_id < 127", "/root/root4/orion/hw/ipsx/inc/pe_id.h", 201); asm("	break "); } } ; } ;
	return	ipsx_construct_address_id (slot_id, 7 );
}



# 12 "/root/root4/orion/hw/include/../../cosine/ipsx/inc/ipsx.includes.h" 2

# 1 "/root/root4/orion/hw/ipsx/inc/pe_attrib.h" 1
 

 








	 


	 


typedef	uint32_t				ipsx_pe_capability_t;

	 




	 











	 



 
 
# 59 "/root/root4/orion/hw/ipsx/inc/pe_attrib.h"


extern char *ipsx_filename_extensions[];


	 



	 



	 





	 


extern	void	ipsx_mng_register_pe_type (
	address_space_t				address_id,
	ipsx_pe_type_t				type
	);
extern	ipsx_blade_type_t	ipsx_get_blade_type (
	ipsx_slot_id_t				slot_id
	);
extern	ipsx_pe_type_t	ipsx_get_pe_type (
	address_space_t				address_id
	);
extern	ipsx_pe_capability_t	ipsx_get_pe_capability (
	address_space_t				address_id
	);

extern	int ipsx_check_pe_for_capability (
	address_space_t				address_id,
	ipsx_pe_capability_t		capability
	);


typedef	struct	rls_results_s	rls_results_t;

extern	int	ipsx_get_pe_by_type (
	address_space_t			start_addr,
	ipsx_pe_type_t			type,
	rls_results_t			*results,
	int						max_results
	);
extern	int	ipsx_get_pe_by_capability (
	address_space_t			start_addr,
	ipsx_pe_capability_t	capability,
	rls_results_t			*results,
	int						max_results
	);





static inline 
ipsx_blade_type_t
ipsx_get_blade_type (
	address_space_t				address_id
	)
{
	ipsx_slot_id_t			slot_id = ipsx_get_slot_id (address_id);

	return	ipsx_get_pe_type (ipsx_construct_address_id (slot_id, 0 ));
}

static inline 
int
ipsx_check_pe_for_capability (
	address_space_t				address_id,
	ipsx_pe_capability_t		capability
	)
{
	return	(0 != (capability & ipsx_get_pe_capability (address_id)));
}




# 13 "/root/root4/orion/hw/include/../../cosine/ipsx/inc/ipsx.includes.h" 2



# 46 "/root/root4/orion/hw/include/hw.boot.h" 2




	 
# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/bits.h" 1
 

 








	 







	 
















	 




	 








	 
extern	uint16_t	BIT_SWAP16 (uint16_t	x);
extern	uint32_t	BIT_SWAP32 (uint32_t	x);
	 
extern	uint16_t	GET16 (uint8_t	*x);
extern	uint32_t	GET32 (uint8_t	*x);
	 
extern	void	PUT16 (uint8_t	*x, uint16_t val);
extern	void	PUT32 (uint8_t	*x, uint32_t val);




	 
static inline 
uint16_t
BIT_SWAP16 (uint16_t	x)
{
	return	(((x & 0xFF00) >> 8) | ((x & 0x00FF) << 8));
}
static inline 
uint32_t
BIT_SWAP32 (uint32_t	x)
{
	return	( ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) |
			  ((x & 0x0000FF00) << 8)  | ((x & 0x000000FF) << 24) );
}


	 
static inline 
uint16_t
GET16 (uint8_t	*x)
{
	uint16_t		val;
	val = *x++;
	val <<= 8;
	val |= *x;
	return	val;
}
static inline 
uint32_t
GET32 (uint8_t	*x)
{
	uint32_t		val;
	val = *x++;
	val <<= 8;
	val |= *x++;
	val <<= 8;
	val |= *x++;
	val <<= 8;
	val |= *x;
	return	val;
}


	 
static inline 
void
PUT16 (uint8_t	*x, uint16_t val)
{
	*x++ = (uint8_t) val;
	val >>= 8;
	*x = (uint8_t) val;
}
static inline 
void
PUT32 (uint8_t	*x, uint32_t val)
{
	*x++ = (uint8_t) val;
	val >>= 8;
	*x++ = (uint8_t) val;
	val >>= 8;
	*x++ = (uint8_t) val;
	val >>= 8;
	*x = (uint8_t) val;
}




# 51 "/root/root4/orion/hw/include/hw.boot.h" 2

# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/tokval.h" 1
 

 








	 



 




 




 








typedef
struct tokval_s
{
	int			value;
	char		*token;
} tokval_t;

static inline 
char *
tok_val2tok(int value, tokval_t *table, int numOfEntries, char *default_token)
{
	static char * undef = "Unknown";
	int		i;

	for(i = 0; i < numOfEntries; i++, table++)
	{
		if(table->value == value)
			return table->token;
	}
	return (default_token != ((void	*) 0) )? default_token: "Unknown";
}

static inline 
int
tok_tok2val(char *token, tokval_t *table, int numOfEntries, int default_value)
{
	int		i;

	for(i = 0; i < numOfEntries; i++, table++)
	{
		if(strcmp(table->token,token) == 0)
			return table->value;
	}
	return default_value;
}



# 52 "/root/root4/orion/hw/include/hw.boot.h" 2

# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/custom.h" 1
 

 




























# 53 "/root/root4/orion/hw/include/hw.boot.h" 2

# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h" 1
 

 








	 

typedef
struct	dlcl_list_s
{
	struct	dlcl_list_s	*next;	 
	struct	dlcl_list_s	*prev;	 
}		dlcl_list_t;




	 
extern	int	dlcl_ListIsValid (dlcl_list_t *l);
extern	int	dlcl_ListIsEmpty (dlcl_list_t *l);
extern	void	dlcl_ListInit (dlcl_list_t *l);
extern	dlcl_list_t	*dlcl_ListPrev (dlcl_list_t *l);
extern	dlcl_list_t	*dlcl_ListNext (dlcl_list_t *l);
extern	void	dlcl_ListAdd (dlcl_list_t *l1,dlcl_list_t *l2);
extern	void	dlcl_ListAddSingleton (dlcl_list_t *l1,dlcl_list_t *single);
extern	void	dlcl_ListAddNew (dlcl_list_t *l1, dlcl_list_t *new);
extern	void	dlcl_ListAddNewAfter (dlcl_list_t *l1, dlcl_list_t *new);
extern	void	dlcl_ListRemove (dlcl_list_t *l);







	 
static inline 
int
dlcl_ListIsValid (
	dlcl_list_t	*l
)
{
	return	((l != ((void	*) 0) ) &&
		(l->prev != ((void	*) 0) ) &&
		(l->next != ((void	*) 0) ) &&
		(l->prev->next == l->next->prev));
}
static inline 
int
dlcl_ListIsEmpty (
	dlcl_list_t	*l
)
{
	{ { if (! (  dlcl_ListIsValid (l)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 60); asm("	break "); } } ; } ;
	return	(l->prev == l);
}
static inline 
void
dlcl_ListInit (
	dlcl_list_t	*l
)
{
	l->prev = l->next = l;
}
static inline 
dlcl_list_t	*
dlcl_ListPrev (
	dlcl_list_t	*l
)
{
	{ { if (! (  dlcl_ListIsValid (l)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 77); asm("	break "); } } ; } ;
	return	l->prev;
}
static inline 
dlcl_list_t	*
dlcl_ListNext (
	dlcl_list_t	*l
)
{
	{ { if (! (  dlcl_ListIsValid (l)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 86); asm("	break "); } } ; } ;
	return	l->next;
}
static inline 
void
dlcl_ListAdd (
	dlcl_list_t	*l1,
	dlcl_list_t	*l2
)
{
	dlcl_list_t	*l1_last;
	dlcl_list_t	*l2_last;

	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 99); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (l2)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l2)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 100); asm("	break "); } } ; } ;
	l1_last = l1->prev;
	l2_last = l2->prev;

	l1_last->next = l2;
	l2->prev = l1_last;
	l2_last->next = l1;
	l1->prev = l2_last;
	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 108); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (l2)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l2)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 109); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (l1_last)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1_last)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 110); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (l2_last)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l2_last)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 111); asm("	break "); } } ; } ;
}
static inline 
void
dlcl_ListAddSingleton (
	dlcl_list_t	*l1,
	dlcl_list_t	*single
)
{
	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 120); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (single)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (single)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 121); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsEmpty (single)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsEmpty (single)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 122); asm("	break "); } } ; } ;
	 
	single->next = l1;
	single->prev = l1->prev;
	 
	l1->prev->next = single;
	l1->prev = single;
	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 129); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (single)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (single)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 130); asm("	break "); } } ; } ;
}
static inline 
void
dlcl_ListAddNew (
	dlcl_list_t	*l1,
	dlcl_list_t	*new
)
{
	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 139); asm("	break "); } } ; } ;
	 
	new->next = l1;
	new->prev = l1->prev;
	 
	l1->prev->next = new;
	l1->prev = new;
	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 146); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (new)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (new)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 147); asm("	break "); } } ; } ;
}
static inline 
void
dlcl_ListAddNewAfter (
	dlcl_list_t	*l1,
	dlcl_list_t	*new
)
{
	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 156); asm("	break "); } } ; } ;
	 
	new->next = l1->next;
	new->prev = l1;
	 
	l1->next->prev = new;
	l1->next = new;
	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 163); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (new)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (new)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 164); asm("	break "); } } ; } ;
}
static inline 
void
dlcl_ListRemove (
	dlcl_list_t	*l
)
{
	{ { if (! (  dlcl_ListIsValid (l)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 172); asm("	break "); } } ; } ;
	l->prev->next = l->next;
	l->next->prev = l->prev;
	{ { if (! (  dlcl_ListIsValid (l->prev)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l->prev)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 175); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (l->next)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l->next)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 176); asm("	break "); } } ; } ;
	l->prev = l->next = l;
	{ { if (! (  dlcl_ListIsValid (l)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 178); asm("	break "); } } ; } ;
}
static inline 
void
dlcl_ListInsertBefore (
	dlcl_list_t	*l1,
	dlcl_list_t	*l2
)
{
	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 187); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (l2)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l2)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 188); asm("	break "); } } ; } ;
	dlcl_ListAdd (l1,l2);
}
static inline 
void
dlcl_ListInsertAfter (
	dlcl_list_t	*l1,
	dlcl_list_t	*l2
)
{
	{ { if (! (  dlcl_ListIsValid (l1)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l1)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 198); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (l2)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (l2)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 199); asm("	break "); } } ; } ;
	dlcl_ListAdd (l1->next,l2);
}

static inline 
void
dlcl_ListDump (
	dlcl_list_t	*l
)
{
	printf ("dlcl element %lx, next %lx, prev %lx\n",
		(long) l, (long) l->next, (long) l->prev);
}

static inline 
void
dlcl_insert_in_order (
	dlcl_list_t				*list,
	dlcl_list_t				*element,
	int						(*compare)(dlcl_list_t *e1, dlcl_list_t *e2)
)
{
	  
	if (dlcl_ListIsEmpty (list))
	{
		dlcl_ListAdd (list,element);
	}
	else
	{
		dlcl_list_t				*next; 
		next = dlcl_ListNext (list);
		while (next != list)  
		{ 
			if ((*compare)(element,next) < 0) 
			{
				break;
			}
			next = dlcl_ListNext (next);
		}
		if (next == list)
		{
			 
			next = dlcl_ListPrev (next);
		}
		if ((*compare)(element,next) < 0) 
		{
			dlcl_ListInsertBefore (next, element);
			{ { if (! (  dlcl_ListPrev (next) == element  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListPrev (next) == element", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 246); asm("	break "); } } ; } ;
		}
		else
		{
			dlcl_ListInsertAfter (next, element);
			{ { if (! (  dlcl_ListNext (next) == element  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListNext (next) == element", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 251); asm("	break "); } } ; } ;
		}
	}
	{ { if (! (  ! dlcl_ListIsEmpty (list)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "! dlcl_ListIsEmpty (list)", "/root/root4/orion/hw/include/../../cosine/common/inc/dlclist.h", 254); asm("	break "); } } ; } ;
}





# 54 "/root/root4/orion/hw/include/hw.boot.h" 2

# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h" 1
 

 








	 

typedef
struct	q_head_s
{
	dlcl_list_t		head;
	int					count;
}		q_head_t;

typedef	dlcl_list_t	q_element_t;



extern	int	q_isEmpty (q_head_t *qh);
extern	int	q_elementNotInQueue (q_element_t *qe);
extern	int	q_hasElement (q_head_t *qh,q_element_t *qe);
extern	int	q_count (q_head_t *qh);
extern	void	q_headInit (q_head_t *qh);
extern	void	q_elementInit (q_element_t *qe);
extern	void	q_addToTail (q_head_t *qh,q_element_t *qe);
extern	void	q_addToHead (q_head_t *qh,q_element_t *qe);
extern	void	q_concatenate (q_head_t *destination,q_head_t *source);
extern	q_element_t	*q_get (q_head_t *qh);
extern	void	q_deleteElement (q_head_t *qh,q_element_t *elem);
extern	q_element_t	*q_first (q_head_t *qh);
extern	q_element_t	*q_last (q_head_t *qh);
extern	q_element_t	*q_next (q_head_t *qh,q_element_t *qe);
extern	q_element_t	*q_prev (q_element_t *qe);






static inline 
int
q_isEmpty (
	q_head_t		*qh
)
{
	{ { if (! (  dlcl_ListIsValid (&qh->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&qh->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 52); asm("	break "); } } ; } ;
	return	dlcl_ListIsEmpty (&qh->head);
}
static inline 
int
q_elementNotInQueue (
	q_element_t	*qe
)
{
	{ { if (! (  dlcl_ListIsValid (qe)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (qe)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 61); asm("	break "); } } ; } ;
	return	dlcl_ListIsEmpty (qe);
}

static inline 
int
q_hasElement (
	q_head_t		*qh,
	q_element_t	*qe
)
{
	q_element_t	*curr;
	{ { if (! (  dlcl_ListIsValid (&qh->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&qh->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 73); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (qe)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (qe)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 74); asm("	break "); } } ; } ;
	for (curr = qh->head.next; curr != &qh->head; curr = curr->next)
	{
		if (curr == qe)
			return	1 ;
	}
	return	0 ;
}

static inline 
int
q_count (
	q_head_t		*qh
)
{
	{ { if (! (  dlcl_ListIsValid (&qh->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&qh->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 89); asm("	break "); } } ; } ;
	return	qh->count;
}



static inline 
void
q_headInit (
	q_head_t		*qh
)
{
	dlcl_ListInit (&qh->head);
	qh->count = 0;
}

static inline 
void
q_elementInit (
	q_element_t	*qe
)
{
	dlcl_ListInit (qe);
}



static inline 
void
q_addToTail (
	q_head_t		*qh,
	q_element_t	*qe
)
{
	{ { if (! (  dlcl_ListIsValid (&qh->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&qh->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 123); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (qe)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (qe)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 124); asm("	break "); } } ; } ;
	{ { if (! (  q_elementNotInQueue (qe)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "q_elementNotInQueue (qe)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 125); asm("	break "); } } ; } ;
	dlcl_ListAddSingleton (&qh->head, qe);
	qh->count ++;
}

static inline 
void
q_addToHead (
	q_head_t		*qh,
	q_element_t	*qe
)
{
	{ { if (! (  dlcl_ListIsValid (&qh->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&qh->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 137); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (qe)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (qe)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 138); asm("	break "); } } ; } ;
	{ { if (! (  q_elementNotInQueue (qe)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "q_elementNotInQueue (qe)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 139); asm("	break "); } } ; } ;
	dlcl_ListAddSingleton (qh->head.next, qe);
	qh->count ++;
}

static inline 
void
q_concatenate (
	q_head_t		*destination,
	q_head_t		*source
)
{
	{ { if (! (  dlcl_ListIsValid (&destination->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&destination->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 151); asm("	break "); } } ; } ;
	{ { if (! (  dlcl_ListIsValid (&source->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&source->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 152); asm("	break "); } } ; } ;
	if (! q_isEmpty (source))
	{
		dlcl_list_t	*srcList;

		srcList = source->head.next;
		dlcl_ListRemove (&source->head);
		dlcl_ListAdd (&destination->head, srcList);
		destination->count += source->count;
		source->count = 0;
	}
}


static inline 
q_element_t	*
q_get (
	q_head_t		*qh
)
{
	q_element_t	*ret = ((void	*) 0) ;

	{ { if (! (  dlcl_ListIsValid (&qh->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&qh->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 174); asm("	break "); } } ; } ;
	if (! q_isEmpty (qh))
	{
		{ { if (! (  q_count (qh) > 0  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "q_count (qh) > 0", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 177); asm("	break "); } } ; } ;
		ret = qh->head.next;
		dlcl_ListRemove (ret);
		qh->count --;
	}
	return	ret;
}

static inline 
void
q_deleteElement (
	q_head_t		*qh,
	q_element_t	*elem
)
{
	{ { if (! (  dlcl_ListIsValid (elem)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (elem)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 192); asm("	break "); } } ; } ;
	{ { if (! (  ! q_isEmpty (qh)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "! q_isEmpty (qh)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 193); asm("	break "); } } ; } ;
	{ { if (! (  q_count (qh) > 0  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "q_count (qh) > 0", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 194); asm("	break "); } } ; } ;
	{ { if (! (  q_hasElement (qh, elem)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "q_hasElement (qh, elem)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 195); asm("	break "); } } ; } ;
	dlcl_ListRemove (elem);
	qh->count --;
	{ { if (! (  q_count (qh) >= 0  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "q_count (qh) >= 0", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 198); asm("	break "); } } ; } ;
}



static inline 
q_element_t	*
q_first (
	q_head_t		*qh
)
{
	q_element_t	*ret = ((void	*) 0) ;

	{ { if (! (  dlcl_ListIsValid (&qh->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&qh->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 211); asm("	break "); } } ; } ;
	if (! q_isEmpty (qh))
	{
		ret = qh->head.next;
	}
	return	ret;
}

static inline 
q_element_t	*
q_last (
	q_head_t		*qh
)
{
	q_element_t	*ret = ((void	*) 0) ;

	{ { if (! (  dlcl_ListIsValid (&qh->head)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (&qh->head)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 227); asm("	break "); } } ; } ;
	if (! q_isEmpty (qh))
	{
		ret = qh->head.prev;
	}
	return	ret;
}

static inline 
q_element_t	*
q_next (
	q_head_t		*qh,
	q_element_t	*qe
)
{
	q_element_t	*ret = ((void	*) 0) ;

	{ { if (! (  dlcl_ListIsValid (qe)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (qe)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 244); asm("	break "); } } ; } ;
	{ { if (! (  ! q_elementNotInQueue (qe)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "! q_elementNotInQueue (qe)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 245); asm("	break "); } } ; } ;
	if (qe->next != &qh->head)
	{
		ret = qe->next;
	}
	return	ret;
}

static inline 
q_element_t	*
q_prev (
	q_element_t	*qe
)
{
	q_element_t	*ret = ((void	*) 0) ;

	{ { if (! (  dlcl_ListIsValid (qe)  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "dlcl_ListIsValid (qe)", "/root/root4/orion/hw/include/../../cosine/common/inc/queue.h", 261); asm("	break "); } } ; } ;
	if (! dlcl_ListIsEmpty (qe))
	{
		ret = qe->prev;
	}
	return	ret;
}






# 55 "/root/root4/orion/hw/include/hw.boot.h" 2

# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h" 1
 

 








 
 
 













 
 
 

struct tlv1_s {
    uchar_t tag;
    uchar_t length;
};
typedef struct tlv1_s tlv1_t;

struct tlv2_s {
    ushort_t tag;
    ushort_t length;
};
typedef struct tlv2_s tlv2_t;

struct tlv3_s {
    ulong_t tag;
    ulong_t length;
};
typedef struct tlv3_s tlv3_t;

 
 
 

static inline 
tlv1_t *tlv1_get_next_tag(tlv1_t *tlv_p, uchar_t tag)
{
    { { if (! (  tlv_p  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "tlv_p", "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h", 57); asm("	break "); } } ; } ;
    while(tlv_p->tag != 0xFF ) {
        if (tlv_p->tag == tag)
	  return tlv_p;
	tlv_p = (tlv1_t *)((char *) tlv_p  + sizeof(tlv1_t) +  tlv_p ->length) ;
    }

    return ((void	*) 0) ;
}
static inline 
void tlv1_get_value(tlv1_t *tlv_p, void *value_p)
{
    { { if (! (  value_p  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "value_p", "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h", 69); asm("	break "); } } ; } ;
    { { if (! (  tlv_p  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "tlv_p", "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h", 70); asm("	break "); } } ; } ;
    memcpy(value_p, (char *)(tlv_p+1), tlv_p->length);
}

static inline 
tlv2_t *tlv2_get_next_tag(tlv2_t *tlv_p, ushort_t tag)
{
    { { if (! (  tlv_p  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "tlv_p", "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h", 77); asm("	break "); } } ; } ;
    while(tlv_p->tag != 0xFFFF ) {
        if (tlv_p->tag == tag)
	  return tlv_p;
	tlv_p = (tlv2_t *)((char *) tlv_p  + sizeof(tlv2_t) +  tlv_p ->length) ;
    }

    return ((void	*) 0) ;
}
static inline 
void tlv2_get_value(tlv2_t *tlv_p, void *value_p)
{
    { { if (! (  value_p  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "value_p", "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h", 89); asm("	break "); } } ; } ;
    { { if (! (  tlv_p  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "tlv_p", "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h", 90); asm("	break "); } } ; } ;
    memcpy(value_p, (char *)(tlv_p+1), tlv_p->length);
}

static inline 
tlv3_t *tlv3_get_next_tag(tlv3_t *tlv_p, ulong_t tag)
{
    { { if (! (  tlv_p  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "tlv_p", "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h", 97); asm("	break "); } } ; } ;
    while(tlv_p->tag != 0xFFFFFFFF ) {
        if (tlv_p->tag == tag)
	  return tlv_p;
	tlv_p = (tlv3_t *)((char *) tlv_p  + sizeof(tlv3_t) +  tlv_p ->length) ;
    }

    return ((void	*) 0) ;
}
static inline 
void tlv3_get_value(tlv3_t *tlv_p, void *value_p)
{
    { { if (! (  value_p  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "value_p", "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h", 109); asm("	break "); } } ; } ;
    { { if (! (  tlv_p  )) { extern _cosine_task_suspend_on_error (void); printf  ("Assertion: \"%s\"\n\tfailed in file %s at line %d\n", "tlv_p", "/root/root4/orion/hw/include/../../cosine/common/inc/tlv.h", 110); asm("	break "); } } ; } ;
    memcpy(value_p, (char *)(tlv_p+1), tlv_p->length);
}






# 56 "/root/root4/orion/hw/include/hw.boot.h" 2



	 
# 1 "/root/root4/orion/hw/include/../../cosine/common/inc/modules.h" 1
 

 








 
 
 
 
 
 
 
 
 
 



 































































 






# 60 "/root/root4/orion/hw/include/hw.boot.h" 2




# 11 "/root/root4/orion/hw/orion.hw.init.c" 2

# 1 "/root/root4/orion/hw/include/board.h" 1
 
 
 
 

 
 
 
 






 
 
 
# 1 "/root/root4/orion/hw/include/board_map.h" 1
 
 
 
 
 
 
 
 








# 1 "/root/root4/orion/os/regaeos/include/cpu4000.h" 1




 
 
 












 
 
 







 
 
 









 
 
 









































 
 
 







 
 
 



























 
 
 













 





 


 

 


 


 


 
 
 
















 
 
 


































 
 
 



 
 
 














	  


















 
 
 








 
 
 
 

 













 
 
 




































































































































































































































 
 
 




 
 

typedef U32 instr_t;









# 17 "/root/root4/orion/hw/include/board_map.h" 2



   
   
   
   
   


 













 







 





 




 












 





 
 









 




 




 












 





 
 
 














 







 





 




 












 





 

 

 


 



 



 
# 1 "/root/root4/orion/hw/include/gt64120.h" 1
 
 
 
 
 
 
 
 




 
 
 
 











 
 
 


 


 
 
 




































































































































 
 
 
















 
 
 








 
 
 











 
 
 


































 
 
 






 
 
 
























 
 
 








 
 
 




 
 
 









 
 
 

























 
 
 










 
 
 






















 
 
 













# 190 "/root/root4/orion/hw/include/board_map.h" 2









# 19 "/root/root4/orion/hw/include/board.h" 2



 
 
 












# 12 "/root/root4/orion/hw/orion.hw.init.c" 2


# 1 "/root/root4/orion/hw/include/qpic.h" 1
 














typedef struct Qpic_Registers_S 
{
	U16		QpicId;
	U16		MiscCtrl;
	U16		IntStat;
	U16		IntMask;
	U16		BusErrorCapture[2];
	U16		Status;

	 
	U16		filler[0x79];

	 
	U16		LocalBusConfig[32];
} Qpic_Registers_T;

 


 


 













# 14 "/root/root4/orion/hw/orion.hw.init.c" 2

# 1 "/root/root4/orion/hw/include/cupid.h" 1
 
















typedef struct Cupid_Registers_S 
{
	U16	Id;
	U16	PeConfig;
	U16	Led;
	U16	TempSensor;
	U16	LaserId;
	U16	RtcReg;
	U16	Reset;

	U16 	filler1[9];

	U16	InterruptMap[7 ][2 ];

	U16	filler2[2];

	U16	Id_Struc[8];

	U16	filler3[8];

	U16	Control[2];
	U16	Status[2];

	U16	filler4[12];

	U16	Ds3BitsLed;
	U16	Ds3Led1;
	U16	Ds3Led2;
	U16	Ds3Led3;
	U16	Ds3ClkCtrl;
	 
} Cupid_Registers_T;

 
 
 

 


 





typedef struct Cupid_CIB_Ver1_S
{
	U8	cib_version;
	U8	blade_id;
	U8	pe_id;
	U8	blade_type;
	U8	pe_type;
	U8	Fpga_Set_Id[4];
	U8	Fpga_Set_Rev[6];	 

	U8	Expansion_TLV[17];   
} Cupid_CIB_Ver1_T;

extern Cupid_CIB_Ver1_T *find_cib_ptr();

 


 


 












 






 
	 



 





 


 
	 









 




























 





 
	 













	 




	 


















	 









	 




 
  

 

 

 

 

 

 

 


# 15 "/root/root4/orion/hw/orion.hw.init.c" 2












typedef unsigned char UCHAR;
ipsx_pe_type_t local_pe_type;


address_space_t    local_address_space_id;
    ipsx_slot_id_t local_slot_id;
ipsx_pe_id_t     local_pe_id;
    ipsx_blade_type_t local_blade_type;






extern U32	GetRamBanks(U32 *base_array, U32 *top_array);

 
 
 


typedef struct ChipSelect_S
{
	U8	cs_number;
	U16	cs_config;
} ChipSelect_T;



typedef struct InterruptMap_S
{
	U8	cpu_int;
	U32	int_mapping;
} InterruptMap_T;

 
 
 
typedef unsigned long ULONG;
 
 
 
ULONG cpuClkRate;        

 
 
 
void InitCIB(void);
void InitQpic(void);
void InitCupid(void);

 
 
 

extern ulong_t memsize;
 
 
 
extern void SerChipInit();
extern long SerChipOpen();
extern long SerChipSend();
extern long SerChipIoctl();
extern long SerChipClose();

 
 
 

 
static ChipSelect_T EthernetChipSelectConfig[] = 
{
	{0 ,		(0x0008  | 0x0000 )},
	{6 ,		(0x0010  | 0x0000 )},
	{7 ,		(0x0004  | 0x0000 )},
	{8 ,	(0x0000  | 0x0000 )},
	{9 ,	(0x0000  | 0x0000 )},
	{10 ,		(0x0000  | 0x0000 )},
	{16 ,	(0x0000  | 0x0000 )},
	{17 ,		(0x0000  | 0x0000 )},
	{18 ,			(0x0000  | 0x0000 )},
	{27 ,		(0x0000  | 0x0000 )},
	{28 ,	(0x0004  | 0x0000 )},
	{0xff , 0}
};

 
static ChipSelect_T CryptoChipSelectConfig[] = 
{
	{6 ,		(0x0000  | 0x0000 )},
	{7 ,		(0x0000  | 0x0000 )},
	{8 ,	(0x0000  | 0x0000 )},
	{16 ,	(0x0000  | 0x0000 )},
	{17 ,		(0x0000  | 0x0000 )},
	{18 ,			(0x0000  | 0x0000 )},
	{27 ,		(0x0000  | 0x0000 )},
	{28 ,	(0x0000  | 0x0000 )},
	{0xff , 0}
};

 
static ChipSelect_T Ds3ChipSelectConfig[] = 
{
	{0 ,			(0x0008  | 0x0000 )},
	{1 ,			(0x0008  | 0x0000 )},
	{2 ,		(0x0008  | 0x0000 )},
	{3 ,	(0x0008  | 0x0000 )},
	{4 ,	(0x0008  | 0x0000 )},
	{5 ,	(0x0008  | 0x0000 )},
	{8 ,		(0x0000  | 0x0000 )},
	{10 ,			(0x0000  | 0x0000 )},
	{6 ,	(0x0008  | 0x0000 )},
	{11 ,	(0x0008  | 0x0000 )},
	{12 ,	(0x0008  | 0x0000 )},
	{13 ,	(0x0008  | 0x0000 )},
	{15 ,			(0x0008  | 0x0004  | 0x0000 )},
	{14 ,			(0x0008  | 0x0004  | 0x0000 )},
	{16 ,		(0x0000  | 0x0000 )},
	{17 ,			(0x0000  | 0x0000 )},
	{18 ,				(0x0000  | 0x0000 )},
	{27 ,			(0x0000  | 0x0000 )},
	{28 ,		(0x0000  | 0x0000 )},
	{0xff , 0}
};

 
static ChipSelect_T SearchChipSelectConfig[] = 
{
	{6 , 	(0x0000  | 0x0000 )},
	{7 , 	(0x0000  | 0x0000 )},
	{8 ,	(0x0000  | 0x0000 )},
	{9 , 	(0x0000  | 0x0000 )},
	{16 ,	(0x0000  | 0x0000 )},
	{17 ,		(0x0000  | 0x0000 )},
	{18 ,			(0x0000  | 0x0000 )},
	{27 ,		(0x0000  | 0x0000 )},
	{28 ,	(0x0000  | 0x0000 )},
	{0xff , 0}
};

 
static ChipSelect_T ProcessorChipSelectConfig[] = 
{
	{0 ,		(0x0008  | 0x0000 )},
	{16 ,	(0x0000  | 0x0000 )},
	{17 ,		(0x0000  | 0x0000 )},
	{18 ,			(0x0000  | 0x0000 )},
	{27 ,		(0x0000  | 0x0000 )},
	{28 ,	(0x0000  | 0x0000 )},
	{0xff , 0}
};

 
static ChipSelect_T Oc3ChipSelectConfig[] = 
{
	{0 ,		(0x0008  | 0x0000 )},
	{10 ,		(0x0000  | 0x0000 )},
	{16 ,	(0x0000  | 0x0000 )},
	{17 ,		(0x0000  | 0x0000 )},
	{18 ,			(0x0000  | 0x0000 )},
	{27 ,		(0x0000  | 0x0000 )},
	{28 ,	(0x0000  | 0x0000 )},
	{0xff , 0}
};


 







 
static InterruptMap_T EthernetIntMap[] =
{
	{(2 -2),	(0x00010000 )},
	{(3 -2),		(0x00040000  | 0x02000000  | 0x04000000 )},
	{(4 -2),		(0x00080000  | 0x00100000  | 0x00200000 )},
	{3 ,				(0x00000800   | 0x00001000  )},
	{4 ,				(0x00008000   | 0x00020000 )},
	{0xff , 0}
};

 
static InterruptMap_T CryptoIntMap[] =
{
	{(2 -2),	(0x00010000 )},
	{(3 -2),		(0x00040000  | 0x00000008   | 0x00000010  )},
	{(4 -2),		(0x00080000  | 0x00100000  | 0x00200000 )},
	{3 ,				(0x00000002   | 0x00000004  )},
	{4 ,				(0x00020000 )},
	{0xff , 0}
};

 
static InterruptMap_T Ds3ChanIntMap[] =
{
	{(2 -2),	(0x00010000 )},
	{(3 -2),		(0x00040000  | 0x02000000  | 0x04000000 )},
	{(4 -2),		(0x00080000  | 0x00100000  | 0x00200000 )},
	{2 ,				(0x00000001   | 0x00000002  )},
	{3 ,				(0x00000200  | 0x00000100  | 0x00000080  | 0x00000040  | 0x00000020  | 0x00000010  | 0x00000008  )},
	{4 ,				(0x00008000   | 0x00020000 )},
	{0xff , 0}
};
static InterruptMap_T Ds3UnchanIntMap[] =
{
	{(2 -2),	(0x00010000 )},
	{(3 -2),		(0x00040000  | 0x00000400  )},
	{(4 -2),		(0x00080000  | 0x00100000  | 0x00200000 )},
	{2 ,				(0x00000002  )},
	{3 ,				(0x00000400  )},
	{4 ,				(0x00008000   | 0x00020000 )},
	{0xff , 0}
};

 
static InterruptMap_T SearchIntMap[] =
{
	{(2 -2),	(0x00010000 )},
	{(3 -2),		(0x00040000 )},
	{0xff , 0}
};

 
static InterruptMap_T ProcessorIntMap[] =
{
	{(2 -2),	(0x00010000 )},
	{(3 -2),		(0x00040000 )},
	{4 ,				(0x00008000   | 0x00020000 )},
	{0xff , 0}
};

 
static InterruptMap_T Oc3IntMap[] =
{
	{(2 -2),	(0x00010000 )},
	{(3 -2),		(0x00040000  | 0x00000004  )},
	{(4 -2),		(0x00080000  | 0x00100000  | 0x00000040  )},
	{3 ,				(0x00000008  )},
	{4 ,				(0x00008000   | 0x00020000 )},
	{0xff , 0}
};

 













void InitQpic(void)
{
	Qpic_Registers_T	*regs;
	U32					i = 0;
	ChipSelect_T		*ChipSelectConfig;
	U32					bank_cnt;
	U32					bank_base[4], bank_top[4];

	regs = (Qpic_Registers_T *)((unsigned)( 0x17B00000  )|0xA0000000)  ;

	 
	switch (local_pe_type)
	{
		case 1 :
			ChipSelectConfig = (ChipSelect_T *)&EthernetChipSelectConfig[0].cs_number;
			break;
		case 2 :
			ChipSelectConfig = (ChipSelect_T *)&EthernetChipSelectConfig[0].cs_number;
			break;
		case 3 :
		case 12 :
			ChipSelectConfig = (ChipSelect_T *)&ProcessorChipSelectConfig[0].cs_number;
			break;
		case 4 :
			ChipSelectConfig = (ChipSelect_T *)&SearchChipSelectConfig[0].cs_number;
			break;
		case 5 :
			ChipSelectConfig = (ChipSelect_T *)&CryptoChipSelectConfig[0].cs_number;
			break;
		case 6 :
		case 7 :
			ChipSelectConfig = (ChipSelect_T *)&Ds3ChipSelectConfig[0].cs_number;
			break;
		case 8 :
		case 9 :
		case 10 :
		case 11 :
			ChipSelectConfig = (ChipSelect_T *)&Oc3ChipSelectConfig[0].cs_number;
			break;
		default:
		  while(1 ); 

			break;
	}

	 
	while(ChipSelectConfig[i].cs_number != 0xff )
	{
		regs->LocalBusConfig[ChipSelectConfig[i].cs_number] =
							ChipSelectConfig[i].cs_config;
		i++;
	}

	 
	if ( (regs->Status & 0x0001 ) == 0x0001 )
		regs->LocalBusConfig[28 ] |= 0x0004 ;
	else
		regs->LocalBusConfig[28 ] &= ~0x0004 ;

	 
	regs->MiscCtrl = 0x0001 ;

	 
	bank_cnt = GetRamBanks(bank_base, bank_top);

	return;
}

 












void InitCupid(void)
{
	U32	i;
	Cupid_Registers_T	*regs;
	InterruptMap_T		*CupidIntMap;

	regs = (Cupid_Registers_T *)((unsigned)( 0x17100000  )|0xA0000000)  ;

	 
	switch (local_pe_type)
	{
		case 1 :
			CupidIntMap = (InterruptMap_T *)&EthernetIntMap[0].cpu_int;
			break;
		case 2 :
			CupidIntMap = (InterruptMap_T *)&EthernetIntMap[0].cpu_int;
			break;
		case 3 :
		case 12 :
			CupidIntMap = (InterruptMap_T *)&ProcessorIntMap[0].cpu_int;
			break;
		case 4 :
			CupidIntMap = (InterruptMap_T *)&SearchIntMap[0].cpu_int;
			break;
		case 5 :
			CupidIntMap = (InterruptMap_T *)&CryptoIntMap[0].cpu_int;
			break;
		case 6 :
			CupidIntMap = (InterruptMap_T *)&Ds3ChanIntMap[0].cpu_int;
			break;
		case 7 :
			CupidIntMap = (InterruptMap_T *)&Ds3UnchanIntMap[0].cpu_int;
			break;
		case 8 :
		case 9 :
		case 10 :
		case 11 :
			CupidIntMap = (InterruptMap_T *)&Oc3IntMap[0].cpu_int;
			break;
		default:
		  while(1 ); 


			break;
	}

	 
	for(i = 0; i < 7 ; i++)
	{
		regs->InterruptMap[i][0 ] = 0;
		regs->InterruptMap[i][1 ] = 0;
	}

	i = 0;
	while(CupidIntMap[i].cpu_int != 0xff )
	{
		regs->InterruptMap[CupidIntMap[i].cpu_int][0 ] |=
								(U16)(CupidIntMap[i].int_mapping & 0xffff);
		regs->InterruptMap[CupidIntMap[i].cpu_int][1 ] |=
								(U16)(CupidIntMap[i].int_mapping >> 16);
		i++;
	}

	return;
}


 
 
 
 
void InitCIB()
{
    Cupid_CIB_Ver1_T *cib_p;

    cib_p = find_cib_ptr() ;

    local_address_space_id = (cib_p->blade_id << 3) | cib_p->pe_id;
    local_slot_id = cib_p->blade_id;
    local_pe_id = cib_p->pe_id;
    local_blade_type = cib_p->blade_type;
    local_pe_type = cib_p->pe_type;

}

 
 
 
 
 
 
 
 
unsigned long BspRamBase(void)
{
return ((unsigned)( 0x00000000  )|0x80000000)  ;
}


 
 
 
 
 
 
unsigned long BspCpuType(void)
{
    return(4 );
}


 
 
 
 
 
 
 
 
 
 
void BoardSysStartCO(void)
{
}

U32	GetRamBanks(U32 *base_array, U32 *top_array)
{
	U32	bank_count = 0;
	U32	i;
	U32	gt_base_reg, gt_top_reg;

	 
	if ( (((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x400)  )&0xff)<<24)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x400)  )&0xff00)<<8)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x400)  )&0xff0000)>>8)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x400)  )&0xff000000)>>24))  != 0x000000ff)
	{
		bank_count++;
		if ( (((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x408)  )&0xff)<<24)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x408)  )&0xff00)<<8)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x408)  )&0xff0000)>>8)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x408)  )&0xff000000)>>24))  != 0x000000ff)
		{
			bank_count++;
			if ( (((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x410)  )&0xff)<<24)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x410)  )&0xff00)<<8)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x410)  )&0xff0000)>>8)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x410)  )&0xff000000)>>24))  != 0x000000ff)
			{
				bank_count++;
				if ( (((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x418)  )&0xff)<<24)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x418)  )&0xff00)<<8)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x418)  )&0xff0000)>>8)+	((( *(U32 *)(((unsigned)( 0x14000000  )|0xA0000000)   + 0x418)  )&0xff000000)>>24))  != 0x000000ff)
					bank_count++;
			}
		}
	}

	if (bank_count > 4)
		return(0);
		
	 
	for (i = 0; i < bank_count; i++)
	{
		switch (i)
		{
			case 0:
				gt_base_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x400) ;
				gt_top_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x404) ;
				break;

			case 1:
				gt_base_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x408) ;
				gt_top_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x40C) ;
				break;

			case 2:
				gt_base_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x410) ;
				gt_top_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x414) ;
				break;

			case 3:
				gt_base_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x418) ;
				gt_top_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x41C) ;
				break;

			default:
				gt_base_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x400) ;
				gt_top_reg = (((unsigned)( 0x14000000  )|0xA0000000)   + 0x404) ;
				break;
		}
		base_array[i] = ((unsigned)( (((( *(U32 *)gt_base_reg )&0xff)<<24)+	((( *(U32 *)gt_base_reg )&0xff00)<<8)+	((( *(U32 *)gt_base_reg )&0xff0000)>>8)+	((( *(U32 *)gt_base_reg )&0xff000000)>>24))  << 20 )|0x80000000) ;
		top_array[i] = ((unsigned)( ((((( *(U32 *)gt_top_reg )&0xff)<<24)+	((( *(U32 *)gt_top_reg )&0xff00)<<8)+	((( *(U32 *)gt_top_reg )&0xff0000)>>8)+	((( *(U32 *)gt_top_reg )&0xff000000)>>24))  << 20) | 0x000fffff )|0x80000000) ;
	}

	return(bank_count);
}



 
 
 
 
 
 
 
 
 
Cupid_CIB_Ver1_T *cib;

 
 
 
void read_fpga_revs();
Cupid_CIB_Ver1_T *find_cib_ptr();


 
 
 

 
 
 


# 606 "/root/root4/orion/hw/orion.hw.init.c"

Cupid_CIB_Ver1_T *find_cib_ptr()
{
    return ((Cupid_CIB_Ver1_T *)(((unsigned)( 0x17100000  )|0xA0000000)  + 0x220 ) );
}



struct fpga_header_s {
  uchar_t  x1;
  uchar_t  t1[3];
  uchar_t  x2;
  uchar_t  t2;
  uchar_t  lhi[2];
  uchar_t  x3;
  uchar_t  llo[2];
  uchar_t  z1;
  uchar_t  x4;
  uchar_t  z2[3];
};
typedef struct fpga_header_s fpga_header_t;

struct fpga_tlv_s {
    ulong_t length;
    ulong_t tag;
};
typedef struct fpga_tlv_s fpga_tlv_t;
		  


char fpga_rev_str[0x1000];
