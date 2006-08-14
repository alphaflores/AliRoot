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
// Author: Andrei Gheata, 31/05/2006

//==============================================================================
//   AliAnalysysDataSlot - Class representing a data slot of an analysis task.
//      An analysis slot enforces a certain data type required by the Exec()
//      method of the analysis task. The slot must be connected to a data 
//      container with data of the same type.
//
// The class should not be directly created by users - it is created by
// each AliAnalysisTask when defining its input/output slots using:
//
//    AliAnalysisTask::SetInput(Int_t index, TClass *type);
//    AliAnalysisTask::SetOutput(TClass *type);
//
// An existing data contaner (AliAnalysisDataContainer) can be connected to the
// input/output slots of an analysis task using:
//
//   AliAnalysisModule::ConnectInput(AliAnalysisTask *task, Int_t islot,
//                                   AliAnalysisDataContainer *cont)
//   AliAnalysisModule::ConnectOutput(AliAnalysisTask *task,
//                                    AliAnalysisDataContainer *cont)
// To connect a slot to a data container, the data types declared by both must
// match.
//==============================================================================

#include "TClass.h"
#include "AliLog.h"

#include "AliAnalysisDataSlot.h"
#include "AliAnalysisTask.h"
#include "AliAnalysisDataContainer.h"

ClassImp(AliAnalysisDataSlot)

//______________________________________________________________________________
AliAnalysisDataSlot& AliAnalysisDataSlot::operator=(const AliAnalysisDataSlot &slot)
{
// Assignment
   if (&slot == this) return *this;
   TObject::operator=(slot);
   fType = slot.fType;
   fParent = slot.fParent;
   fContainer = slot.fContainer;   
   return *this;
}

//______________________________________________________________________________
Bool_t AliAnalysisDataSlot::ConnectContainer(AliAnalysisDataContainer *cont)
{
// Connect the data slot with a given container. The operation will succeed only
// if the type defined by the slot inherits from the type enforced by the container.
// The error message in case of failure is posted by the caller.
   if (!cont || !fType) return kFALSE;
   if (!fType->InheritsFrom(cont->GetType())) {
      AliError(Form("Data slot of type %s of task %s cannot be connected to data container %s of type %s", 
                    fType->GetName(), fParent->GetName(), cont->GetName(), cont->GetType()->GetName()));
      return kFALSE;
   }   
   fContainer = cont;
   return kTRUE;
}   

//______________________________________________________________________________
TObject *AliAnalysisDataSlot::GetData() const
{
// Retreives the data from the container if it is ready.
   if (!fContainer) {
      AliError(Form("Data slot of type %s of task %s has no connected data container",
               fType->GetName(), fParent->GetName()));    
      return NULL;
   }
   if (!fContainer->IsDataReady()) return NULL;
   return fContainer->GetData();
}

//______________________________________________________________________________
Bool_t  AliAnalysisDataSlot::IsDataReady() const
{
// Check if data for this slot is ready in its container.
   if (!fContainer) {
      AliError(Form("Data slot of type %s of task %s has no connected data container",
               fType->GetName(), fParent->GetName()));    
      return kFALSE;
   }
   return fContainer->IsDataReady();
}
   
