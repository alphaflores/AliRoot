/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/
/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <iomanip.h>
#include <TObjArray.h>
#include <TRandom.h>
#include <TMath.h>

#include "AliITSmodule.h"
#include "AliITSMapA2.h"
#include "AliITSpList.h"
#include "AliITSresponseSSD.h"
#include "AliITSsegmentationSSD.h"
#include "AliITSdcsSSD.h"
#include "AliITS.h"
#include "AliRun.h"
#include "AliITSgeom.h"
#include "AliITSsimulationSSD.h"

ClassImp(AliITSsimulationSSD);
////////////////////////////////////////////////////////////////////////
// Version: 0
// Written by Enrico Fragiacomo
// July 2000
//
// AliITSsimulationSSD is the simulation of SSDs.

//----------------------------------------------------------------------
AliITSsimulationSSD::AliITSsimulationSSD(){
    //default Constructor

    fDCS     = 0;
    fDifConst[0] = fDifConst[1] = 0.0;
    fDriftVel[0] = fDriftVel[1] = 0.0;
    fMapA2   = 0;
}
//----------------------------------------------------------------------
AliITSsimulationSSD::AliITSsimulationSSD(AliITSsegmentation *seg,
                                         AliITSresponse *resp){
    // Constructor

    fDCS     = 0;
    fDifConst[0] = fDifConst[1] = 0.0;
    fDriftVel[0] = fDriftVel[1] = 0.0;
    fMapA2   = 0;
    Init((AliITSsegmentationSSD*)seg,(AliITSresponseSSD*)resp);
}
//----------------------------------------------------------------------
void AliITSsimulationSSD::Init(AliITSsegmentationSSD *seg,
			       AliITSresponseSSD *resp){
    // Constructor

    fSegmentation    = seg;
    fResponse        = resp;
    Float_t noise[2] = {0.,0.};
    fResponse->GetNoiseParam(noise[0],noise[1]); // retrieves noise parameters
    fDCS             = new AliITSdcsSSD(seg,resp); 

    SetDriftVelocity(); // use default values in .h file
    SetIonizeE();       // use default values in .h file
    SetDiffConst();     // use default values in .h file
    fMapA2           = new AliITSMapA2(fSegmentation);
}
//______________________________________________________________________
AliITSsimulationSSD& AliITSsimulationSSD::operator=(
                                                const AliITSsimulationSSD &s){
    // Operator =

    if(this==&s) return *this;

    this->fDCS         = new AliITSdcsSSD(*(s.fDCS));
    this->fMapA2       = s.fMapA2;
    this->fIonE        = s.fIonE;
    this->fDifConst[0] = s.fDifConst[0];
    this->fDifConst[1] = s.fDifConst[1];
    this->fDriftVel[0] = s.fDriftVel[0];
    this->fDriftVel[1] = s.fDriftVel[1];
    return *this;
}
//______________________________________________________________________
AliITSsimulationSSD::AliITSsimulationSSD(const AliITSsimulationSSD &source){
    // copy constructor

    *this = source;
}
//______________________________________________________________________
AliITSsimulationSSD::~AliITSsimulationSSD() {
    // destructor
    delete fMapA2;
    delete fDCS;
}
//______________________________________________________________________
void AliITSsimulationSSD::DigitiseModule(AliITSmodule *mod,
					 Int_t dummy0,Int_t dummy1) {
    // Digitizes hits for one SSD module
    Int_t module     = mod->GetIndex();
    AliITSpList *pList = new AliITSpList(2,GetNStrips());

    HitsToAnalogDigits(mod,pList);
    SDigitToDigit(module,pList);

    delete pList;
    fMapA2->ClearMap();
}
//______________________________________________________________________
void AliITSsimulationSSD::SDigitiseModule(AliITSmodule *mod,Int_t dummy0,
					  Int_t dummy1) {
    // Produces Summable/Analog digits and writes them to the SDigit tree.
    AliITSpList *pList = new AliITSpList(2,GetNStrips()); 

    HitsToAnalogDigits(mod,pList);

    WriteSDigits(pList);

    delete pList;
    fMapA2->ClearMap();
}
//______________________________________________________________________
void AliITSsimulationSSD::SDigitToDigit(Int_t module,AliITSpList *pList){
    // Takes the pList and finishes the digitization.

    FillMapFrompList(pList);

    ApplyNoise(pList,module);
    ApplyCoupling(pList,module);

    ChargeToSignal(pList);
}
//______________________________________________________________________
void AliITSsimulationSSD::HitsToAnalogDigits(AliITSmodule *mod,
					     AliITSpList *pList){
    // Loops over all hits to produce Analog/floating point digits. This
    // is also the first task in producing standard digits.
    Int_t indexRange[4] = {0,0,0,0};
    static Bool_t first = kTRUE;
    Int_t lasttrack     = -2;
    Int_t idtrack       = -2;
    Double_t x0=0.0, y0=0.0, z0=0.0;
    Double_t x1=0.0, y1=0.0, z1=0.0;
    Double_t de=0.0;
    Int_t module = mod->GetIndex();

    TObjArray *hits = mod->GetHits();
    Int_t nhits     = hits->GetEntriesFast();
    if (nhits<=0) return;

    module = mod->GetIndex();
    if ( mod->GetLayer() == 6 ) GetSegmentation()->SetLayer(6);
    if ( mod->GetLayer() == 5 ) GetSegmentation()->SetLayer(5);

    for(Int_t i=0; i<nhits; i++) {    
	// LineSegmentL returns 0 if the hit is entering
	// If hits is exiting returns positions of entering and exiting hits
	// Returns also energy loss

	if (mod->LineSegmentL(i, x0, x1, y0, y1, z0, z1, de, idtrack)) {
	    HitToDigit(module, x0, y0, z0, x1, y1, z1, de, indexRange, first);
       
	    if (lasttrack != idtrack || i==(nhits-1)) {
		GetList(idtrack,i,module,pList,indexRange);
		first=kTRUE;
	    } // end if
	    lasttrack=idtrack;
	} // end if
    }  // end loop over hits
    return;
}
//----------------------------------------------------------------------
void AliITSsimulationSSD::HitToDigit(Int_t module, Double_t x0, Double_t y0, 
				     Double_t z0, Double_t x1, Double_t y1, 
				     Double_t z1, Double_t de,
				     Int_t *indexRange, Bool_t first) {
    // Turns hits in SSD module into one or more digits.

    Float_t tang[2] = {0.0,0.0};
    GetSegmentation()->Angles(tang[0], tang[1]);//stereo<<->tan(stereo)~=stereo
    Double_t x, y, z;
    Double_t dex=0.0, dey=0.0, dez=0.0;
    Double_t pairs; // pair generation energy per step.
    Double_t sigma[2] = {0.,0.};// standard deviation of the diffusion gaussian
    Double_t tdrift[2] = {0.,0.}; // time of drift
    Double_t w;
    Double_t inf[2], sup[2], par0[2];                 

    // Steps in the module are determined "manually" (i.e. No Geant)
    // NumOfSteps divide path between entering and exiting hits in steps 
    Int_t numOfSteps = NumOfSteps(x1, y1, z1, dex, dey, dez);
  
    // Enery loss is equally distributed among steps
    de    = de/numOfSteps;
    pairs = de/GetIonizeE(); // e-h pairs generated

    for(Int_t j=0; j<numOfSteps; j++) {     // stepping
	x = x0 + (j+0.5)*dex;
	y = y0 + (j+0.5)*dey;
	if ( y > (GetSegmentation()->Dy()/2+10)*1.0E-4 ) {
	    // check if particle is within the detector
	    cout<<"AliITSsimulationSSD::HitToDigit: Warning: hit "
		"out of detector y0,y,dey,j ="
		<<y0<<","<<y<<","<<dey<<","<<j<<endl;
	    return;
	} // end if
	z = z0 + (j+0.5)*dez;

	// calculate drift time
	// y is the minimum path
	tdrift[0] = (y+(GetSegmentation()->Dy()*1.0E-4)/2)/GetDriftVelocity(0);
	tdrift[1] = ((GetSegmentation()->Dy()*1.0E-4)/2-y)/GetDriftVelocity(1);

	for(Int_t k=0; k<2; k++) {   // both sides    remember: 0=Pside 1=Nside

	    tang[k]=TMath::Tan(tang[k]);

	    // w is the coord. perpendicular to the strips
	    if(k==0) {
		w = (x+(GetSegmentation()->Dx()*1.0E-4)/2) -
		    (z+(GetSegmentation()->Dz()*1.0E-4)/2)*tang[k]; 
	    }else{
		w = (x+(GetSegmentation()->Dx()*1.0E-4)/2) + 
		    (z-(GetSegmentation()->Dz()*1.0E-4)/2)*tang[k];
	    } // end if
	    w /= (GetStripPitch()*1.0E-4); // w is converted in units of pitch

	    if((w<(-0.5)) || (w>(GetNStrips()-0.5))) {
		// this check rejects hits in regions not covered by strips
		// 0.5 takes into account boundaries 
		if(k==0) cout<<"AliITSsimulationSSD::HitToDigit: "
			     "Warning: no strip in this region of P side"
			     <<endl;
		else cout<<"AliITSsimulationSSD::HitToDigit: "
			 "Warning: no strip in this region of N side"<<endl;
		return;
	    } // end if

	    // sigma is the standard deviation of the diffusion gaussian
	    if(tdrift[k]<0) return;
	    sigma[k] = TMath::Sqrt(2*GetDiffConst(k)*tdrift[k]);
	    sigma[k] /= (GetStripPitch()*1.0E-4);  //units of Pitch
	    if(sigma[k]==0.0) { 	
		cout<<"AliITSsimulationSSD::DigitiseModule: Error: sigma=0"
		    <<endl; 
		exit(0);
	    } // end if

	    par0[k] = pairs;
	    // we integrate the diffusion gaussian from -3sigma to 3sigma 
	    inf[k] = w - 3*sigma[k]; // 3 sigma from the gaussian average  
	    sup[k] = w + 3*sigma[k]; // 3 sigma from the gaussian average
	                             // IntegrateGaussian does the actual
                                     // integration of diffusion gaussian
	    IntegrateGaussian(k, par0[k], w, sigma[k], inf[k], sup[k], 
			      indexRange, first);
	}  // end for loop over side (0=Pside, 1=Nside)      
    } // end stepping
    //delete seg;
}
//______________________________________________________________________
void AliITSsimulationSSD::ApplyNoise(AliITSpList *pList,Int_t module){
    // Apply Noise.
    Int_t    k,ix;
    Double_t signal,noise;
    Double_t noiseP[2] = {0.,0.};
    Float_t a,b;

    fResponse->GetNoiseParam(a,b); // retrieves noise parameters
    noiseP[0] = (Double_t) a; noiseP[1] = (Double_t) b;
    for(k=0;k<2;k++){                    // both sides (0=Pside, 1=Nside)
	for(ix=0;ix<GetNStrips();ix++){      // loop over strips
	    noise  = gRandom->Gaus(0,noiseP[k]);// get noise to signal
	    signal = noise + fMapA2->GetSignal(k,ix);//get signal from map
	    if(signal<0.) signal=0.0;           // in case noise is negative...
	    fMapA2->SetHit(k,ix,signal); // give back signal to map
	    if(signal>0.0) pList->AddNoise(k,ix,module,noise);
	} // loop over strip 
    } // loop over k (P or N side)
}
//______________________________________________________________________
void AliITSsimulationSSD::ApplyCoupling(AliITSpList *pList,Int_t module) {
    // Apply the effect of electronic coupling between channels
    Int_t ix;
    Double_t signalLeft=0, signalRight=0,signal=0;

    for(ix=0;ix<GetNStrips();ix++){
	// P side coupling
	if(ix>0.)signalLeft = fMapA2->GetSignal(0,ix-1)*fDCS->GetCouplingPL();
	else signalLeft = 0.0;
	if(ix<(GetNStrips()-1)) signalRight = fMapA2->GetSignal(0,ix+1)*
                                                        fDCS->GetCouplingPR();
	else signalRight = 0.0;
	signal = signalLeft + signalRight;
	fMapA2->AddSignal(0,ix,signal);
	if(signal>0.0) pList->AddNoise(0,ix,module,signal);

	signalLeft = signalRight = signal = 0.0;
	// N side coupling
	if(ix>0.) signalLeft = fMapA2->GetSignal(1,ix-1)*fDCS->GetCouplingNL();
	else signalLeft = 0.0;
	if(ix<(GetNStrips()-1)) signalRight = fMapA2->GetSignal(1,ix+1)*
                                                         fDCS->GetCouplingNR();
	else signalRight = 0.0;
	signal = signalLeft + signalRight;
	fMapA2->AddSignal(1,ix,signal);
	if(signal>0.0) pList->AddNoise(1,ix,module,signal);
    } // loop over strips 
}
//______________________________________________________________________
Float_t AliITSsimulationSSD::F(Float_t av, Float_t x, Float_t s) {
    // Computes the integral of a gaussian using Error Function
    Float_t sqrt2 = TMath::Sqrt(2.0);
    Float_t sigm2 = sqrt2*s;
    Float_t integral;

    integral = 0.5 * TMath::Erf( (x - av) / sigm2);
    return integral;
}
//______________________________________________________________________
void AliITSsimulationSSD::IntegrateGaussian(Int_t k,Double_t par, Double_t w,
					    Double_t sigma, 
					    Double_t inf, Double_t sup,
					    Int_t *indexRange, Bool_t first) {
    // integrate the diffusion gaussian
    // remind: inf and sup are w-3sigma and w+3sigma
    //         we could define them here instead of passing them
    //         this way we are free to introduce asimmetry

    Double_t a=0.0, b=0.0;
    Double_t dXCharge1 = 0.0, dXCharge2 = 0.0;
    // dXCharge1 and 2 are the charge to two neighbouring strips
    // Watch that we only involve at least two strips
    // Numbers greater than 2 of strips in a cluster depend on
    //  geometry of the track and delta rays, not charge diffusion!   
  
    Double_t strip = TMath::Floor(w);         // clostest strip on the left

    if ( TMath::Abs((strip - w)) < 0.5) { 
	// gaussian mean is closer to strip on the left
	a = inf;                         // integration starting point
	if((strip+0.5)<=sup) {
	    // this means that the tail of the gaussian goes beyond
	    // the middle point between strips ---> part of the signal
	    // is given to the strip on the right
	    b = strip + 0.5;               // integration stopping point
	    dXCharge1 = F( w, b, sigma) - F(w, a, sigma);
	    dXCharge2 = F( w, sup, sigma) - F(w ,b, sigma); 
	}else { 
	    // this means that all the charge is given to the strip on the left
	    b = sup;
	    dXCharge1 = 0.9973;   // gaussian integral at 3 sigmas
	    dXCharge2 = 0.0;
	} // end if

	dXCharge1 = par * dXCharge1;// normalize by mean of number of carriers
	dXCharge2 = par * dXCharge2;

	// for the time being, signal is the charge
	// in ChargeToSignal signal is converted in ADC channel
	fMapA2->AddSignal(k,strip,dXCharge1);
	if(((Int_t) strip) < (GetNStrips()-1)) {
	    // strip doesn't have to be the last (remind: last=GetNStrips()-1)
	    // otherwise part of the charge is lost
	    fMapA2->AddSignal(k,(strip+1),dXCharge2);
	} // end if
    
	if(dXCharge1 > 1.) {
	    if (first) {
		indexRange[k*2+0] = indexRange[k*2+1]=(Int_t) strip;
		first=kFALSE;
	    } // end if first

	    indexRange[k*2+0]=TMath::Min(indexRange[k*2+0],(Int_t) strip);
	    indexRange[k*2+1]=TMath::Max(indexRange[k*2+1],(Int_t) strip);
	}      // dXCharge > 1 e-

    }else{
	// gaussian mean is closer to strip on the right
	strip++;     // move to strip on the rigth
	b = sup;     // now you know where to stop integrating
	if((strip-0.5)>=inf) { 
	    // tail of diffusion gaussian on the left goes left of
	    // middle point between strips
	    a = strip - 0.5;        // integration starting point
	    dXCharge1 = F(w, b, sigma) - F(w, a, sigma);
	    dXCharge2 = F(w, a, sigma) - F(w, inf, sigma);
	}else {
	    a = inf;
	    dXCharge1 = 0.9973;   // gaussian integral at 3 sigmas
	    dXCharge2 = 0.0;
	} // end if
    
	dXCharge1 = par * dXCharge1;    // normalize by means of carriers
	dXCharge2 = par * dXCharge2;

	// for the time being, signal is the charge
	// in ChargeToSignal signal is converted in ADC channel
	fMapA2->AddSignal(k,strip,dXCharge1);
	if(((Int_t) strip) > 0) {
	    // strip doesn't have to be the first
	    // otherwise part of the charge is lost
	    fMapA2->AddSignal(k,(strip-1),dXCharge2);
	} // end if
    
	if(dXCharge1 > 1.) {
	    if (first) {
		indexRange[k*2+0]=indexRange[k*2+1]=(Int_t) strip;
		first=kFALSE;
	    } // end if first

	    indexRange[k*2+0]=TMath::Min(indexRange[k*2+0],(Int_t) strip);
	    indexRange[k*2+1]=TMath::Max(indexRange[k*2+1],(Int_t) strip);
	}      // dXCharge > 1 e-
    } // end if
}
//______________________________________________________________________
Int_t AliITSsimulationSSD::NumOfSteps(Double_t x, Double_t y, Double_t z,
				 Double_t & dex,Double_t & dey,Double_t & dez){
    // number of steps
    // it also returns steps for each coord
    //AliITSsegmentationSSD *seg = new AliITSsegmentationSSD();

    Double_t step = 25E-4;
    //step = (Double_t) seg->GetStepSize();  // step size (cm)
    Int_t numOfSteps = (Int_t) (TMath::Sqrt(x*x+y*y+z*z)/step); 

    if (numOfSteps < 1) numOfSteps = 1;       // one step, at least

    // we could condition the stepping depending on the incident angle
    // of the track
    dex = x/numOfSteps;
    dey = y/numOfSteps;
    dez = z/numOfSteps;

    return numOfSteps;
}
//----------------------------------------------------------------------
void AliITSsimulationSSD::GetList(Int_t label,Int_t hit,Int_t mod,
				  AliITSpList *pList,Int_t *indexRange) {
    // loop over nonzero digits
    Int_t ix,i;
    Double_t signal=0.;

    for(Int_t k=0; k<2; k++) {
	for(ix=indexRange[k*2+0];ix<indexRange[k*2+1]+1;ix++){
//	    if(indexRange[k*2+0]<indexRange[k*2+1]) 
		signal = fMapA2->GetSignal(k,ix);
		if(signal==0.0) continue;
		// check the signal magnitude
		for(i=0;i<pList->GetNSignals(k,ix);i++) 
		    signal -= pList->GetTSignal(k,ix,i);
		//  compare the new signal with already existing list
		pList->AddSignal(k,ix,label,hit,mod,signal);
	} // end of loop pixels in x
    } // end of loop over pixels in z
}
//----------------------------------------------------------------------
void AliITSsimulationSSD::ChargeToSignal(AliITSpList *pList) {
    // charge to signal
    static AliITS *aliITS = (AliITS*)gAlice->GetModule("ITS");
    Float_t threshold = 0.;
    Int_t   digits[3], tracks[3],hits[3],j1;
    Float_t charges[3] = {0.0,0.0,0.0};
    Float_t signal;
    Float_t noise[2] = {0.,0.};

    ((AliITSresponseSSD*)fResponse)->GetNoiseParam(noise[0],noise[1]);

    for(Int_t k=0;k<2;k++){         // both sides (0=Pside, 1=Nside)
	// Threshold for zero-suppression
	// It can be defined in AliITSresponseSSD
	//             threshold = (Float_t)fResponse->MinVal(k);
	// I prefer to think adjusting the threshold "manually", looking
	// at the scope, and considering noise standard deviation
	threshold = 4.0*noise[k]; // 4 times noise is a choice
	for(Int_t ix=0;ix<GetNStrips();ix++){     // loop over strips
	    if(fMapA2->GetSignal(k,ix) <= threshold) continue;
	    // convert to ADC signal
	    signal = ((AliITSresponseSSD*)fResponse)->DEvToADC(
		                                   fMapA2->GetSignal(k,ix));
	    if(signal>1024.) signal = 1024.;//if exceeding, accumulate last one
	    digits[0] = k;
	    digits[1] = ix;
	    digits[2] = (Int_t) signal;
	    for(j1=0;j1<3;j1++){ // only three in digit.
		tracks[j1]  = pList->GetTrack(k,ix,j1);
		hits[j1]    = pList->GetHit(k,ix,j1);
	    } // end for j1
	    // finally add digit
	    aliITS->AddSimDigit(2,0,digits,tracks,hits,charges);
	} // end for ix
    } // end for k
}
//______________________________________________________________________
void AliITSsimulationSSD::WriteSDigits(AliITSpList *pList){
    // Fills the Summable digits Tree
    Int_t i,ni,j,nj;
    static AliITS *aliITS = (AliITS*)gAlice->GetModule("ITS");

    pList->GetMaxMapIndex(ni,nj);
    for(i=0;i<ni;i++)for(j=0;j<nj;j++){
	if(pList->GetSignalOnly(i,j)>0.0){
	    aliITS->AddSumDigit(*(pList->GetpListItem(i,j)));
//	    cout << "pListSSD: " << *(pList->GetpListItem(i,j)) << endl;
	} // end if
    } // end for i,j
    return;
}
//______________________________________________________________________
void AliITSsimulationSSD::FillMapFrompList(AliITSpList *pList){
    // Fills fMap2A from the pList of Summable digits
    Int_t k,ix;

    for(k=0;k<2;k++)for(ix=0;ix<GetNStrips();ix++) 
	fMapA2->AddSignal(k,ix,pList->GetSignal(k,ix));
    return;
}
//______________________________________________________________________
void AliITSsimulationSSD::Print(ostream *os){
    //Standard output format for this class

    //AliITSsimulation::Print(os);
    *os << fIonE <<",";
    *os << fDifConst[0] <<","<< fDifConst[1] <<",";
    *os << fDriftVel[0] <<","<< fDriftVel[1];
    //*os <<","; fDCS->Print(os);
    //*os <<","; fMapA2->Print(os);
}
//______________________________________________________________________
void AliITSsimulationSSD::Read(istream *is){
    // Standard output streaming function.

    //AliITSsimulation::Read(is);
    *is >> fIonE;
    *is >> fDifConst[0] >> fDifConst[1];
    *is >> fDriftVel[0] >> fDriftVel[1];
    //fDCS->Read(is);
    //fMapA2->Read(is);
}
//______________________________________________________________________
ostream &operator<<(ostream &os,AliITSsimulationSSD &source){
    // Standard output streaming function.

    source.Print(&os);
    return os;
}
//______________________________________________________________________
istream &operator>>(istream &os,AliITSsimulationSSD &source){
    // Standard output streaming function.

    source.Read(&os);
    return os;
}
//______________________________________________________________________
