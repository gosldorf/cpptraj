// Action_Closest
// Find closest waters to atoms in mask.
#include <cmath>
#include <algorithm> // sort
#include <cfloat> // DBL_MAX
#ifdef _OPENMP
#  include "omp.h"
#endif
#include "Action_Closest.h"
#include "CpptrajStdio.h"
#include "ParmFile.h"

// CONSTRUCTOR
Action_Closest::Action_Closest() :
  outFile_(NULL),
  framedata_(NULL),
  moldata_(NULL),
  distdata_(NULL),
  atomdata_(NULL),
  Nclosest_(0),
  closestWaters_(0),
  firstAtom_(false),
  newParm_(NULL),
  NsolventMolecules_(0)
{} 

void Action_Closest::Help() {

}

// DESTRUCTOR
Action_Closest::~Action_Closest() {
  //fprintf(stderr,"Closest Destructor.\n");
  if (newParm_!=NULL) delete newParm_;
}

// Action_Closest::init()
/** Expected call: closest <# to keep> <mask> [noimage] [first/oxygen] 
  *                [closestout <filename> [outprefix <parmprefix>]
  */
int Action_Closest::init( ) {
  // Get Keywords
  closestWaters_ = actionArgs.getNextInteger(-1);
  if (closestWaters_ < 0) {
    mprinterr("Error: closest: Invalid # solvent molecules to keep (%i).\n",
              closestWaters_);
    return 1;
  }
  if ( actionArgs.hasKey("oxygen") || actionArgs.hasKey("first") )
    firstAtom_=true;
  InitImaging( !(actionArgs.hasKey("noimage")) );
  prefix_ = actionArgs.GetStringKey("outprefix");
  // Setup output file and sets if requested.
  // Will keep track of Frame, Mol#, Distance, and first solvent atom
  ArgList::ConstArg filename = actionArgs.getKeyString("closestout");
  if (filename != NULL) {
    // Set up datasets
    framedata_ = outList_.Add(DataSet::INT, "Frame", "Frame");
    moldata_   = outList_.Add(DataSet::INT, "Mol", "Mol");
    distdata_  = outList_.Add(DataSet::DOUBLE, "Dist", "Dist");
    atomdata_  = outList_.Add(DataSet::INT, "FirstAtm", "FirstAtm");
    if (framedata_==NULL || moldata_==NULL || distdata_==NULL || atomdata_==NULL) {
      mprinterr("Error: closest:: Could not setup data sets for output file %s\n",
                filename);
      return 1;
    }
    // Add sets to datafile in list.
    outFile_ = DFL->Add(filename, framedata_);
    outFile_ = DFL->Add(filename, moldata_);
    outFile_ = DFL->Add(filename, distdata_);
    outFile_ = DFL->Add(filename, atomdata_);
    if (outFile_==NULL) {
      mprintf("Error: closest: Could not setup output file %s\n",filename);
      return 1;
    }
  }

  // Get Masks
  ArgList::ConstArg mask1 = actionArgs.getNextMask();
  if (mask1==NULL) {
    mprinterr("Error: closest: No mask specified.\n");
    return 1;
  }
  distanceMask_.SetMaskString(mask1);

  mprintf("    CLOSEST: Finding closest %i solvent molecules to atoms in mask %s\n",
          closestWaters_, distanceMask_.MaskString());
  if (!UseImage()) 
    mprintf("\tImaging will be turned off.\n");
  if (firstAtom_)
    mprintf("\tOnly first atom of solvent molecule used for distance calc.\n");
  if (outFile_!=NULL)
    mprintf("\tClosest molecules will be saved to %s\n",outFile_->Filename());
  if (!prefix_.empty())
    mprintf("\tStripped topology file will be written with prefix %s\n",
            prefix_.c_str());
  return 0;
}

// Action_Closest::setup()
/** Like the strip action, closest will modify the current parm keeping info
  * for atoms in mask plus the closestWaters solvent molecules. Set up the
  * vector of MolDist objects, one for every solvent molecule in the original
  * parm file. Atom masks for each solvent molecule will be set up.
  */
int Action_Closest::setup() {
  // If there are no solvent molecules this action is not valid.
  if (currentParm->Nsolvent()==0) {
    mprintf("Warning: closest: Parm %s does not contain solvent.\n",currentParm->c_str());
    return 1;
  }
  // If # solvent to keep >= solvent in this parm the action is not valid.
  // TODO: Just use max # waters?
  if (closestWaters_ >= currentParm->Nsolvent()) {
    mprintf("Warning: closest: # solvent to keep (%i) >= # solvent molecules in\n",
            closestWaters_);
    mprintf("                           %s (%i).\n",currentParm->c_str(),
            currentParm->Nsolvent());
    return 1;
  }
  SetupImaging( currentParm->BoxType() ); 

  // LOOP OVER MOLECULES
  // 1: Check that all solvent molecules contain same # atoms. Solvent 
  //    molecules must be identical for the command to work properly; 
  //    the prmtop strip occurs only once so the solvent params become fixed.
  int NsolventAtoms = -1;
  // 2: Set up a mask for all solvent molecules.
  SolventMols_.clear();
  // NOTE: May not be necessary to init 'solvent'
  MolDist solvent;
  solvent.D = 0.0;
  solvent.mol = 0;
  SolventMols_.resize(currentParm->Nsolvent(), solvent);
  std::vector<MolDist>::iterator mdist = SolventMols_.begin();
  int molnum = 1;
  // 3: Set up the soluteMask for all non-solvent molecules.
  stripMask_.ResetMask();
  int newnatom = 0;
  int nclosest = 0;
  keptWaterAtomNum_.resize(closestWaters_);
  for (Topology::mol_iterator Mol = currentParm->MolStart();
                              Mol != currentParm->MolEnd(); ++Mol)
  {
    if ( !(*Mol).IsSolvent() ) { // Not solvent, add to solute mask.
      stripMask_.AddAtomRange( (*Mol).BeginAtom(), (*Mol).EndAtom() );
      newnatom += (*Mol).NumAtoms();
    } else {                         // Solvent, check for same # of atoms.
      if (NsolventAtoms == -1)
        NsolventAtoms = (*Mol).NumAtoms();
      else if ( NsolventAtoms != (*Mol).NumAtoms() ) {
        mprinterr("Error: closest: Solvent molecules in %s are not of uniform size.\n",
                  currentParm->c_str());
        mprinterr("       First solvent mol = %i atoms, solvent mol %i = %i atoms.\n",
                  NsolventAtoms, molnum, (*Mol).NumAtoms());
        return 1;
      }
      // NOTE: mol here is the output molecule number which is why it
      //       starts from 1.
      (*mdist).mol = molnum;
      (*mdist).mask.AddAtomRange( (*Mol).BeginAtom(), (*Mol).EndAtom() );
      // For solvent molecules that will be kept, record what the atom number
      // will be in the new stripped parm.
      if (nclosest < closestWaters_) {
        //mprintf("CDBG: Mol %i old atom# %i, new atom# %i\n", molnum, 
        //        (*Mol).BeginAtom()+1, newnatom+1);
        keptWaterAtomNum_[nclosest] = newnatom;
        stripMask_.AddAtomRange( (*Mol).BeginAtom(), (*Mol).EndAtom() );
        newnatom += (*Mol).NumAtoms();
        ++nclosest;
      }
      //SolventMols[solventMol].mask.PrintMaskAtoms("solvent");
      ++mdist;
    }
    ++molnum;
  }

  // Setup distance atom mask
  // NOTE: Should ensure that no solvent atoms are selected!
  if ( currentParm->SetupIntegerMask(distanceMask_) ) return 1;
  if (distanceMask_.None()) {
    mprintf("Warning: closest: Distance mask %s contains no atoms.\n",
            distanceMask_.MaskString());
    return 1;
  }

  // Check the total number of solvent atoms to be kept.
  NsolventAtoms *= closestWaters_;
  mprintf("\tKeeping %i solvent atoms.\n",NsolventAtoms);
  if (NsolventAtoms < 1) {
    mprinterr("Error: closest: # of solvent atoms to be kept is < 1.\n");
    return 1;
  }

  // Store original # of molecules.
  // NOTE: This is stored so that it can be used in the OpenMP section
  //       of action. I dont think iterators are thread-safe.
  NsolventMolecules_ = currentParm->Nsolvent();
 
  // Create stripped Parm
  if (newParm_!=NULL) delete newParm_;
  newParm_ = currentParm->modifyStateByMask(stripMask_);
  if (newParm_==NULL) {
    mprinterr("Error: closest: Could not create new parmtop.\n");
    return 1;
  }
  newParm_->Summary();

  // Allocate space for new frame
  // FIXME: Should this be set up for velocity as well?
  newFrame_.SetupFrameM( newParm_->Atoms() );

  // If prefix given then output stripped parm
  if (!prefix_.empty()) {
    std::string newfilename = prefix_ + "." + currentParm->OriginalFilename();
    mprintf("\tWriting out amber topology file %s to %s\n",newParm_->c_str(),
            newfilename.c_str());
    ParmFile pfile;
    pfile.SetDebug( debug );
    if ( pfile.Write(*newParm_, newfilename, ParmFile::AMBERPARM ) ) {
      mprinterr("Error: closest: Could not write out stripped parm file %s\n",
              newParm_->c_str());
    }
  }

  // Set parm
  currentParm = newParm_;

  return 0;  
}

// Action_Closest::action()
/** Find the minimum distance between atoms in distanceMask and each 
  * solvent Mask.
  */
int Action_Closest::action() {
  int solventMol; 
  double Dist, maxD, ucell[9], recip[9];
  AtomMask::const_iterator solute_atom, solvent_atom;

  if (ImagingEnabled()) {
    currentFrame->BoxToRecip(ucell, recip);
    // Calculate max possible imaged distance
    maxD = currentFrame->BoxX() + currentFrame->BoxY() + currentFrame->BoxZ();
    maxD *= maxD;
  } else {
    // If not imaging, set max distance to an arbitrarily large number
    maxD = DBL_MAX;
  }

  Vec3 boxXYZ(currentFrame->BoxX(), currentFrame->BoxY(), currentFrame->BoxZ() );
  // Loop over all solvent molecules in original frame
  // DEBUG
  //mprintf("Closest: Begin parallel loop for %i\n",frameNum);
#ifdef _OPENMP
#pragma omp parallel private(solventMol,solute_atom,Dist,solvent_atom)
{
  //mprintf("OPENMP: %i threads\n",omp_get_num_threads());
#pragma omp for
#endif
  for (solventMol=0; solventMol < NsolventMolecules_; solventMol++) {
    //mprintf("[%i] Calculating distance for molecule %i\n",omp_get_thread_num(),solventMol);
    // Set the initial minimum distance for this solvent mol to be the
    // max possible distance.
    SolventMols_[solventMol].D = maxD;
    // DEBUG - show solvent mask
    //fprintf(stdout,"      Solvent %i %i %i\n", MaskList[solventMol]->Selected[0]+1,
    //        MaskList[solventMol]->Selected[1]+1,MaskList[solventMol]->Selected[2]+1);

    // Calculate distance between each atom in distanceMask and atoms in solvent Mask
    solvent_atom = SolventMols_[solventMol].mask.begin();
    for (solute_atom = distanceMask_.begin(); solute_atom != distanceMask_.end(); solute_atom++)
    {
      Dist = DIST2(currentFrame->XYZ(*solute_atom),
                   currentFrame->XYZ(*solvent_atom), ImageType(), 
                   boxXYZ, ucell, recip);
      if (Dist < SolventMols_[solventMol].D) 
        SolventMols_[solventMol].D = Dist;
      //fprintf(stdout,"D atom %i %i = %lf image %i\n",*solute_atom,*solvent_atom,minD,imageType);
      // Check the rest of the solvent atoms if specified
      if (!firstAtom_) {
        ++solvent_atom;
        for (; solvent_atom != SolventMols_[solventMol].mask.end(); solvent_atom++) 
        {
          Dist = DIST2(currentFrame->XYZ(*solute_atom),
                       currentFrame->XYZ(*solvent_atom), ImageType(), 
                       boxXYZ, ucell, recip);
          if (Dist < SolventMols_[solventMol].D) 
            SolventMols_[solventMol].D = Dist;
        }
      }
    }

    // DEBUG - Print distances
    //mprintf("DEBUG:\tMol %8i minD= %lf\n",solventMol, SolventMols[solventMol].D);
  } // END for loop over solventMol
#ifdef _OPENMP
} // END pragma omp parallel
#endif
  // DEBUG
  //mprintf("Closest: End parallel loop for %i, got %i Distances.\n",frameNum,(int)SolventMols.size());
  // DEBUG

  // Sort distances
  sort( SolventMols_.begin(), SolventMols_.end(), moldist_cmp() );

  // Add first closestWaters solvent atoms to stripMask
  std::vector<MolDist>::iterator solventend = SolventMols_.begin() + closestWaters_;
  std::vector<int>::iterator katom = keptWaterAtomNum_.begin();
  for ( std::vector<MolDist>::iterator solvent = SolventMols_.begin();
                                       solvent != solventend; ++solvent ) 
  {
    //mprintf("DEBUG:\tmol %i ",(*solvent).mol);
    //(*solvent).mask.PrintMaskAtoms("Mask");
    stripMask_.AddMaskAtPosition( (*solvent).mask, *katom );
    ++katom;

    // Record which water molecules are closest if requested
    if (outFile_!=NULL) {
      int fnum = frameNum + 1;
      framedata_->Add(Nclosest_, &fnum);
      moldata_->Add(Nclosest_, &((*solvent).mol));
      Dist = sqrt( (*solvent).D );
      distdata_->Add(Nclosest_, &Dist);
      solvent_atom = (*solvent).mask.begin();
      int solvent_first_atom = *solvent_atom + 1; 
      atomdata_->Add(Nclosest_, &solvent_first_atom);
      ++Nclosest_;
    }
    // DEBUG - print first closestWaters distances
    //mprintf("DEBUG: Mol %i   D2= %lf   Atom0= %i\n",(*it).mol, (*it).D, (*it).mask->Selected[0]);
  }

  // Modify and set frame
  //mprintf("DEBUG:\t");
  //stripMask.PrintMaskAtoms("action_stripMask");
  newFrame_.SetFrame(*currentFrame, stripMask_);
  currentFrame = &newFrame_;

  return 0;
} 

// Action_Closest::print()
/** Set up the closest output file for writing. Since the datasets used in
  * closest are local and not part of the master data set list, call Sync
  * for them here. Also set so that the X column (which is just an index)
  * is not written.
  */
void Action_Closest::print() {
  if (outFile_==NULL) return;
  // Sync up the dataset list here since it is not part of the master 
  // dataset list.
  outList_.Sync();
  // Set specific datafile options
  outFile_->ProcessArgs("noxcol");
}

