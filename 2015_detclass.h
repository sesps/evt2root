////////////////////////////////////////////////////////
// Nabin Rijal - December 2015.
////////////////////////////////////////////////////////
using namespace std;

#define MaxHits 416
#define MaxCaenHits 500
#define MaxMesyHits 200
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
class ASICHit {
// This class is for ASICs hit
public:
	ASICHit(){};
	Int_t Nhits,MBID[MaxHits],CBID[MaxHits],ChNum[MaxHits];
        Int_t Energy[MaxHits];
        Int_t Time[MaxHits];

        void ResetASICHit()
	{
	   Nhits = 0;

           for (Int_t i=0; i<MaxHits;i++) {
              MBID[i] = 0;
              CBID[i] = 0;
              ChNum[i] = 0;
              Energy[i] = 0;
              Time[i]  = 0;
           } 	
        };
};
////////////////////////////////////////////////////////
class CAENHit {
//This class is for CAEN hit
public:
	CAENHit(){};
	Int_t Nhits,ID[MaxCaenHits],ChNum[MaxCaenHits];
        Int_t Data[MaxCaenHits];

        void ResetCAENHit()
	{
	    Nhits = 0;

            for (Int_t i=0; i<MaxCaenHits;i++) {
              ID[i] = 0;
              ChNum[i] = 0;
              Data[i]  = 0;
           }
         };
};
////////////////////////////////////////////////////////
class MesyHit {
//This class is for MesyTech hit
public:
	MesyHit(){};
	Int_t Nhits,ID[MaxMesyHits],ChNum[MaxMesyHits];
        Int_t Data[MaxMesyHits];

        void ResetMesyHit()
	{
	    Nhits = 0;

            for (Int_t i=0; i<MaxMesyHits;i++) {
              ID[i] = 0;
              ChNum[i] = 0;
              Data[i]  = 0;
           }
         };
};
////////////////////////////////////////////////////////
