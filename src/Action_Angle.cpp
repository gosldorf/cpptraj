// Action_Angle 
#include "Action_Angle.h"
#include "CpptrajStdio.h"
#include "Constants.h" // RADDEG
#include "TorsionRoutines.h"

// CONSTRUCTOR
Action_Angle::Action_Angle() : ang_(0), useMass_(false) {} 

void Action_Angle::Help() {
  mprintf("\t[<name>] <mask1> <mask2> <mask3> [out <filename>] [mass]\n"
          "  Calculate the angle between atoms in masks 1-3.\n");
}

// Action_Angle::init()
Action::RetType Action_Angle::Init(ArgList& actionArgs, ActionInit& state, int debugIn)
{
  // Get keywords
  DataFile* outfile = state.DFL().AddDataFile( actionArgs.GetStringKey("out"), actionArgs );
  useMass_ = actionArgs.hasKey("mass");

  // Get Masks
  std::string mask1 = actionArgs.GetMaskNext();
  std::string mask2 = actionArgs.GetMaskNext();
  std::string mask3 = actionArgs.GetMaskNext();
  if (mask1.empty() || mask2.empty() || mask3.empty()) {
    mprinterr("Error: angle: Requires 3 masks\n");
    return Action::ERR;
  }
  Mask1_.SetMaskString(mask1);
  Mask2_.SetMaskString(mask2);
  Mask3_.SetMaskString(mask3);

  // Dataset to store angles
  ang_ = state.DSL().AddSet(DataSet::DOUBLE,
                            MetaData(actionArgs.GetStringNext(),MetaData::M_ANGLE),"Ang");
  if (ang_==0) return Action::ERR;
  // Add dataset to data file list
  if (outfile != 0) outfile->AddDataSet( ang_ );

  mprintf("    ANGLE: [%s]-[%s]-[%s]\n",Mask1_.MaskString(), Mask2_.MaskString(), 
          Mask3_.MaskString());
  if (useMass_)
    mprintf("\tUsing center of mass of atoms in masks.\n");

  return Action::OK;
}

// Action_Angle::setup()
/** Set angle up for this parmtop. Get masks etc.
  */
Action::RetType Action_Angle::Setup(ActionSetup& state) {
  if (state.Top().SetupIntegerMask(Mask1_)) return Action::ERR;
  if (state.Top().SetupIntegerMask(Mask2_)) return Action::ERR;
  if (state.Top().SetupIntegerMask(Mask3_)) return Action::ERR;
  mprintf("\t");
  Mask1_.BriefMaskInfo();
  Mask2_.BriefMaskInfo();
  Mask3_.BriefMaskInfo();
  mprintf("\n");
  if (Mask1_.None() || Mask2_.None() || Mask3_.None()) {
    mprintf("Warning: angle: One or more masks contain 0 atoms.\n");
    return Action::SKIP;
  }
  return Action::OK;  
}

// Action_Angle::action()
Action::RetType Action_Angle::DoAction(int frameNum, ActionFrame& state) {
  Vec3 a1, a2, a3;
  if (useMass_) {
    a1 = state.Frm().VCenterOfMass( Mask1_ );
    a2 = state.Frm().VCenterOfMass( Mask2_ );
    a3 = state.Frm().VCenterOfMass( Mask3_ );
  } else {
    a1 = state.Frm().VGeometricCenter( Mask1_ );
    a2 = state.Frm().VGeometricCenter( Mask2_ );
    a3 = state.Frm().VGeometricCenter( Mask3_ );
  }
  double aval = CalcAngle( a1.Dptr(), a2.Dptr(), a3.Dptr() );

  aval *= Constants::RADDEG;

  ang_->Add(frameNum, &aval);

  return Action::OK;
}
