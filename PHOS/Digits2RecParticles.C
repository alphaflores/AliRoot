void Digits2RecParticles(){
  gSystem->Setenv("CONFIG_SPLIT_FILE","1") ;   

  TFile * fRootFile = TFile::Open("galice.root","update");

  //========== Get AliRun object from file 
  gAlice = (AliRun*) fRootFile->Get("gAlice") ;

  //=========== Get the PHOS object and associated geometry from the file      
  AliPHOSv1 * fPHOS  = (AliPHOSv1 *)gAlice->GetDetector("PHOS") ;
  AliPHOSGeometry * fGeom  = AliPHOSGeometry::GetInstance( fPHOS->GetGeometry()->GetName(), fPHOS->GetGeometry()->GetTitle() );
 
  AliPHOSIndexToObject * fObjGetter = AliPHOSIndexToObject::GetInstance(fPHOS) ; 


  
  //========== Create the Clusterizer
  AliPHOSClusterizerv1 * fClu = new AliPHOSClusterizerv1() ; 
  
  //========== Creates the track segment maker
  AliPHOSTrackSegmentMakerv1 * fTrs = new AliPHOSTrackSegmentMakerv1()  ;
  //	  fTrs->UnsetUnfoldFlag() ; 
  
  //========== Creates the particle identifier
  AliPHOSPIDv1* fPID = new AliPHOSPIDv1() ;
  fPID->SetShowerProfileCuts(0.3, 1.8, 0.3, 1.8 ) ;       
      
  //========== Creates the Reconstructioner
  AliPHOSReconstructor * fRec = new AliPHOSReconstructor(fClu, fTrs, fPID) ; 
  // fRec -> SetDebugReconstruction(kTRUE);     
     
  gAlice->GetEvent(0) ;

  gAlice->TreeD()->GetEvent(0) ;

  if (gSystem->Getenv("CONFIG_SPLIT_FILE"))    
   if(gAlice->TreeR() == 0) gAlice->MakeTree("R","Reco.PHOS.root");
  else 
   if(gAlice->TreeR() == 0) gAlice->MakeTree("R");    

  //=========== Do the reconstruction
  fPHOS->Reconstruction(fRec);    
  

}
