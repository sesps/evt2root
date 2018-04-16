/////////////////////////////////////////////////////////////////////////////////////
// ROOT script: evt2root_NSCL11_mADC.C
// See readme.md for general instructions.
// Adopted & tested for the NSCLDAQ11 version.
//
// Nabin, ddc, DSG, KTM et.al. // December 2015.
//
// Inputs & Comments are Welcome!           --Nabin
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
#include <TNtuple.h>
#include <TH1I.h>
#include <TH2I.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TRint.h>
#include <TObjArray.h>
#include <TH1.h>
#include <TCutG.h>
#include <TDirectory.h>
#include <TBrowser.h>
#include <TThread.h>
#include <TStyle.h>
#include <TGFrame.h>
#include <TGlobal.h> 

//Detectors' libraries
#include "2016_detclass.h"
#include "configfile.h"
#include "SimpleInPipe.h"
#include "VM_Module.hpp"
#include "VM_BaseClass.hpp"

using namespace std;
//////////////////////////////////////////////////////////////////////////////////////
void ReadPhysicsBuffer();

const int BufferWords = 13328;
const int BufferBytes = BufferWords*2;
const int unsigned buflen = 26656;
char buffer[buflen];

const string files_list = "evt_files.lst";

// Global variables
TFile* fileR;
TTree* DataTree;
TObjArray* RootObjects;

// Set VME module names and positions
CAEN_ADC* caen_adc1 = new CAEN_ADC("First ADC", 2);
CAEN_TDC* caen_tdc1 = new CAEN_TDC("First TDC", 5);

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

//- Detectors' classes --------------------------------------------------------  
CAENHit ADC;
CAENHit TDC;

/***************************************************************
 * a little construct to have an array initialized with zeroes *
 ***************************************************************/
template<class T,int n>

class ZeroedArray {
  T histo[n];
public:
  ZeroedArray() {
    for(int i = 0; i < n; i++)
      histo[i] = 0;
  }
  T& operator[](int idx)
  { return histo[idx]; }
};

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

  //- ROOT objects' definitions -------------------------------------------------  
  // ROOT output file
  ROOTFile = new char [OutputROOTFile.size()+1];
  strcpy (ROOTFile, OutputROOTFile.c_str());
  fileR = new TFile(ROOTFile,"RECREATE");

  // Data Tree
  DataTree = new TTree("DataTree","DataTree");

  DataTree->Branch("ADC.Nhits",&ADC.Nhits,"ADCNhits/I");
  DataTree->Branch("ADC.ID",ADC.ID,"ID[ADCNhits]/I");
  DataTree->Branch("ADC.ChNum",ADC.ChNum,"ChNum[ADCNhits]/I");
  DataTree->Branch("ADC.Data",ADC.Data,"Data[ADCNhits]/I");

  DataTree->Branch("TDC.Nhits",&TDC.Nhits,"TDCNhits/I");
  DataTree->Branch("TDC.ID",TDC.ID,"ID[TDCNhits]/I");
  DataTree->Branch("TDC.ChNum",TDC.ChNum,"ChNum[TDCNhits]/I");
  DataTree->Branch("TDC.Data",TDC.Data,"Data[TDCNhits]/I");
  
  // Histograms
  Int_t xbins=288;
  Int_t ybins=4096;
  ADC_vs_Chan = new TH2I("ADC_vs_Chan","",xbins,0,xbins,ybins,0,ybins);
  TDC_vs_Chan = new TH2I("TDC_vs_Chan","",xbins,0,xbins,ybins,0,ybins);

  //List of root objects.
  RootObjects = new TObjArray();
  RootObjects->Add(DataTree);
  RootObjects->Add(ADC_vs_Chan);
  RootObjects->Add(TDC_vs_Chan); 

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
  bool fileProblem = 0;
  bool endOfRun = 0;
  cout << "Loop over evt files " <<endl; //debug
 
  int run_number;
  int nseg;
  ListEVT >> run_number;

  //Loop over files in the data file list.
  while(!ListEVT.eof()) {

    if (evtfile.is_open()) cout << "  * Problem previous file not closed!" << endl;

    endOfRun=0;
    fileProblem = 0;   
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
	  cout << "unexpected subheader..." << endl;
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
	cout << "   run number="<< runNum << endl;
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
    ADC.ResetCAENHit();
    TDC.ResetCAENHit();
    
    //create pointer inside of each  event
    unsigned short * fpoint = epoint;		    
    words = *fpoint++;  

    caen_adc1->Reset();
    caen_tdc1->Reset();

    caen_adc1->Unpack(fpoint); if(fpoint>epoint + words + 1) break;
    caen_tdc1->Unpack(fpoint); if(fpoint>epoint + words + 1) break;    

    epoint += words+1; // This skips the rest of the event
    ///////////////////////////////////////////////////////////////////////////////////  
    //---------------------------------------------------
    for(int i=0;i<32;i++) {
      ADC.ID[ADC.Nhits] = 1;
      ADC.ChNum[ADC.Nhits] =i;
      ADC.Data[ADC.Nhits++] = (Int_t) caen_adc1->fChValue[i];
      ADC_vs_Chan->Fill(i,caen_adc1->fChValue[i]);
    }    

    //---------------------------------------------------
    for(int i=0;i<8;i++) {
      TDC.ID[TDC.Nhits] = 1;
      TDC.ChNum[TDC.Nhits] =i;
      TDC.Data[TDC.Nhits++] = (Int_t) caen_tdc1->fChValue[i];   
      TDC_vs_Chan->Fill(i,caen_tdc1->fChValue[i]);
    }	    
 
    EventCounter++;
    
    DataTree->Fill();   

  }//end for over events
}//end of void ReadPhysicsBuffer()
/////////////////////////////////////////////////////////////////
