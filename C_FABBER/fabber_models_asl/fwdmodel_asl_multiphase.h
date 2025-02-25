/*  fwdmodel_asl_multiphase.h -

 Michael Chappell, QuBIc (IBME) & FMRIB Image Analysis Group

 Copyright (C) 2013 University of Oxford  */

/*  CCOPYRIGHT */

#ifndef __FABBER_ASL_MULTIPHASE_FWDMODEL_H
#define __FABBER_ASL_MULTIPHASE_FWDMODEL_H 1

#include "fabber_core/fwdmodel.h"
#include "fabber_core/inference.h"
#include <string>

using namespace std;

class MultiPhaseASLFwdModel : public FwdModel
{
public:
    static FwdModel *NewInstance();

    // Virtual function overrides
    virtual void Initialize(ArgsType &args);
    virtual void Evaluate(const NEWMAT::ColumnVector &params, NEWMAT::ColumnVector &result) const;
    virtual string ModelVersion() const;
    virtual void GetOptions(vector<OptionSpec> &opts) const;
    virtual string GetDescription() const;

    virtual void NameParams(vector<string> &names) const;
    virtual int NumParams() const { return 3 + (incvel ? 1 : 0); }
    virtual ~MultiPhaseASLFwdModel() { return; }
    virtual void HardcodedInitialDists(MVNDist &prior, MVNDist &posterior) const;
    virtual void InitParams(MVNDist &posterior) const;

protected: // Constants
    int repeats;

    // phases in data
    int nph;
    NEWMAT::ColumnVector ph_list;

    // modulation function
    string modfn;

    // inference options
    bool incvel;
    bool infervel;

    // fermi function variables
    double alpha;
    double beta;

    // modulation matrix
    double mod_fn(const double inphase, const double v) const;
    double interp(
        const NEWMAT::ColumnVector &x, const NEWMAT::ColumnVector &y, const double xi) const;
    NEWMAT::Matrix mod_mat;
    NEWMAT::ColumnVector mod_phase;
    NEWMAT::ColumnVector mod_v;
    double vmax;
    double vmin;
    int nvelpts;

private:
    /** Auto-register with forward model factory. */
    static FactoryRegistration<FwdModelFactory, MultiPhaseASLFwdModel> registration;
};

#endif //__FABBER_ASL_MULTIPHASE_FWDMODEL_H
