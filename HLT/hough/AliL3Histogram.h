// @(#) $Id$

#ifndef ALIL3_HISTOGRAM
#define ALIL3_HISTOGRAM

#include "AliL3RootTypes.h"
#include "AliL3StandardIncludes.h"

#ifdef use_root
#include <TH2.h>
#endif

class AliL3Histogram {
  
 private:
  Double_t fBinwidthX;
  Double_t fBinwidthY;
  
 protected:
  Int_t *fContent; //!
  Char_t fName[100];
  Int_t fNxbins;
  Int_t fNybins;
  Int_t fNcells;
  Int_t fEntries;
  Int_t fFirstXbin;
  Int_t fFirstYbin;
  Int_t fLastXbin;
  Int_t fLastYbin;
  Int_t fThreshold;

  Double_t fXmin;
  Double_t fYmin;
  Double_t fXmax;
  Double_t fYmax;

#ifdef use_root
  TH2F *fRootHisto;
#endif  
  
 public:

  AliL3Histogram();
  AliL3Histogram(Char_t *name,Char_t *id,Int_t nxbin,Double_t xmin,Double_t xmax,Int_t nybin,Double_t ymin,Double_t ymax);
  virtual ~AliL3Histogram();
  
  void Reset();
  virtual void Fill(Double_t x,Double_t y,Int_t weight=1);
  virtual void Fill(Double_t x,Int_t ybin,Int_t weight=1);
  virtual void Fill(Int_t xbin,Double_t y,Int_t weight=1);
  virtual void Fill(Int_t xbin,Int_t ybin,Int_t weight=1);
  virtual Int_t FindBin(Double_t x,Double_t y) const;
  virtual Int_t FindXbin(Double_t x) const;
  virtual Int_t FindYbin(Double_t y) const;
  Int_t GetBin(Int_t xbin,Int_t ybin) const;
  Int_t GetBinContent(Int_t bin) const;
  void SetBinContent(Int_t xbin,Int_t ybin,Int_t value);
  void SetBinContent(Int_t bin,Int_t value);
  void AddBinContent(Int_t xbin,Int_t ybin,Int_t weight);
  void AddBinContent(Int_t bin,Int_t weight);
  void Add(AliL3Histogram *h1,Double_t weight=1);
  void SetThreshold(Int_t i) {fThreshold = i;}
  void CreateRootHisto();
  virtual void Draw(Char_t *option="hist");
  virtual void Print() {};

  friend ofstream& operator<< (ofstream &o, const AliL3Histogram &h);

#ifdef use_root
  TH2F *GetRootHisto();
#else
  void *GetRootHisto();
#endif
    
  Double_t GetXmin() const {return fXmin;}
  Double_t GetXmax() const {return fXmax;}
  Double_t GetYmin() const {return fYmin;}
  Double_t GetYmax() const {return fYmax;}
  virtual Double_t GetBinCenterX(Int_t xbin) const;
  virtual Double_t GetBinCenterY(Int_t ybin) const;
  Double_t GetPreciseBinCenterX(Float_t xbin) const;
  Double_t GetPreciseBinCenterY(Float_t ybin) const;
  Double_t GetBinWidthX() const {return fBinwidthX;}
  Double_t GetBinWidthY() const {return fBinwidthY;}
  Int_t GetFirstXbin() const {return fFirstXbin;}
  Int_t GetLastXbin() const {return fLastXbin;}
  Int_t GetFirstYbin() const {return fFirstYbin;}
  Int_t GetLastYbin() const {return fLastYbin;}
  Int_t GetNbinsX() const {return fNxbins;}
  Int_t GetNbinsY() const {return fNybins;}
  Int_t GetNEntries() const {return fEntries;}
  
  ClassDef(AliL3Histogram,1) //2D histogram class
    
};

#ifdef use_root
inline TH2F *AliL3Histogram::GetRootHisto()
{
  if(!fRootHisto)
    {
      STDCERR<<"AliL3Histogram::GetRootHisto() : You must first Draw histogram before accessing it"<<STDENDL;
      return 0;
    }
  else
    return fRootHisto;
}
#else
inline void *AliL3Histogram::GetRootHisto()
{
  STDCERR<<"AliL3Histogram::GetRootHisto() : You must compile with ROOT in order to interface the ROOT histogram"<<STDENDL;
  return 0;
}
#endif

#endif
