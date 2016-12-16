/////////////////////////////////////////////////////////////////////////////////////
// ROOT script: evt2root_NSCL11.C
//
// Description: Takes .evt files from a list and converts them to ROOT format. 
// There is only one output file named "output.root". The list of evt files 
// is "data_files.list" and its first line is the directory where all the data 
// files are located. This script also needs a library where the detectors 
// classes are defined.
// 
// Adopted & tested for the NSCLDAQ11 version.
//
// to run it: root -l evt2root_NSCL11.C++
// make sure your .evt files are included in the data_files.list
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

using namespace std;

//Detectors' libraries
#include "2015_detclass.h"
//////////////////////////////////////////////////////////////////////////////////////
void ReadPhysicsBuffer();

const int BufferWords = 13328;
const int BufferBytes = BufferWords*2;

int unsigned buflen = 26656;
//char buffer[buflen];
char buffer[26656];

const string files_list = "data_files.list";

// Global variables
TFile* fileR;
TTree* DataTree;
TObjArray* RootObjects;

float CalParamF[128][3];
float CalParamB[128][3];

TH1I* hE1[9][32];
TH1I* hT1[9][32];
TH1I* hE2[9][32];
TH1I* hT2[9][32];
TH1I* hCaenADC[5][32];
TH1I* hCaenTDC[5][32];
TH1I* HitPattern_MB1;
TH1I* HitPattern_MB2;
TH2I* ChanEn_MB1;
TH2I* ChanEn_MB2;
TH2I* ChanT_MB1;
TH2I* ChanT_MB2;

TH1I* CMonTot;
TH1I* CMonLive;

int unsigned Nevents;
int unsigned TotEvents=0;
int unsigned words;  
unsigned short *point,*epoint;

Int_t EventCounter = 0;
int unsigned EOB_NEvents=0;
int unsigned ASICsCounter=0;
int unsigned CAENCounter=0;

//- Detectors' classes --------------------------------------------------------  
ASICHit Si;
CAENHit ADC;
CAENHit TDC; 
////////////////////////////////////////////////////////
//- Main function -------------------------------------------------------------  
int evt2root_NSCL11(){

  gROOT->Reset();

  cout << "==============================================================================" <<endl;
  cout << "evt2root: Takes .evt files from a list and converts the data into ROOT format." <<endl;
  cout << "==============================================================================" <<endl;

  const int unsigned buflen = 26656;
  char buffer[buflen]; 

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
  else{
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
  //DataTree->SetMaxTreeSize(1900000000LL);

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

  DataTree->Branch("TDC.Nhits",&TDC.Nhits,"TDCNhits/I");
  DataTree->Branch("TDC.ID",TDC.ID,"ID[TDCNhits]/I");
  DataTree->Branch("TDC.ChNum",TDC.ChNum,"ChNum[TDCNhits]/I");
  DataTree->Branch("TDC.Data",TDC.Data,"Data[TDCNhits]/I");
  
  // Histograms
  HitPattern_MB1 = new TH1I("HitPattern_MB1","",288,0,288);
  HitPattern_MB2 = new TH1I("HitPattern_MB2","",288,0,288);
  ChanEn_MB1 = new TH2I("EnVsCh_MB1","",288,0,288,4096,0,16384);
  ChanEn_MB2 = new TH2I("EnVsCh_MB2","",288,0,288,4096,0,16384);
  ChanT_MB1 = new TH2I("TiVsCh_MB1","",288,0,288,4096,0,16384);
  ChanT_MB2 = new TH2I("TiVsCh_MB2","",288,0,288,4096,0,16384);
  
  CMonTot  = new TH1I("MonTot","",300,0,300);
  CMonLive = new TH1I("MonLive","",300,0,300);

  //List of root objects.
  RootObjects = new TObjArray();
  RootObjects->Add(DataTree);
  RootObjects->Add(HitPattern_MB1);
  RootObjects->Add(HitPattern_MB2);
  RootObjects->Add(ChanEn_MB1);
  RootObjects->Add(ChanEn_MB2);
  RootObjects->Add(ChanT_MB1);
  RootObjects->Add(ChanT_MB2);
  RootObjects->Add(CMonTot);
  RootObjects->Add(CMonLive);

  string data_dir = "";

  //Check if this file exists.
  if (!ListEVT.is_open()) {
    cout << "*** Error: could not open " << files_list << endl;
    return 0;
  }
  else{
    // cout << "File " << files_list << " opened." <<endl;
    ListEVT >> aux >> aux >> data_dir;
  }

  ifstream evtfile;
  bool fileProblem = 0;
  bool endOfRun = 0;
  cout << "Loop over evt files " <<endl; //debug
 
  int run_number; 
  ListEVT >> run_number;

  //Loop over files in the data file list.
  while(!ListEVT.eof()) {

    if (evtfile.is_open()) cout << "  * Problem previous file not closed!" << endl;

    endOfRun=0;
    fileProblem = 0;    
   
    string name = data_dir + Form("run-%.4d-00.evt", run_number);
    cout << "  Data file: " << name << endl;

    //open evt file
    evtfile.clear();
    evtfile.open(name.c_str(),ios::binary);      

    if (evtfile.bad()) cout << "  ** Bad evt file." << endl;
    if (evtfile.fail()) cout << "  ** Fail evt file" << endl;
    
    if (!evtfile) {
      cout << "  Could not open evt file" << endl;
      //return 1;
    }
    else {
      cout << "  Converting data ..." << endl;
    }

    ////-----------------------------------------------------------------------------
    for (;;) {     
      evtfile.read(buffer,8);
      evtfile.read(buffer+8,*(unsigned int*)buffer-8);

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
      
      switch(type){

      case 11: 
	runNum = *(epoint+8);
	cout << "run number="<< runNum << endl;
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

    ListEVT >> run_number;      
  }

  cout << setprecision(3);
  cout << "Total buffers = " << Nbuffers << endl;
  cout << "  Physics buffers = " << BufferPhysics  << " (" <<100.0*BufferPhysics/Nbuffers<< "\% of total buffers)"<< endl;  
  cout << "Number of events based on buffer headers: " << TotEvents << endl; 
  cout << "Number of events based on event counter: " <<  EventCounter << endl;
    
  RootObjects->Write();
  fileR->Close();	
  
  return 1;

}//end of evt2root

////////////////////////////////////////////////////////////////////////////
// Function where the root objects are filled.
////////////////////////////////////////////////////////////////////////////

void ReadPhysicsBuffer(){

  Nevents = 1;
  TotEvents += Nevents;

  for (unsigned int ievent=0;ievent<Nevents;ievent++) {
    Si.ResetASICHit();
    ADC.ResetCAENHit();
    TDC.ResetCAENHit();

    //create pointer inside of each  event
    unsigned short * fpoint = epoint;
    words = *fpoint++;
    int XLMdata1 = *fpoint++;
  
    if (XLMdata1==0xaaaa) {
      ASICsCounter++;
   
      fpoint +=3 ;
      unsigned short Nstrips = *fpoint;

      //cout << Nstrips << endl ;

      fpoint+=5;

      for (int istrip=0;istrip<Nstrips;istrip++) {
	//cout << "istrip=" << istrip << endl;
	unsigned short *gpoint = fpoint;
	unsigned short id = *gpoint;
	unsigned short chipNum = (id&0x1FE0)>>5;
	unsigned short chanNum = id& 0x1F;
	//cout << "chip=" << chipNum << " ch=" << chanNum;
	gpoint++;
	int energy = *gpoint;
	gpoint++;
	unsigned short time = *gpoint;
	//cout << " e=" << energy << " t=" << time << endl;

	//========================MB1===============================================
	time = time;
	if(chanNum<16){ 
	  if (chipNum == 1 || chipNum == 2 ) energy =16384-energy;
	  if (chipNum == 3 || chipNum == 4 ) energy =energy;				
	  if (chipNum == 5 || chipNum == 6 ) energy =16384-energy;
	  if (chipNum == 7 || chipNum == 8 ) energy =energy;
	  if (chipNum == 9 || chipNum == 10 ) energy =16384-energy;
	  if (chipNum == 11 || chipNum == 12 ) energy =16384-energy;
	  if (chipNum == 13 || chipNum == 14 ) energy =16384-energy;
	  //============================================================================   

	  HitPattern_MB1->Fill(chipNum*16-16+chanNum);
	  ChanEn_MB1->Fill(chipNum*16-16+chanNum,energy);
	  ChanT_MB1->Fill(chipNum*16-16+chanNum,time);
        
          Si.MBID[Si.Nhits]=1;
          Si.CBID[Si.Nhits]=chipNum;
          Si.ChNum[Si.Nhits]=chanNum;
          Si.Energy[Si.Nhits]=energy;
          Si.Time[Si.Nhits++]=time;	    
	}
	else
	  {
	    
	  } 
	fpoint +=3;
      }// end for(isrtip)
    }//end if(XLMdata1)	
  
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	    	     	
    int XLMdata2 = *fpoint++;
    int counter;

    counter = 0;
    while (XLMdata2 != 0xbbbb) {
      XLMdata2 = *fpoint++;
      counter++;
      if (counter>10) break;
    }

    if (XLMdata2==0xbbbb) {              
  
      ASICsCounter++;
      fpoint += 3;

      unsigned short Nstrips = *fpoint;	      
      fpoint+=5;
	      
      for (int istrip=0;istrip<Nstrips;istrip++) {
	unsigned short *gpoint = fpoint;
	unsigned short id = *gpoint;
	unsigned short chipNum = (id&0x1FE0)>>5;
	unsigned short chanNum = id& 0x1F;
	gpoint++;
	unsigned short energy = *gpoint;
	gpoint++;
	unsigned short time = *gpoint;

	//==============================MB2========================================
	time = time;
	if(chanNum<16){ 
	  if (chipNum == 1 || chipNum == 2 ) energy =16384-energy;
	  if (chipNum == 3 || chipNum == 4 ) energy =energy;	
	  if (chipNum == 5 || chipNum == 6 ) energy =16384-energy;
	  if (chipNum == 7 || chipNum == 8 ) energy =energy;
	  if (chipNum == 9 || chipNum == 10 ) energy =16384-energy;			
	  if (chipNum == 11 || chipNum == 12 ) energy =energy;
	  //===========================================================================

	  HitPattern_MB2->Fill(chipNum*16-16+chanNum);
	  ChanEn_MB2->Fill(chipNum*16-16+chanNum,energy);
	  ChanT_MB2->Fill(chipNum*16-16+chanNum,time);
	 
          Si.MBID[Si.Nhits]=2;
          Si.CBID[Si.Nhits]=chipNum;
          Si.ChNum[Si.Nhits]=chanNum;
          Si.Energy[Si.Nhits]=energy;
          Si.Time[Si.Nhits++]=time;
		
	  fpoint +=3;
        }
      }// end second for(istrip)
    }//end if(XMLdata2)
	    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //--- CAEN readout section -------------------------------------------
    //cout << "CAEN" << endl;

    int CAEN = *fpoint++;
	
    counter = 0;
    while (CAEN != 0xcccc) {
      CAEN = *fpoint++;
      counter++;    

      if(fpoint>epoint+words) break;
    }
       	
    if (CAEN==0xcccc) CAENCounter++;
    	    
    while (fpoint < epoint + words){

      if(*fpoint == 0xffff ){

	fpoint++;
	continue;
      }
		  
      unsigned short *gpoint = fpoint;
      unsigned short chanCount = (*(gpoint++) & 0xff00)>>8;
      unsigned short GEOaddress = (*(gpoint++) & 0xf800)>>11; 
      unsigned short data[32]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			       0,0,0,0,0,0,0,0,0,0,0,0,0};

      int i;
		      
      for (i=0;i<chanCount;i++){

	if(i>31) continue;

	unsigned short ov  = (*gpoint&0x1000)>>12;
	unsigned short un  = (*gpoint&0x2000)>>13;
	unsigned short dat = (*(gpoint++)&0xfff);
	unsigned short geo = (*gpoint&0xf800)>>11;
	unsigned short chn = (*(gpoint++)&0x1f);
	
	if (geo == GEOaddress) {

	  ///////////////////////////////////////////	
	  ////for CsI 17 && mADC 9 && 10;
	  //if((GEOaddress == 2 && chn<32) || (GEOaddress == 3 && chn<32) || (GEOaddress == 17 && chn<32)){
	  //for IC GEOaddress 3 && chn ==24 && chn ==28

	  ///////////////////////////////////////////	  
	  ////for PC & IC
	//if((GEOaddress == 2 && chn<32) || (GEOaddress == 3 && chn<16)){
	  if((GEOaddress == 2 && chn<32) || (GEOaddress == 3 && chn<32)){

            ADC.ID[ADC.Nhits] = GEOaddress;
            ADC.ChNum[ADC.Nhits] = chn;
	    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	    //// when you run without CAEN data i.e. Si-Alpha cal in vacuum
	    //// turn this part off
	    if (ADC.Nhits >= MaxCaenHits) {
	      continue;
	    }
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

	//if (GEOaddress == 12 && (chn == 0 || chn == 7)) {
	  if (GEOaddress == 12 && chn<32) {
	    if (TDC.Nhits >= MaxCaenHits){
	      continue;
	    }
	    TDC.ID[TDC.Nhits] = GEOaddress-10;	   
            TDC.ChNum[TDC.Nhits] = chn;
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

      while ((gpoint < epoint + words )&&(*gpoint==0xffff)){
	gpoint ++;
      }

      // go to next CAEN data
      fpoint = gpoint;		      
    }
		    
    epoint += words+1; // This skips the rest of the event	    
 
    EventCounter++;
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if ( (ADC.Nhits >= MaxCaenHits) || (TDC.Nhits >= MaxCaenHits) || (Si.Nhits >= MaxHits) ){
      TDC.ResetCAENHit();
      ADC.ResetCAENHit();
      Si.ResetASICHit();
    }
    DataTree->Fill();

  }//end for over events
}//end of void ReadPhysicsBuffer()
/////////////////////////////////////////////////////////////////
