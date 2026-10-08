#ifndef KERNEL_IO_H
#define KERNEL_IO_H
#include <stdint.h>
#include <stdbool.h>
typedef int ER, ID, INT, TMO;
typedef uint32_t RELTIM;
typedef unsigned char UB;
typedef unsigned UW;
typedef void (*FP)(INT,void *);
typedef struct { unsigned mtxatr; } T_CMTX;
typedef struct { unsigned iflgptn; } T_CFLG;
typedef struct { unsigned flgptn; } T_RFLG;
typedef struct { uint32_t lo; } SYSTIM;
#define E_OK 0
#define E_OBJ (-41)
#define TMO_POL 0
#define TA_INHERIT 2
#define TP_PRC2 2
#define TSK_SELF 0
ER tk_set_flg(ID id,unsigned bits);
ER tk_clr_flg(ID id,unsigned bits);
ER tk_ref_flg(ID id,T_RFLG *f);
ER tk_wup_tsk(ID id);
ER tk_can_wup(ID id);
ER tk_loc_mtx(ID id,TMO timeout);
ER tk_unl_mtx(ID id);
ER tk_get_otm(SYSTIM *t);
ER tk_slp_tsk(TMO timeout);
ID tk_cre_mtx(const T_CMTX *m);
ID tk_cre_flg(const T_CFLG *f);
ID app_start_task(FP task,int priority,UW core,const char *name);
ER tk_dly_tsk(RELTIM timeout);
bool gpio_get_val(unsigned pin);
void tm_printf(const UB *format, ...);
#endif
