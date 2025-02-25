/*  fwdmodel_asl_turboquasar.cc -resting stat ASL model for TURBO QUASAR
   acquisition

    Moss Zhao and Michael Chappell, IBME & FMRIB Image Analysis Group

    Copyright (C) 2010 University of Oxford  */

/*  CCOPYRIGHT */

#include "fwdmodel_asl_turboquasar.h"

#include <fabber_core/inference.h>
#include <fabber_core/tools.h>

#include <miscmaths/miscprob.h>
#include <newmatio.h>

using namespace std;
using namespace NEWMAT;
using namespace MISCMATHS;

FactoryRegistration<FwdModelFactory, TurboQuasarFwdModel> TurboQuasarFwdModel::registration(
    "turboquasar");

FwdModel *TurboQuasarFwdModel::NewInstance() { return new TurboQuasarFwdModel(); }
string TurboQuasarFwdModel::ModelVersion() const
{
    string version = "fwdmodel_asl_turboquasar.cc";
#ifdef GIT_SHA1
    version += string(" Revision ") + GIT_SHA1;
#endif
#ifdef GIT_DATE
    version += string(" Last commit ") + GIT_DATE;
#endif
    return version;
}

static OptionSpec OPTIONS[] = {
    { "repeats", OPT_INT, "Number of repeats in data", OPT_REQ, "" },
    { "disp", OPT_STR, "AIF dispersion type", OPT_NONREQ, "gamma" },
    { "t1", OPT_FLOAT, "T1 value", OPT_NONREQ, "1.3" },
    { "t1b", OPT_FLOAT, "T1b value", OPT_NONREQ, "1.65" },
    { "t1wm", OPT_FLOAT, "T1wm value", OPT_NONREQ, "1.1" },
    { "infert1", OPT_BOOL, "Infer T1 parameter", OPT_NONREQ, "" },
    { "infertau", OPT_BOOL, "Infer bolus duration parameter", OPT_NONREQ, "" },
    { "inferart", OPT_BOOL, "Infer arterial parameters", OPT_NONREQ, "" },
    { "inferwm", OPT_BOOL, "Infer WM parameters", OPT_NONREQ, "" },
    { "tau", OPT_FLOAT, "Single tau value", OPT_NONREQ, "" },
    { "slicedt", OPT_FLOAT, "Increase in TI per slice", OPT_NONREQ, "0.0" },
    { "ardoff", OPT_BOOL, "Turn off ARD", OPT_NONREQ, "" },
    { "tauboff", OPT_BOOL, "Force the inference of arterial bolus off", OPT_NONREQ, "" },
    { "usepve", OPT_BOOL, "Use PVE", OPT_NONREQ, "" },
    { "artdir", OPT_BOOL, "Infer direction of arterial blood", OPT_NONREQ, "" },
    { "usecalib", OPT_BOOL, "use calibration images (provided as image priors)", OPT_NONREQ, "" },
    { "tissoff", OPT_BOOL, "Turn off tissue CPT", OPT_NONREQ, "" },
    { "onephase", OPT_BOOL, "Special - a single phase of data (if we have "
                            "already processed the phases)",
        OPT_NONREQ, "" },
    { "tissardon", OPT_BOOL, "Tissue ARD on", OPT_NONREQ, "" },
    { "artardoff", OPT_BOOL, "Arterial ARD on", OPT_NONREQ, "" },
    { "wmardoff", OPT_BOOL, "WM ARD on", OPT_NONREQ, "" },
    { "ti<n>", OPT_FLOAT, "List of TI values", OPT_NONREQ, "" },
    { "bolus_<n>", OPT_FLOAT, "Whether the bolus is on or off. E.g --bolus_1=1 --bolus_2=0. n<=7",
        OPT_NONREQ, "" },
    { "slice_shift", OPT_FLOAT, "Slice shifting factor (default: 1)", OPT_NONREQ, "" },
    { "fa", OPT_FLOAT, "Flip angle in degrees", OPT_NONREQ, "30" }, { "" },
};

void TurboQuasarFwdModel::GetOptions(vector<OptionSpec> &opts) const
{
    for (int i = 0; OPTIONS[i].name != ""; i++)
    {
        opts.push_back(OPTIONS[i]);
    }
}

std::string TurboQuasarFwdModel::GetDescription() const { return "TURBO_QUASAR ASL model"; }
void TurboQuasarFwdModel::Initialize(ArgsType &args)
{
    // simulation mode, put everything in fixed manner
    simulation = false;
    simulation = args.ReadBool("simulation_mode");
    if (simulation)
    {
        // Warning message
        cout << "Warming: T1_blood=1.6 and T1_tissue=1.3 are harded coded "
                "values!!!!!"
             << endl;
    }

    // specify command line parameters here
    // dispersion model
    disptype = args.ReadWithDefault("disp", "gamma");

    repeats = convertTo<int>(args.Read("repeats")); // number of repeats in data
    t1 = convertTo<double>(args.ReadWithDefault("t1", "1.3"));
    t1b = convertTo<double>(args.ReadWithDefault("t1b", "1.65"));
    t1wm = convertTo<double>(args.ReadWithDefault("t1wm", "1.1"));
    lambda = convertTo<double>(
        args.ReadWithDefault("lambda", "0.9")); // NOTE that this parameter is not used!!
    // n_bolus = convertTo<int>(args.Read("n_bolus")); // total number of
    // boluses in turbo QUASAR
    // delta_bolus = convertTo<double>(args.ReadWithDefault("delta_bolus",
    // "0.6")); // time duration between each successive bolus
    slice_shifting_factor
        = convertTo<int>(args.ReadWithDefault("slice_shift", "1")); // slice shifting factor,
                                                                    // default is 1 meaning no
                                                                    // increaing sampling rate
    // delta_ti_gap_factor = convertTo<int>(args.ReadWithDefault("bolus_skip",
    // "1")); // Number of boluses to skip, default is 1 meaning one bolus
    // duration is skipped between each successive bolus

    infertau = args.ReadBool("infertau"); // infer on bolus length?
    infert1 = args.ReadBool("infert1");   // infer on T1 values?
    inferart = args.ReadBool("inferart"); // infer on arterial compartment?
    inferwm = args.ReadBool("inferwm");

    seqtau
        = convertTo<double>(args.ReadWithDefault("tau", "1000")); // bolus length as set by sequence
                                                                  // (default of 1000 is effectively
                                                                  // infinite
    slicedt = convertTo<double>(args.ReadWithDefault("slicedt", "0.0")); // increase in TI per slice

    bool ardoff = false;
    ardoff = args.ReadBool("ardoff");
    bool tauboff = false;
    tauboff = args.ReadBool("tauboff"); // forces the inference of arterial bolus off

    usepve = args.ReadBool("usepve");

    artdir = args.ReadBool("artdir"); // infer direction of arterial blood

    calibon = args.ReadBool("usecalib"); // use calibration images (provided as image priors)

    // combination options
    infertaub = false;
    if (inferart && infertau && !tauboff)
        infertaub = true;

    // special - turn off tissue cpt
    infertiss = true;
    bool tissoff = args.ReadBool("tissoff");
    if (tissoff)
        infertiss = false;

    // special - a single phase of data (if we have already processed the
    // phases)
    onephase = false;
    onephase = args.ReadBool("onephase");

    // deal with ARD selection
    doard = false;
    tissard = false;
    artard = true;
    wmard = true; // default ARD flags
    // if (inferart==true && ardoff==false) { doard=true;}
    // if (inferwm==true && ardoff==false) {doard=true; }
    // special, individual ARD switches
    bool tissardon = args.ReadBool("tissardon");
    if (tissardon)
        tissard = true;
    bool artardoff = args.ReadBool("artardoff");
    if (artardoff)
        artard = false;
    bool wmardoff = args.ReadBool("wmardoff");
    if (wmardoff)
        wmard = false;

    // ** ardoff overrides all other ARD options
    if ((tissard || artard || wmard) && !ardoff)
        doard = true;

    /* if (infertrailing) {
  if (!infertau) {
    // do not permit trailing edge inference without inferring on bolus length
    throw Invalid_option("--infertrailing has been set without setting
  --infertau");
  }
  else if (inferinveff)
    //do not permit trailing edge inference and inversion efficiency inference
  (they are mututally exclusive)
    throw Invalid_option("--infertrailing and --inferinveff may not both be
  set");
    }*/

    // Deal with tis
    tis.ReSize(1); // will add extra values onto end as needed
    tis(1) = atof(args.Read("ti1").c_str());

    // get the rest of the tis
    while (true)
    {
        int N = tis.Nrows() + 1;
        string tiString = args.ReadWithDefault("ti" + stringify(N), "stop!");
        if (tiString == "stop!")
            break; // we have run out of tis

        // append the new ti onto the end of the list
        ColumnVector tmp(1);
        tmp = convertTo<double>(tiString);
        tis &= tmp; // vertical concatenation
    }

    timax = tis.Maximum(); // dtermine the final TI
    // determine the TI interval (assume it is even throughout)
    dti = (tis(2) - tis(1)) * slice_shifting_factor;

    // Bolus duration parameters
    // determine delta bolus (time between each successive bolus)
    // delta_bolus = delta_ti_gap_factor * dti;
    delta_bolus = dti;
    // Deal with bolus duration vectors (which one is labelled, which one is
    // skipped)
    // Technique similar with TIs
    bolus_order.ReSize(1);
    bolus_order(1) = atof(args.Read("bolus_1").c_str()); // Get the first bolus
    // Get the rest of bolus
    while (true)
    {
        int next_bolus_index = bolus_order.Nrows() + 1;
        string next_bolus_string
            = args.ReadWithDefault("bolus_" + stringify(next_bolus_index), "stop!");

        // Reached end of bolus order, then stop reading
        if (next_bolus_string == "stop!")
        {
            break;
        }

        // Add the new string to end of bolus_order list
        ColumnVector tmp(1);
        tmp = convertTo<int>(next_bolus_string);
        bolus_order &= tmp;
    }
    n_bolus = bolus_order.Nrows();

    // Lowest limit of bolus duration. Default value is 0.4s
    tau_lowest = convertTo<double>(args.ReadWithDefault("tau_lowest", "0.4"));

    float fadeg = convertTo<double>(args.ReadWithDefault("fa", "30"));
    FA = fadeg * M_PI / 180;

    // setup crusher directions
    // crushdir.ReSize(4);
    // crushdir << 45.0 << -45.0 << 135.0 << -135.0; //in degees
    // crushdir = crushdir * M_PI/180;
    // cout << crushdir << endl;
    crushdir.ReSize(4, 3);
    crushdir << 1 << 1 << 1 << -1 << 1 << 1 << 1 << -1 << 1 << -1 << -1 << 1;

    crushdir /= sqrt(3); // make unit vectors;

    singleti = false; // normally we do multi TI ASL
                      /*if (tis.Nrows()==1) {
                    //only one TI therefore only infer on CBF and ignore other inference options
                    LOG << "--Single inversion time mode--" << endl;
                    LOG << "Only a sinlge inversion time has been supplied," << endl;
                    LOG << "Therefore only tissue perfusion will be inferred." << endl;
                    LOG << "-----" << endl;
                    singleti = true;
                    // force other inference options to be false
                    infertau = false; infert1 = false; inferart = false; //inferinveff = false;
                    }*/

    // add information about the parameters to the log
    LOG << "Inference using development model" << endl;
    LOG << "    Data parameters: #repeats = " << repeats << ", total number of bolus = " << n_bolus
        << ", duration (sec) between each bolus = " << delta_bolus << ", t1 = " << t1
        << ", t1b = " << t1b;
    LOG << ", bolus length (tau) = " << seqtau << endl;
    if (infertau)
    {
        LOG << "Infering on bolus length " << endl;
    }
    if (doard)
    {
        LOG << "ARD subsystem is enabled" << endl;
    }
    if (infertiss)
    {
        LOG << "Infertting on tissue component " << endl;
    }
    if (doard && tissard)
    {
        LOG << "ARD has been set on the tissue component " << endl;
    }
    if (inferart)
    {
        LOG << "Infering on artertial compartment " << endl;
    }
    if (doard && artard)
    {
        LOG << "ARD has been set on arterial compartment " << endl;
    }
    if (inferwm)
    {
        LOG << "Inferring on white matter component" << endl;
        if (doard && wmard)
        {
            LOG << "ARD has been set on wm component" << endl;
        }
    }
    if (infert1)
    {
        LOG << "Infering on T1 values " << endl;
    }
    LOG << "TIs: ";
    for (int i = 1; i <= tis.Nrows(); i++)
        LOG << tis(i) << " ";
    LOG << endl;
}

void TurboQuasarFwdModel::NameParams(vector<string> &names) const
{
    names.clear();

    if (infertiss)
    {
        names.push_back("ftiss");
        // if (!singleti)
        names.push_back("delttiss");
    }
    if (infertau && infertiss)
    {
        names.push_back("tautiss");
    }
    if (inferart)
    {
        names.push_back("fblood");
        names.push_back("deltblood");
    }
    if (infert1)
    {
        names.push_back("T_1");
        names.push_back("T_1b");
    }
    if (infertaub)
    {
        names.push_back("taublood");
    }
    /*if (inferart) {
    names.push_back("R");
    }*/

    if (inferwm)
    {
        names.push_back("fwm");
        names.push_back("deltwm");

        if (infertau)
            names.push_back("tauwm");
        if (infert1)
            names.push_back("T_1wm");

        if (usepve)
        {
            names.push_back("p_gm");
            names.push_back("p_wm");
        }
    }
    names.push_back("sp_log");
    names.push_back("s_log");

    if (inferart)
    {
        if (artdir)
        {
            names.push_back("thblood");
            names.push_back("phiblood");
            names.push_back("bvblood");
            // names.push_back("crusheff");
        }
        else
        {
            names.push_back("fbloodc1");
            names.push_back("fbloodc2");
            names.push_back("fbloodc3");
            names.push_back("fbloodc4");
            // names.push_back("fbloodc5");
        }
    }

    if (calibon)
    {
        names.push_back("g");
    }
}

void TurboQuasarFwdModel::HardcodedInitialDists(MVNDist &prior, MVNDist &posterior) const
{
    assert(prior.means.Nrows() == NumParams());

    SymmetricMatrix precisions = IdentityMatrix(NumParams()) * 1e-12;

    // Set priors
    // Tissue bolus perfusion
    if (infertiss)
    {
        prior.means(tiss_index()) = 0;
        precisions(tiss_index(), tiss_index()) = 1e-12;

        // if (!singleti) {
        // Tissue bolus transit delay
        prior.means(tiss_index() + 1) = 0.7;
        // prior.means(tiss_index()+1) = 0.6;
        // prior.means(tiss_index()+1) = 1;
        precisions(tiss_index() + 1, tiss_index() + 1) = 1;
        // precisions(tiss_index()+1,tiss_index()+1) = 1e-12;
        // }
    }

    // Tissue bolus length
    if (infertau && infertiss)
    {
        prior.means(tau_index()) = seqtau;
        // precisions(tau_index(),tau_index()) = 1;
        precisions(tau_index(), tau_index()) = 1;
    }

    if (infertaub)
    {
        prior.means(taub_index()) = seqtau;
        // precisions(taub_index(),taub_index()) = 1;
        precisions(taub_index(), taub_index()) = 1;
    }

    // Arterial Perfusion & bolus delay

    if (inferart)
    {
        int aidx = art_index();
        prior.means(aidx) = 0;
        precisions(aidx, aidx) = 1e-12;

        // prior.means(aidx+1) = 0.1;
        prior.means(aidx + 1) = 0.5;
        precisions(aidx + 1, aidx + 1) = 1;
    }

    // T1 & T1b
    if (infert1)
    {
        int tidx = t1_index();
        prior.means(tidx) = t1;
        prior.means(tidx + 1) = t1b;
        precisions(tidx, tidx) = 100;
        // if (calibon) precisions(tidx,tidx) = 1e99;
        precisions(tidx + 1, tidx + 1) = 100;
    }

    /* if (inferart) {
      prior.means(R_index()) = log(10);
      precisions(R_index(),R_index()) = 1;
      }*/

    if (inferwm)
    {
        int wmi = wm_index();
        prior.means(wmi) = 0;
        // prior.means(wmi+1) = 1.2;
        prior.means(wmi + 1) = 1;
        precisions(wmi, wmi) = 1e-12;
        precisions(wmi + 1, wmi + 1) = 1;

        if (infertau)
        {
            prior.means(wmi + 2) = seqtau;
            // precisions(wmi+2,wmi+2) = 1;
            precisions(wmi + 2, wmi + 2) = 1;
        }

        if (infert1)
        {
            prior.means(wmi + 3) = t1wm;
            precisions(wmi + 3, wmi + 3) = 100;
        }

        if (usepve)
        {
            // PV entries, the means get overwritten elsewhere if the right sort
            // of prior is specified
            // default is to allow both (NB artifically defies sum(pve)=1)
            int pvi = pv_index();
            prior.means(pvi) = 1;     // GM is first
            prior.means(pvi + 1) = 1; // WM is second

            // precisions are big as we treat PV parameters as correct
            // NB they are not accesible from the data anyway
            precisions(pvi, pvi) = 1e12;
            precisions(pvi + 1, pvi + 1) = 1e12;
        }
    }

    // dispersion parameters
    ColumnVector prvec(4);
    if (disptype == "none")
    {
        /* Modified by Moss 20160430
      1e99 is too large to be inverted to a non-zero value
      we use 1e12 here.
      */
        prvec << 0 << 1e12 << 0 << 1e12;
        // prvec << 0 << 1e99 << 0 << 1e99;
    }
    if (disptype == "gvf")
    {
        prvec << 2 << 10 << 0.7 << 10;
    }
    if (disptype == "gamma")
    {
        prvec << 2 << 10 << -0.3 << 10;
    }
    if (disptype == "gauss")
    {
        prvec << -1 << 10 << 0 << 1e12;
        // prvec << -1 << 10 << 0 << 1e99;
    }
    prior.means(disp_index()) = prvec(1); // 0.05;
    prior.means(disp_index() + 1) = prvec(3);
    precisions(disp_index(), disp_index()) = prvec(2); // 400;
    precisions(disp_index() + 1, disp_index() + 1) = prvec(4);

    // crushed data flow parameters
    int cidx = crush_index();
    if (inferart)
    {
        if (artdir)
        {
            prior.means(cidx) = 0.0;
            precisions(cidx, cidx) = 1e12; // artdir is multiplied by 1e6 to
                                           // make its numerical diff work
            prior.means(cidx + 1) = 0.0;
            precisions(cidx + 1, cidx + 1) = 1e12;
            prior.means(cidx + 2) = 1.6;
            precisions(cidx + 2, cidx + 2) = 0.1;
            // prior.means(cidx+1) = 0.8;
            // precisions(cidx+1,cidx+1) = 10;
        }
        else
        {
            prior.means(cidx) = 0.0;
            precisions(cidx, cidx) = 1e-12;
            prior.means(cidx + 1) = 0.0;
            precisions(cidx + 1, cidx + 1) = 1e-12;
            prior.means(cidx + 2) = 0.0;
            precisions(cidx + 2, cidx + 2) = 1e-12;
            prior.means(cidx + 3) = 0.0;
            precisions(cidx + 3, cidx + 3) = 1e-12;
            // prior.means(cidx+4) = 0.0;
            // precisions(cidx+4,cidx+4) = 1e-12;
        }
    }

    // calibration parameters
    if (calibon)
    {
        prior.means(calib_index()) = 1;                 // should be overwritten by an image
        precisions(calib_index(), calib_index()) = 100; // small uncertainty
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
        posterior.means(tiss_index()) = 10;
        precisions(tiss_index(), tiss_index()) = 1;
    }
    // Arterial perfusion
    if (inferart)
    {
        posterior.means(art_index()) = 10;
        precisions(art_index(), art_index()) = 1;
    }

    if (inferwm)
    {
        posterior.means(wm_index()) = 10;
        precisions(wm_index(), wm_index()) = 1;
    }

    // posterior.means(disp_index()) = 0.05;

    if (inferart)
    {
        if (!artdir)
        {
            posterior.means(cidx) = 10.0;
            precisions(cidx, cidx) = 1;
            posterior.means(cidx + 1) = 10.0;
            precisions(cidx + 1, cidx + 1) = 1;
            posterior.means(cidx + 2) = 10.0;
            precisions(cidx + 2, cidx + 2) = 1;
            posterior.means(cidx + 3) = 10.0;
            precisions(cidx + 3, cidx + 3) = 1;
            // posterior.means(cidx+4) = 10.0;
            // precisions(cidx+4,cidx+4) = 1;
        }
    }

    posterior.SetPrecisions(precisions);
}

void TurboQuasarFwdModel::Evaluate(const ColumnVector &params, ColumnVector &result) const
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

    // sensible limits on transit times
    if (infertiss)
    {
        if (params(tiss_index() + 1) > timax - 0.2)
        {
            paramcpy(tiss_index() + 1) = timax - 0.2;
        }
    }
    if (inferart)
    {
        if (params(art_index() + 1) > timax - 0.2)
        {
            paramcpy(art_index() + 1) = timax - 0.2;
        }
    }

    // parameters that are inferred - extract and give sensible names
    float ftiss;
    float delttiss;
    float tauset; // the value of tau set by the sequence (may be effectively
                  // infinite)
    float taubset;
    float fblood;
    float deltblood;
    float T_1;
    float T_1b;

    float pv_gm;
    float pv_wm;

    float fwm;
    float deltwm;
    float tauwmset;
    float T_1wm;

    float p;
    float s;

    // float blooddir;
    // float crusheff;
    float bloodth;
    float bloodphi;
    float bloodbv;
    float fbloodc1 = 0.0;
    float fbloodc2 = 0.0;
    float fbloodc3 = 0.0;
    float fbloodc4 = 0.0;
    // float fbloodc5;

    // extra calibration parameters
    float g;

    //  float RR;
    //  float inveffslope;
    // float trailingperiod;

    if (infertiss)
    {
        ftiss = paramcpy(tiss_index());
        // if (!singleti) {
        delttiss = paramcpy(tiss_index() + 1);
        //}
        // else {
        // only inferring on tissue perfusion, assume fixed value for tissue
        // arrival time
        // delttiss = 0;
        //}
    }
    else
    {
        ftiss = 0;
        delttiss = 0;
    }

    if (infertau && infertiss)
    {
        tauset = paramcpy(tau_index());
        // tauset = (dti * 0.5) * (tanh(tauset) + 1);
        // tauset = 0.1 * tanh(tauset) + dti - 0.1;
        tauset = ((dti - tau_lowest) / 2) * tanh(tauset) + (dti - (dti - tau_lowest) / 2);
        // cout << "tanh function used" << endl;
        // tauset = dti * ((1 / M_PI) * atan(tauset) + 0.5);
        // cout << tauset << endl;
        // getchar();
    }
    else
    {
        tauset = seqtau;
        // tauset = (dti * 0.5) * (tanh(tauset) + 1);
    }

    if (infertaub)
    {
        taubset = paramcpy(taub_index());
    }
    else
    {
        taubset = tauset;
    } // taub comes from the tauset which will be tissue value (or if no tissue
      // then that set by the sequence)

    if (inferart)
    {
        fblood = paramcpy(art_index());
        deltblood = paramcpy(art_index() + 1);
    }
    else
    {
        fblood = 0;
        deltblood = 0;
    }

    if (infert1)
    {
        T_1 = paramcpy(t1_index());
        T_1b = paramcpy(t1_index() + 1);

        // T1 cannot be zero!
        if (T_1 < 0.01)
            T_1 = 0.01;
        if (T_1b < 0.01)
            T_1b = 0.01;
    }
    else
    {
        T_1 = t1;
        T_1b = t1b;
    }

    /*if (inferart) {
    RR = exp( paramcpy(R_index()) );
    if (RR<1) RR=1;
    }*/

    if (inferwm)
    {
        fwm = paramcpy(wm_index());
        // fwm=20;
        deltwm = paramcpy(wm_index() + 1);

        if (infertau)
        {
            tauwmset = paramcpy(wm_index() + 2);
        }
        else
            tauwmset = seqtau;

        if (infert1)
        {
            T_1wm = paramcpy(wm_index() + 3);
            if (T_1 < 0.01)
                T_1 = 0.01;
        }
        else
            T_1wm = t1wm;

        if (usepve)
        {
            pv_gm = paramcpy(pv_index());
            pv_wm = paramcpy(pv_index() + 1);
        }
        else
        {
            pv_gm = 1;
            pv_wm = 1;
        }
    }
    else
    {
        fwm = 0;
        deltwm = 0;
        T_1wm = t1wm;
        tauwmset = seqtau;

        pv_gm = 1;
        pv_wm = 1;
    }

    if (inferart)
    {
        if (artdir)
        {
            bloodth = params(crush_index());
            bloodth = 2 * M_PI * tanh(bloodth * 1e6); // convert to within
                                                      // -360->360 range
                                                      // (reasonably linear over
                                                      // the -180->180 range);
            bloodphi = params(crush_index() + 1);
            bloodphi = 2 * M_PI * tanh(bloodphi * 1e6);
            bloodbv = paramcpy(crush_index() + 2);
            // blooddir = params(crush_index());
            // blooddir = 2*M_PI*tanh(blooddir*1e6); //convert to within
            // -360->360 range (reasonably linear over the -180->180 range)
            // crusheff = 1.0; //paramcpy(crush_index()+1);
            // if(crusheff>1.0) crusheff=1.0;
        }
        else
        {
            fbloodc1 = paramcpy(crush_index());
            fbloodc2 = paramcpy(crush_index() + 1);
            fbloodc3 = paramcpy(crush_index() + 2);
            fbloodc4 = paramcpy(crush_index() + 3);
            // fbloodc5 = paramcpy(crush_index()+4);
        }
    }

    // dispersion parameters
    // p = paramcpy(disp_index());
    // if (p>timax-0.2) { p = timax-0.2; }
    s = exp(params(disp_index()));
    if (disptype == "gamma" || disptype == "gvf")
    {
        float sp = exp(params(disp_index() + 1));
        if (sp > 10)
            sp = 10;
        p = sp / s;
    }

    // calibration parameters
    // NOTE: T1 of tissue is handled by infert1 and not this calibration routine
    if (calibon)
    {
        g = params(calib_index());
    }
    else
    {
        g = 1;
    }

    float lambdagm = 0.9; // the general 'all' tissue lambda value
    float lambdawm = 0.82;
    if (inferwm)
    {
        lambdagm = 0.98;
    }

    // flip angle correction (only if calibon)
    float FAtrue = FA;
    float dg = 0.023;
    if (calibon)
        FAtrue = (g + dg) * FA;

    // cout << T_1 << " " << FAtrue << " ";

    float T_1app = 1 / (1 / T_1 + 0.01 / lambdagm - log(cos(FAtrue)) / dti);
    float T_1appwm = 1 / (1 / T_1wm + 0.01 / lambdawm - log(cos(FAtrue)) / dti);

    // Need to be careful with T1 values
    if (T_1b < 0.1)
        T_1b = 0.1;
    if (T_1app < 0.1)
        T_1app = 0.1;
    if (fabs(T_1app - T_1b) < 0.01)
        T_1app += 0.01;
    if (T_1appwm < 0.1)
        T_1appwm = 0.1;
    if (fabs(T_1appwm - T_1b) < 0.01)
        T_1appwm += 0.01;

    // calculate the 'LL T1' of the blood
    float T_1ll = 1 / (1 / T_1b - log(cos(FAtrue)) / dti);
    float deltll = deltblood; // the arrival time of the blood within the
                              // readout region i.e. where it sees the LL
                              // pulses.

    float tau = tauset;   // bolus length as seen by kintic curve
    float taub = taubset; // bolus length of blood as seen in signal
    float tauwm = tauwmset;

    // float F=0;
    // float Fwm=0;
    // float Fblood=0;
    ColumnVector kctissue(tis.Nrows());
    kctissue = 0.0;
    ColumnVector kcblood(tis.Nrows());
    kcblood = 0.0;
    ColumnVector kcwm(tis.Nrows());
    kcwm = 0.0;

    ColumnVector thetis;
    thetis = tis;
    thetis += slicedt * coord_z; // account here for an increase in delay between slices

    // generate the kinetic curves
    if (disptype == "none")
    {
        /* Added by Moss */
        if (simulation)
        {
            T_1b = 1.60;
            T_1 = 1.3;
            T_1app = T_1;
            T_1ll = T_1b;
        }

        if (infertiss)
            kctissue = kctissue_nodisp(thetis, delttiss, tau, T_1b, T_1app, deltll, T_1ll, n_bolus,
                delta_bolus, bolus_order);
        // cout << kctissue << endl;
        if (inferwm)
            kcwm = kctissue_nodisp(thetis, deltwm, tauwm, T_1b, T_1appwm, deltll, T_1ll, n_bolus,
                delta_bolus, bolus_order);
        if (inferart)
            kcblood = kcblood_nodisp(
                thetis, deltblood, taub, T_1b, deltll, T_1ll, n_bolus, delta_bolus, bolus_order);
        // cout << kcblood << endl;
    }
    else if (disptype == "gamma")
    {
        if (infertiss)
            kctissue = kctissue_gammadisp(thetis, delttiss, tau, T_1b, T_1app, s, p, deltll, T_1ll,
                n_bolus, delta_bolus, bolus_order);

        // cout << kctissue << endl;
        if (inferwm)
            kcwm = kctissue_gammadisp(thetis, deltwm, tauwm, T_1b, T_1appwm, s, p, deltll, T_1ll,
                n_bolus, delta_bolus, bolus_order);
        if (inferart)
            kcblood = kcblood_gammadisp(thetis, deltblood, taub, T_1b, s, p, deltll, T_1ll, n_bolus,
                delta_bolus, bolus_order);

        // cout << kcblood << endl;
    }
    else if (disptype == "gvf")
    {
        if (infertiss)
            kctissue = kctissue_gvf(thetis, delttiss, tau, T_1b, T_1app, s, p, deltll, T_1ll,
                n_bolus, delta_bolus, bolus_order);

        // cout << kctissue << endl;
        if (inferwm)
            kcwm = kctissue_gvf(thetis, deltwm, tauwm, T_1b, T_1appwm, s, p, deltll, T_1ll, n_bolus,
                delta_bolus, bolus_order);
        if (inferart)
            kcblood = kcblood_gvf(thetis, deltblood, taub, T_1b, s, p, deltll, T_1ll, n_bolus,
                delta_bolus, bolus_order);

        // cout << kcblood << endl;
    }
    else if (disptype == "gauss")
    {
        if (infertiss)
            kctissue = kctissue_gaussdisp(thetis, delttiss, tau, T_1b, T_1app, s, s, deltll, T_1ll,
                n_bolus, delta_bolus, bolus_order);

        // cout << kctissue << endl;
        if (inferwm)
            kcwm = kctissue_gaussdisp(thetis, deltwm, tauwm, T_1b, T_1appwm, s, s, deltll, T_1ll,
                n_bolus, delta_bolus, bolus_order);
        if (inferart)
            kcblood = kcblood_gaussdisp(thetis, deltblood, tau, T_1b, s, s, deltll, T_1ll, n_bolus,
                delta_bolus, bolus_order);

        // cout << kcblood << endl;
    }
    else
    {
        throw Exception("Unrecognised dispersion model ");
    }

    /* KC debugging
  T_1 = 1.3;
  T_1app = 1/( 1/T_1 + 0.01/lambdagm);
  cout << T_1app << "  " << endl;
  T_1b = 1.6; tau=1; delttiss=0.7; s=100, p=0.05;
  kctissue=kctissue_nodisp(tis,delttiss,tau,T_1b,T_1app);
  cout << kctissue.t() << endl;
  kctissue=kctissue_gammadisp(tis,delttiss,tau,T_1b,T_1app,s,p);
  cout << kctissue.t() << endl;
  kctissue=kctissue_gvf(tis,delttiss,T_1b,T_1app,s,p);
  cout << kctissue.t() << endl;

  assert(1==0);
  */

    // Nan catching
    bool cont = true;
    int it = 1;
    while (cont)
    {
        if (isnan(kctissue(it)) | isinf(kctissue(it)))
        {
            LOG << "Warning NaN in kctissue" << endl;
            LOG << "params: " << params.t() << endl;
            LOG << "kctissue: " << kctissue.t() << endl;
            cont = false;
            kctissue = 0.0;
        }
        it++;
        if (it > kctissue.Nrows())
            cont = false;
    }

    cont = true;
    it = 1;
    while (cont)
    {
        if (isnan(kcblood(it)) | isinf(kcblood(it)))
        {
            LOG << "Warning NaN in kcblood" << endl;
            LOG << "params: " << params.t() << endl;
            LOG << "kcblood: " << kcblood.t() << endl;
            cont = false;
            kcblood = 0.0;
        }
        it++;
        if (it > kcblood.Nrows())
            cont = false;
    }

    // assemble the result
    int nti = tis.Nrows();
    int nphases = 6;
    if (onephase)
        nphases = 1;
    result.ReSize(tis.Nrows() * repeats * nphases);

    ColumnVector artweight(crushdir.Nrows());
    if (artdir)
    {
        // sot out the arterial weightings for all the crushed images
        artweight = 1.0;
        // float angle;
        ColumnVector artdir(3);
        artdir(1) = sin(bloodphi) * cos(bloodth);
        artdir(2) = sin(bloodphi) * sin(bloodth);
        artdir(3) = cos(bloodphi);

        for (int i = 1; i <= crushdir.Nrows(); i++)
        {
            // artweight(i) = 1.0 -
            // std::max(DotProduct(artdir,crushdir.Row(i)),0.0); # original
            // linear approximation
            // artweight(i) = exp( -bloodbv *
            // std::max(DotProduct(artdir,crushdir.Row(i)),0.0) ); # exponential
            // based on simple diffusion expt
            artweight(i) = Sinc(2 * bloodbv * std::max(DotProduct(artdir, crushdir.Row(i)),
                                                  0.0)); // based on laminar flow profile c.f.
                                                         // perfusion tensor imaging

            // angle = fabs(crushdir(i) - blooddir);
            // if (angle<M_PI/2) {
            //  artweight(i) = crusheff*sin(angle) + 1.0 - crusheff;
            //}
        }
    }

    for (int it = 1; it <= tis.Nrows(); it++)
    {
        /* output */
        // loop over the repeats
        for (int rpt = 1; rpt <= repeats; rpt++)
        {
            // go through all the phases
            // phases: C1, C2, NC, C3, C4, NC, (NC LFA)
            int tiref = (it - 1) * repeats + rpt;

            if (onephase)
            {
                result(tiref)
                    = pv_gm * ftiss * kctissue(it) + fblood * kcblood(it) + pv_wm * fwm * kcwm(it);
            }
            else if (artdir)
            {
                result(tiref) = pv_gm * ftiss * kctissue(it) + artweight(1) * fblood * kcblood(it)
                    + pv_wm * fwm * kcwm(it);
                result(nti * repeats + tiref) = pv_gm * ftiss * kctissue(it)
                    + artweight(2) * fblood * kcblood(it) + pv_wm * fwm * kcwm(it);
                result(2 * nti * repeats + tiref)
                    = pv_gm * ftiss * kctissue(it) + fblood * kcblood(it) + pv_wm * fwm * kcwm(it);
                result(3 * nti * repeats + tiref) = pv_gm * ftiss * kctissue(it)
                    + artweight(3) * fblood * kcblood(it) + pv_wm * fwm * kcwm(it);
                result(4 * nti * repeats + tiref) = pv_gm * ftiss * kctissue(it)
                    + artweight(4) * fblood * kcblood(it) + pv_wm * fwm * kcwm(it);
                result(5 * nti * repeats + tiref)
                    = pv_gm * ftiss * kctissue(it) + fblood * kcblood(it) + pv_wm * fwm * kcwm(it);
                // result( 6*nti*repeats + tiref ) = pv_gm*ftiss*kctissue +
                // fblood*kcblood + pv_wm*fwm*kcwm;
            }
            else
            {
                result(tiref) = pv_gm * ftiss * kctissue(it) + fbloodc1 * kcblood(it)
                    + pv_wm * fwm * kcwm(it);
                result(nti * repeats + tiref) = pv_gm * ftiss * kctissue(it)
                    + fbloodc2 * kcblood(it) + pv_wm * fwm * kcwm(it);
                result(2 * nti * repeats + tiref)
                    = pv_gm * ftiss * kctissue(it) + fblood * kcblood(it) + pv_wm * fwm * kcwm(it);
                result(3 * nti * repeats + tiref) = pv_gm * ftiss * kctissue(it)
                    + fbloodc3 * kcblood(it) + pv_wm * fwm * kcwm(it);
                result(4 * nti * repeats + tiref) = pv_gm * ftiss * kctissue(it)
                    + fbloodc4 * kcblood(it) + pv_wm * fwm * kcwm(it);
                result(5 * nti * repeats + tiref)
                    = pv_gm * ftiss * kctissue(it) + fblood * kcblood(it) + pv_wm * fwm * kcwm(it);
                // result( 6*nti*repeats + tiref ) = pv_gm*ftiss*kctissue +
                // fblood*kcblood + pv_wm*fwm*kcwm;
            }
        }
    }
    // cout << result.t();

    return;
}

void TurboQuasarFwdModel::SetupARD(const MVNDist &theta, MVNDist &thetaPrior, double &Fard)
{
    if (doard)
    {
        // sort out ARD indices
        if (tissard)
            ard_index.push_back(tiss_index());
        if (artard)
            ard_index.push_back(art_index());
        if (wmard)
            ard_index.push_back(wm_index());

        Fard = 0;

        int ardindex;
        for (unsigned int i = 0; i < ard_index.size(); i++)
        {
            // iterate over all ARD parameters
            ardindex = ard_index[i];

            SymmetricMatrix PriorPrec;
            PriorPrec = thetaPrior.GetPrecisions();

            PriorPrec(ardindex, ardindex) = 1e-12; // set prior to be initally non-informative

            thetaPrior.SetPrecisions(PriorPrec);

            thetaPrior.means(ardindex) = 0;

            // set the Free energy contribution from ARD term
            SymmetricMatrix PostCov = theta.GetCovariance();
            double b
                = 2 / (theta.means(ardindex) * theta.means(ardindex) + PostCov(ardindex, ardindex));
            Fard += -1.5 * (log(b) + digamma(0.5)) - 0.5 - gammaln(0.5)
                - 0.5 * log(b); // taking c as 0.5 - which it will be!
        }
    }
    return;
}

void TurboQuasarFwdModel::UpdateARD(const MVNDist &theta, MVNDist &thetaPrior, double &Fard) const
{
    if (doard)
        Fard = 0;
    {
        int ardindex;
        for (unsigned int i = 0; i < ard_index.size(); i++)
        {
            // iterate over all ARD parameters
            ardindex = ard_index[i];

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
            Fard += -1.5 * (log(b) + digamma(0.5)) - 0.5 - gammaln(0.5)
                - 0.5 * log(b); // taking c as 0.5 - which it will be!
        }
    }

    return;
}

// --- Kinetic curve functions ---

// Arterial
ColumnVector TurboQuasarFwdModel::kcblood_nodisp(const ColumnVector &tis, float deltblood,
    float taub, float T_1bin, float deltll, float T_1ll, int n_bolus, float delta_bolus,
    const ColumnVector &bolus_order) const
{
    ColumnVector kcblood(tis.Nrows());
    kcblood = 0.0;
    // Non dispersed arterial curve (pASL)
    float ti = 0.0;
    float T_1b;

    // Turbo QUASAR specific parameters
    float current_value;
    int n_bolus_arrived = 0;          // bolus arrived (passed) or processed
    float bolus_time_passed = 0;      // total time passed since the first bolus
                                      // arrived (excluding arrival time)
    float current_arrival_time = 0;   // total time since TI1 (including arrival time)
    float current_bolus_duration = 0; // Current bolus duration

    while (n_bolus_arrived < n_bolus)
    {
        bolus_time_passed = n_bolus_arrived * delta_bolus;

        current_arrival_time = bolus_time_passed + deltblood;

        current_bolus_duration
            = taub * bolus_order(n_bolus_arrived + 1); // Column vector index starts from one

        for (int it = 1; it <= tis.Nrows(); it++)
        {
            ti = tis(it);

            // correct Look-locker T1
            if (ti < deltll)
            {
                T_1b = T_1bin;
            }
            else
            {
                T_1b = T_1ll;
            }

            // start model fitting
            if (ti < current_arrival_time)
            {
                // use a arti�fical lead in period for arterial bolus to improve
                // model fitting
                kcblood(it) = kcblood(it)
                    + 2 * exp(-deltblood / T_1b)
                        * (0.98 * exp(((ti - bolus_time_passed) - deltblood) / 0.05)
                              + 0.02 * ti / deltblood);
            }

            else if (ti > current_arrival_time
                && ti <= (current_arrival_time + current_bolus_duration))
            {
                kcblood(it) = kcblood(it) + 2 * exp(-(ti - bolus_time_passed) / T_1b);
            }

            else
            {
                // artifical lead out period for taub model fitting
                current_value = 2 * exp(-(deltblood + current_bolus_duration) / T_1b);
                current_value *= (0.98
                        * exp(-((ti - bolus_time_passed) - deltblood - current_bolus_duration)
                                      / 0.05)
                    + 0.02 * (1
                                 - ((ti - bolus_time_passed) - deltblood - current_bolus_duration)
                                     / 5));
                if (current_value >= 0)
                {
                    kcblood(it) = kcblood(it) + current_value;
                }
                else
                {
                    kcblood(it) = kcblood(it) + 0;
                }
            }
        }

        n_bolus_arrived++;
    }

    /*
  for(int it=1; it<=tis.Nrows(); it++)
    {
      ti = tis(it);

      if (ti< deltll) T_1b = T_1bin;
      else            T_1b = T_1ll;

      if(ti < deltblood)
  {
    kcblood(it) = 2 * exp(-deltblood/T_1b) * (0.98 * exp( (ti-deltblood)/0.05 )
  + 0.02 * ti/deltblood );
    // use a arti�fical lead in period for arterial bolus to improve model
  fitting
  }
      else if(ti >= deltblood && ti <= (deltblood + taub))
  {
    kcblood(it) = 2 * exp(-ti/T_1b);
  }
      else //(ti > deltblood + tau)
  {
    kcblood(it) = 2 * exp(-(deltblood+taub)/T_1b);
    kcblood(it) *= (0.98 * exp( -(ti - deltblood - taub)/0.05) + 0.02 * (1-(ti -
  deltblood - taub)/5));
    // artifical lead out period for taub model fitting
    if (kcblood(it)<0) kcblood(it)=0; //negative values are possible with the
  lead out period equation
  }

    }
  */

    return kcblood;
}

/*
ColumnVector TurboQuasarFwdModel::kcblood_gammadisp(const ColumnVector &tis,
float deltblood, float taub, float T_1bin, float s, float p, float deltll, float
T_1ll) const
{
    ColumnVector kcblood(tis.Nrows());
    kcblood = 0.0;
    float T_1b;
    // Gamma dispersed arterial curve (pASL)

    float k = 1 + p * s;
    float ti = 0.0;

    for (int it = 1; it <= tis.Nrows(); it++)
    {
        ti = tis(it);

        if (ti < deltll)
            T_1b = T_1bin;
        else
            T_1b = T_1ll;

        if (ti < deltblood)
        {
            kcblood(it) = 0.0;
        }
        else if (ti >= deltblood && ti <= (deltblood + taub))
        {
            kcblood(it) = 2 * exp(-ti / T_1b) * (1 - igamc(k, s * (ti -
deltblood)));
        }
        else //(ti > deltblood + taub)
        {
            kcblood(it) = 2 * exp(-ti / T_1b) * (igamc(k, s * (ti - deltblood -
taub)) - igamc(k, s * (ti - deltblood)));
        }
        //if (isnan(kcblood(it))) { kcblood(it)=0.0; cout << "Warning NaN in
blood KC"; }
    }
    return kcblood;
}
*/

ColumnVector TurboQuasarFwdModel::kcblood_gammadisp(const ColumnVector &tis, float deltblood,
    float taub, float T_1bin, float s, float p, float deltll, float T_1ll, int n_bolus_total,
    float delta_bolus, const ColumnVector &bolus_order) const
{
    ColumnVector kcblood(tis.Nrows());
    kcblood = 0.0;
    float T_1b;
    // Gamma dispersed arterial curve (pASL)

    float k = 1 + p * s;
    float ti = 0.0;

    // Turbo QUASAR specific parameters
    float current_value;
    int n_bolus_arrived = 0;          // bolus arrived (passed) or processed
    float bolus_time_passed = 0;      // total time passed since the first bolus
                                      // arrived (excluding arrival time)
    float current_arrival_time = 0;   // total time since TI1 (including arrival time)
    float current_bolus_duration = 0; // Current bolus duration

    while (n_bolus_arrived < n_bolus_total)
    {
        bolus_time_passed = n_bolus_arrived * delta_bolus;

        current_arrival_time = bolus_time_passed + deltblood;

        current_bolus_duration
            = taub * bolus_order(n_bolus_arrived + 1); // Column vector index starts from one

        for (int it = 1; it <= tis.Nrows(); it++)
        {
            ti = tis(it);

            if (ti < deltll)
                T_1b = T_1bin;
            else
                T_1b = T_1ll;

            if (ti < current_arrival_time)
            {
                kcblood(it) = kcblood(it) + 0.0;
            }
            else if (ti >= current_arrival_time
                && ti <= (current_arrival_time + current_bolus_duration))
            {
                kcblood(it) = kcblood(it)
                    + 2 * exp(-(ti - bolus_time_passed) / T_1b)
                        * (1 - igamc(k, s * ((ti - bolus_time_passed) - deltblood)));
            }
            else //(ti > deltblood + taub)
            {
                kcblood(it) = kcblood(it)
                    + 2 * exp(-(ti - bolus_time_passed) / T_1b)
                        * (igamc(k,
                               s * ((ti - bolus_time_passed) - deltblood - current_bolus_duration))
                              - igamc(k, s * ((ti - bolus_time_passed) - deltblood)));
            }
            // if (isnan(kcblood(it))) { kcblood(it)=0.0; cout << "Warning NaN
            // in blood KC"; }
        }

        n_bolus_arrived++;
    }

    return kcblood;
}

/*
ColumnVector TurboQuasarFwdModel::kcblood_gvf(const ColumnVector &tis, float
deltblood, float taub, float T_1bin, float s, float p, float deltll, float
T_1ll) const
{
    ColumnVector kcblood(tis.Nrows());
    kcblood = 0.0;
    float T_1b;
    if (s < 1)
        s = 1; // dont allow this to become too extreme

    // gamma variate arterial curve
    // NOTES: this model is only suitable for pASL
    //        no explicit taub (see below). However, it does scale the area
under the curve
    //                                      (since it affects the original
ammount of labeled blood).
    float ti = 0.0;

    for (int it = 1; it <= tis.Nrows(); it++)
    {
        ti = tis(it);

        if (ti < deltll)
            T_1b = T_1bin;
        else
            T_1b = T_1ll;

        if (ti < deltblood)
        {
            kcblood(it) = 0.0;
        }
        else // if(ti >= deltblood) && ti <= (deltblood + taub))
        {
            kcblood(it) = 2 * exp(-ti / T_1b) * gvf(ti - deltblood, s, p);
        }
        // we do not have bolus duration with a GVF AIF - the duration is
'built' into the function shape
        //else //(ti > deltblood + taub)
        //  {
        //    kcblood(it) = 0.0 ;
        //
        //  }
    }
    return kcblood * taub;
}
*/

ColumnVector TurboQuasarFwdModel::kcblood_gvf(const ColumnVector &tis, float deltblood, float taub,
    float T_1bin, float s, float p, float deltll, float T_1ll, int n_bolus_total, float delta_bolus,
    const ColumnVector &bolus_order) const
{
    ColumnVector kcblood(tis.Nrows());
    kcblood = 0.0;
    float T_1b;
    if (s < 1)
        s = 1; // dont allow this to become too extreme

    // gamma variate arterial curve
    // NOTES: this model is only suitable for pASL
    //        no explicit taub (see below). However, it does scale the area
    //        under the curve
    //                                      (since it affects the original
    //                                      ammount of labeled blood).
    float ti = 0.0;

    // Turbo QUASAR specific parameters
    int n_bolus_arrived = 0;          // bolus arrived (passed) or processed
    float bolus_time_passed = 0;      // total time passed since the first bolus
                                      // arrived (excluding arrival time)
    float current_arrival_time = 0;   // total time since TI1 (including arrival time)
    float current_bolus_duration = 0; // Current bolus duration

    while (n_bolus_arrived < n_bolus_total)
    {
        bolus_time_passed = n_bolus_arrived * delta_bolus;

        current_arrival_time = bolus_time_passed + deltblood;

        current_bolus_duration
            = taub * bolus_order(n_bolus_arrived + 1); // Column vector index starts from zero

        for (int it = 1; it <= tis.Nrows(); it++)
        {
            ti = tis(it);

            if (ti < deltll)
                T_1b = T_1bin;
            else
                T_1b = T_1ll;

            if (ti < current_arrival_time)
            {
                kcblood(it) = kcblood(it) + 0.0;
            }
            else // if(ti >= deltblood) && ti <= (deltblood + taub))
            {
                kcblood(it) = kcblood(it)
                    + 2 * exp(-(ti - bolus_time_passed) / T_1b)
                        * gvf((ti - bolus_time_passed) - deltblood, s, p) * current_bolus_duration;
            }
            // we do not have bolus duration with a GVF AIF - the duration is
            // 'built' into the function shape
            // else //(ti > deltblood + taub)
            //  {
            //    kcblood(it) = 0.0 ;
            //
            //  }
        }

        n_bolus_arrived++;
    }
    return kcblood;
}

/*
ColumnVector TurboQuasarFwdModel::kcblood_gaussdisp(const ColumnVector &tis,
float deltblood, float taub, float T_1bin, float sig1, float sig2, float deltll,
float T_1ll) const
{
    ColumnVector kcblood(tis.Nrows());
    kcblood = 0.0;
    float T_1b;
    // Gaussian dispersion arterial curve
    // after Hrabe & Lewis, MRM, 2004
    float ti = 0.0;
    float sqrt2 = sqrt(2);

    for (int it = 1; it <= tis.Nrows(); it++)
    {
        ti = tis(it);

        if (ti < deltll)
            T_1b = T_1bin;
        else
            T_1b = T_1ll;

        kcblood(it) = 0.5 * exp(-ti / T_1b) * (MISCMATHS::erf((ti - deltblood) /
(sqrt2 * sig1)) - MISCMATHS::erf((ti - deltblood + taub) / (sqrt2 * sig2)));
    }
    return kcblood;
}
*/

ColumnVector TurboQuasarFwdModel::kcblood_gaussdisp(const ColumnVector &tis, float deltblood,
    float taub, float T_1bin, float sig1, float sig2, float deltll, float T_1ll, int n_bolus_total,
    float delta_bolus, const ColumnVector &bolus_order) const
{
    ColumnVector kcblood(tis.Nrows());
    kcblood = 0.0;
    float T_1b;
    // Gaussian dispersion arterial curve
    // after Hrabe & Lewis, MRM, 2004
    float ti = 0.0;
    float sqrt2 = sqrt(2);

    // Turbo QUASAR specific parameters
    int n_bolus_arrived = 0;          // bolus arrived (passed) or processed
    float bolus_time_passed = 0;      // total time passed since the first bolus
                                      // arrived (excluding arrival time)
    float current_arrival_time = 0;   // total time since TI1 (including arrival time)
    float current_bolus_duration = 0; // Current bolus duration

    while (n_bolus_arrived < n_bolus_total)
    {
        bolus_time_passed = n_bolus_arrived * delta_bolus;

        current_arrival_time = bolus_time_passed + deltblood;

        current_bolus_duration
            = taub * bolus_order(n_bolus_arrived + 1); // Column vector index starts from zero

        for (int it = 1; it <= tis.Nrows(); it++)
        {
            ti = tis(it);

            if (ti < deltll)
                T_1b = T_1bin;
            else
                T_1b = T_1ll;

            kcblood(it) = 0.5 * exp(-(ti - bolus_time_passed) / T_1b)
                * (MISCMATHS::erf(((ti - bolus_time_passed) - deltblood) / (sqrt2 * sig1))
                              - MISCMATHS::erf(
                                    ((ti - bolus_time_passed) - deltblood + current_bolus_duration)
                                    / (sqrt2 * sig2)));
        }

        n_bolus_arrived++;
    }
    return kcblood;
}

// Tissue
ColumnVector TurboQuasarFwdModel::kctissue_nodisp(const ColumnVector &tis, float delttiss,
    float tau, float T_1bin, float T_1app, float deltll, float T_1ll, int n_bolus_total,
    float delta_bolus, const ColumnVector &bolus_order) const
{
    ColumnVector kctissue(tis.Nrows());
    kctissue = 0.0;
    float ti = 0.0;
    float T_1b;

    // hardcoded parameters
    // int n_bolus_total = 4; // total number of boluses
    // float delta_bolus = 0.6; // actual gap between boluses

    // Tissue kinetic curve no dispersion (pASL)
    // Buxton (1998) model

    float R;

    // Turbo QUASAR specific parameters
    int n_bolus_arrived = 0;          // bolus arrived (passed) or processed
    float bolus_time_passed = 0;      // total time passed since the first bolus
                                      // arrived (excluding arrival time)
    float current_arrival_time = 0;   // total time since TI1 (including arrival time)
    float current_bolus_duration = 0; // Current bolus duration

    // float k;

    // tau = ti * (1 / M_PI * atan(k) + 0.5);

    // now start model fitting
    while (n_bolus_arrived < n_bolus_total)
    {
        bolus_time_passed = n_bolus_arrived * delta_bolus;

        current_arrival_time = bolus_time_passed + delttiss;

        current_bolus_duration
            = tau * bolus_order(n_bolus_arrived + 1); // Column vector index starts from zero

        for (int it = 1; it <= tis.Nrows(); it++)
        {
            // cout << tau << "hahaha" << endl;

            ti = tis(it);
            float F = 2 * exp(-(ti - bolus_time_passed) / T_1app);

            if (ti < deltll)
                T_1b = T_1bin;
            else
                T_1b = T_1ll;

            R = 1 / T_1app - 1 / T_1b;

            if (ti < current_arrival_time)
            {
                kctissue(it) = kctissue(it) + 0;
            }

            else if (ti >= current_arrival_time
                && ti <= (current_arrival_time + current_bolus_duration))
            {
                kctissue(it) = kctissue(it)
                    + F / R * ((exp(R * (ti - bolus_time_passed)) - exp(R * delttiss)));
                // kctissue(it) = F/R * ( (exp(R*ti) - exp(R*delttiss)) ) * exp(
                // (-1) * R * ti );
            }

            else //(ti > delttiss + tau)
            {
                kctissue(it) = kctissue(it)
                    + F / R * ((exp(R * (delttiss + current_bolus_duration)) - exp(R * delttiss)));
                // kctissue(it) = F/R * ( (exp(R*(delttiss+tau)) -
                // exp(R*delttiss))  ) * exp( (-1) * R * ti );
            }
        }

        n_bolus_arrived++;
    }

    /*
  for(int it=1; it<=tis.Nrows(); it++) {
    ti = tis(it);
    float F = 2 * exp(-ti/T_1app);
    if (ti< deltll)
      T_1b = T_1bin;
    else
      T_1b = T_1ll;

    R = 1/T_1app - 1/T_1b;
    if(ti < delttiss) {
      kctissue(it) = 0;
    }
    else if(ti >= delttiss && ti <= (delttiss + tau)) {
      kctissue(it) = F/R * ( (exp(R*ti) - exp(R*delttiss)) );
      //kctissue(it) = F/R * ( (exp(R*ti) - exp(R*delttiss)) ) * exp( (-1) * R *
  ti );
    }
    else //(ti > delttiss + tau)
    {
      kctissue(it) = F/R * ( (exp(R*(delttiss+tau)) - exp(R*delttiss))  );
      //kctissue(it) = F/R * ( (exp(R*(delttiss+tau)) - exp(R*delttiss))  ) *
  exp( (-1) * R * ti );
    }
  }
  */

    return kctissue;
}

/*
ColumnVector TurboQuasarFwdModel::kctissue_gammadisp(const ColumnVector &tis,
float delttiss, float tau, float T_1bin, float T_1app, float s, float p, float
deltll, float T_1ll) const
{
    ColumnVector kctissue(tis.Nrows());
    kctissue = 0.0;
    float ti = 0.0;
    float A;
    float B;
    float C;
    float k = 1 + p * s;
    float T_1b;

    // cout << T_1app << " " << A << " " << B << " "<< C << " " << endl ;

    for (int it = 1; it <= tis.Nrows(); it++)
    {
        ti = tis(it);

        if (ti < deltll)
            T_1b = T_1bin;
        else
            T_1b = T_1ll;

        A = T_1app - T_1b;
        B = A + s * T_1app * T_1b;
        if (B < 1e-12)
            B = 1e-12; //really shouldn't happen, but combination of parameters
may arise in artefactual voxels?
        C = pow(s - 1 / T_1app + 1 / T_1b, p * s);
        if (s - 1 / T_1app + 1 / T_1b <= 0)
            C = 1e-12; //really shouldn't happen, but combination of parameters
may arise in artefactual voxels?

        if (ti < delttiss)
        {
            kctissue(it) = 0;
        }
        else if (ti >= delttiss && ti <= (delttiss + tau))
        {
            kctissue(it) = 2 * 1 / A * exp(-(T_1app * delttiss + (T_1app + T_1b)
* ti) / (T_1app * T_1b)) * T_1app * T_1b * pow(B, -k) * (exp(delttiss / T_1app +
ti / T_1b) * pow(s * T_1app * T_1b, k) * (1 - igamc(k, B / (T_1app * T_1b) * (ti
- delttiss))) + exp(delttiss / T_1b + ti / T_1app) * pow(B, k) * (-1 + igamc(k,
s * (ti - delttiss))));
        }
        else //(ti > delttiss + tau)
        {
            kctissue(it) = 2 * 1 / (A * B) * (exp(-A / (T_1app * T_1b) *
(delttiss + tau) - ti / T_1app) * T_1app * T_1b / C * (pow(s, k) * T_1app * T_1b
* (-1 + exp((-1 / T_1app + 1 / T_1b) * tau) * (1 - igamc(k, B / (T_1app * T_1b)
* (ti - delttiss))) + igamc(k, B / (T_1app * T_1b) * (ti - delttiss - tau))) -
exp(-A / (T_1app * T_1b) * (ti - delttiss - tau)) * C * B * (igamc(k, s * (ti -
delttiss - tau)) - igamc(k, s * (ti - delttiss)))));
        }
        //if (isnan(kctissue(it))) { kctissue(it)=0.0; cout << "Warning NaN in
tissue KC"; }
    }
    // cout << kctissue.t() << endl;
    return kctissue;
}
*/

ColumnVector TurboQuasarFwdModel::kctissue_gammadisp(const ColumnVector &tis, float delttiss,
    float tau, float T_1bin, float T_1app, float s, float p, float deltll, float T_1ll,
    int n_bolus_total, float delta_bolus, const ColumnVector &bolus_order) const
{
    ColumnVector kctissue(tis.Nrows());
    kctissue = 0.0;
    float ti = 0.0;
    float A;
    float B;
    float C;
    float k = 1 + p * s;
    float T_1b;

    // Turbo QUASAR specific parameters
    int n_bolus_arrived = 0;          // bolus arrived (passed) or processed
    float bolus_time_passed = 0;      // total time passed since the first bolus
                                      // arrived (excluding arrival time)
    float current_arrival_time = 0;   // total time since TI1 (including arrival time)
    float current_bolus_duration = 0; // Current bolus duration

    // cout << T_1app << " " << A << " " << B << " "<< C << " " << endl ;
    while (n_bolus_arrived < n_bolus_total)
    {
        bolus_time_passed = n_bolus_arrived * delta_bolus;

        current_arrival_time = bolus_time_passed + delttiss;

        current_bolus_duration
            = tau * bolus_order(n_bolus_arrived + 1); // Column vector index starts from zero

        for (int it = 1; it <= tis.Nrows(); it++)
        {
            ti = tis(it);

            if (ti < deltll)
                T_1b = T_1bin;
            else
                T_1b = T_1ll;

            A = T_1app - T_1b;
            B = A + s * T_1app * T_1b;
            if (B < 1e-12)
                B = 1e-12; // really shouldn't happen, but combination of
                           // parameters may arise in artefactual voxels?
            C = pow(s - 1 / T_1app + 1 / T_1b, p * s);
            if (s - 1 / T_1app + 1 / T_1b <= 0)
                C = 1e-12; // really shouldn't happen, but combination of
                           // parameters may arise in artefactual voxels?

            if (ti < current_arrival_time)
            {
                kctissue(it) = kctissue(it) + 0;
            }
            else if (ti >= current_arrival_time
                && ti <= (current_arrival_time + current_bolus_duration))
            {
                kctissue(it) = kctissue(it)
                    + 2 * 1 / A
                        * exp(-(T_1app * delttiss + (T_1app + T_1b) * (ti - bolus_time_passed))
                              / (T_1app * T_1b))
                        * T_1app * T_1b * pow(B, -k)
                        * (exp(delttiss / T_1app + (ti - bolus_time_passed) / T_1b)
                                  * pow(s * T_1app * T_1b, k)
                                  * (1 - igamc(k, B / (T_1app * T_1b)
                                                 * ((ti - bolus_time_passed) - delttiss)))
                              + exp(delttiss / T_1b + (ti - bolus_time_passed) / T_1app) * pow(B, k)
                                  * (-1 + igamc(k, s * ((ti - bolus_time_passed) - delttiss))));
            }
            else //(ti > delttiss + tau)
            {
                kctissue(it) = kctissue(it)
                    + 2 * 1 / (A * B)
                        * (exp(-A / (T_1app * T_1b) * (delttiss + current_bolus_duration)
                               - (ti - bolus_time_passed) / T_1app)
                              * T_1app * T_1b / C
                              * (pow(s, k) * T_1app * T_1b
                                        * (-1
                                              + exp((-1 / T_1app + 1 / T_1b)
                                                    * current_bolus_duration)
                                                  * (1 - igamc(k, B / (T_1app * T_1b)
                                                                 * ((ti - bolus_time_passed)
                                                                       - delttiss)))
                                              + igamc(k, B / (T_1app * T_1b)
                                                        * ((ti - bolus_time_passed) - delttiss
                                                              - current_bolus_duration)))
                                    - exp(-A / (T_1app * T_1b)
                                          * ((ti - bolus_time_passed) - delttiss
                                                - current_bolus_duration))
                                        * C * B * (igamc(k, s * ((ti - bolus_time_passed) - delttiss
                                                                    - current_bolus_duration))
                                                      - igamc(k, s * ((ti - bolus_time_passed)
                                                                         - delttiss)))));
            }
            // if (isnan(kctissue(it))) { kctissue(it)=0.0; cout << "Warning NaN
            // in tissue KC"; }
        }
        n_bolus_arrived++;
    }
    // cout << kctissue.t() << endl;
    return kctissue;
}

/*
ColumnVector TurboQuasarFwdModel::kctissue_gvf(const ColumnVector &tis, float
delttiss, float tau, float T_1bin, float T_1app, float s, float p, float deltll,
float T_1ll) const
{
    ColumnVector kctissue(tis.Nrows());
    kctissue = 0.0;
    float ti = 0.0;
    float T_1b;

    float k = 1 + p * s;
    float A;
    float B;
    float C;
    float sps = pow(s, k);

    for (int it = 1; it <= tis.Nrows(); it++)
    {
        ti = tis(it);

        if (ti < deltll)
            T_1b = T_1bin;
        else
            T_1b = T_1ll;

        A = T_1app - T_1b;
        B = A + s * T_1app * T_1b;
        C = pow(s - 1 / T_1app + 1 / T_1b, p * s);

        if (ti < delttiss)
        {
            kctissue(it) = 0.0;
        }
        else // if(ti >= delttiss && ti <= (delttiss + tau))
        {
            kctissue(it) = 2 * 1 / (B * C) * exp(-(ti - delttiss) / T_1app) *
sps * T_1app * T_1b * (1 - igamc(k, (s - 1 / T_1app - 1 / T_1b) * (ti -
delttiss)));
        }
        // bolus duraiton is specified by the CVF AIF shape and is not an
explicit parameter
        //else //(ti > delttiss + tau)
        //  {
        //    kctissue(it) = exp(-(ti-delttiss-tau)/T_1app) * 2* 1/(B*C) *
exp(-(delttiss+tau)/T_1app)*sps*T_1app*T_1b * (1 -
igamc(k,(s-1/T_1app-1/T_1b)*(delttiss+tau)));
        //  }
    }
    return kctissue * tau;
}
*/

ColumnVector TurboQuasarFwdModel::kctissue_gvf(const ColumnVector &tis, float delttiss, float tau,
    float T_1bin, float T_1app, float s, float p, float deltll, float T_1ll, int n_bolus_total,
    float delta_bolus, const ColumnVector &bolus_order) const
{
    ColumnVector kctissue(tis.Nrows());
    kctissue = 0.0;
    float ti = 0.0;
    float T_1b;

    float k = 1 + p * s;
    float A;
    float B;
    float C;
    float sps = pow(s, k);

    // Turbo QUASAR specific parameters
    int n_bolus_arrived = 0;          // bolus arrived (passed) or processed
    float bolus_time_passed = 0;      // total time passed since the first bolus
                                      // arrived (excluding arrival time)
    float current_arrival_time = 0;   // total time since TI1 (including arrival time)
    float current_bolus_duration = 0; // Current bolus duration

    while (n_bolus_arrived < n_bolus_total)
    {
        bolus_time_passed = n_bolus_arrived * delta_bolus;

        current_arrival_time = bolus_time_passed + delttiss;

        current_bolus_duration
            = tau * bolus_order(n_bolus_arrived + 1); // Column vector index starts from zero

        for (int it = 1; it <= tis.Nrows(); it++)
        {
            ti = tis(it);

            if (ti < deltll)
                T_1b = T_1bin;
            else
                T_1b = T_1ll;

            A = T_1app - T_1b;
            B = A + s * T_1app * T_1b;
            C = pow(s - 1 / T_1app + 1 / T_1b, p * s);

            if (ti < current_arrival_time)
            {
                kctissue(it) = kctissue(it) + 0.0;
            }
            else // if(ti >= delttiss && ti <= (delttiss + tau))
            {
                kctissue(it) = kctissue(it)
                    + 2 * 1 / (B * C) * exp(-((ti - bolus_time_passed) - delttiss) / T_1app) * sps
                        * T_1app * T_1b * (1 - igamc(k, (s - 1 / T_1app - 1 / T_1b)
                                                       * ((ti - bolus_time_passed) - delttiss)))
                        * current_bolus_duration;
            }
            // bolus duraiton is specified by the CVF AIF shape and is not an
            // explicit parameter
            // else //(ti > delttiss + tau)
            //  {
            //    kctissue(it) = exp(-(ti-delttiss-tau)/T_1app) * 2* 1/(B*C) *
            //    exp(-(delttiss+tau)/T_1app)*sps*T_1app*T_1b * (1 -
            //    igamc(k,(s-1/T_1app-1/T_1b)*(delttiss+tau)));
            //  }
        }

        n_bolus_arrived++;
    }

    return kctissue;
}

/*
ColumnVector TurboQuasarFwdModel::kctissue_gaussdisp(const ColumnVector &tis,
float delttiss, float tau, float T_1bin, float T_1app, float sig1, float sig2,
float deltll, float T_1ll) const
{
    ColumnVector kctissue(tis.Nrows());
    kctissue = 0.0;
    float ti = 0.0;
    float T_1b;
    // Tissue kinetic curve gaussian dispersion (pASL)
    // Hrabe & Lewis, MRM, 2004

    float R;
    float sqrt2 = sqrt(2);

    for (int it = 1; it <= tis.Nrows(); it++)
    {
        ti = tis(it);

        if (ti < deltll)
            T_1b = T_1bin;
        else
            T_1b = T_1ll;
        R = 1 / T_1app - 1 / T_1b;

        float F = 2 * exp(-ti / T_1app);
        float u1 = (ti - delttiss) / (sqrt2 * sig1);
        float u2 = (ti - delttiss - tau) / (sqrt2 * sig2);

        kctissue(it) = F / (2 * R) * ((MISCMATHS::erf(u1) - MISCMATHS::erf(u2))
* exp(R * ti)
                                         - (1 + MISCMATHS::erf(u1 - (R * sig1) /
sqrt2)) * exp(R * (delttiss + (R * sig1 * sig1) / 2))
                                         + (1 + MISCMATHS::erf(u2 - (R * sig2) /
sqrt2)) * exp(R * (delttiss + tau + (R * sig2 * sig2) / 2)));
    }
    return kctissue;
}
*/

ColumnVector TurboQuasarFwdModel::kctissue_gaussdisp(const ColumnVector &tis, float delttiss,
    float tau, float T_1bin, float T_1app, float sig1, float sig2, float deltll, float T_1ll,
    int n_bolus_total, float delta_bolus, const ColumnVector &bolus_order) const
{
    ColumnVector kctissue(tis.Nrows());
    kctissue = 0.0;
    float ti = 0.0;
    float T_1b;
    // Tissue kinetic curve gaussian dispersion (pASL)
    // Hrabe & Lewis, MRM, 2004

    float R;
    float sqrt2 = sqrt(2);

    // Turbo QUASAR specific parameters
    int n_bolus_arrived = 0;          // bolus arrived (passed) or processed
    float bolus_time_passed = 0;      // total time passed since the first bolus
                                      // arrived (excluding arrival time)
    float current_arrival_time = 0;   // total time since TI1 (including arrival time)
    float current_bolus_duration = 0; // Current bolus duration

    while (n_bolus_arrived < n_bolus_total)
    {
        bolus_time_passed = n_bolus_arrived * delta_bolus;

        current_arrival_time = bolus_time_passed + delttiss;

        current_bolus_duration
            = tau * bolus_order(n_bolus_arrived + 1); // Column vector index starts from zero

        for (int it = 1; it <= tis.Nrows(); it++)
        {
            ti = tis(it);

            if (ti < deltll)
                T_1b = T_1bin;
            else
                T_1b = T_1ll;
            R = 1 / T_1app - 1 / T_1b;

            float F = 2 * exp(-(ti - bolus_time_passed) / T_1app);
            float u1 = ((ti - bolus_time_passed) - delttiss) / (sqrt2 * sig1);
            float u2
                = ((ti - bolus_time_passed) - delttiss - current_bolus_duration) / (sqrt2 * sig2);

            kctissue(it) = kctissue(it)
                + F / (2 * R)
                    * ((MISCMATHS::erf(u1) - MISCMATHS::erf(u2)) * exp(R * (ti - bolus_time_passed))
                          - (1 + MISCMATHS::erf(u1 - (R * sig1) / sqrt2))
                              * exp(R * (delttiss + (R * sig1 * sig1) / 2))
                          + (1 + MISCMATHS::erf(u2 - (R * sig2) / sqrt2))
                              * exp(R * (delttiss + tau + (R * sig2 * sig2) / 2)));
        }

        n_bolus_arrived++;
    }

    return kctissue;
}

// --- useful general functions ---
float TurboQuasarFwdModel::icgf(float a, float x) const
{
    // incomplete gamma function with a=k, based on the incomplete gamma
    // integral

    return MISCMATHS::gamma(a) * igamc(a, x);
}

float TurboQuasarFwdModel::gvf(float t, float s, float p) const
{
    // The Gamma Variate Function (correctly normalised for area under curve)
    // Form of Rausch 2000
    // NB this is basically a gamma pdf

    if (t < 0)
        return 0.0;
    else
        return pow(s, 1 + s * p) / MISCMATHS::gamma(1 + s * p) * pow(t, s * p) * exp(-s * t);
}
