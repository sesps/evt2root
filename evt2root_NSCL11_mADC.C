/////////////////////////////////////////////////////////////////////////////////////
// ROOT script: evt2root_NSCL11_mADC.C
// See readme.md for general instructions.
// Adopted & tested for the NSCLDAQ11 version.
//
// to run it: 
//  root -l
//  .x VM_BaseClass.cpp+
//  .L VM_Module.cpp+
//  .L SimpleInPipe.cpp+
//  .x evt2root_NSCL11.C+
//
// make sure your .evt files are included in the evt_files.lst
// 
// Nabin, ddc, DSG, KTM et.al. // December 2015.
//
// It takes all types of Modules in the same way & unpacks it.. 
// Adapted for MesyTech ADC Unpacking
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
#include <TH1.h>
#include <TCutG.h>
#include <TDirectory.h>
#include <TBrowser.h>
#include <TThread.h>
#include <TStyle.h>
#include <math.h>
#include <TGFrame.h>
#include <unistd.h>
#include <TGlobal.h> 

//Detectors' libraries
#include "2015_detclass.h"
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

VMUSBMARK* marka = new VMUSBMARK("Mark A",0xaaaa);
CHINP* chinp1 = new CHINP(1024,"hinp1", 8);

VMUSBMARK* markb = new VMUSBMARK("Mark B",0xbbbb);
CHINP* chinp2 = new CHINP(1024,"hinp2", 16);

VMUSBMARK* markc = new VMUSBMARK("Mark C",0xcccc);
CAEN_ADC* caen_adc1 = new CAEN_ADC("First ADC", 2);
CAEN_ADC* caen_adc2 = new CAEN_ADC("Second ADC", 3);
MESY_ADC* mesy_adc1 = new MESY_ADC("First MADC", 9);
MESY_ADC* mesy_adc2 = new MESY_ADC("Second MADC", 10);
CAEN_TDC* caen_tdc1 = new CAEN_TDC("First TDC", 12);
CAEN_ADC* caen_adc3 = new CAEN_ADC("Third ADC", 17);

float CalParamF[128][3];
float CalParamB[128][3];

TH1I* HitPattern_MB1;
TH1I* HitPattern_MB2;
TH2I* ChanEn_MB1;
TH2I* ChanEn_MB2;
TH2I* ChanT_MB1;
TH2I* ChanT_MB2;

int unsigned Nevents;
int unsigned TotEvents=0;
int unsigned words;  
unsigned short *point,*epoint;

Int_t EventCounter = 0;
int unsigned EOB_NEvents=0;
int unsigned ASICsCounter=0;
int unsigned CAENCounter=0;

//TH1I* hCaenADC[3][32];
//TH1I* hCaenTDC[1][32];
//TH1I* hMADC[2][32];

TH2I* CsI_vs_Chan_CAEN;
TH2I* CsI_vs_Chan_MESY1;
TH2I* CsI_vs_Chan_MESY2;
TH2I* PC_vs_Chan1;
TH2I* PC_vs_Chan2;
TH2I* TDC_vs_Chan;

//- Detectors' classes --------------------------------------------------------  
ASICHit Si;
CAENHit ADC;
CAENHit TDC;
MesyHit mADC;

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

  DataTree->Branch("Si.Nhits",&Si.Nhits,"SiNhits/I");
  DataTree->Branch("Si.MBID",Si.MBID,"MBID[SiNhits]/I");
  DataTree->Branch("Si.CBID",Si.CBID,"CBID[SiNhits]/I");
  DataTree->Branch("Si.ChNum",Si.ChNum,"ChNum[SiNhits]/I");
  DataTree->Branch("Si.Energy",Si.Energy,"Energy[SiNhits]/I");
  DataTree->Branch("Si.Time",Si.Time,"Time[SiNhits]/I");

  DataTree->Branch("ADC.Nhits",&ADC.Nhits,"ADCNhits/I");
  DataTree->Branch("ADC.ID",ADC.ID,"ID[ADCNhits]/I");
  DataTree->Branch("ADC.ChNum",ADC.ChNum,"ChNum[ADCNhits]/I");
  DataTree->Branch("ADC.Data",ADC.Data,"Data[ADCNhits]/I");

  DataTree->Branch("mADC.Nhits",&mADC.Nhits,"mADCNhits/I");
  DataTree->Branch("mADC.ID",mADC.ID,"ID[mADCNhits]/I");
  DataTree->Branch("mADC.ChNum",mADC.ChNum,"ChNum[mADCNhits]/I");
  DataTree->Branch("mADC.Data",mADC.Data,"Data[mADCNhits]/I");

  DataTree->Branch("TDC.Nhits",&TDC.Nhits,"TDCNhits/I");
  DataTree->Branch("TDC.ID",TDC.ID,"ID[TDCNhits]/I");
  DataTree->Branch("TDC.ChNum",TDC.ChNum,"ChNum[TDCNhits]/I");
  DataTree->Branch("TDC.Data",TDC.Data,"Data[TDCNhits]/I");
  
  // Histograms
  Int_t xbins=288;
  Int_t ybins=4096;
  HitPattern_MB1 = new TH1I("HitPattern_MB1","",xbins,0,xbins);
  HitPattern_MB2 = new TH1I("HitPattern_MB2","",xbins,0,xbins);
  ChanEn_MB1 = new TH2I("EnVsCh_MB1","",xbins,0,xbins,ybins,0,4*ybins);
  ChanEn_MB2 = new TH2I("EnVsCh_MB2","",xbins,0,xbins,ybins,0,4*ybins);
  ChanT_MB1 = new TH2I("TiVsCh_MB1","",xbins,0,xbins,ybins,0,4*ybins);
  ChanT_MB2 = new TH2I("TiVsCh_MB2","",xbins,0,xbins,ybins,0,4*ybins);

  xbins=33;
  ybins=1024;
  CsI_vs_Chan_CAEN = new TH2I("CsI_vs_Chan_CAEN","",xbins,0,xbins,ybins,0,4*ybins);
  CsI_vs_Chan_MESY1 = new TH2I("CsI_vs_Chan_MESY1","",xbins,0,xbins,ybins,0,4*ybins);
  CsI_vs_Chan_MESY2 = new TH2I("CsI_vs_Chan_MESY2","",xbins,0,xbins,ybins,0,4*ybins);
  PC_vs_Chan1 = new TH2I("PC_vs_Chan1","",xbins,0,xbins,ybins,0,4*ybins);
  PC_vs_Chan2 = new TH2I("PC_vs_Chan2","",xbins,0,xbins,ybins,0,4*ybins);
  TDC_vs_Chan = new TH2I("TDC_vs_Chan","",xbins,0,xbins,ybins,0,4*ybins);

  //List of root objects.
  RootObjects = new TObjArray();
  RootObjects->Add(DataTree);
  RootObjects->Add(HitPattern_MB1);
  RootObjects->Add(HitPattern_MB2);
  RootObjects->Add(ChanEn_MB1);
  RootObjects->Add(ChanEn_MB2);
  RootObjects->Add(ChanT_MB1);
  RootObjects->Add(ChanT_MB2);

  RootObjects->Add(CsI_vs_Chan_CAEN);
  RootObjects->Add(CsI_vs_Chan_MESY1);
  RootObjects->Add(CsI_vs_Chan_MESY2);
  RootObjects->Add(PC_vs_Chan1);
  RootObjects->Add(PC_vs_Chan2);
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
    Si.ResetASICHit();
    ADC.ResetCAENHit();
    TDC.ResetCAENHit();
    mADC.ResetMesyHit();

    //create pointer inside of each  event
    unsigned short * fpoint = epoint;		    
    words = *fpoint++;  
    marka->Unpack(fpoint);
    int XLMdata1 = (int) marka->fChValue[0];
    chinp1->Reset();

    if (XLMdata1==0xaaaa) {
      if(!chinp1->Unpack(fpoint))break;
    
      unsigned short Nstrips = chinp1->hits.size();    

      for (int istrip=0;istrip<Nstrips;istrip++) {
	unsigned short id = chinp1->hits[istrip];
	unsigned short chipNum = (id/2)/16 + 1;
	unsigned short chanNum = (id/2)%16;
	unsigned short energy = (int) chinp1->fChValue[id];
	unsigned short time = (int) chinp1->fChValue[id+1];
	
	//time = time;
	if(chanNum<16) { 	   
	  HitPattern_MB1->Fill(chipNum*16-16+chanNum);
	  ChanEn_MB1->Fill(chipNum*16-16+chanNum,energy);
	  ChanT_MB1->Fill(chipNum*16-16+chanNum,time);
        
          Si.MBID[Si.Nhits]=1;
          Si.CBID[Si.Nhits]=chipNum;
          Si.ChNum[Si.Nhits]=chanNum;
          Si.Energy[Si.Nhits]=energy;
          Si.Time[Si.Nhits++]=time;	    
	}
	else {
	    //No problem
	  }
      }//end of for (istrip)
    }//end of if (XLMdata1) 

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	    	     	
    
    // Second XLM readout if present   
    markb->Unpack(fpoint);
    int XLMdata2 = (int) markb->fChValue[0];

    //Not clear that this is (still?) required...//ddc
    //Prevents from breaking at the bad event buffers....like (wordCount >4095) ??//NR
    while (XLMdata2 != 0xbbbb) {
      XLMdata2 = *fpoint++;
      if(fpoint>epoint+words) break;
    }

    chinp2->Reset();
    if (XLMdata2==0xbbbb) {
      if(!chinp2->Unpack(fpoint)) break;

      unsigned short Nstrips = chinp2->hits.size();		 

      for (int istrip=0;istrip<Nstrips;istrip++) {
	  unsigned short id = chinp2->hits[istrip];
	  unsigned short chipNum = (id/2)/16 + 1;
	  unsigned short chanNum = (id/2)%16;
	  unsigned short energy = (int) chinp2->fChValue[id];
	  unsigned short time = (int) chinp2->fChValue[id+1];

	  //time = time;
	  if(chanNum<16) { 	    
	    HitPattern_MB2->Fill(chipNum*16-16+chanNum);
	    ChanEn_MB2->Fill(chipNum*16-16+chanNum,energy);
	    ChanT_MB2->Fill(chipNum*16-16+chanNum,time);
	    
	    Si.MBID[Si.Nhits]=2;
	    Si.CBID[Si.Nhits]=chipNum;
	    Si.ChNum[Si.Nhits]=chanNum;
	    Si.Energy[Si.Nhits]=energy;
	    Si.Time[Si.Nhits++]=time;
	  }	
	}// end second for(istrip)
    }//end if(XMLdata2)
	    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //--- CAEN readout section -------------------------------------------
    //cout << "CAEN" << endl;
 
    markc->Unpack(fpoint);
    int CAEN = (int) markc->fChValue[0];
		
    while (CAEN != 0xcccc) {
      CAEN = *fpoint++;
      if(fpoint>epoint+words) break;
    }
	
    caen_adc1->Reset();
    caen_adc2->Reset();
    caen_adc3->Reset();
    mesy_adc1->Reset();
    mesy_adc2->Reset();
    caen_tdc1->Reset();

    caen_adc1->Unpack(fpoint); if(fpoint>epoint + words + 1) break;
    caen_adc2->Unpack(fpoint); if(fpoint>epoint + words + 1) break;
    caen_adc3->Unpack(fpoint); if(fpoint>epoint + words + 1) break;
    mesy_adc1->Unpack(fpoint); if(fpoint>epoint + words + 1) break;
    mesy_adc2->Unpack(fpoint); if(fpoint>epoint + words + 1) break;
    caen_tdc1->Unpack(fpoint); if(fpoint>epoint + words + 1) break;    

    epoint += words+1; // This skips the rest of the event
    ///////////////////////////////////////////////////////////////////////////////////  
    //---------------------------------------------------
    for(int i=0;i<32;i++) {
      ADC.ID[ADC.Nhits] = 2;
      ADC.ChNum[ADC.Nhits] =i;
      ADC.Data[ADC.Nhits++] = (Int_t) caen_adc1->fChValue[i];
      //hCaenADC[0][i]->Fill(caen_adc1->fChValue[i]);
      PC_vs_Chan1->Fill(i,caen_adc1->fChValue[i]);
    }    

    for(int i=0;i<16;i++) {//
      ADC.ID[ADC.Nhits] = 3;
      ADC.ChNum[ADC.Nhits] =i;
      ADC.Data[ADC.Nhits++] = (Int_t) caen_adc2->fChValue[i];   
      //hCaenADC[1][i]->Fill(caen_adc2->fChValue[i]);  
      PC_vs_Chan2->Fill(i,caen_adc2->fChValue[i]);
    }
    //---------------------------------------------------
    for(int i=0;i<32;i++) {  
      mADC.ID[mADC.Nhits] = 1;
      mADC.ChNum[mADC.Nhits] =i;
      mADC.Data[mADC.Nhits++] = (Int_t) mesy_adc1->fChValue[i];
      //hMADC[0][i]->Fill(mesy_adc1->fChValue[i]);
      CsI_vs_Chan_MESY1->Fill(i,mesy_adc1->fChValue[i]);     
    }  

    for(int i=0;i<32;i++) {
      mADC.ID[mADC.Nhits] = 2;
      mADC.ChNum[mADC.Nhits] =i;
      mADC.Data[mADC.Nhits++] = (Int_t) caen_adc3->fChValue[i];  
      //hCaenADC[2][i]->Fill(caen_adc3->fChValue[i]);
      CsI_vs_Chan_CAEN->Fill(i,caen_adc3->fChValue[i]);
    }   
    //---------------------------------------------------
    for(int i=0;i<8;i++) {
      if( i==0 || i ==7) {
	TDC.ID[TDC.Nhits] = 12;
	TDC.ChNum[TDC.Nhits] =i;
	TDC.Data[TDC.Nhits++] = (Int_t) caen_tdc1->fChValue[i];   
	//hCaenTDC[0][i]->Fill(caen_tdc1->fChValue[i]);
	TDC_vs_Chan->Fill(i,caen_tdc1->fChValue[i]);
      }	
    }	    
 
    EventCounter++;
    
    DataTree->Fill();   

  }//end for over events
}//end of void ReadPhysicsBuffer()
/////////////////////////////////////////////////////////////////
