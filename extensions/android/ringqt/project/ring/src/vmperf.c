/* Copyright (c) 2013-2023 Mahmoud Fayed <msfclipper@yahoo.com> */
#include "ring.h"
/* For Better Performance */

void ring_vm_pushp ( VM *pVM )
{
    RING_VM_STACK_PUSHP ;
    RING_VM_STACK_OBJTYPE = RING_OBJTYPE_VARIABLE ;
    /* Update Scope Information */
    if ( pVM->nLoadAddressScope  == RING_VARSCOPE_NOTHING ) {
        pVM->nLoadAddressScope = RING_VARSCOPE_GLOBAL ;
    }
    pVM->nVarScope = RING_VARSCOPE_GLOBAL ;
    /* Check lNewRef Flag */
    ring_list_resetlnewref((List *) RING_VM_STACK_READP);
}

void ring_vm_pushplocal ( VM *pVM )
{
    /* Check Scope Life Time */
    if ( RING_VM_IR_READIVALUE(5) != pVM->nActiveScopeID ) {
        ring_vm_loadaddress(pVM);
        return ;
    }
    RING_VM_STACK_PUSHPVALUE(RING_VM_IR_READPVALUE(4)) ;
    RING_VM_STACK_OBJTYPE = RING_OBJTYPE_VARIABLE ;
    /* Update Scope Information */
    if ( pVM->nLoadAddressScope  == RING_VARSCOPE_NOTHING ) {
        pVM->nLoadAddressScope = RING_VARSCOPE_LOCAL ;
    }
    pVM->nVarScope = RING_VARSCOPE_LOCAL ;
    /* Check lNewRef Flag */
    ring_list_resetlnewref((List *) RING_VM_STACK_READP);
}

void ring_vm_incp ( VM *pVM )
{
    List *pVar  ;
    pVar = (List *) RING_VM_IR_READP ;
    ring_list_setdouble_gc(pVM->pRingState,pVar,RING_VAR_VALUE,ring_list_getdouble(pVar,RING_VAR_VALUE) + 1);
}

void ring_vm_pushpv ( VM *pVM )
{
    ring_vm_pushp(pVM);
    ring_vm_varpushv(pVM);
}

void ring_vm_incjump ( VM *pVM )
{
    List *pVar  ;
    Item *pItem  ;
    double nNum1,nNum2  ;
    if ( ring_vm_findvar(pVM, RING_VM_IR_READC ) == 0 ) {
        ring_vm_newvar(pVM, RING_VM_IR_READC);
    }
    pVar = (List *) RING_VM_STACK_READP ;
    RING_VM_STACK_POP ;
    pItem = ring_list_getitem(pVar,RING_VAR_VALUE) ;
    nNum1 = ring_list_getdouble(pVM->aForStep,ring_list_getsize(pVM->aForStep));
    /* Check Data */
    if ( ! ring_list_isnumber(pVar,RING_VAR_VALUE) ) {
        ring_vm_error(pVM,RING_VM_ERROR_FORLOOPDATATYPE);
        return ;
    }
    nNum2 = ring_list_getdouble(pVar,RING_VAR_VALUE) ;
    ring_list_setdouble_gc(pVM->pRingState,pVar,RING_VAR_VALUE,nNum2 +nNum1);
    /* Change Instruction for Performance */
    if ( pVM->nVarScope == RING_VARSCOPE_GLOBAL ) {
        /* Replace ICO_INCJUMP with IncPJUMP for better performance */
        if ( nNum1 == 1.0 ) {
            RING_VM_IR_OPCODE = ICO_INCPJUMPSTEP1 ;
        }
        else {
            RING_VM_IR_OPCODE = ICO_INCPJUMP ;
        }
        RING_VM_IR_ITEMSETPOINTER(RING_VM_IR_ITEM(4),pItem);
    }
    else if ( pVM->nVarScope == RING_VARSCOPE_LOCAL ) {
        /* Replace ICO_INCJUMP with IncLPJUMP for better performance */
        if ( nNum1 == 1.0 ) {
            RING_VM_IR_OPCODE = ICO_INCLPJUMPSTEP1 ;
        }
        else {
            RING_VM_IR_OPCODE = ICO_INCLPJUMP ;
        }
        RING_VM_IR_ITEMSETPOINTER(RING_VM_IR_ITEM(4),pItem);
        RING_VM_IR_ITEMSETINT(RING_VM_IR_ITEM(5),ring_list_getint(pVM->aScopeID,ring_list_getsize(pVM->aScopeID)));
    }
    /* Jump */
    pVM->nPC = RING_VM_IR_READIVALUE(2) ;
    RING_VM_STACK_PUSHNVALUE(ring_list_getdouble(pVar,RING_VAR_VALUE));
    RING_VM_IR_SETLINENUMBER(RING_VM_IR_READIVALUE(3));
}

void ring_vm_incpjump ( VM *pVM )
{
    Item *pItem  ;
    double nNum1,nNum2  ;
    pItem = (Item *) RING_VM_IR_READPVALUE(4) ;
    nNum1 = ring_list_getdouble(pVM->aForStep,ring_list_getsize(pVM->aForStep));
    /* Check Data */
    if ( ! ring_item_isdouble(pItem) ) {
        ring_vm_error(pVM,RING_VM_ERROR_FORLOOPDATATYPE);
        return ;
    }
    nNum2 = ring_item_getdouble(pItem) ;
    ring_item_setdouble_gc(pVM->pRingState,pItem,nNum2 + nNum1);
    /* Jump */
    pVM->nPC = RING_VM_IR_READIVALUE(2) ;
    RING_VM_STACK_PUSHNVALUE(ring_item_getdouble(pItem));
    RING_VM_IR_SETLINENUMBER(RING_VM_IR_READIVALUE(3));
}

void ring_vm_inclpjump ( VM *pVM )
{
    /* Check Scope Life Time */
    if ( RING_VM_IR_READIVALUE(5) != pVM->nActiveScopeID ) {
        ring_vm_incjump(pVM);
        return ;
    }
    ring_vm_incpjump(pVM);
}

void ring_vm_loadfuncp ( VM *pVM )
{
    List *pList  ;
    FuncCall *pFuncCall  ;
    pVM->nFuncExecute++ ;
    pVM->nFuncExecute2++ ;
    pList = ring_list_newlist_gc(pVM->pRingState,pVM->pFuncCallList);
    /* Add FuncCall Structure */
    pFuncCall = ring_vmfunccall_new(pVM,pList);
    pFuncCall->nType = RING_VM_IR_GETFLAGREG ;
    pFuncCall->cName = RING_VM_IR_READC ;
    pFuncCall->nPC = RING_VM_IR_READIVALUE(2) ;
    pFuncCall->nSP = pVM->nSP ;
    pFuncCall->pFunc = (void(*)(void *)) RING_VM_IR_READPVALUE(3) ;
    pFuncCall->pTempMem = ring_list_new_gc(pVM->pRingState,0) ;
    pFuncCall->cFileName = pVM->cFileName ;
    if ( pFuncCall->nType == RING_FUNCTYPE_SCRIPT ) {
        pVM->cFileName = (char *) RING_VM_IR_READPVALUE(4) ;
        pVM->cPrevFileName = pVM->cFileName ;
    }
    pFuncCall->cNewFileName = pVM->cFileName ;
    pFuncCall->nMethodOrFunc = RING_VM_IR_GETCHARREG ;
    pFuncCall->nLineNumber = RING_VM_IR_READIVALUE(5) ;
    pFuncCall->nLoadAddressScope = pVM->nLoadAddressScope ;
    pVM->nLoadAddressScope = RING_VARSCOPE_NOTHING ;
    /* Store List information */
    pFuncCall->nListStart = pVM->nListStart ;
    pFuncCall->pNestedLists = pVM->pNestedLists ;
    pVM->nListStart = 0 ;
    pVM->pNestedLists = ring_list_new_gc(pVM->pRingState,0);
}
/* For Loop Optimization When Step = 1 */

void ring_vm_incpjumpstep1 ( VM *pVM )
{
    Item *pItem  ;
    pItem = (Item *) RING_VM_IR_READPVALUE(4) ;
    /* Jump */
    pVM->nPC = RING_VM_IR_READIVALUE(2) ;
    /* Check Data */
    if ( ! ring_item_isdouble(pItem) ) {
        ring_vm_error(pVM,RING_VM_ERROR_FORLOOPDATATYPE);
        return ;
    }
    RING_VM_STACK_PUSHNVALUE(ring_item_incdouble(pItem));
    RING_VM_IR_SETLINENUMBER(RING_VM_IR_READIVALUE(3));
}

void ring_vm_inclpjumpstep1 ( VM *pVM )
{
    /* Check Scope Life Time */
    if ( RING_VM_IR_READIVALUE(5) != pVM->nActiveScopeID ) {
        ring_vm_incjump(pVM);
        return ;
    }
    ring_vm_incpjumpstep1(pVM);
}

void ring_vm_setopcode ( VM *pVM )
{
    int nIns,nOPCode  ;
    nIns = RING_VM_IR_READIVALUE(2) - 1 ;
    nOPCode = RING_VM_IR_READI ;
    RING_VM_IR_OPCODEVALUE(nIns) = nOPCode ;
}
