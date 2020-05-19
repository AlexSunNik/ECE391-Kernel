#define ASM     1
#include "exception_linkage.h"

/* wrapper function for exception handler */
/* addl $4, %esp to skip %id pushed before */
#define EXP_WRP(type, id)            \
    .globl type                     ;\
    type:                           ;\
        cli                         ;\
        pushl   $id                 ;\
        call    exception_handler   ;\
        addl    $4, %esp            ;\
        iret

/* wrap all the handlers */
/* EXP_WRP(EXCEPTION_NAME, EXCEPTION_ID); */
EXP_WRP(Divide_by_zero_Error,            0x00);
EXP_WRP(Debug,                           0x01);
EXP_WRP(Non_maskable_interrupt,          0x02);
EXP_WRP(Breakpoint,                      0x03);
EXP_WRP(Overflow,                        0x04);
EXP_WRP(Bound_Range_Exceeded,            0x05);
EXP_WRP(Invalid_Opcode,                  0x06);
EXP_WRP(Device_Not_Available,            0x07);
EXP_WRP(Double_Fault,                    0x08);
EXP_WRP(Coprocessor_Segment_Overrun,     0x09);
EXP_WRP(Invalid_TSS,                     0x0A);
EXP_WRP(Segment_Not_Present,             0x0B);
EXP_WRP(Stack_Segment_Fault,             0x0C);
EXP_WRP(General_Protection_Fault,        0x0D);
EXP_WRP(Page_Fault,                      0x0E);
EXP_WRP(Reserved1,                       0x0F);
EXP_WRP(x87_Floating_Point_Exception,    0x10);
EXP_WRP(Alignment_Check,                 0x11);
EXP_WRP(Machine_Check,                   0x12); 
EXP_WRP(SIMD_Floating_Point_Exception,   0x13); 
EXP_WRP(Virtualization_Exception,        0x14); 
EXP_WRP(Reserved2,                       0x15);
EXP_WRP(Reserved3,                       0x16);
EXP_WRP(Reserved4,                       0x17);
EXP_WRP(Reserved5,                       0x18);
EXP_WRP(Reserved6,                       0x19);
EXP_WRP(Reserved7,                       0x1A);
EXP_WRP(Reserved8,                       0x1B);
EXP_WRP(Reserved9,                       0x1C);
EXP_WRP(ReservedA,                       0x1D);
EXP_WRP(Security_Exception,              0x1E); 
EXP_WRP(ReservedB,                       0x1F);
