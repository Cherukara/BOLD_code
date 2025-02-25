/*  CCOPYRIGHT */
#include "fwdmodel_asl_rest.h"
#include "miscmaths/miscprob.h"
#include "newimage/newimageall.h"
#include <iostream>
#include <newmatio.h>
#include <stdexcept>
using namespace NEWIMAGE;
#include "fabber_core/easylog.h"
#include "fabber_core/tools.h"

FactoryRegistration<FwdModelFactory, ASLFwdModel> ASLFwdModel::registration("aslrest");
string ASLFwdModel::ModelVersion() const
{
    string version = "fwdmodel_asl_rest.cc";
#ifdef GIT_SHA1
    version += string(" Revision ") + GIT_SHA1;
#endif
#ifdef GIT_DATE
    version += string(" Last commit ") + GIT_DATE;
#endif
    return version;
}
static OptionSpec OPTIONS[] = {
    { "disp", OPT_STR, "AIF dispersion type", OPT_NONREQ, "none" },
    { "exch", OPT_STR, "Type of exchange in tissue compartment", OPT_NONREQ, "mix" },
    { "forceconv", OPT_BOOL, "Force numerical convolution for evaluation of model", OPT_NONREQ,
        "" },
    { "inctiss", OPT_BOOL, "Include tissue parameters", OPT_NONREQ, "" },
    { "infertiss", OPT_BOOL, "Infer tissue parameters", OPT_NONREQ, "" },
    { "incart", OPT_BOOL, "Include arterial parameters", OPT_NONREQ, "" },
    { "inferart", OPT_BOOL, "Infer arterial parameters", OPT_NONREQ, "" },
    { "incwm", OPT_BOOL, "Include white matter parameters", OPT_NONREQ, "" },
    { "incbat", OPT_BOOL, "Include bolus arrival time parameter", OPT_NONREQ, "" },
    { "inferbat", OPT_BOOL, "Infer bolus arrival time parameter", OPT_NONREQ, "" },
    { "incpc", OPT_BOOL, "Include pre-capillary parameters", OPT_NONREQ, "" },
    { "inferpc", OPT_BOOL, "Infer pre-capillary parameters", OPT_NONREQ, "" },
    { "inctau", OPT_BOOL, "Include bolus duration parameter", OPT_NONREQ, "" },
    { "infertau", OPT_BOOL, "Infer bolus duration parameter", OPT_NONREQ, "" },
    { "septau", OPT_BOOL, "Separate values of tau for each component", OPT_NONREQ, "" },
    { "inct1", OPT_BOOL, "Include T1 parameter", OPT_NONREQ, "" },
    { "infert1", OPT_BOOL, "Infer T1 parameter", OPT_NONREQ, "" },
    { "inferdisp", OPT_BOOL, "Infer dispersion parameters (if present in model)", OPT_NONREQ, "" },
    { "sepdisp", OPT_BOOL, "Separate tissue dispersion parameters for each component", OPT_NONREQ,
        "" },
    { "inferexch", OPT_BOOL, "Infer exchange parameters (if present in model)", OPT_NONREQ, "" },
    { "incpve", OPT_BOOL, "Include PVE parameters", OPT_NONREQ, "" },
    { "pvcorr", OPT_BOOL, "Partial volume correction", OPT_NONREQ, "" },
    { "incstattiss", OPT_BOOL, "Include static tissue parameters", OPT_NONREQ, "" },
    { "inferstattiss", OPT_BOOL, "Infer static tissue parameters", OPT_NONREQ, "" },
    { "ardoff", OPT_BOOL, "Turn off ARD", OPT_NONREQ, "" },
    { "repeats", OPT_INT, "Number of repeats in data", OPT_NONREQ, "1" },
    { "pretisat", OPT_FLOAT, "Deal with saturation of the bolus a fixed time pre TI measurement",
        OPT_NONREQ, "0.0" },
    { "slicedt", OPT_FLOAT, "Increase in TI per slice (s)", OPT_NONREQ, "0.0" },
    { "casl", OPT_BOOL, "Data is CASL (not PASL)", OPT_NONREQ, "PASL" },
    { "bat", OPT_FLOAT, "Bolus arrival time", OPT_NONREQ, "0.7" },
    { "batwm", OPT_FLOAT, "Bolus arrival time (white matter)", OPT_NONREQ, "bat+0.3" },
    { "batart", OPT_FLOAT, "Bolus arrival time (arterial)", OPT_NONREQ, "bat-0.3" },
    { "batsd", OPT_FLOAT, "Bolus arrival time standard deviation", OPT_NONREQ, "0.316" },

    { "iaf", OPT_STR, "Data information", OPT_NONREQ, "diff" },
    { "calib", OPT_BOOL, "Data has already been subjected to calibration", OPT_NONREQ, "" },
    { "t1", OPT_FLOAT, "T1 value", OPT_NONREQ, "1.3" },
    { "t1b", OPT_FLOAT, "T1b value", OPT_NONREQ, "1.65" },
    { "t1wm", OPT_FLOAT, "T1wm value", OPT_NONREQ, "1.1" },
    { "lambda", OPT_FLOAT, "lambda value", OPT_NONREQ, "0.9 (0.98 with WM component)" },
    { "ti", OPT_FLOAT, "Single TI value (s)", OPT_NONREQ, "" },
    { "ti<n>", OPT_FLOAT, "List of TI values (s)", OPT_NONREQ, "" },
    { "pld", OPT_FLOAT, "Single PLD value (s)", OPT_NONREQ, "" },
    { "pld<n>", OPT_FLOAT, "List of PLD values (s)", OPT_NONREQ, "" },
    { "hadamard", OPT_INT, "Number of Hadamard encoding lines", OPT_NONREQ, "0" },
    { "fullhad", OPT_BOOL, "Activate full Hadamard matrix", OPT_NONREQ, "" },
    { "tau", OPT_FLOAT, "Single tau value", OPT_NONREQ, "" },
    { "tau<n>", OPT_FLOAT, "List of tau values", OPT_NONREQ, "" },
    { "crush<n>", OPT_STR, "List of vascular crushing specifications values", OPT_NONREQ, "" },
    { "FA", OPT_FLOAT, "Look-Locker correction", OPT_NONREQ, "" },
    { "facorr", OPT_BOOL, "Do FA correction", OPT_NONREQ, "" }, { "" },
};
void ASLFwdModel::GetOptions(vector<OptionSpec> &opts) const
{
    for (int i = 0; OPTIONS[i].name != ""; i++)
    {
        opts.push_back(OPTIONS[i]);
    }
}
std::string ASLFwdModel::GetDescription() const { return "Resting state ASL model"; }
void ASLFwdModel::HardcodedInitialDists(MVNDist &prior, MVNDist &posterior) const
{
    assert(prior.means.Nrows() == NumParams());
    // Set priors
    SymmetricMatrix precisions = IdentityMatrix(NumParams())
        * 1e12;      // by default all paramerers are included as fully informative
    prior.means = 0; // set a default zero value - this should get overwritten
                     // if a non-zero mean is not applicable
    // Flow
    if (infertiss)
    {
        prior.means(flow_index()) = 0;
        precisions(flow_index(), flow_index()) = 1e-12;
    }
    if (inferwm)
    {
        prior.means(flow_index() + wmidx) = 0;
        precisions(flow_index() + wmidx, flow_index() + wmidx) = 1e-12;
    }
    if (inferart)
    {
        prior.means(flow_index() + artidx) = 0;
        precisions(flow_index() + artidx, flow_index() + artidx) = 1e-12;
    }
    // BAT
    if (incbat)
    {
        if (inctiss)
        {
            prior.means(bat_index()) = setdelt;
            if (infertiss & inferbat)
            {
                precisions(bat_index(), bat_index()) = deltprec;
            }
        }
        if (incwm)
        {
            prior.means(bat_index() + wmidx) = setdeltwm;
            if (inferwm & inferbat)
            {
                precisions(bat_index() + wmidx, bat_index() + wmidx) = deltprec;
            }
        }
        if (incart)
        {
            prior.means(bat_index() + artidx) = setdeltart;
            if (inferart & inferbat)
            {
                precisions(bat_index() + artidx, bat_index() + artidx) = deltartprec;
            }
        }
    }
    // tau
    if (inctau)
    {
        if (septau)
        {
            if (inctiss)
            {
                prior.means(tau_index()) = seqtau;
                if (infertau)
                {
                    precisions(tau_index(), tau_index()) = 10;
                }
            }
            if (incwm)
            {
                prior.means(tau_index() + wmidx) = seqtau;
                if (infertau)
                {
                    precisions(tau_index() + wmidx, tau_index() + wmidx) = 10;
                }
            }
            if (incart)
            {
                prior.means(tau_index() + artidx) = seqtau;
                if (infertau)
                {
                    precisions(tau_index() + artidx, tau_index() + artidx) = 10;
                }
            }
        }
        else
        {
            prior.means(tau_index()) = seqtau;
            if (infertau)
            {
                precisions(tau_index(), tau_index()) = 10;
            }
        }
    }
    // T1
    if (inct1)
    {
        if (inctiss)
        {
            prior.means(t1_index()) = t1;
            if (infert1)
            {
                precisions(t1_index(), t1_index()) = 100;
            }
        }
        if (incwm)
        {
            prior.means(t1_index() + wmidx) = t1wm;
            if (infert1)
            {
                precisions(t1_index() + wmidx, t1_index() + wmidx) = 100;
            }
        }
        prior.means(t1_index() + artidx)
            = t1b; // always include t1b, the artidx is still okay to use here
                   // even if we dont have an explicit arterial component
        if (infert1)
        {
            precisions(t1_index() + artidx, t1_index() + artidx) = 100;
        }
    }
    // PVE
    if (incpve)
    {
        // PVE if used are set as image priors (so this is overwridden)
        // PVE if only included should be pure GM
        prior.means(pv_index()) = 1.0;
        prior.means(pv_index() + 1) = 0.0;
    }
    // taupc
    if (incpc)
    {
        if (inctiss)
        {
            prior.means(taupc_index()) = 0.3;
            if (inferpc)
            {
                precisions(taupc_index(), taupc_index()) = 10;
            }
        }
        if (incwm)
        {
            prior.means(taupc_index() + wmidx) = 0.5; // WM longer than GM
            if (inferpc)
            {
                precisions(taupc_index() + wmidx, taupc_index() + wmidx) = 10;
            }
        }
    }
    // dispersion
    if (incdisp)
    {
        ColumnVector disppriors;
        ColumnVector dispmean;
        ColumnVector dispprec;
        if (sepdisp)
        {
            if (inctiss)
            {
                // load priors from tissue model
                int ndisp;
                ndisp = tiss_model->NumDisp();
                disppriors = tiss_model->DispPriors();
                dispmean.ReSize(ndisp);
                dispmean = disppriors.Rows(1, ndisp);
                dispprec.ReSize(ndisp);
                dispprec = disppriors.Rows(ndisp + 1, 2 * ndisp);
                for (int i = 1; i <= ndisp; i++)
                {
                    prior.means(disp_index() + i - 1) = dispmean(i);
                }
                if (inferdisp)
                {
                    for (int i = 1; i <= ndisp; i++)
                    {
                        precisions(disp_index() + i - 1, disp_index() + i - 1) = dispprec(i);
                    }
                }
            }
            if (incwm)
            {
                // load priors from tissue model
                int ndisp;
                ndisp = tiss_model->NumDisp();
                disppriors = tiss_model->DispPriors();
                dispmean.ReSize(ndisp);
                dispmean = disppriors.Rows(1, ndisp);
                dispprec.ReSize(ndisp);
                dispprec = disppriors.Rows(ndisp + 1, 2 * ndisp);
                for (int i = 1; i <= ndisp; i++)
                {
                    prior.means(disp_index() + ndisp * wmidx + i - 1) = dispmean(i);
                }
                if (inferdisp)
                {
                    for (int i = 1; i <= ndisp; i++)
                    {
                        precisions(disp_index() + ndisp * wmidx + i - 1,
                            disp_index() + ndisp * wmidx + i - 1)
                            = dispprec(i);
                    }
                }
            }
            if (incart)
            {
                // load priors from art model
                int ndisp;
                ndisp = art_model->NumDisp();
                dispmean.ReSize(ndisp);
                dispmean = disppriors.Rows(1, ndisp);
                dispprec.ReSize(ndisp);
                dispprec = disppriors.Rows(ndisp + 1, 2 * ndisp);

                for (int i = 1; i <= ndisp; i++)
                {
                    prior.means(disp_index() + ndisp * artidx + i - 1) = dispmean(i);
                }
                if (inferdisp)
                {
                    for (int i = 1; i <= ndisp; i++)
                    {
                        precisions(disp_index() + ndisp * artidx + i - 1,
                            disp_index() + ndisp * artidx + i - 1)
                            = dispprec(i);
                    }
                }
            }
        }
        else
        {
            // dispersion parameters are shared between arterial and tissue
            // models
            // load the defaults from the tissue model
            int ndisp;
            ndisp = tiss_model->NumDisp();
            disppriors = art_model->Priors();
            // cout << "DISPERSION PRIORS" << endl << disppriors << endl;
            disppriors = tiss_model->DispPriors();
            dispmean.ReSize(ndisp);
            dispmean = disppriors.Rows(1, ndisp);
            dispprec.ReSize(ndisp);
            dispprec = disppriors.Rows(ndisp + 1, 2 * ndisp);

            for (int i = 1; i <= ndisp; i++)
            {
                prior.means(disp_index() + i - 1) = dispmean(i);
            }
            if (inferdisp)
            {
                for (int i = 1; i <= ndisp; i++)
                {
                    precisions(disp_index() + i - 1, disp_index() + i - 1) = dispprec(i);
                }
            }
        }
    }
    // residue function (exchange) parameters
    if (incexch)
    {
        ColumnVector residpriors;
        ColumnVector residmean;
        ColumnVector residprec;
        int nresid;
        nresid = tiss_model->NumResid();
        residpriors = tiss_model->ResidPriors();
        residmean.ReSize(nresid);
        residmean = residpriors.Rows(1, nresid);
        residprec.ReSize(nresid);
        residprec = residpriors.Rows(nresid + 1, 2 * nresid);
        for (int i = 1; i <= nresid; i++)
        {
            prior.means(resid_index() + i - 1) = residmean(i);
        }
        if (inferexch)
        {
            for (int i = 1; i <= nresid; i++)
            {
                precisions(resid_index() + i - 1, resid_index() + i - 1) = residprec(i);
            }
        }
    }
    if (incfacorr)
    {
        prior.means(facorr_index()) = 1;                  // should be overwritten by an image
        precisions(facorr_index(), facorr_index()) = 100; // small uncertainty
    }
    // Static tissue contribution to the signal (e.g. non-subtracted data)
    if (inferstattiss)
    {
        prior.means(stattiss_index()) = 0;
        precisions(stattiss_index(), stattiss_index()) = 1e-12;
    }
    // Set precsions on priors
    prior.SetPrecisions(precisions);

    // Set initial posterior
    posterior = prior;
    // For parameters with uniformative prior chosoe more sensible inital
    // posterior
    // Tissue perfusion
    if (infertiss)
    {
        posterior.means(flow_index()) = 10;
        precisions(flow_index(), flow_index()) = 1;
    }
    // Arterial perfusion
    if (inferart)
    {
        posterior.means(flow_index() + artidx) = 10;
        precisions(flow_index() + artidx, flow_index() + artidx) = 1;
    }
    if (inferstattiss)
    {
        posterior.means(stattiss_index()) = 1000;
        precisions(stattiss_index(), stattiss_index()) = 1e2;
    }
    posterior.SetPrecisions(precisions);
}

void ASLFwdModel::InitParams(MVNDist &posterior) const
{
    if (inferstattiss)
    {
        // if we have static tissue then this should be init from the raw data
        // intensities
        double dataval = data.Maximum();
        posterior.means(stattiss_index()) = dataval;

        if (infertiss)
        {
            // init the CBF  - 1% of static tissue
            posterior.means(flow_index()) = 0.01 * dataval;
        }

        if (inferart)
        {
            // init the aBV - 1% of static tissue
            posterior.means(flow_index() + artidx) = 0.01 * dataval;
        }
    }
    else
    {
        if (infertiss)
        {
            // init the CBF  - to max value in the data
            posterior.means(flow_index()) = data.Maximum();
        }

        if (inferart)
        {
            // init the aBV - use max value in data
            posterior.means(flow_index() + artidx) = data.Maximum();
        }
    }
}
void ASLFwdModel::Evaluate(const ColumnVector &params, ColumnVector &result) const
{
    // ensure that values are reasonable
    // negative check
    ColumnVector paramcpy = params;
    for (int i = 1; i <= NumParams(); i++)
    {
        if (params(i) < 0)
        {
            paramcpy(i) = 0;
        }
    }
    // cout << params.t() << endl;

    // define parameters
    // set defaults to be used if the parameters are fixed (not 'included')
    double ftiss = 0.0;
    double fwm = 0.0;
    double fblood = 0.0;
    double delttiss = setdelt;
    double deltwm = setdeltwm;
    double deltblood = setdeltart;
    double tautiss = seqtau;
    double tauwm = seqtau;
    double taublood = seqtau;
    double T_1 = t1;
    double T_1wm = t1wm;
    double T_1b = t1b;
    double pvgm = 1.0;
    double pvwm = 0.0;
    double taupctiss;
    ColumnVector pcvec(1);
    double taupcwm;
    ColumnVector pcvecwm(1);
    ColumnVector disptiss;
    ColumnVector dispwm;
    ColumnVector dispart;
    ColumnVector residtiss;
    ColumnVector residwm;
    double g = 1.0;
    double stattiss = 0.0;
    // extract parameter values
    // Flow
    if (inctiss)
    {
        ftiss = paramcpy(flow_index());
    }
    if (incwm)
    {
        fwm = paramcpy(flow_index() + wmidx);
    }
    if (incart)
    {
        fblood = paramcpy(flow_index() + artidx);
    }
    // BAT
    if (incbat)
    {
        if (inctiss)
        {
            delttiss = paramcpy(bat_index());
            if (delttiss > timax - 0.2)
                delttiss = timax - 0.2;
        }
        if (incwm)
        {
            deltwm = paramcpy(bat_index() + wmidx);
            if (deltwm > timax - 0.2)
                deltwm = timax - 0.2;
        }
        if (incart)
        {
            deltblood = paramcpy(bat_index() + artidx);
            if (deltblood > timax - 0.2)
                deltblood = timax - 0.2;
        }
    }
    // Tau
    if (inctau)
    {
        if (septau)
        {
            if (inctiss)
            {
                tautiss = paramcpy(tau_index());
            }
            if (incwm)
            {
                tauwm = paramcpy(tau_index() + wmidx);
            }
            if (incart)
            {
                taublood = paramcpy(tau_index() + artidx);
            }
        }
        else
        {
            tautiss = paramcpy(tau_index());
            tauwm = paramcpy(tau_index());
            taublood = paramcpy(tau_index());
        }
    }
    // T1
    if (inct1)
    {
        if (inctiss)
        {
            T_1 = paramcpy(t1_index());
            if (T_1 < 0.01)
                T_1 = 0.01;
        }
        if (incwm)
        {
            T_1wm = paramcpy(t1_index() + wmidx);
            if (T_1wm < 0.01)
                T_1wm = 0.01;
        }
        T_1b = paramcpy(t1_index() + artidx); // always have T_1b even without
                                              // an explicit arterial component
        if (T_1b < 0.01)
            T_1b = 0.01;
    }
    // PVE
    if (incpve)
    {
        pvgm = paramcpy(pv_index());
        pvwm = paramcpy(pv_index() + 1);
    }
    // taupc
    if (incpc)
    {
        if (inctiss)
        {
            taupctiss = paramcpy(taupc_index());
            pcvec << taupctiss;
        }
        if (incwm)
        {
            taupcwm = paramcpy(taupc_index() + wmidx);
            pcvecwm << taupcwm;
        }
    }
    // dispersion
    if (incdisp)
    {
        if (sepdisp)
        {
            int end = disp_index();
            if (inctiss)
            {
                disptiss = params.Rows(disp_index(), disp_index() - 1 + tiss_model->NumDisp());
                end = disp_index() + tiss_model->NumDisp();
            }
            if (incwm)
            {
                dispwm = params.Rows(end + 1, end + tiss_model->NumDisp());
                end += tiss_model->NumDisp();
            }
            if (incart)
            {
                dispart = params.Rows(end + 1, end + art_model->NumDisp());
            }
        }
        else
        {
            disptiss = params.Rows(disp_index(), disp_index() - 1 + tiss_model->NumDisp());
            dispwm = disptiss;
            dispart = disptiss;
        }
    }
    // exchange (residue function) parameters
    if (incexch)
    {
        // tissue and white matter have same residue function parameters (at
        // present)
        residtiss = params.Rows(resid_index(), resid_index() - 1 + tiss_model->NumResid());
        residwm = residtiss;
    }
    // flip angle correction
    if (incfacorr)
    {
        g = params(facorr_index());
    }
    if (incstattiss)
    {
        // we have a static tissue component
        stattiss = params(stattiss_index());
    }
    // look-locker readout
    // NOTE: this is only valid for even spaced readout
    double T_1ll = T_1b;
    double deltll = 0.0;
    if (looklocker)
    {
        // flip angle correction (if reqd)
        double FAtrue = (g + dg) * FA;
        // modify the T1 values according to the simplified relationship for LL
        // FA in radians
        T_1 = 1 / (1 / T_1 - log(cos(FAtrue)) / dti);
        T_1wm = 1 / (1 / T_1wm - log(cos(FAtrue)) / dti);
        // T1 for blood once it has entered imaging region
        T_1ll = 1 / (1 / T_1b - log(cos(FAtrue)) / dti);
        deltll = deltblood; // the arrival time of the blood within the readout
                            // region i.e. where it sees the LL pulses.
    }
    double f_calib;
    double f_calibwm;
    // if we are using calibrated data then we can use ftiss to calculate T_1app
    if (calib)
    {
        f_calib = ftiss;
        f_calibwm = fwm;
    }
    else
    {
        f_calib = 0.01;
        f_calibwm = 0.003;
    } // otherwise assume sensible value (units of s^-1)

    /*
  ColumnVector artdir(3);
  artdir(1) = sin(bloodphi)*cos(bloodth);
  artdir(2) = sin(bloodphi)*sin(bloodth);
  artdir(3) = cos(bloodphi);
  */
    double kctissue;
    kctissue = 0.0;
    double kcblood;
    kcblood = 0.0;
    double kcwm;
    kcwm = 0.0;
    double kcpc = 0.0;
    double kcpcwm = 0.0;
    // Calcualte the Kinetic Model contirbutions at each TI
    // loop over tis
    double ti;
    ColumnVector statcont(tis.Nrows()); // this stores the static tissue contribution at each TI
    statcont = 0.0;
    ColumnVector kctotal(tis.Nrows()); // this stores the total kinetic signal contribution
    kctotal = 0.0;

    for (int it = 1; it <= tis.Nrows(); it++)
    {
        // account here for an increase in the TI due to delays between slices
        int thisz = coord_z; // the slice number
        if (sliceband > 0)
        {
            // multiband setup in which we have specified the number of slices
            // per band
            div_t divresult;
            divresult = div(coord_z, sliceband);
            thisz = divresult.rem; // the number of sluices above the base of
                                   // this band (lowest slice in volume for
                                   // normal (non multi-band) data)
        }
        ti = tis(it) + slicedt * thisz; // calcualte the actual TI for this
                                        // slice
        if (multitau)
        {
            // overwrite default tau value with the TI specific one
            tautiss = taus(it);
            tauwm = tautiss;
            taublood = tautiss;
        }
        // look-locker correction for arterial blood
        if (looklocker)
        {
            if (ti > deltll)
                T_1b = T_1ll;
        }

        // crushers
        double artweight = 1.0;
        artweight = 1.0 - crush(it); // arterial weight is opposte of crsuh extent
        /*
    if (artdir) {
      artweight = Sinc( 2 * bloodbv *
    std::max(DotProduct(artdir,crushdir.Row(it)),0.0) ); // based on laminar
    flow profile c.f. perfusion tensor imaging
    }
    */
        // Tissue
        if (inctiss)
            kctissue = pvgm * ftiss
                * tiss_model->kctissue(
                      ti, f_calib, delttiss, tautiss, T_1b, T_1, lambda, casl, disptiss, residtiss);
        // White matter
        if (incwm)
            kcwm = pvwm * fwm
                * tiss_model->kctissue(
                      ti, f_calibwm, deltwm, tauwm, T_1b, T_1wm, lamwm, casl, dispwm, residwm);
        // Arterial
        if (incart)
            kcblood = artweight * fblood
                * art_model->kcblood(ti, deltblood, taublood, T_1b, casl, dispart);

        if (incpc)
        {
            // pre-capilliary component
            // NB its arrival time is the tissue arrival time minus the pc
            // transit time
            if (inctiss)
                kcpc = pvgm * ftiss
                    * pc_model->kctissue(ti, f_calib, delttiss - taupctiss, tautiss, T_1b, T_1,
                          lambda, casl, disptiss, pcvec);
            if (incwm)
                kcpcwm = pvwm * fwm
                    * pc_model->kctissue(ti, f_calibwm, deltwm - taupcwm, tauwm, T_1b, T_1, lamwm,
                          casl, dispwm, pcvecwm);
        }
        // if (isnan(kctissue)) { kctissue=0; LOG << "Warning NaN in tissue
        // curve at TI:" << ti << " with f:" << ftiss << " delt:" << delttiss <<
        // " tau:" << tau << " T1:" << T_1 << " T1b:" << T_1b << endl; }
        // total kinetic contribution
        kctotal(it) = kctissue + kcblood + kcwm + kcpc + kcpcwm;
        // static tissue contribution
        if (incstattiss)
        {
            // a simple static contirbution at all TIs
            statcont(it) = stattiss;
        }
    }
    // Assemble result
    if (hadamard)
    {
        // Hadamard encoded ASL data
        ColumnVector signal;
        signal = HadEncMatrix * kctotal; // collect the KC contirbutions from each block of label
        signal = statcont - signal;      // inlcude static tissue signal
        // loop over repeats - we assume we always get blocks of each set of
        // encodings
        result = signal;
        for (int rpt = 2; rpt <= repeats[0]; rpt++)
        {
            result &= result;
        }
    }
    else if (raw)
    {
        // ASL data - unstracted (raw)
        result.ReSize(2 * tpoints);
        int ent = 0;
        for (int it = 1; it <= tis.Nrows(); it++)
        {
            // data is in blocks of repeated TIs
            // loop over the repeats
            for (int rpt = 1; rpt <= repeats[it - 1]; rpt++)
            {
                result(ent + 2 * rpt - 1) = statcont(it) + kctotal(it); // TAG
                result(ent + 2 * rpt) = statcont(it);                   // CONTROL
            }
            ent += 2 * repeats[it - 1];
        }
    }
    else
    {
        // normal (differenced) ASL data
        result.ReSize(tpoints);
        int ent = 0;
        for (int it = 1; it <= tis.Nrows(); it++)
        {
            // data is in blocks of repeated TIs
            // loop over the repeats
            for (int rpt = 1; rpt <= repeats[it - 1]; rpt++)
            {
                result(ent + rpt) = statcont(it) + kctotal(it);
            }
            ent += repeats[it - 1];
        }
    }

    if (result.Nrows() != data.Nrows())
    {
        //        string reason =
        throw InvalidOptionValue("num volumes", stringify(data.Nrows()),
            "Expected " + stringify(result.Nrows()) + " volumes - check the number of repeats/TIs");
    }

    // cout << result.t();

    return;
}
FwdModel *ASLFwdModel::NewInstance() { return new ASLFwdModel(); }
void ASLFwdModel::Initialize(ArgsType &args)
{
    string scanParams = args.ReadWithDefault("scan-params", "cmdline");

    bool forceconv;
    if (scanParams == "cmdline")
    {
        // specify command line parameters here
        // model choices
        disptype = args.ReadWithDefault("disp", "none"); // set the AIF dispersion type
        exchtype
            = args.ReadWithDefault("exch", "mix"); // set the type of exchange in tissue compartment
        forceconv = args.ReadBool("forceconv");    // force numerical convolution
                                                   // for the evaluation of the
                                                   // model
        // inference/inclusion
        // -components
        inctiss = args.ReadBool("inctiss");
        infertiss = args.ReadBool("infertiss");
        incart = args.ReadBool("incart");
        inferart = args.ReadBool("inferart");
        if (inferart)
            incart = true;
        incwm = args.ReadBool("incwm");
        inferwm = false; // we only infer WM if we are doing PV correction (below)
        if (!inctiss & !incart)
        {
            throw invalid_argument("Error: neither tissue nor arterial "
                                   "components have been selected: make sure "
                                   "you set either (or both) of --inctiss and "
                                   "--incart");
        }
        // -common things
        incbat = args.ReadBool("incbat");
        inferbat = args.ReadBool("inferbat");
        incpc = args.ReadBool("incpc");
        inferpc = args.ReadBool("inferpc");
        if (inferpc)
            incpc = true;
        inctau = args.ReadBool("inctau");
        infertau = args.ReadBool("infertau");
        if (infertau)
            inctau = true;
        septau = args.ReadBool("septau");
        inct1 = args.ReadBool("inct1");
        infert1 = args.ReadBool("infert1");
        if (infert1)
            inct1 = true;
        // incdisp = args.ReadBool("incdisp");
        // incdisp is set based on whether there are dispersion parameters in
        // the model
        inferdisp = args.ReadBool("inferdisp");
        sepdisp = args.ReadBool("sepdisp");
        // incexch = args.ReadBool("incexch");
        // incexch is set based on whether there are residue function parameters
        // in the model
        inferexch = args.ReadBool("inferexch");
        // if (inferexch) incexch = true;
        // -special
        incpve = args.ReadBool("incpve");
        // PV correction
        pvcorr = args.ReadBool("pvcorr");
        if (pvcorr)
        {
            incpve = true;
            incwm = true;
            inferwm = true;
        }
        // make sure if we include PVE that we always have WM component in the
        // model
        if (incpve)
            incwm = true;
        // include the static tissue
        incstattiss = args.ReadBool("incstattiss");
        inferstattiss = args.ReadBool("inferstattiss");

        // some useful (relative) indices for WM and arterial components
        ncomps = (inctiss ? 1 : 0) + (incart ? 1 : 0) + (incwm ? 1 : 0);
        wmidx = 0;
        if (inctiss)
            wmidx++;
        artidx = 0;
        if (inctiss)
            artidx++;
        if (incwm)
            artidx++;

        // deal with ARD selection for aBV
        bool ardoff = false;
        ardoff = args.ReadBool("ardoff");
        doard = false;
        if (inferart == true && ardoff == false)
        {
            ardindices.push_back(flow_index() + artidx);
        }
        // scan parameters
        // repeats = convertTo<int>(args.ReadWithDefault("repeats", "1"));      // number of repeats
        // in data
        pretisat = convertTo<double>(args.ReadWithDefault(
            "pretisat", "0")); // deal with saturation of the bolus a fixed time pre TI measurement
        slicedt
            = convertTo<double>(args.ReadWithDefault("slicedt", "0.0")); // increase in TI per slice
        sliceband = convertTo<int>(args.ReadWithDefault("sliceband",
            "0")); // number of slices in a band in a multi-band setup (zero implies single band)
        casl = args.ReadBool("casl"); // set if the data is CASL or PASL (default)
        seqtau = convertTo<double>(args.ReadWithDefault("tau","1000")); //bolus length as set by sequence (default of 1000 is effectively infinite

        setdelt = convertTo<double>(args.ReadWithDefault("bat", "0.7"));
        string deltwm = args.ReadWithDefault("batwm", "null");
        if (deltwm == "null")
        {
            setdeltwm = setdelt + 0.3; // by default choose delt WM longer then GM
        }
        else
        {
            setdeltwm = convertTo<double>(deltwm);
        }
        string deltart = args.ReadWithDefault("batart", "null");
        if (deltart == "null")
        {
            setdeltart = setdelt - 0.3; // by default choose delt blood shorter then GM
        }
        else
        {
            setdeltart = convertTo<double>(deltart);
        }
        // std dev for delt prior (same for all tissue BAT)
        double deltsd;
        deltsd = convertTo<double>(args.ReadWithDefault("batsd", "0.316"));
        deltprec = 1 / (deltsd * deltsd);
        // now arterial BAT precision
        string batartsd = args.ReadWithDefault("batartsd", "null");
        if (batartsd == "null")
        {
            // by default the arterial BAT SD is same as tissue
        }
        else
        {
            deltsd = convertTo<double>(batartsd);
        }
        deltartprec = 1 / (deltsd * deltsd);
        // data information
        raw = false;
        tagfirst = true;
        string iaf = args.ReadWithDefault("iaf", "diff");
        if ((iaf == "tc") | (iaf == "ct"))
        {
            // data is in raw form
            raw = true;
            if (iaf == "ct")
                tagfirst = false;
        }
        // analysis options/settings
        calib = args.ReadBool("calib"); // data has already been subjected to calibration
        // T1 values
        string default_t1 = "1.3"; // T1 for generic tissue (i.e. mixed GM and WM)
        if (incwm)
            default_t1 = "1.3"; // possibly a different default for T1 of tissue
                                // if this represents GM only
        t1 = convertTo<double>(args.ReadWithDefault("t1", default_t1));
        t1b = convertTo<double>(args.ReadWithDefault("t1b", "1.65"));
        t1wm = convertTo<double>(args.ReadWithDefault("t1wm", "1.1"));
        // other model parameters
        string default_lambda = "0.9"; // lambda for generic tissue (mixed GM and WM)
        if (incwm)
            default_lambda = "0.98"; // different default for labda if we have a
                                     // WM component (since then the 'tissue' is
                                     // a GM component)
        lambda = convertTo<double>(args.ReadWithDefault("lambda", default_lambda));
        lamwm = 0.82;
        // Read in timing parameters (TIs / PLDs)
        bool ti_set = false;
        bool pld_set = false;
        ColumnVector ti_list;
        ColumnVector pld_list;
        // TIs
        string ti_temp = args.ReadWithDefault("ti", "none");
        if (ti_temp != "none")
        {
            // a single TI
            ti_set = true;
            ti_list.ReSize(1);
            ti_list(1) = convertTo<double>(ti_temp);
        }
        else
        {
            ti_temp = args.ReadWithDefault("ti1", "none");
            if (ti_temp != "none")
            {
                // a list of TIs
                ti_set = true;
                ti_list.ReSize(1); // will add extra values onto end as needed
                ti_list(1) = convertTo<double>(ti_temp);
                while (true) // get the rest of the tis
                {
                    int N = ti_list.Nrows() + 1;
                    ti_temp = args.ReadWithDefault("ti" + stringify(N), "stop!");
                    if (ti_temp == "stop!")
                        break; // we have run out of tis

                    // append the new ti onto the end of the list
                    ColumnVector tmp(1);
                    tmp = convertTo<double>(ti_temp);
                    ti_list &= tmp; // vertical concatenation
                }
            }
        }
        // PLDs
        string pld_temp = args.ReadWithDefault("pld", "none");
        if (pld_temp != "none")
        {
            // a single PLD
            pld_set = true;
            pld_list.ReSize(1);
            pld_list(1) = convertTo<double>(pld_temp);
        }
        else
        {
            pld_temp = args.ReadWithDefault("pld1", "none");
            if (pld_temp != "none")
            {
                // a list of TIs
                pld_set = true;
                pld_list.ReSize(1); // will add extra values onto end as needed
                pld_list(1) = convertTo<double>(pld_temp);
                while (true) // get the rest of the tis
                {
                    int N = pld_list.Nrows() + 1;
                    pld_temp = args.ReadWithDefault("pld" + stringify(N), "stop!");
                    if (pld_temp == "stop!")
                        break; // we have run out of tis

                    // append the new ti onto the end of the list
                    ColumnVector tmp(1);
                    tmp = convertTo<double>(pld_temp);
                    pld_list &= tmp; // vertical concatenation
                }
            }
        }

        // Hadamard time encoding
        string hadamardin = args.ReadWithDefault(
            "hadamard", "none");                 // if nothing is stated, no Hadamard encoding
                                                 // is assumed. If it is set to an integer N, N
                                                 // encoding lines are assumed.
        bool FullHad = args.ReadBool("fullhad"); // in some cases the full
                                                 // Hadamard matrix is needed,
                                                 // i.e. all N rows (and not
                                                 // only N-1). This is activated
                                                 // by this command.
        if (hadamardin == "none")
        {
            hadamard = false;
            HadamardSize = 1;
        }
        else
        {
            hadamard = true;
            HadamardSize = convertTo<int>(hadamardin); // This gives the number of encoding lines
            if (((HadamardSize % 2) != 0) & (HadamardSize != 12))
            {
                // check that we have a sensible hadamard scheme
                throw invalid_argument("Hadamard encoding is only possible "
                                       "with a number of encodings that are "
                                       "modulo 2 (2,4,8,16...) or number of "
                                       "encodings equal to 12");
            }
            if (FullHad)
                NumberOfSubBoli = HadamardSize; // This gives the number of subboli
            else
                NumberOfSubBoli = HadamardSize - 1;
            HadEncMatrix = HadamardMatrix(HadamardSize);
            // we will usually ingore the first column (all control), unless we
            // are doign FullHad
            HadEncMatrix = HadEncMatrix.SubMatrix(
                1, HadamardSize, HadamardSize - (NumberOfSubBoli - 1), HadamardSize);
        }
        // Populate the inflow time (TI) vector
        if (hadamard)
        {
            // set TIs up for hadamard data
            if (pld_list.Nrows() == 0)
            {
                throw invalid_argument("For Hadamard time encoding please specify a PLD (--pld=)");
            }
            else if (pld_list.Nrows() > 1)
            {
                throw invalid_argument("Hadamard time encoding with more than "
                                       "one PLD is not currently supported");
            }
            // calculate the TIs corresponding to the indiviual subboli
            tis.ReSize(1);
            tis(1) = NumberOfSubBoli * seqtau + pld_list(1);
            for (int i = 1; i < NumberOfSubBoli; i++)
            {
                ColumnVector tmp(1);
                tmp = pld_list(1) + (NumberOfSubBoli - i) * seqtau;
                tis &= tmp; // vertical concatenation
            }
        }
        else
        {
            // normal ASL data
            if (ti_set)
            {
                tis = ti_list;
            }
            if (pld_set)
            {
                if (casl)
                {
                    tis = pld_list + seqtau;
                }
                else
                {
                    // unlikely to happen, but permits the user to supply PLDs
                    // for a pASL acquisition
                    tis = pld_list;
                }
            }
        }

        timax = tis.Maximum(); // dtermine the final TI

        // repeats
        string rpt_temp = args.ReadWithDefault("repeats", "NULL");
        if (rpt_temp != "NULL")
        {
            // number of repeats has been specified - same for each TI
            for (int it = 0; it < tis.Nrows(); it++)
            {
                repeats.push_back(convertTo<int>(rpt_temp));
            }
        }
        else
        {
            rpt_temp = args.ReadWithDefault("rpt1", "none");
            if (rpt_temp != "none")
            {
                // a list of repeats
                repeats.push_back(convertTo<int>(rpt_temp));
                int N = 2;
                while (true) // get the rest of the repeats
                {
                    if (hadamard)
                    {
                        throw invalid_argument(
                            "Cannot specify more than one set of repeats with Hadmard encoding");
                    }

                    rpt_temp = args.ReadWithDefault("rpt" + stringify(N), "stop!");
                    if (rpt_temp == "stop!")
                        break; // we have run out of repeats

                    repeats.push_back(convertTo<int>(rpt_temp));
                    N++;
                }
                // check that number of entries for repeats matches number of TIs
                if (int(repeats.size()) != tis.Nrows())
                {
                    throw invalid_argument("Mismatch between number of inflow times (TIs/PLDs) and "
                                           "entries for repeats - these should be equal");
                }
            }
            else
            {
                // The number of repeats has not been specified by the user - default is one.
                for (int it = 0; it < tis.Nrows(); it++)
                {
                    repeats.push_back(1);
                }
            }
        }
        // total number of time points in data
        tpoints = 0;
        for (int it = 0; it < tis.Nrows(); it++)
        {
            tpoints += repeats[it];
        }

        // bolus durations

        multitau = false;
        string tau_temp = args.ReadWithDefault("tau", "none");
        if (tau_temp == "none")
        {
            tau_temp = args.ReadWithDefault("tau1", "none");
            if (tau_temp != "none")
            {
                // a list of taus
                taus.ReSize(1); // will add extra values onto end as needed
                taus(1) = convertTo<double>(tau_temp);
                while (true) // get the rest of the taus
                {
                    if (inctau)
                    {
                        throw invalid_argument("Inference/Inclusion of (variable) bolus duration "
                                               "is not compatible with multiple bolus durations "
                                               "use a single value (--tau=)");
                    }
                    if (septau)
                    {
                        throw invalid_argument("Separate bolus duration for "
                                               "different components not valid "
                                               "with multiple bolus durations "
                                               "use a single value (--tau=)");
                    }
                    multitau = true;
                    int N = taus.Nrows() + 1;
                    tau_temp = args.ReadWithDefault("tau" + stringify(N), "stop!");
                    if (tau_temp == "stop!")
                        break; // we have run out of tis

                    // append the new tau onto the end of the list
                    ColumnVector tmp(1);
                    tmp = convertTo<double>(tau_temp);
                    taus &= tmp; // vertical concatenation
                }
                // check that number of bolus durations matches number of TIs
                if (taus.Nrows() != tis.Nrows())
                {
                    throw invalid_argument("Mismatch between number of inflow "
                                           "times (TIs/PLDs) and bolus "
                                           "durations - these should be equal");
                }
            }
        }

        // vascular crushing
        // to specify a custom combination of vascular crushing
        string crush_temp = args.ReadWithDefault("crush1", "notsupplied");
        // if crush_temp = none then we assume all data has same crushing
        // parameters we will represent this as no crushers
        crush.ReSize(tis.Nrows());
        crush = 0.0; // default is no crusher
        crushdir.ReSize(tis.Nrows(), 3);
        crushdir = 0.0;
        crushdir.Column(3) = 1.0; // default (which should remain ignored normally) is z-only
        if (crush_temp != "notsupplied")
        {
            // we need to assemble crusher information
            int N = 1;
            while (true)
            {
                if (N > 1)
                {
                    crush_temp = args.ReadWithDefault("crush" + stringify(N), "stop!");
                    if (crush_temp == "stop!")
                        break; // we have run out of crusher specifications
                }
                // determine what crusher type we have
                if (crush_temp == "off" || crush_temp == "none")
                {
                    crush(N) = 0.0;
                }
                else if (crush_temp == "on")
                {
                    crush(N) = 1.0;
                }
                else if (crush_temp == "xyz")
                {
                    crush(N) = 1.0;
                    crushdir.Row(N) << 1 << 1 << 1;
                    crushdir.Row(N) /= sqrt(3);
                }
                else if (crush_temp == "-xyz")
                {
                    crush(N) = 1.0;
                    crushdir.Row(N) << -1 << 1 << 1;
                    crushdir.Row(N) /= sqrt(3);
                }
                else if (crush_temp == "x-yz")
                {
                    crush(N) = 1.0;
                    crushdir.Row(N) << 1 << -1 << 1;
                    crushdir.Row(N) /= sqrt(3);
                }
                else if (crush_temp == "-x-yz")
                {
                    crush(N) = 1.0;
                    crushdir.Row(N) << -1 << -1 << 1;
                    crushdir.Row(N) /= sqrt(3);
                }
                N++;
            }
        }
        // Look-Locker correction
        string FAin = args.ReadWithDefault("FA", "none");
        if (FAin == "none")
            looklocker = false;
        else
        {
            looklocker = true;
            FA = convertTo<double>(FAin);
            FA *= M_PI / 180;      // convert to radians
            dti = tis(2) - tis(1); // NOTE LL correction is only valid with
                                   // evenly spaced TIs
        }
        incfacorr = args.ReadBool("facorr"); // indicate that we want to do FA
                                             // correction - the g image will
                                             // need to be separately loaded in
                                             // as an image prior
        dg = convertTo<double>(args.ReadWithDefault("dg", "0.0"));
        // need to set the voxel coordinates to a default of 0 (for the times we
        // call the model before we start handling data)
        coord_x = 0;
        coord_y = 0;
        coord_z = 0;
        // setup the models
        // same tissue model for GM and WM
        // NB it is feasible to have different dispersion for arterial and
        // tissue models, but not implemented
        // > Arterial Model (only depend upon dispersion type)
        art_model = NULL;
        if (disptype == "none")
        {
            art_model = new AIFModel_nodisp();
        }
        else if (disptype == "gamma")
        {
            art_model = new AIFModel_gammadisp();
        }
        else if (disptype == "gauss")
        {
            art_model = new AIFModel_gaussdisp();
        }
        else if (disptype == "sgauss")
        {
            art_model = new AIFModel_spatialgaussdisp();
        }
        // > PC models
        pc_model = NULL;
        if (disptype == "none")
        {
            pc_model = new TissueModel_nodisp_imperm();
        }
        // default is to use convolution model with the impermeable residue
        // function
        if ((pc_model == NULL) | forceconv)
        {
            ResidModel *imperm_resid;
            imperm_resid = new ResidModel_imperm();
            pc_model = new TissueModel_aif_residue(art_model, imperm_resid);
        }
        // > Tissue model
        tiss_model = NULL;
        resid_model = NULL;
        // This should ALWAYS set a resid_model and maybe a MATCHING tiss_model
        //   - well mixed single compartment
        if (exchtype == "mix")
        {
            resid_model = new ResidModel_wellmix();
            if (disptype == "none")
            {
                tiss_model = new TissueModel_nodisp_wellmix();
            }
            else if ((disptype == "gamma") && !casl)
            {
                tiss_model = new TissueModel_gammadisp_wellmix();
            }
        }
        //    - simple single impermeable compartment that decays with T1b
        if (exchtype == "simple")
        {
            resid_model = new ResidModel_simple();
            if (disptype == "none")
            {
                tiss_model = new TissueModel_nodisp_simple();
            }
        }
        //    - 2 compartment exchange (simplest model - no backflow, no venous
        //    outflow)
        else if (exchtype == "2cpt")
        {
            resid_model = new ResidModel_twocpt;
            if ((disptype == "none") && !casl)
            {
                tiss_model = new TissueModel_nodisp_2cpt();
            }
        }
        //    - SPA 2 compartment model
        else if (exchtype == "spa")
        {
            resid_model = new ResidModel_spa();
            if ((disptype == "none") && !casl)
            {
                tiss_model = new TissueModel_nodisp_spa();
            }
        }
        // > default is to use the convolution model with the residue function
        // if no analytical tissue model can be found
        if ((tiss_model == NULL) | args.ReadBool("forceconv"))
        {
            // note the 'forceconv' option, this forces the model to use the
            // convolution formulation voer any analytic form it has
            if (resid_model == NULL)
            {
                // we cannot do a convolution in this case as a residue function
                // has not been found either!
                throw invalid_argument("A residue function model for this "
                                       "exchange type cannot be found");
            }
            else
            {
                tiss_model = new TissueModel_aif_residue(art_model, resid_model);
            }
        }
        // include dispersion parameters if the model has them
        if ((art_model->NumDisp() > 0) | (tiss_model->NumDisp() > 0))
        {
            incdisp = true;
        }
        // include resdue function (i.e. exchange) parameters is the model has
        // them
        if (tiss_model->NumResid() > 0)
        {
            incexch = true;
        }

        // dispersion model - load priors from command line
        for (int i = 1; i <= art_model->NumDisp(); i++)
        {
            string priormean = args.ReadWithDefault("disp_prior_mean_" + stringify(i), "null");
            if (priormean != "null")
            {
                art_model->SetPriorMean(i, convertTo<double>(priormean));
                tiss_model->SetDispPriorMean(i,
                    convertTo<double>(priormean)); // ASSUME that dispersion
                                                   // model same for arterial
                                                   // and tissue model (and they
                                                   // share the same priors)
            }
        }

        // add information about the parameters to the log
        LOG << "Inference using resting state ASL model" << endl;
        LOG << "INCLUSIONS:" << endl;
        if (inctiss)
            LOG << "Tissue" << endl;
        if (incart)
            LOG << "Arterial/MV" << endl;
        if (incwm)
            LOG << "White matter" << endl;
        LOG << "Number of components: " << ncomps << endl;
        if (incbat)
            LOG << "Bolus arrival time" << endl;
        if (incpc)
            LOG << "Pre capilliary" << endl;
        if (inctau)
            LOG << "Bolus duration (tau)" << endl;
        if (inct1)
            LOG << "T1 values" << endl;
        if (incdisp)
            LOG << "Dispersion" << endl;
        if (incexch)
            LOG << "Restricted exchange" << endl;
        if (incpve)
            LOG << "Partial volume estimates" << endl;
        if (incstattiss)
            LOG << "Statis tissue" << endl;
        LOG << "INFERENCE:" << endl;
        if (infertiss)
            LOG << "Tissue" << endl;
        if (inferart)
            LOG << "Arterial/MV" << endl;
        if (inferwm)
            LOG << "White matter" << endl;
        if (inferbat)
            LOG << "Bolus arrival time" << endl;
        if (inferpc)
            LOG << "Pre capilliary" << endl;
        if (infertau)
            LOG << "Bolus duration (tau)" << endl;
        if (septau)
            LOG << "  Separate values of tau for each component" << endl;
        if (infert1)
            LOG << "T1 values" << endl;
        if (inferdisp)
            LOG << "Dispersion" << endl;
        if (sepdisp)
            LOG << "  Separate dispersion parameters for each component" << endl;
        if (inferexch)
            LOG << "Restricted exchange" << endl;
        if (pvcorr)
            LOG << "Partial volume correction" << endl;
        if (inferstattiss)
            LOG << "Static tissue" << endl;
        LOG << "------" << endl;
        // kinetic model
        LOG << "Kinetic model:" << endl;
        if (!casl)
            LOG << "Data being analysed using PASL inversion profile" << endl;
        if (casl)
            LOG << "Data being analysed using CASL inversion profile" << endl;
        LOG << "Tissue model:" << tiss_model->Name() << endl;
        if (tiss_model->NumDisp() > 0)
            LOG << "Dispersion parameter priors (means then precisions): "
                << tiss_model->DispPriors() << endl;
        if (tiss_model->NumResid() > 0)
            LOG << "Residue function parameter priors (means then precisions): "
                << tiss_model->ResidPriors() << endl;
        if (incart)
            LOG << "Arterial model:" << art_model->Name() << endl;
        if (art_model->NumDisp() > 0)
            LOG << "Dispersion parameter priors (means then precisions): " << art_model->Priors()
                << endl;
        if (incpc)
            LOG << "Pre-capillary model:" << pc_model->Name() << endl;
        LOG << "------" << endl;
        // scan parameters
        if (pretisat > 0)
            LOG << "Saturation of " << pretisat << " s before TI has been specified" << endl;
        if (calib)
            LOG << "Input data is in physioligcal units, using estimated CBF "
                   "in T_1app calculation"
                << endl;
        LOG << "Data parameters: #repeats = " << repeats << endl;
        LOG << " t1 = " << t1 << ", t1b = " << t1b;
        if (incwm)
            LOG << "t1wm= " << t1wm << endl;
        LOG << " bolus duration (tau) = " << seqtau << endl;
        // Hadamard
        if (hadamard)
        {
            LOG << "Hadamard time encoding:" << endl;
            if (FullHad)
                LOG << "  Full nxn-Hadamard matrix is used." << endl;
            LOG << "  Number of Hadamard encoded images: " << HadamardSize << endl;
            LOG << "  Number of Hadamard subboli: " << NumberOfSubBoli << endl;
            LOG << "  Subbolus length: " << seqtau << endl;
            LOG << "  Post labeling delay (PLD): " << pld_list(1) << endl;
        }

        if (doard)
        {
            LOG << "ARD has been set on arterial compartment " << endl;
        }
        LOG << "TIs: ";
        for (int i = 1; i <= tis.Nrows(); i++)
            LOG << tis(i) << " ";
        LOG << endl;
    }
    else
        throw invalid_argument("Only --scan-params=cmdline is accepted at the moment");
}

void ASLFwdModel::NameParams(vector<string> &names) const
{
    names.clear();

    if (inctiss)
        names.push_back("ftiss");
    if (incwm)
        names.push_back("fwm");
    if (incart)
        names.push_back("fblood");
    if (incbat)
    {
        if (inctiss)
            names.push_back("delttiss");
        if (incwm)
            names.push_back("deltwm");
        if (incart)
            names.push_back("deltblood");
    }
    if (inctau)
    {
        if (septau)
        {
            if (inctiss)
                names.push_back("tautiss");
            if (incwm)
                names.push_back("tauwm");
            if (incart)
                names.push_back("taublood");
        }
        else
        {
            names.push_back("tau");
        }
    }
    if (inct1)
    {
        if (inctiss)
            names.push_back("T_1");
        if (incwm)
            names.push_back("T_1wm");
        names.push_back("T_1b");
    }
    if (incpve)
    {
        names.push_back("pvgm");
        names.push_back("pvwm");
    }
    if (incpc)
    {
        if (inctiss)
            names.push_back("taupctiss");
        if (incwm)
            names.push_back("taupcwm");
    }
    if (incdisp)
    {
        if (sepdisp)
        {
            if (inctiss)
            {
                for (int i = 1; i <= tiss_model->NumDisp(); i++)
                {
                    names.push_back("disptiss" + stringify(i));
                }
            }
            if (incwm)
            {
                for (int i = 1; i <= tiss_model->NumDisp(); i++)
                {
                    names.push_back("dispwm" + stringify(i));
                }
            }
            if (incart)
            {
                for (int i = 1; i <= art_model->NumDisp(); i++)
                {
                    names.push_back("dispart" + stringify(i));
                }
            }
        }
        else
        {
            for (int i = 1; i <= tiss_model->NumDisp(); i++)
            {
                names.push_back("disp" + stringify(i));
            }
        }
    }
    if (incexch)
    {
        for (int i = 1; i <= tiss_model->NumResid(); i++)
        {
            names.push_back("exch" + stringify(i));
        }
    }
    if (incstattiss)
    {
        names.push_back("stattiss");
    }
}
// Useful other functions
Matrix ASLFwdModel::HadamardMatrix(const int size) const
{
    // generate a Hadamard matrix
    // This has zeros (for control) and ones (for label) in place of the classic
    // +1 and -1
    Matrix matrix;

    if (size == 12)
    {
        // special case
        Real b[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1,
            1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1,
            0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0,
            0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1,
            1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0,
            1, 1 };
        matrix.ReSize(12, 12);
        matrix << b;
    }
    else if ((size % 2) == 0)
    {
        // build the hadamard matrix - using Sylvester's construction
        Matrix H2k(2, 2);
        H2k << 1.0 << 1.0 << 1.0 << -1.0;
        matrix = H2k;
        // TODO check that size is a power of 2 first
        for (int i = 4; i <= size; i *= 2)
        {
            // matrix.ReSize(i,i);
            matrix = KP(H2k, matrix);
            // H2k=matrix; - not required
        }
        // get the matrix into the form we want for ASL
        matrix = -0.5 * (matrix - 1.0);
    }
    return matrix;
}
void ASLFwdModel::SetupARD(const MVNDist &theta, MVNDist &thetaPrior, double &Fard) const
{
    int ardindex = ard_index();
    if (doard)
    {
        SymmetricMatrix PriorPrec;
        PriorPrec = thetaPrior.GetPrecisions();

        PriorPrec(ardindex, ardindex) = 1e-12; // set prior to be initally non-informative

        thetaPrior.SetPrecisions(PriorPrec);
        thetaPrior.means(ardindex) = 0;
        // set the Free energy contribution from ARD term
        SymmetricMatrix PostCov = theta.GetCovariance();
        double b
            = 2 / (theta.means(ardindex) * theta.means(ardindex) + PostCov(ardindex, ardindex));
        Fard = -1.5 * (log(b) + digamma(0.5)) - 0.5 - gammaln(0.5)
            - 0.5 * log(b); // taking c as 0.5 - which it will be!
    }
    return;
}
void ASLFwdModel::UpdateARD(const MVNDist &theta, MVNDist &thetaPrior, double &Fard) const
{
    int ardindex = ard_index();
    if (doard)
    {
        SymmetricMatrix PriorCov;
        SymmetricMatrix PostCov;
        PriorCov = thetaPrior.GetCovariance();
        PostCov = theta.GetCovariance();
        PriorCov(ardindex, ardindex)
            = theta.means(ardindex) * theta.means(ardindex) + PostCov(ardindex, ardindex);

        thetaPrior.SetCovariance(PriorCov);
        // Calculate the extra terms for the free energy
        double b
            = 2 / (theta.means(ardindex) * theta.means(ardindex) + PostCov(ardindex, ardindex));
        Fard = -1.5 * (log(b) + digamma(0.5)) - 0.5 - gammaln(0.5)
            - 0.5 * log(b); // taking c as 0.5 - which it will be!
    }
    return;
}
