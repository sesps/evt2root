/////////////////////////////////////////////////////////////////////////////////////
// ROOT script: evt2root_NSCL11_mADC.C
// See readme.md for general instructions.
// Adopted & tested for the NSCLDAQ11 version.
//
// Nabin, ddc, DSG, KTM et.al. // December 2015.
// kgh -- Sept. 2018
// 
/////////////////////////////////////////////////////////////////////////////////////
//C and C++ libraries
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <math.h>
#include <unistd.h>

//ROOT libraries
#include <TFile.h>
#include <TTree.h>
#include <TROOT.h>
#include <TH1I.h>
#include <TH2I.h>
#include <TObjArray.h>

//Detectors' libraries
#include "2016_detclass.h"
//#include "configfile.h"
//#include "SimpleInPipe.h"
#include "VM_Module.hpp"
//#include "VM_BaseClass.hpp"

using namespace std;
//////////////////////////////////////////////////////////////////////////////////////
void ReadPhysicsBuffer();

const int BufferWords = 13328;
const int BufferBytes = BufferWords*2;
const int unsigned buflen = 26656;
char buffer[buflen];

const string files_list = "evt_files.lst";

const float NSPERCH = 0.0625;

// Global variables
TFile* fileR;
TTree* DataTree;
TObjArray* RootObjects;
Int_t run;

// Set VME module names and positions
//***must include ALL modules read in by TCL
CAEN_ADC* caen_adc1 = new CAEN_ADC("First ADC", 2);
CAEN_ADC* caen_adc2 = new CAEN_ADC("Second ADC", 3);
CAEN_ADC* caen_adc3 = new CAEN_ADC("Third ADC", 6);
CAEN_TDC* caen_tdc1 = new CAEN_TDC("First TDC", 5);
MESY_ADC* mesy_tdc2 = new MESY_ADC("Second TDC", 9);
///////using channels 1-4; 1&2 are two ends of fp1, 3&4 are fp2

float CalParamF[128][3];
float CalParamB[128][3];

int unsigned Nevents;
int unsigned TotEvents=0;
int unsigned words;  
unsigned short *point,*epoint;

Int_t EventCounter = 0;
int unsigned EOB_NEvents=0;
int unsigned ASICsCounter=0;
int unsigned CAENCounter=0;

TH2I* ADC_vs_Chan;
TH2I* TDC_vs_Chan;
TH2I* mTDC_vs_Chan;
TH2I* ParticleID_Plastic_Anode;

//- Detectors' classes --------------------------------------------------------  
CAENHit ADC1;
CAENHit ADC2;
CAENHit ADC3;
CAENHit TDC;
MesyHit mTDC;
Int_t TAC;
Int_t TAC1;
Int_t TAC2;
Int_t TAC3;
Int_t TAC4;
Int_t Cath;
Int_t FP1;
Int_t FP2;
Int_t Scint1;
Int_t Scint2;
Int_t Mon;

//spectcl variables -- kgh
Float_t fp_plane1_tdiff,
  fp_plane1_tsum,
  fp_plane1_tave,
  fp_plane2_tdiff,
  fp_plane2_tsum,
  fp_plane2_tave,
  plastic_sum;

Float_t FP1_time_left,
  FP1_time_right,
  FP2_time_left,
  FP2_time_right;

Float_t anode1_time, anode2_time, plastic_time;

////////////////////////////////////////////////////////
//- Main function -------------------------------------------------------------  
int evt2root_NSCL11_mADC() {

  gROOT->Reset();

  cout << "==============================================================================" <<endl;
  cout << "evt2root: Takes .evt files from a list and converts the data into ROOT format." <<endl;
  cout << "==============================================================================" <<endl;

  int unsigned type;
  UInt_t BufferType = 0;
  int unsigned Nbuffers=0;
  int BufferPhysics = 0;
  int runNum;

  ifstream ListEVT;
  string OutputROOTFile;
  string aux;
  char *ROOTFile; 

  ListEVT.open(files_list.c_str());
  if (!ListEVT.is_open()) {
    cout << "*** Error: could not open " << files_list << endl;
    return 0;
  }
  else {
    cout << "File " << files_list << " opened." <<endl;
    ListEVT >> aux >> aux >> aux >> OutputROOTFile;
  }

  cout << "Creating output file: " << OutputROOTFile << "...\n\n";

  //- ROOT objects' definitions -------------------------------------------------  
  // ROOT output file
  ROOTFile = new char [OutputROOTFile.size()+1];
  strcpy (ROOTFile, OutputROOTFile.c_str());
  fileR = new TFile(ROOTFile,"RECREATE");

  // Data Tree
  DataTree = new TTree("DataTree","DataTree");

  DataTree->Branch("ADC1.Nhits",&ADC1.Nhits,"ADC1Nhits/I");
  DataTree->Branch("ADC1.ID",ADC1.ID,"ID[ADC1Nhits]/I");
  DataTree->Branch("ADC1.ChNum",ADC1.ChNum,"ChNum[ADC1Nhits]/I");
  DataTree->Branch("ADC1.Data",ADC1.Data,"Data[ADC1Nhits]/I");

  DataTree->Branch("ADC2.Nhits",&ADC2.Nhits,"ADC2Nhits/I");
  DataTree->Branch("ADC2.ID",ADC2.ID,"ID[ADC2Nhits]/I");
  DataTree->Branch("ADC2.ChNum",ADC2.ChNum,"ChNum[ADC2Nhits]/I");
  DataTree->Branch("ADC2.Data",ADC2.Data,"Data[ADC2Nhits]/I");

  DataTree->Branch("ADC3.Nhits",&ADC3.Nhits,"ADC3Nhits/I");
  DataTree->Branch("ADC3.ID",ADC3.ID,"ID[ADC3Nhits]/I");
  DataTree->Branch("ADC3.ChNum",ADC3.ChNum,"ChNum[ADC3Nhits]/I");
  DataTree->Branch("ADC3.Data",ADC3.Data,"Data[ADC3Nhits]/I");

  DataTree->Branch("TDC.Nhits",&TDC.Nhits,"TDCNhits/I");
  DataTree->Branch("TDC.ID",TDC.ID,"ID[TDCNhits]/I");
  DataTree->Branch("TDC.ChNum",TDC.ChNum,"ChNum[TDCNhits]/I");
  DataTree->Branch("TDC.Data",TDC.Data,"Data[TDCNhits]/I");

  DataTree->Branch("mTDC.Nhits",&mTDC.Nhits,"mTDCNhits/I");
  DataTree->Branch("mTDC.ID",mTDC.ID,"ID[mTDCNhits]/I");
  DataTree->Branch("mTDC.ChNum",mTDC.ChNum,"ChNum[mTDCNhits]/I");
  DataTree->Branch("mTDC.Data",mTDC.Data,"Data[mTDCNhits]/I");

  DataTree->Branch("TAC",&TAC,"TAC/I");
  DataTree->Branch("TAC1",&TAC1,"TAC1/I");
  DataTree->Branch("TAC2",&TAC2,"TAC2/I");
  DataTree->Branch("TAC3",&TAC3,"TAC3/I");
  DataTree->Branch("TAC4",&TAC4,"TAC4/I");
  DataTree->Branch("Cath",&Cath,"Cath/I");
  DataTree->Branch("FP1",&FP1,"FP1/I");
  DataTree->Branch("FP2",&FP2,"FP2/I");
  DataTree->Branch("Scint1",&Scint1,"Scint1/I");
  DataTree->Branch("Scint2",&Scint2,"Scint2/I");
  DataTree->Branch("Mon",&Mon,"Mon/I");

  DataTree->Branch("FP1_time_left",&FP1_time_left,"FP1_time_left/F");
  DataTree->Branch("FP1_time_right",&FP1_time_right,"FP1_time_right/F");
  DataTree->Branch("FP2_time_left",&FP2_time_left,"FP2_time_left/F");
  DataTree->Branch("FP2_time_right",&FP2_time_right,"FP2_time_right/F");

  DataTree->Branch("fp_plane1_tdiff",&fp_plane1_tdiff,"fp_plane1_tdiff/F");
  DataTree->Branch("fp_plane1_tsum",&fp_plane1_tsum,"fp_plane1_tsum/F");
  DataTree->Branch("fp_plane1_tave",&fp_plane1_tave,"fp_plane1_tave/F");
  DataTree->Branch("fp_plane2_tdiff",&fp_plane2_tdiff,"fp_plane2_tdiff/F");
  DataTree->Branch("fp_plane2_tsum",&fp_plane2_tsum,"fp_plane2_tsum/F");
  DataTree->Branch("fp_plane2_tave",&fp_plane2_tave,"fp_plane2_tave/F");
  DataTree->Branch("plastic_sum",&plastic_sum,"plastic_sum/F");

  DataTree->Branch("anode1_time",&anode1_time,"anode1_time/F");
  DataTree->Branch("anode2_time",&anode2_time,"anode2_time/F");
  DataTree->Branch("plastic_time",&plastic_time,"plastic_time/F");
  
  // Histograms
  Int_t xbins=4096;
  Int_t ybins=32;
  ADC_vs_Chan = new TH2I("ADC_vs_Chan","",xbins,0,xbins,ybins,0,ybins);
  TDC_vs_Chan = new TH2I("TDC_vs_Chan","",xbins,0,xbins,ybins,0,ybins);
  mTDC_vs_Chan = new TH2I("mTDC_vs_Chan","",65536,0,65536,ybins,0,ybins);
  ParticleID_Plastic_Anode = new TH2I("ParticleID_Plastic_Anode","",xbins,0,xbins,xbins,0,xbins);

  //List of root objects.
  RootObjects = new TObjArray();
  RootObjects->Add(DataTree);
  RootObjects->Add(ADC_vs_Chan);
  RootObjects->Add(TDC_vs_Chan);
  RootObjects->Add(mTDC_vs_Chan);
  RootObjects->Add(ParticleID_Plastic_Anode);

  string data_dir = "";

  //Check if this file exists.
  if (!ListEVT.is_open()) {
    cout << "*** Error: could not open " << files_list << endl;
    return 0;
  }
  else {
    // cout << "File " << files_list << " opened." <<endl;
    ListEVT >> aux >> aux >> data_dir;
  }

  ifstream evtfile;
  
  cout << "Loop over evt files " <<endl; //debug
 
  int run_number;
  int nseg;
  ListEVT >> run_number;
  run=run_number;

  //Loop over files in the data file list.
  while(!ListEVT.eof()) {

    if (evtfile.is_open()) cout << "  * Problem previous file not closed!" << endl;

    nseg=0;
    for(int seg_number=0;seg_number<3;seg_number++) {
      string name = data_dir + Form("run-%.4d-%.2d.evt",run_number,seg_number);

      //open evt file
      evtfile.clear();
      evtfile.open(name.c_str(),ios::binary);      
    
      if(seg_number==0) {//should be true for all files in list
	cout << "  Data file: " << name << endl;
    
	if (evtfile.bad()) cout << "   ** Bad evt file." << endl;
	if (evtfile.fail()) cout << "   ** Fail evt file" << endl;
      
	if (!evtfile) {
	  cout << "   Could not open evt file" << endl;
	  //return 1;
	}
	else {
	  cout << "   Converting data ..." << endl;
	  nseg++;
	}
      }
      else {//should only be true for multi-segment files; limit output in case of single-segment
	if(evtfile.good()) {
	  cout << "  Data file: " << name << endl;
	  cout << "   Converting data ..." << endl;
	  nseg++;
	}
      }

      ////-----------------------------------------------------------------------------
      for (;;) {     
	evtfile.read(buffer,8);
	evtfile.read(buffer+8,*(unsigned int*)buffer-8);
	
	//--ddc daq11, the data starts right after the body subheader, which should be zero.
	if( *(unsigned int*)(buffer+8) > 0 ) {
	  cout << "   unexpected subheader... " << (unsigned int*)(buffer+8) << endl;
	}

	if (!evtfile) {
	  //this could be a bad file or the file is subdivided into parts
	  break;
	}      
	
	point = ((unsigned short*)buffer) + 6;
      
	Nbuffers++;     
	epoint = point; 
	Nevents = 1;      
	string TitleStr;

	BufferType = *(unsigned int*)(buffer+4);
	switch(BufferType) {
	case 0x1E : type=1;
	  break;
	case 0x01 : type=11;
	  break;
	case 0x02 : type=12;
	  break;
	default: type=0;
	}
      
	switch(type) {

	case 11: 
	  runNum = *(epoint+8);
	  cout << "   Run number = "<< runNum << endl;
	  if(runNum!=run)
	    cout << "   Expected run number = " << run << endl;
	  break;
	  
	case 12:
	  break;
	
	case 2:
	  break;

	case 1:
	  BufferPhysics++;
	  ReadPhysicsBuffer();	
	  break; //end of physics buffer		
	}//end switch(type)  
      } //end for(;;) over evtfile
      ////---------------------------------------------------------------------------------
    
      evtfile.close();
      evtfile.clear(); // clear event status in case we had a bad file
    }//end of segment loop
    if(nseg>1)
      printf("   %d segments found\n",nseg);
    ListEVT >> run_number;
    run = run_number;
  }

  cout << setprecision(3);
  cout << "Total buffers = " << Nbuffers << endl;
  cout << "  Physics buffers = " << BufferPhysics  << " (" <<100.0*BufferPhysics/Nbuffers<< "\% of total buffers)"<< endl;  
  cout << "  Number of events based on buffer headers: " << TotEvents << endl; 
  cout << "  Number of events based on event counter:  " <<  EventCounter << endl;
 
  RootObjects->Write();
  fileR->Close();	
  
  return 1;

}//end of evt2root

////////////////////////////////////////////////////////////////////////////
// Function where the root objects are filled.
////////////////////////////////////////////////////////////////////////////

void ReadPhysicsBuffer() {

  Nevents = 1;
  TotEvents += Nevents;

  for (unsigned int ievent=0;ievent<Nevents;ievent++) {
    ADC1.ResetCAENHit();
    ADC2.ResetCAENHit();
    ADC3.ResetCAENHit();
    TDC.ResetCAENHit();
    mTDC.ResetMesyHit();
    
    //create pointer inside of each  event
    unsigned short * fpoint = epoint;		    
    words = *fpoint++;  

    caen_adc1->Reset();
    caen_adc2->Reset();
    caen_adc3->Reset();
    caen_tdc1->Reset();
    mesy_tdc2->Reset();
    caen_adc1->Unpack(fpoint); if(fpoint>epoint + words + 1)  break;
    caen_adc2->Unpack(fpoint); if(fpoint>epoint + words + 1)  break;
    caen_adc3->Unpack(fpoint); if(fpoint>epoint + words + 1)  break;
    caen_tdc1->Unpack(fpoint); if(fpoint>epoint + words + 1)  break;
    mesy_tdc2->Unpack(fpoint); if(fpoint>epoint + words + 1)  break;

    epoint += words+1; // This skips the rest of the event
    ///////////////////////////////////////////////////////////////////////////////////  
    //---------------------------------------------------
    for(int i=0; i<32; i++) {
      ADC1.ID[ADC1.Nhits] = 1;
      ADC1.ChNum[ADC1.Nhits] = i;
      ADC1.Data[ADC1.Nhits++] = (Int_t)caen_adc1->fChValue[i];
      
      ADC2.ID[ADC2.Nhits] = 1;
      ADC2.ChNum[ADC2.Nhits] = i;
      ADC2.Data[ADC2.Nhits++] = (Int_t)caen_adc2->fChValue[i];
      
      ADC3.ID[ADC3.Nhits] = 1;
      ADC3.ChNum[ADC3.Nhits] = i;
      ADC3.Data[ADC3.Nhits++] = (Int_t)caen_adc3->fChValue[i];
      
      ADC_vs_Chan->Fill(caen_adc1->fChValue[i],i);

      if (run < 45) {
	if (EventCounter==0 && ievent==0 && i==0)
	  cout << "   Sorting data into TAC (1), Cath (1)" << endl;
	switch(i) {
	case 0 :
	  TAC = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 1 :
	  Cath = (Int_t)caen_adc1->fChValue[i];
	  break;
	}
      }
      else if (run < 74) {//June 19-20, 2018
	if (EventCounter==0 && ievent==0 && i==0)
	  cout << "   Sorting data into TAC (1), Delay (2), Scint (2)" << endl;
	switch(i) {
	case 0 :
	  if ((Int_t)caen_adc1->fChValue[i] > 100)
	    TAC = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 1 :
	  FP1 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 2 :
	  FP2 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 3 :
	  Scint1 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 4 :
	  Scint2 = (Int_t)caen_adc1->fChValue[i];
	  break;
	}
      }
      else if (run < 138) {//June 25, 2018
	if (EventCounter==0 && ievent==0 && i==0)
	  cout << "   Sorting data into TAC (1), Delay (2), Scint (2), Mon (1)" << endl;
	switch(i) {
	case 0 :
	  TAC = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 1 :
	  FP1 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 2 :
	  FP2 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 3 :
	  Scint1 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 4 :
	  Scint2 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 5 :
	  Mon = (Int_t)caen_adc1->fChValue[i];
	  break;
	}
      }

      else if (run < 224) {//July 20, 2018
	if (EventCounter==0 && ievent==0 && i==0)
	  cout << "   Sorting data into TAC (4), Andode (2), Scint (2), Cath (1)" << endl;
	switch(i) {
	case 0 :
	  if ((Int_t)caen_adc1->fChValue[i]>100)
	    TAC1 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 1 :
	  if ((Int_t)caen_adc1->fChValue[i]>100)
	    TAC2 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 2 :
	  if ((Int_t)caen_adc1->fChValue[i]>100)
	    TAC3 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 3 :
	  if ((Int_t)caen_adc1->fChValue[i]>100)
	    TAC4 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 4 :
	  FP1 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 5 :
	  FP2 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 6 :
	  Scint1 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 7 :
	  Scint2 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 8 :
	  Cath = (Int_t)caen_adc1->fChValue[i];
	  break;
	}
      }
      else {//Oct-Nov 2018
	switch(i) {
	case 0 :
	  if ((Int_t)caen_adc1->fChValue[i]>100)
	    TAC1 = (Int_t) caen_adc1->fChValue[i];
	  break;
	case 1 :
	  if ((Int_t)caen_adc1->fChValue[i]>100)
	    TAC2 = (Int_t) caen_adc1->fChValue[i];
	  break;
	case 2 :
	  if ((Int_t)caen_adc1->fChValue[i]>100)
	    TAC3 = (Int_t) caen_adc1->fChValue[i];
	  break;
	case 3 :
	  if ((Int_t)caen_adc1->fChValue[i]>100)
	    TAC4 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 4 :
	  FP1 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 5 :
	  FP2 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 6 :
	  Scint1 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 7 :
	  Scint2 = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 8 :
	  Cath = (Int_t)caen_adc1->fChValue[i];
	  break;
	case 10:
	  Mon = (Int_t)caen_adc1->fChValue[i];
	  break;
	}
      }
    }    

    ParticleID_Plastic_Anode->Fill(Scint1, FP1);

    //---------------------------------------------------
    for(int i=0; i<8; i++) {
      TDC.ID[TDC.Nhits] = 1;
      TDC.ChNum[TDC.Nhits] = i;
      TDC.Data[TDC.Nhits++] = (Int_t)caen_tdc1->fChValue[i];   
      TDC_vs_Chan->Fill(caen_tdc1->fChValue[i],i);
    }	    

    for (int i=0; i<=31; i++) {
      	mTDC.ID[mTDC.Nhits] = 1; //arbitrary value? --kgh
      	mTDC.ChNum[mTDC.Nhits] = i;
      	mTDC.Data[mTDC.Nhits++] = (Int_t)mesy_tdc2->fChValue[i];
	mTDC_vs_Chan->Fill(mesy_tdc2->fChValue[i],i);
    }

    if ((Int_t)mesy_tdc2->fChValue[0] > 10) plastic_time = (Float_t)mesy_tdc2->fChValue[0]*NSPERCH;
    if ((Int_t)mesy_tdc2->fChValue[1] > 10) FP1_time_right = (Float_t)mesy_tdc2->fChValue[1]*NSPERCH;
    if ((Int_t)mesy_tdc2->fChValue[2] > 10) FP1_time_left = (Float_t)mesy_tdc2->fChValue[2]*NSPERCH;
    if ((Int_t)mesy_tdc2->fChValue[3] > 10) FP2_time_right = (Float_t)mesy_tdc2->fChValue[3]*NSPERCH;
    if ((Int_t)mesy_tdc2->fChValue[4] > 10) FP2_time_left = (Float_t)mesy_tdc2->fChValue[4]*NSPERCH;
    if ((Int_t)mesy_tdc2->fChValue[5] > 10) anode1_time = (Float_t)mesy_tdc2->fChValue[5]*NSPERCH;
    if ((Int_t)mesy_tdc2->fChValue[6] > 10) anode2_time = (Float_t)mesy_tdc2->fChValue[6]*NSPERCH;

    fp_plane1_tdiff = (Float_t)(FP1_time_left - FP1_time_right)/2; //div by 2 to center in the middle
    fp_plane1_tsum = (Float_t)(FP1_time_left + FP1_time_right); //good to have for checks
    fp_plane1_tave = (Float_t)(FP1_time_left + FP1_time_right)/2;
    fp_plane2_tdiff = (Float_t)(FP2_time_left - FP2_time_right)/2;
    fp_plane2_tsum = (Float_t)(FP2_time_left + FP2_time_right);
    fp_plane2_tave = (Float_t)(FP2_time_left + FP2_time_right)/2;
    plastic_sum = (Float_t)(Scint1 + Scint2);
  
    EventCounter++;
    
    DataTree->Fill();   

  }//end for over events
}//end of void ReadPhysicsBuffer()
/////////////////////////////////////////////////////////////////
