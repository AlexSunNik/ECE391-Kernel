#ifndef _EXP_LINK_H
#define _EXP_LINK_H

#ifndef ASM

/* define the wrappers as functions */
extern void Divide_by_zero_Error            ();
extern void Debug                           ();
extern void Non_maskable_interrupt          ();
extern void Breakpoint                      ();
extern void Overflow                        ();
extern void Bound_Range_Exceeded            ();
extern void Invalid_Opcode                  ();
extern void Device_Not_Available            ();
extern void Double_Fault                    ();
extern void Coprocessor_Segment_Overrun     ();
extern void Invalid_TSS                     ();
extern void Segment_Not_Present             ();
extern void Stack_Segment_Fault             ();
extern void General_Protection_Fault        ();
extern void Page_Fault                      ();
extern void Reserved1                       ();
extern void x87_Floating_Point_Exception    ();
extern void Alignment_Check                 ();
extern void Machine_Check                   (); 
extern void SIMD_Floating_Point_Exception   (); 
extern void Virtualization_Exception        (); 
extern void Reserved2                       ();
extern void Reserved3                       ();
extern void Reserved4                       ();
extern void Reserved5                       ();
extern void Reserved6                       ();
extern void Reserved7                       ();
extern void Reserved8                       ();
extern void Reserved9                       ();
extern void ReservedA                       ();
extern void Security_Exception              (); 
extern void ReservedB                       ();

#endif /* ASM */
#endif /* _EXP_LINK_H */
