/************************************************************************
VM_BaseClass
All classes created specific for this package will inherit from VM_BaseClass.
This means that all the classes will inherit from TNamed and TObject.
This allows us to use the ROOT I/O methods and also use the ROOT containers
which handle TObject addresses.

For example:
The VM_BaseClass_Stack consists of a TList which is a doubly linked list of TObjects.  Since VM_BaseClass inherits from TObject we can then add BaseClass object(i.e. any class in the ResoNeut package) to this TList. This stack then contains methods which mirror those of the class it contains (BaseClasses) that way, when you execute the particular method, it will execute all of the analagous methods in the stack.

Author: Nabin Rijal, 2015

***************************************************************************/

#ifndef __VMBASECLASS_H
#define __VMBASECLASS_H

//C and C++ libraries.
#include <iostream>
#include <vector>

//ROOT libraties
#include <TString.h>
#include <TObject.h>
#include <TNamed.h>
#include <TList.h>
#include <TTree.h>

class VM_BaseClass:public TNamed{
protected:
public:
  using TObject::Execute;
  using TNamed::Print;
  VM_BaseClass(){}
  VM_BaseClass(const TString&name, const TString&title="",const unsigned int& id=0);
  virtual void Bind(){};
  virtual void Execute(){};
  virtual void Print(){};
  virtual void Reset(){};

  ClassDef(VM_BaseClass,1);  
};

class VM_BaseClass_Stack:public VM_BaseClass{
protected:
  TList *fVMStack;//!  
public: 
  using TNamed::Print;
  VM_BaseClass_Stack(const TString& name="");
  ~VM_BaseClass_Stack(){
    if(fVMStack){
      ClearStack();
      delete fVMStack;
      fVMStack=NULL;
    }
  }
  virtual void Init();
  virtual UInt_t GetSize()const{return fVMStack ? fVMStack->GetSize() : 0;}
  virtual UInt_t GetNum() const{return GetSize();}
  //virtual UInt_t AddBranches(TTree* _tree){return 0;}//
  //virtual UInt_t SetBranches(TTree* _tree){return 0;}//
  virtual UInt_t Add(VM_BaseClass * base);
  virtual void ClearStack(){if(fVMStack)fVMStack->Clear();} 
  virtual void Print();
  virtual void Reset();
  
  ClassDef(VM_BaseClass_Stack,1);
};

#endif
///////////////////////////////////////////////////////////////////////////////////
