# 1 "/root/root4/orion/os/regaeos/sys/libsys/serial.8530.c"
 
 
 
 
 
 
# 1 "/root/root4/orion/os/regaeos/sys/include/basics.h" 1
 
 
 
 
 
 
 
 
 





typedef char		S8;
typedef unsigned char	U8;
typedef short		S16;
typedef unsigned short	U16;
typedef int		S32;
typedef unsigned int	U32;
typedef int		BOOL;

 








 
typedef U32 (*U32FUNC)();
typedef S32 (*S32FUNC)();
typedef void (*VFUNC)();




extern void  SerialSetup(U32 baud, U32 console, U32 host, U32 intr_desc);
extern void  SerialPollOn(void);
extern void  SerialPollOff(void);
extern void  SerialPollInit(void);
extern U32 SerialPollConsts(U32 typecode);
extern U8 SerialPollConin(void);
extern void  SerialPollConout(U8 c);
extern U32 SerialPollHststs(U32 typecode);
extern U8 SerialPollHstin(void);
void  SerialPollHstout(U8 c);
void *SerialIntInit(U32 channel,
		    void (*rx_isr)(U32 channel),
		    void (*tx_char)(U32 unit, U32 isr_flag));
U32 SerialIntBaud(U32 channel, void *hdp, U32 baudrate);
void  SerialIntRxioff(U32 channel, void *hdp);
void  SerialIntRxion(U32 channel, void *hdp);
void  SerialIntTxioff(U32 channel, void *hdp);
void  SerialIntTxion(U32 channel, void *hdp);
U32 SerialIntRead(U32 channel, void *hdp, U8 *ch_ptr);
U32 SerialIntWrite(U32 channel, void *hdp, U8 ch);




 
 
 




 
 

typedef U32 instr_t;




 
 
 
typedef volatile struct
{
  U8 control;
  U8 pad1[3];
  U8 data;
  U8 pad2[3];
} Z8530_SCC;

 
 
 



static Z8530_SCC * const scc_port[2  + 1] = { 0, ((Z8530_SCC *) ((unsigned)( 0x1F200108  )|0xA0000000)  ) , ((Z8530_SCC *) ((unsigned)( 0x1F200108  )|0xA0000000)  )  };
static U8 pin;

 
 
 
 
 
 
 
 

typedef volatile struct
{
  U16 time_const;       
  U8 int_mode;          
  U8 rxint_mask;        
  U8 txint_mask;        
  U8 wr3_val;
  U8 wr5_val;
} SCC_DATA;

 
 
 
static SCC_DATA scc_data[2  + 1];
static U32 pROBE_console, pROBE_host;   

 
 
 
 
static U8 Break_Recorded[2  + 1];

 
 
 
 
 
static void (*Rx_Isr[2  + 1])(U32 channel);
static void (*Tx_Isr[2  + 1])(U32 channel, U32 isr_flag);

 
 
 








 
 
 
static U8 rd_ctrl(U32 channel, U8 regnum);
static void  wr_ctrl(U32 channel, U8 regnum, U8 value);
static void  isr_uart(void);
static void  dummy_isr(void);

 
 
 
 
volatile U32 serial_unused;


 
 
 

 
 
 
 
 
 
 
 
static void wr_ctrl(U32 channel, U8 regnum, U8 value)
{
  U32 imask;
  char	tmp;

  if (regnum == 0)     
    {
	 
      tmp = *(char *)(((unsigned)( 0x1FC00000  )|0xA0000000)  );
      scc_port[channel]->control = value;
	 
      tmp = *(char *)(((unsigned)( 0x1FC00000  )|0xA0000000)  );
    }
  else
    {

      imask = splx(7);
	 
      tmp = *(char *)(((unsigned)( 0x1FC00000  )|0xA0000000)  );
      scc_port[channel]->control = regnum;
	 
      tmp = *(char *)(((unsigned)( 0x1FC00000  )|0xA0000000)  );
      scc_port[channel]->control = value;
      splx(imask);
    }
}

 
 
 
 
 
 
 
static U8 rd_ctrl(U32 channel, U8 regnum)
{
  U32 imask;
  U8 value;
  char	tmp;

  if (regnum == 0)     
    {
	 
      tmp = *(char *)(((unsigned)( 0x1FC00000  )|0xA0000000)  );
      value = scc_port[channel]->control;
	 
      tmp = *(char *)(((unsigned)( 0x1FC00000  )|0xA0000000)  );
    }
  else
    {
      imask = splx(7);
	 
      tmp = *(char *)(((unsigned)( 0x1FC00000  )|0xA0000000)  );
      scc_port[channel]->control = regnum;
	 
      tmp = *(char *)(((unsigned)( 0x1FC00000  )|0xA0000000)  );
      value = scc_port[channel]->control;
      splx(imask);
    }

  return value;
}

 
 
 
 
 
 
static void Channel_Enable(U32 channel)
{
 
 
 
  wr_ctrl(channel, 12 , scc_data[channel].time_const & 0xFF);
  wr_ctrl(channel, 13 , scc_data[channel].time_const >> 8);

 
 
 
  wr_ctrl(channel, 14 ,   0x01 );

 
 
 
  wr_ctrl(channel, 3 , scc_data[channel].wr3_val | 1);

 
 
 
  wr_ctrl(channel, 5 , scc_data[channel].wr5_val | 8);

 
 
 
	 
  wr_ctrl(channel, 0 , 0x10 );
  wr_ctrl(channel, 9 , 0x0A );
}

 
 
 
 
 
 
static void Channel_Disable(U32 channel)
{
 
 
 
  wr_ctrl(channel, 14 ,   0x00 );

 
 
 
  wr_ctrl(channel, 3 , scc_data[channel].wr3_val & ~1);

 
 
 
  wr_ctrl(channel, 5 , scc_data[channel].wr5_val & ~8);
}

 
 
 
 
 
 
 
 
 
static U16 Baud_to_TC(U32 baud_rate)
{
  int i;

 
 
 
  const static U32 baudtable[9][2] =
  {  
    {300,    (3686400 /(32*300)-2)},
    {600,    (3686400 /(32*600)-2)},
    {1200,   (3686400 /(32*1200)-2)},
    {2400,   (3686400 /(32*2400)-2)},
    {4800,   (3686400 /(32*4800)-2)},
    {9600,   (3686400 /(32*9600)-2)},
    {19200,  (3686400 /(32*19200)-2)},
    {38400,  (3686400 /(32*38400)-2)},
    {0,      (3686400 /(32*9600)-2)}
  };  

 
 
 
  for (i = 0; baudtable[i][0] && (baudtable[i][0] != baud_rate); i++)
    ;

  return baudtable[i][1];
}

 
 
 
static void dummy_rx_isr(U32 channel) { serial_unused = channel; }


 
 
 
 
static void isr_uart(void)
{
  U8 ipend;
  U32 channel_A, channel_B;
  U32 chipNo;

 
 
 
 
  for (chipNo = 1; chipNo <= (2 /2); chipNo++)
    {
      channel_A = ((chipNo - 1) << 1) + 1;
      channel_B = ((chipNo - 1) << 1) + 2;
     
     
     
      if ((ipend = rd_ctrl(channel_A, 3 )) != 0)
	{
        
        
        
	  if (ipend & 0x08 )
	    {
	   
	   
	   
	      wr_ctrl(channel_A, 15 , 0x00 );
	      Break_Recorded[channel_A] = 1;
	      wr_ctrl(channel_A, 0 , 0x10 );
	    }
	  else if (ipend & scc_data[channel_A].rxint_mask)
	    {
	      (*Rx_Isr[channel_A])(channel_A);
	    }
    
	  if (ipend & scc_data[channel_A].txint_mask)
	    (*Tx_Isr[channel_A])(channel_A, 1);
    
        
        
        
	  if (ipend & 0x01 )
	    {
	      wr_ctrl(channel_B, 15 , 0x00 );
	      Break_Recorded[channel_B] = 1;
	      wr_ctrl(channel_B, 0 , 0x10 );
	    }
	  else if (ipend & scc_data[channel_B].rxint_mask)
	    {
	      (*Rx_Isr[channel_B])(channel_B);
	    }
    
	  if (ipend & scc_data[channel_B].txint_mask)
	    (*Tx_Isr[channel_B])(channel_B, 1);
    
        
        
        
        
	  wr_ctrl(channel_A, 0 , 0x38 );
	}
    }
}

 
 
 

 
 
 
 
 
 
unsigned short save_time_const;
 
void SerialSetup(U32 baud_rate, U32 cnsl_chnl, U32 host_chnl, U32 intr_desc)
{
  int channel;

  pin = 0;
 
 
 
  pROBE_console = cnsl_chnl;
  pROBE_host = host_chnl;

 
 
 
  for (channel = 1; channel <= 2 ; channel++)
    {
      scc_data[channel].int_mode   = 0;
      if ((channel == 1) || (channel == 3))
	{
	  scc_data[channel].rxint_mask = 0x20;
	  scc_data[channel].txint_mask = 0x10;
	}
      else
	{
	  scc_data[channel].rxint_mask = 0x04;
	  scc_data[channel].txint_mask = 0x02;
	}
      scc_data[channel].time_const = Baud_to_TC(baud_rate);
      save_time_const =       scc_data[channel].time_const;
     
     
     
     

     


      if ((channel == 1) || (channel == 3))
	wr_ctrl(channel, 9 , 0x80);
      else
	wr_ctrl(channel, 9 , 0x40);

     


      wr_ctrl(channel, 4 , 0x44);

      wr_ctrl(channel, 1 , 0);



     


      wr_ctrl(channel, 3 , scc_data[channel].wr3_val = 0xC0);

     


      wr_ctrl(channel, 5 , scc_data[channel].wr5_val = 0xE2);

      wr_ctrl(channel, 6 , 0);

      wr_ctrl(channel, 7 , 0);

      wr_ctrl(channel, 9 , 0);

      wr_ctrl(channel, 10 , 0);

      wr_ctrl(channel, 11 , 0x52);



      if (channel == pROBE_console)
	{
	  Rx_Isr[channel] = dummy_rx_isr;
	  wr_ctrl(channel, 15 , 0x80 );
	  scc_data[channel].int_mode   = 0x01 ;
	  wr_ctrl(channel, 1 , 0x01 );
	}
      else
	wr_ctrl(channel, 15 , 0x00 );
    }

 
 
 
  Break_Recorded[1] = Break_Recorded[2] = 0;
  Break_Recorded[3] = Break_Recorded[4] = 0;

}

 
 
 
 
 
 
 
void SerialPollOn(void)
{
 
 
 
  wr_ctrl(pROBE_console, 9 , 0);
}

 
 
 
 
 
 
 
 
void SerialPollOff(void)
{
  wr_ctrl(pROBE_console, 9 , 0x0A );
}

 
 
 
 
void SerialPollInit(void)
{
  Channel_Enable(pROBE_console);

  if (pROBE_host)
    {
      Channel_Enable(pROBE_host);
    }
}

 
 
 
 
 
 
 
 
 
 
U32 SerialPollChanSts(long channel)
{
  U8 reg_val;

 
 
 
  if (Break_Recorded[channel])
    {
     
     
     
     
     
      do  {
	wr_ctrl(channel, 0 , 0x30 );
	wr_ctrl(channel, 0 , 0x10 );
      }
      while (rd_ctrl(channel, 0 ) & 0x80 );

      Break_Recorded[channel] = 0;
      wr_ctrl(channel, 15 , 0x80 );
      return 2;    
    }

  for (;;)
    {
      reg_val = rd_ctrl(channel, 0 );

      if (reg_val & 0x80 )
	{
	 
	 
	 
	 
	  do  {
	    wr_ctrl(channel, 0 , 0x30 );
	    wr_ctrl(channel, 0 , 0x10 );
	  }
	  while (rd_ctrl(channel, 0 ) & 0x80 );

	  reg_val = scc_port[channel]->data;   
	  return 2;     
	}
      else if (reg_val & 0x01 )
	{
	 
	 
	 
	 
	 
	 
	  reg_val = rd_ctrl(channel, 1 );

	  if ((reg_val & 0x70 ) == 0)
	    {
	      rd_ctrl(channel, 0 );
	      return 1;
	    }
	  else
	    {
	      SerialPollConin();
	      wr_ctrl(channel, 0 , 0x30 );
	    }
	}
      else
	 
	 
	 
	return 0;
    }
}

 
 
 
 
 
 
 
 
 
 
U32 SerialPollConsts(U32 typecode)
{
  serial_unused = typecode; 
  return (SerialPollChanSts(pROBE_console));
}
 
 
 
 
 
 
 
 
 
U8 SerialPollChanIn(long channel)
{
  return scc_port[channel]->data;
}

 
 
 
 
 
 
 
 
U8 SerialPollConin(void)
{
  return (SerialPollChanIn(pROBE_console));
}

 
 
 
 
 
 
void SerialPollChanOut(U32 channel, U8 c)
{
  while ((rd_ctrl(channel, 0 ) & 0x04 ) == 0)
    ;

  scc_port[channel]->data = c;                         
}

 
 
 
 
 
 
extern char* mem2hex( char* mem,
		      char* buf,
		      int   count);
extern void putpacket(char * buffer);


void SerialPollConout(U8 c)
{
  SerialPollChanOut(pROBE_console, c);
}

 
 
 
 
 
 
 
 
 
U32 SerialPollHststs(U32 typecode)
{
  U8 reg_val;
  U32 ret_code;

  serial_unused = typecode; 

 
 
 
 
  ret_code = 3;

  while (ret_code == 3)
    {
      reg_val = rd_ctrl(pROBE_host, 0 );

      if (reg_val & 0x01 )
	{
	 
	 
	 
	 
	 
	 
	  reg_val = rd_ctrl(pROBE_host, 1 );

	  if ((reg_val & 0x20 ) == 0)
	    {
	      reg_val = rd_ctrl(pROBE_host, 0 );
	      ret_code = 1;
	    }
	  else
	    SerialPollHstin();
	}
      else
	 
	 
	 
	ret_code = 0;
    }

  return ret_code;
}

 
 
 
 
 
 
 
 
U8 SerialPollHstin(void)
{
  return scc_port[pROBE_host]->data;
}

 
 
 
 
 
 
void SerialPollHstout(U8 c)
{
  while ((rd_ctrl(pROBE_host, 0 ) & 0x04 ) == 0)
    ;

  scc_port[pROBE_host]->data = c;       
}

# 1015 "/root/root4/orion/os/regaeos/sys/libsys/serial.8530.c"


void SerialBreak(int channel)
{
  Break_Recorded[channel] = 1;
}





