/***********************************************************************
Class: VM_BaseClass, VM_BaseClass Stack
Author: Nabin Rijal, 2015
***********************************************************************/
#ifndef __VMBaseClass_CXX
#define __VMBaseClass_CXX

#include "VM_BaseClass.hpp"

ClassImp(VM_BaseClass);
ClassImp(VM_BaseClass_Stack);

VM_BaseClass::VM_BaseClass(const TString&name, const TString&title,const unsigned int& id):TNamed(name,title) {
  SetUniqueID(id);
}

VM_BaseClass_Stack::VM_BaseClass_Stack(const TString& name):VM_BaseClass(name,name) {
}

void VM_BaseClass_Stack::Init() {
  fVMStack=new TList();
}

void VM_BaseClass_Stack::Reset() {
  //loop over all modules stored in TList of Modules and reset all of them
  TIter next(fVMStack);
  while( VM_BaseClass*obj = (VM_BaseClass*)next()){
    obj->Reset();
  }
  return;
}

UInt_t VM_BaseClass_Stack::Add(VM_BaseClass *base) {
  if(!fVMStack){
    Init();
  }
  fVMStack->Add(base);
  return 1;
}

void VM_BaseClass_Stack::Print() {
  TIter next(fVMStack);
  while( VM_BaseClass*obj = (VM_BaseClass*)next()) {
    obj->Print();
  }
  return ;
}
#endif
