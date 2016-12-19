/***************************************************************
Class: VMModule, VMModule_Stack
Author: Nabin Rijal, ddc
****************************************************************/
//--ddc nov15 add methods for CHINP unpack... and VMUSB marker words
//--ddc dec15 and some more functions that need to be overriden for vectors 
//and zero suppression in CHINP
///////////////////////////////////////////////////////////////////////
#ifndef _VM_MODULE_CXX_
#define _VM_MODULE_CXX_
#include "VM_Module.hpp"
#include "VM_BaseClass.hpp"

ClassImp(VM_Module);
ClassImp(VM_Module_Stack);
/////////////////////////////////////////////////////////////////////////////////////////////////
VM_Module::VM_Module(const TString& name,const UInt_t& geoaddress):VM_BaseClass(name,name),
								   fNumOfCh(MODULE_MAX_CHANNELS),
								   fZeroSuppression(1),
								   fValid(0),
								   fModuleType(0),
								   fGeoAddress(geoaddress)
{
  fChValue = new Float_t[fNumOfCh];  
}
  /////////////////////////////////////////////////////////////////////////////////////////////////
//--ddc nov15 add to constructor that lets us choose 'modsize' for CHINP unpack.
VM_Module::VM_Module(const UShort_t& modsize,const TString& name,const UInt_t& geoaddress):VM_BaseClass(name,name),
											   fNumOfCh(modsize),
											   fZeroSuppression(1),
											   fValid(0),
											   fModuleType(0),
											   fGeoAddress(geoaddress)
{
  fChValue = new Float_t[fNumOfCh];  
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void VM_Module::Reset(){
  //Reset all values stored in module channels
  fValid = 0;
  for(unsigned int i=0;i<NumOfCh();i++){
    fChValue[i] = 0;
  }  
  return;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
UInt_t VM_Module::SortChVal(const UInt_t& ch,const UInt_t & val){
  fChValue[ch]=(Float_t)val+gRandom->Rndm();
  return 1;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void VM_Module::Print(){
  std::cout<<GetName()<<": "<<"GeoAddress: "<<GeoAddress()<<"\n";
  return ;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
UInt_t VM_Module::AddBranch(TTree* _tree){
  b_module = _tree->Branch(GetName(),fChValue,Form("%s[%d]/F",GetName(),NumOfCh()));
  return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
UInt_t VM_Module::SetBranch(TTree* _tree){
  if(_tree->GetBranch(GetName())){   
    _tree->SetBranchAddress(GetName(),&fChValue, &b_module);  
  }
  else{
    std::cout<<"no "<<GetName()<<" present in Tree, Are you sure this rootfile is compatible with this stack?"<<std::endl;
    return 0;
  }

  return 1;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
VM_Module_Stack::VM_Module_Stack(const TString& name):VM_BaseClass_Stack(name)					     
{

  //VM_Module_Stack(const TString& name="");
  //~VM_Module_Stack(){};
}
/////////////////////////////////////////////////////////////////////////////////////////////////
Bool_t VM_Module_Stack::UnpackModules(unsigned short *& gpointer,int filepos){
  TIter next(fVMStack);
  while( VM_Module*obj = (VM_Module*)next()){
    if(!obj->Unpack(gpointer)){
      std::cout<<"error at file pos: "<< filepos<<std::endl;
      break; //--ddc 15dec ... No point unpacking rest with a serious error.
    }
  }  
  
  return 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
UInt_t VM_Module_Stack::GetNum(UInt_t modtype){
  UInt_t count=0;
  if(!fVMStack)
    return 0;
  if(modtype == 0)
    return GetSize();
  TIter next(fVMStack);
  
  //count how many modules in the stack are equal to a particular type
  while( VM_Module*obj = (VM_Module*)next()){
    if(obj->ModuleType() == modtype)
      count++;
  }
  return count;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
UInt_t VM_Module_Stack::AddBranches(TTree *_tree){
  TIter next(fVMStack);
  while( VM_Module*obj = (VM_Module*)next()){
    obj->AddBranch(_tree);
  }
  return 1;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
UInt_t VM_Module_Stack::SetBranches(TTree *_tree){
  TIter next(fVMStack);
  while( VM_Module*obj = (VM_Module*)next()){
    obj->SetBranch(_tree);
  }
  return 1;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
UInt_t VM_Module_Stack::SortGeoChVal(const UShort_t& geoaddress,const UInt_t& ch, const UInt_t& val){
  TIter next(fVMStack);
  while( VM_Module*obj = (VM_Module*)next()){
    if(obj->GeoAddress() == geoaddress){
      obj->SortChVal(ch,val);
      return 1;
    } 
  }
  return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
UInt_t VM_Module_Stack::Add(VM_BaseClass *base){
  if(!base->InheritsFrom("VM_Module")){
    return 0;
  }
  if(!fVMStack){
    Init();
  }
  fVMStack->Add(base);
  return 1;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
CAEN_ADC::CAEN_ADC(const TString& name,const UInt_t& geoaddress):VM_Module(name,geoaddress){
  fModuleType =kCAENtype;  //CAEN
}
/////////////////////////////////////////////////////////////////////////////////////////////////
Bool_t CAEN_ADC::Unpack(unsigned short *& gpointer){
  short dat(0);
  short chan(0);
  short ch_hits(0);
  short geoaddress(0);
  bool flag(1);
  
  //module is suppressed
  if(*gpointer==0xffff){
    gpointer=gpointer+2; //jump over ffff block
    return 1; //returning 1 cause this is NOT bad
  }
  
  ch_hits = ( *gpointer++ & 0xff00 ) >> 8; //read high byte as low byte
  geoaddress = ( *gpointer++ & 0xf800) >> 11;
  
  if(geoaddress != fGeoAddress){
    std::cout<<geoaddress<<" different from geoaddress in stack: "<< GeoAddress() << " for module "<<GetName() <<std::endl;
    gpointer+=ch_hits*2;
    flag =  0; 
    //THIS is bad, means stack is probably not set up right or there was an incorrectly formatted buffer(which can happen).  
    //Check to find the cause...probably your module stack is not in the right order
  }
  else{
    for (short jj=0;jj<ch_hits;jj++){
      dat =  *gpointer++ & 0xfff;
      chan = *gpointer++ & 0x1f;
      SortChVal(chan,dat);
      
    }
  }
  fCounter= *gpointer++;
  gpointer = gpointer + 3 ; //jump 3 words to skip rest of CAEN End of Block
  return flag;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
MESY_QDC::MESY_QDC(const TString& name,const UInt_t& geoaddress):VM_Module(name,geoaddress){
  fModuleType = kMESYtype;  //MESY
}
/////////////////////////////////////////////////////////////////////////////////////////////////
Bool_t MESY_QDC::Unpack(unsigned short *& gpointer){
  short dat, chan;
  unsigned short ovfl,testresolution;
  short shortwords;
  short ModuleID;
  unsigned short * zpointer; zpointer = gpointer;
  int flag(1);

  if(*zpointer==0xffff){ //module is suppressed
    gpointer = gpointer + 2 ;
    return 1; //this is not bad so just return 1;
  }

  testresolution = (*zpointer & 0x7000) >>12;
  shortwords = 2 * ( *zpointer++ & 0x0fff ); 
  //if (testresolution !=8192 ) {
  if (testresolution !=1 ) {
    std::cout<<" resolution value unexpected:" <<testresolution<<std::endl ;
  }
  ModuleID = *zpointer++ & 0x00ff;
  
  
  if(ModuleID != fGeoAddress ){
    std::cout<<ModuleID<<" different from geoaddress in stack: "<< GeoAddress()<<std::endl;
    gpointer+=shortwords; //move to expected end of this part of buffer

    flag = 0; //THIS is bad, means stack is probably not set up right or there was an incorrectly formatted buffer(which can happen).  Check to find the cause...probably your module stack is not in the right order
  }
  else{
    while( zpointer < ( gpointer + shortwords)){
      ovfl = (*zpointer & 0x4000 )>>14 ; 
      dat = *zpointer++ & 0xfff;
      chan = *zpointer++ & 0x1f;
      
      if (!ovfl){
	SortChVal(chan,dat);
      }
      else dat = 0;
    }
  }
  fCounter = *zpointer++;
  zpointer = zpointer + 3; //jump over EOB + ffff
  gpointer = zpointer; //move gpointer to end of this zpointer

  return flag;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
CHINP::CHINP(const UShort_t& modsize,const TString& name,const UInt_t& geoaddress):VM_Module(modsize,name,geoaddress){
  fModuleType =kCHINPtype; 
}
////////////////////////////////////////////////////////////////////////////////////////////////////
Bool_t CHINP::Unpack(unsigned short *& gpointer){


  UInt_t  channelId, timeStamp[2], channelCount;
  int flag(1);

  hits.clear();
  energies.clear();
  times.clear();

  //--ddc oct15 in daq11(maybe daq10) there is additional module
  //--ddc id word, read out as modid.
  UInt_t  modid = *(gpointer++);
  //--ddc NOTE: for 'geoaddress'.
  //((modid >> 27) & 0x0f | 0x1ff0))
  UInt_t  wordCount = *((unsigned int*) gpointer);
  gpointer+=2;
  //
  // Channel counts > 4095 are error indicators..
  // just skip the data the VMUSB can't help but read ///  ???
  //
  //--ddc sep12.  Seems kind of pointless to set the value of
  //  the pointer to to some arbitrary number.  This whole
  //  thing is patterned after the 'legacy' chinp.cpp file
  //  I'm leaving this in, BUT it was WRONG, so I'm fixing the
  //  part that was wrong (can't print out 'channelcount' before
  //  it is assigned).

  // mask off upper 16-bit field

  wordCount &= 0xffff;
  channelCount  = *gpointer;

  //if (wordCount >65532) { //is it??
  if (wordCount >4095) {
    //std::cerr << "Got an error report from " << GetName() << " "
    //<< wordCount << " (" << channelCount << ")" << std::endl;
    //	 << channelCount << endl;
    wordCount &= 0xfff;

    //--ddc sep12  THIS is wrong too (even if wordcount were right)   
    // offset += wordCount*2 +1;
    //--ddc nov15.. IS only +wordCount
    //    offset=offset+wordCount+7; //oct13 hmm.. this maybe only +wordCount
    gpointer=gpointer+wordCount;
    flag=0;
    return flag;
  }

  //--ddc move UP  channelCount  = event[offset];
  gpointer += 1;

  // word count should always = channelCount * 3 + 7
  if (wordCount < (channelCount*3 + 7)  ||
      wordCount > (channelCount*3 + 10)) {

    //if (channelCount != 128) {
    printf("Event size mismatch wordCount %d  channelCount %d\n",wordCount,channelCount); 
    //--ddc sep12, THIS is WRONG. return offset += wordCount*2;
    //    return offset += wordCount+6;  //oct13 hmm.. maybe only wordCount-1
    //--ddc nov15
    flag=1;
    gpointer=gpointer+wordCount;
    return flag;
  }

  // word count should be evenly divisible by 3
  // loop through groups of 3 words per hit channel
  //  printf("word count is %d, channelCount is %d\n",wordCount,channelCount); 
  //uncommented this line MMc 3/7/11
  timeStamp[0]  = *((unsigned int*) gpointer);
  gpointer+=2;
  timeStamp[1]  = *((unsigned int*) gpointer);
  gpointer+=2;

  for (unsigned int i =0; i < channelCount; i++) {
  // get channel tag
    channelId      = *gpointer;
    //    printf("Channel ID = %x\n",channelId);
    gpointer +=1;


    // now pick up analog data
    UInt_t e     = (*gpointer) & 0x3fff;
    gpointer += 1;
    UInt_t time     = (*gpointer) & 0x3fff;
    gpointer += 1;
 
    UInt_t channel   = channelId & 0x0f;
    UInt_t chip      = (channelId >> 5) & 0xff;

    //       printf("chip %d chan %d E %d Time %d\n",chip, channel,e,time);
    // validate chip ID

    //--ddc nov15 divide e AND t channels by two for chan, divide by 16 for chips (actual chips).
    //below this is divided by two again for chips per board (so chips actually becomes chip boards!)
    //which is used for the mapping to histograms. This is historical.
    //
    UInt_t numChips=NumOfCh()/2/16;
    if (chip > 0 && chip <= numChips) {
      //--ddc provide a method to use inverting(?) chips.  I've added this
      //here before the chip number is modified.  So this should match what
      //I've seen done where the chipnumber directly from the event stream is used..

      TString modname = TString(GetName());

      e = chipEnergy(e,chip,modname);
      time = chipTime(time,chip,modname);


    // map 2 chips on same board as one board with 32 channels
      if ((chip & 0x0001) == 0) {
	chip = chip/2;
	channel = channel + 16;
      } else {
	chip = chip/2 + 1;
      }
      //      printf("chip now %d chan %d\n",chip, channel);
      int index;
      if((index = PutE(chip,channel,e))>-1) {
	hits.push_back(index);
	energies.push_back(e);
	times.push_back(time);
      }
      PutT(chip,channel,time);

    }
  }

  //  printf("offset at end = %d\n",offset);
  // figure out how much garbage to gobble from end of event
 
  //--ddc sep12 The orignal alogorithm had a garbage gobble that was
  // just WRONG (even if it is necessary)

  //--ddc And now the error they ignored... if the channelcount is too small
  //for the wordcount.. make sure we jump past the wordcount.
  if(wordCount > (channelCount*3 + 7)){
    gpointer=gpointer+(wordCount-(channelCount*3+7));
  }

  //--ddc.. there is ALWAYS a (32bit) word here, but I don't know what it is.
  //skip it...
  gpointer = gpointer + 2 ;
  ///
  return flag;

}
//////////////////////////////////////////////////////////////////////////////////////////////

UInt_t CHINP::AddBranch(TTree* _tree){
  b_module = _tree->Branch(Form("%s_hits",GetName()),&hits);
  b_module = _tree->Branch(Form("%s_energies",GetName()),&energies);
  b_module = _tree->Branch(Form("%s_times",GetName()),&times);
  return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////
UInt_t CHINP::SetBranch(TTree* _tree){
  if(_tree->GetBranch(GetName())){   
    //--ddc hmm.    _tree->SetBranchAddress(GetName(),&fChValue, &b_module);  
    _tree->SetBranchAddress(Form("%s_hits",GetName()),&hits,&b_module);
    _tree->SetBranchAddress(Form("%s_energies",GetName()),&energies,&b_module);
    _tree->SetBranchAddress(Form("%s_times",GetName()),&times,&b_module);
  }
  else{
    std::cout<<"no "<<GetName()<<" present in Tree, Are you sure this rootfile is compatible with this stack?"<<std::endl;
    return 0;
  }

  return 1;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void CHINP::Reset(){
  //Reset all values stored in module channels
  fValid = 0;
  for(unsigned int i=0;i<NumOfCh();i++){
    fChValue[i] = 0;
  }
  //and in vectors...
  hits.clear();
  energies.clear();
  times.clear();
  
  return;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

#endif
