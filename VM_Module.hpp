/****************************************************************
Class: VM_Module, VM_Module_Stack
VM_Module for handling the modules needed for the unpacker.

The modules can be added to the VM_Module_stack so that the unpacker
knows in which order the modules should be unpacked. This is done by checking
the expected geoaddress of every VM_Module in the stack with the 
geoaddress as it comes out of the buffer.  Therefore the order in which they are
added to the stack should match the order they are added in the daqconfig.tcl.
Implementations of VM_Module should override Unpack() and be added to the VM_Module_stack VMROOT::gModule_stack

CAEN_ADC and MESY_QDC are implementations each with their own
override of Unpack(). By Calling UnpackModules from the Module_stack, 
the pointer to PhysicsEventBuffer gets passed to each Module

 Author: Nabin Rijal, ddc
******************************************************************/
//--ddc nov15 add class for CHINP, and VMUSB marker  Any variables for 'chinp' added by me.
//Also, this required increasing the number of channels for modules.
//

#ifndef __VM_MODULE_H_
#define __VM_MODULE_H_
//C and C++ libraries.
#include <iostream>
#include <iomanip>
#include <math.h>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <memory>

//ROOT libraties
#include <TString.h>
#include <TH2.h>
#include <TFile.h>
#include <TMath.h>
#include <TRandom3.h>
#include <TChain.h>
#include <TObject.h>
#include <TLine.h>
#include <TTree.h>
#include <TBranch.h>
#include <TRandom3.h>
#include <TPolyLine3D.h>
#include <TVector3.h>
#include <TLorentzVector.h>
#include <TCutG.h>

#include "VM_BaseClass.hpp"

#define MODULE_MAX_CHANNELS 32
#define MODULE_MAX_CHANNELS_CHINP 1024

//use these types for checking how many of a module type are in the stack
//stack.GetNum(kCAENtype) or stack.GetNum(kMESYtype) for 
const static unsigned int kCAENtype(1);
const static unsigned int kMESYtype(2);
const static unsigned int kCHINPtype(3);

//VM_Module is an abstract baseclass, example modules which inherit from VM_Module and override the proper methods are CAEN_ADC and MESY_QDC
class VM_Module:public VM_BaseClass{
protected:

  UShort_t fNumOfCh;//!
  UInt_t fZeroSuppression;//!
  UInt_t fValid;//!
  UInt_t fModuleType;//!
  UShort_t fGeoAddress;//!
  UInt_t fCounter;//!

public:
  using TNamed::Print;
  TBranch * b_module;
  //--ddc nov15 needs to vary with moduletype!  Float_t fChValue[MODULE_MAX_CHANNELS];
  Float_t* fChValue;

  VM_Module(const TString& name = "",const UInt_t& geoaddress = 0);
  VM_Module( const UShort_t& modsize,const TString& name = "",const UInt_t& geoaddress = 0);
  ~VM_Module(){};

  Float_t operator () (UInt_t i) const {
    return ( i<fNumOfCh ? fChValue[i] : 0 ) ;  
  }
  
  Float_t operator [] (UInt_t i) const {
    return operator()(i) ;  
  }

  virtual Bool_t Unpack(unsigned short *& gpointer) = 0;
  UShort_t NumOfCh()const {return fNumOfCh;}
  UShort_t GeoAddress() const{return fGeoAddress;}
  UInt_t GetCounter()const {return fCounter;}
  UInt_t ModuleType()const {return fModuleType;}
  UInt_t ZeroSuppression()const {return fZeroSuppression;}
  UInt_t CheckValid()const {return fValid;}
  UInt_t SortChVal(const UInt_t& ch, const UInt_t& val);
  virtual void Reset();
  virtual UInt_t AddBranch(TTree* _tree);
  virtual UInt_t SetBranch(TTree* _tree);
  virtual void Print(); 

  ClassDef(VM_Module,1);
};

class VM_Module_Stack:public VM_BaseClass_Stack{
protected:

public: 
  using VM_BaseClass_Stack::GetNum;
  VM_Module_Stack(const TString& name="");
  ~VM_Module_Stack(){};
    
  UInt_t GetNum(UInt_t modtype = 0);
  virtual UInt_t AddBranches(TTree* _tree);
  virtual UInt_t SetBranches(TTree* _tree); 
  virtual UInt_t Add(VM_BaseClass * base);
  Bool_t UnpackModules(unsigned short *& pointer, int filepos);
  UInt_t SortGeoChVal(const UShort_t&geoaddress,const UInt_t& ch, const UInt_t& val);
 
  ClassDef(VM_Module_Stack,1);
};

class CAEN_ADC:public VM_Module{
private:
public:

  CAEN_ADC(){};
  CAEN_ADC(const TString& name,const UInt_t& geoaddress);

  virtual Bool_t Unpack(unsigned short*& gpointer);
  ClassDef(CAEN_ADC,1);
};

class CAEN_TDC:public CAEN_ADC{
private:
public:

  CAEN_TDC(){};
  CAEN_TDC(const TString& name,const UInt_t& geoaddress):CAEN_ADC(name,geoaddress){};

  ClassDef(CAEN_TDC,1);
};

class MESY_QDC:public VM_Module{
private:
public:
  MESY_QDC(){};
  MESY_QDC(const TString& name,const UInt_t& geoaddress);
  virtual Bool_t Unpack(unsigned short*& gpointer);

  ClassDef(MESY_QDC,1);
};

class MESY_ADC:public MESY_QDC{
private:
public:
  MESY_ADC(){};
  MESY_ADC(const TString& name,const UInt_t& geoaddress):MESY_QDC(name,geoaddress){};

  ClassDef(MESY_ADC,1);
};

class CHINP:public VM_Module{
private:
public:
  //--ddc dec15.  I will 'clear' the vectors in the unpacker.  just to
  //make sure nothing unfortunate happens.

  std::vector<UShort_t> hits; //list of channels which were actually read.
  std::vector<UShort_t> energies; //list of energies
  std::vector<UShort_t> times; //list of energies
  //--ddc dec15 AND I will override the addbranch and setbranch methods
  UInt_t AddBranch(TTree* _tree);
  UInt_t SetBranch(TTree* _tree);

  CHINP(){};
  CHINP(const UShort_t& modsize,const TString& name,const UInt_t& geoaddress);
  void Reset();

  virtual Bool_t Unpack(unsigned short*& gpointer);
  //--ddc NOTE chips (and chipboards) count starting with '1'.  (channels still count from 0).
  //Go with e,t,e,t arrangement.
  int PutE(UInt_t chipboard, UInt_t channel, UInt_t e) {
    //    UInt_t ind=32*(chipboard-1)+channel; 
    UInt_t ind=32*(chipboard-1)*2+ channel*2;
    int flag=ind;
    if(ind<fNumOfCh){
      SortChVal(ind,e);
    } else {
      flag=-1;
    }
    return flag;
  }
  int PutT(UInt_t chipboard, UInt_t channel, UInt_t t) { 
    //    UInt_t ind=fNumOfCh/2+32*(chipboard-1)+channel;
    UInt_t ind=32*(chipboard-1)*2+channel*2+1;
    int flag=ind;
    if(ind<fNumOfCh){
      SortChVal(ind,t);
    } else {
      flag = -1;
    }
    return flag;
  }

//virtual UInt_t chipTime(UInt_t time,int chip,TString modName){//unused parameters?
  virtual UInt_t chipTime(UInt_t time) {
    time = 16384 - time; 
    return time; 
  }

  virtual UInt_t chipEnergy(UInt_t energy,int chip,TString modName){

    int val;

    if( (val=modName.CompareTo("hinp1"))==0){

      switch (chip) {
	
      case 1:
      case 2:
      case 5:
      case 6:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
	energy=16384 - energy;
	
	break;
      default:
	energy=energy;
	break;
      }
    }

    if( (val=modName.CompareTo("hinp2"))==0){
      switch (chip) {
      case 1:
      case 2:
      case 5:
      case 6:
      case 9:
      case 10:
	energy=16384 - energy;
	break;
      default:
	energy=energy;
	break;
      }
    }

    return energy;
  }

  ClassDef(CHINP,1);
};

class VMUSBMARK:public VM_Module{
private:
public:

  VMUSBMARK(){};
  VMUSBMARK(const TString& name,const UInt_t& geoaddress):
    VM_Module( 1 ,name, geoaddress){
  }

  virtual Bool_t Unpack(unsigned short*& gpointer){
    Bool_t retval=true;
    UInt_t markvalue = *(gpointer++);
    fChValue[0]=markvalue;
    if(markvalue != fGeoAddress) retval=false;
    return retval;
  }
  //--ddc NOTE The VMUSB markers are only one word.  The 'value' can be anything,
  //but will interpret/keep it as the slot number, and as we read it, we can test it

  ClassDef(VMUSBMARK,1);
};

#endif
