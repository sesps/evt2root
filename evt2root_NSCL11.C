/////////////////////////////////////////////////////////////////////////////////////
// ROOT script: evt2root_NSCL11.C
// See readme.md for general instructions.
// Adopted & tested for the NSCLDAQ11 version.
//
// to run it: root -l evt2root_NSCL11.C+
// make sure your .evt files are included in the evt_files.lst
// 
// Nabin, Dev, DSG, KTM et.al. // December 2015.
//
// Note: This version of converter is useful only for the codes that has been
// used by LSU group;
// the names of the Branches & leaves and also the objects of detector classes 
// is different for the Codes that has been used by FSU group. 
//
// Inputs & Comments are Welcome!           --Nabin
/////////////////////////////////////////////////////////////////////////////////////
//C and C++ libraries
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>

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

//Detectors' libraries
#include "2016_detclass.h"

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
////////////////////////////////////////////////////////
//- Main function -------------------------------------------------------------  
int evt2root_NSCL11() {

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

    while (fpoint < epoint + words) {
      if(*fpoint == 0xffff ) {
	fpoint++;
	continue;
      }
		  
      unsigned short *gpoint = fpoint;
      unsigned short chanCount = (*(gpoint++) & 0xff00)>>8;
      unsigned short GEOaddress = (*(gpoint++) & 0xf800)>>11; 
      unsigned short data[32]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			       0,0,0,0,0,0,0,0,0,0,0,0,0};

      int i;
		      
      for (i=0;i<chanCount;i++) {
	if(i>31) continue;

	unsigned short ov  = (*gpoint&0x1000)>>12;
	unsigned short un  = (*gpoint&0x2000)>>13;
	unsigned short dat = (*(gpoint++)&0xfff);
	unsigned short geo = (*gpoint&0xf800)>>11;
	unsigned short chn = (*(gpoint++)&0x1f);
	
	if (geo == GEOaddress) {
	  ///////////////////////////////////////////	
	  ////for CsI 17 && mADC 9 && 10;
	  //if((GEOaddress == 2 && chn<32) || (GEOaddress == 3 && chn<32) || (GEOaddress == 17 && chn<32)) {
	  //for IC GEOaddress 3 && chn ==24 && chn ==28

	  ///////////////////////////////////////////	  
	  ////for PC & IC
	//if((GEOaddress == 2 && chn<32) || (GEOaddress == 3 && chn<16)) {
	//if((GEOaddress == 2 && chn<32) || (GEOaddress == 3 && chn<32)) {
	  if((GEOaddress == 2 && chn<32) || (GEOaddress == 3 && chn<16) || (GEOaddress == 3 && (chn==24 || chn==28))) {
	    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	    //// when you run without CAEN data i.e. Si-Alpha cal in vacuum
	    //// turn this part off
	    if (ADC.Nhits >= MaxCaenHits) {
	      continue;
	    }
	    ADC.ID[ADC.Nhits] = GEOaddress;
            ADC.ChNum[ADC.Nhits] = chn;
	    if (ov) {
	      ADC.Data[ADC.Nhits++] = 5000;
	    } else if (un) {
	      ADC.Data[ADC.Nhits++] = -1000;
	    } else {
	      ADC.Data[ADC.Nhits++] = dat;
	    }	  
	  }

	  ///////////////////////////////////////////
	  //for RF-time & MCPs	  

	  if (GEOaddress == 12 && (chn == 0 || chn == 7)) {
	//if (GEOaddress == 12 && chn<32) {
	    if (TDC.Nhits >= MaxCaenHits) {
	      continue;
	    }
	    TDC.ID[TDC.Nhits] = GEOaddress-10;	   
            TDC.ChNum[TDC.Nhits] = chn;
	    if (chn==7 && dat==1026) {
	      //cout << TDC.Nhits << "  " << ADC.Nhits << "  " << Si.Nhits << endl;
	    }
            TDC.Data[TDC.Nhits++] = dat;
	  }	

	  if (chn<32) data[chn] = dat;   
	  
	}// end of if(geo)	
      }//end of for(chanCount)
      
      unsigned short EOB_l = *(gpoint++);
      unsigned short EOB_h = *(gpoint++);
      unsigned short EOB_bit;
      
      unsigned short geo = (EOB_h&0xf800)>>11;
      EOB_bit = (EOB_h&0x0400)>>10;
      
      if (geo == GEOaddress && EOB_bit) {
	EOB_NEvents = EOB_l+(EOB_h&0x00ff)*65536+1;
      } 

      while ((gpoint < epoint + words )&&(*gpoint==0xffff)) {
	gpoint ++;
      }

      // go to next CAEN data
      fpoint = gpoint;		      
    }    

    epoint += words+1; // This skips the rest of the event
     
    EventCounter++;
    
    DataTree->Fill();   

  }//end for over events
}//end of void ReadPhysicsBuffer()
/////////////////////////////////////////////////////////////////
