#ifndef INC_ACTION_DSSP_H
#define INC_ACTION_DSSP_H
#include "Action.h"
// Class: DSSP
/// Calculate protein secondary structure using DSSP algorithm.
class Action_DSSP : public Action {
  public:
    Action_DSSP();

    static DispatchObject* Alloc() { return (DispatchObject*)new Action_DSSP(); }
    static void Help();

    ~Action_DSSP();

    void print();
  private:
    int init();
    int setup();
    int action();
    // Enum and static vars
    enum SStype { 
      SECSTRUCT_NULL, SECSTRUCT_PARA, SECSTRUCT_ANTI, SECSTRUCT_3_10, 
      SECSTRUCT_ALPHA, SECSTRUCT_PI, SECSTRUCT_TURN 
    };
    static const double DSSP_fac;
    static const char SSchar[];
    static const char SSname[][6];
    /// Hold SS-related data for each residue
    struct Residue {
      int SSprob[7];            ///< Hold count for each SS type
      SStype sstype;            ///< Assigned secondary structure
      bool isSelected;          ///< True if C, H, N, and O atom of res selected 
      int C;                    ///< atom idx of BB carbon
      int O;                    ///< atom idx of BB oxygen
      int N;                    ///< atom idx of BB nitrogen
      int H;                    ///< atom idx of BB hydrogen
      std::vector<int> CO_HN_Hbond;  ///< Size=# residues, 1 if this residue CO-HN hbonded to res X
      DataSet *resDataSet;      ///< Hold SS assignment each frame
    };
    std::vector<Residue> SecStruct_; ///< Hold SS-related data for all residues
    // Class variables
    std::string outfilename_; ///< Data file name
    DataSet *dssp_;     ///< If printString, hold the string dataset
    AtomMask Mask_;     ///< Mask used to determine selected residues
    int Nres_;          ///< Current total # of residues
    int Nframe_;        ///< # of frames, for calculating SS avg.
    std::string sumOut_;///< File to output SS avgs (dssp.dat.sum)
    char *SSline_;      ///< Hold SS propensity for frame, each char represents a residue
    bool printString_;  ///< If true print 1 char per residue indicating ss type
    //CpptrajFile debugout; // DEBUG
    // For printString=false, Int dataset, hold SStype for each residue at each frame
    NameType BB_N;
    NameType BB_H;
    NameType BB_C;
    NameType BB_O;
    // Private fns
    int isBonded(int, int);
    void SSassign(int, int, SStype);
};
#endif
