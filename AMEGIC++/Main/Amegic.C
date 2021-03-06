#ifndef AMEGIC_Main_Amegic_H
#define AMEGIC_Main_Amegic_H

#include "AMEGIC++/Main/Process_Group.H"
#include "PHASIC++/Process/ME_Generator_Base.H"

namespace AMEGIC {

  class Cluster_Algorithm;

  class Amegic: public Process_Group,
		public PHASIC::ME_Generator_Base {
  private :

    std::string  m_path, m_file;

    MODEL::Model_Base *p_mmodel;
    Amegic_Model      *p_amodel;

    Cluster_Algorithm *p_cluster;

    std::vector<PHASIC::Process_Base*> m_rsprocs;

    void DrawLogo(std::ostream &ostr);

  public :

    // constructor
    Amegic();

    // destructor
    ~Amegic();

    // member functions
    bool Initialize(const std::string &path,const std::string &file,
		    MODEL::Model_Base *const model,
		    BEAM::Beam_Spectra_Handler *const beamhandler,
		    PDF::ISR_Handler *const isrhandler);
    PHASIC::Process_Base *InitializeProcess(const PHASIC::Process_Info &pi,
                                            bool add);
    int PerformTests();
    bool NewLibraries();

    void SetClusterDefinitions(PDF::Cluster_Definitions_Base *const defs);

    ATOOLS::Cluster_Amplitude *ClusterConfiguration
    (PHASIC::Process_Base *const proc,const ATOOLS::Vec4D_Vector &p,
     const size_t &mode);

  };// end of class Amegic

}// end of namespace AMEGIC

#endif

#include "AMEGIC++/Main/Topology.H"
#include "AMEGIC++/Main/Process_Base.H"
#include "AMEGIC++/Cluster/Cluster_Algorithm.H"
#include "PHASIC++/Main/Phase_Space_Handler.H"
#include "PHASIC++/Main/Process_Integrator.H"
#include "ATOOLS/Math/Poincare.H"
#include "ATOOLS/Org/Shell_Tools.H"
#include "ATOOLS/Org/Message.H"
#include "ATOOLS/Org/Data_Reader.H"
#include "MODEL/UFO/UFO_Model.H"

using namespace AMEGIC;
using namespace PHASIC;
using namespace ATOOLS;

void Amegic::DrawLogo(std::ostream &ostr)
{
  ostr<<"+-----------------------------------------+\n";
  ostr<<"|   X   X   X XXXX  XXX  XXX  XXX         |\n";
  ostr<<"|  X X  XX XX X    X      X  X     X   X  |\n";
  ostr<<"| X   X X X X XXX  X XXX  X  X    XXX XXX |\n";
  ostr<<"| XXXXX X   X X    X   X  X  X     X   X  |\n";
  ostr<<"| X   X X   X XXXX  XXX  XXX  XXX         |\n";
  ostr<<"+-----------------------------------------+\n";
  ostr<<"| please cite: JHEP 0202:044,2002         |\n";
  ostr<<"+-----------------------------------------+\n";
  rpa->gen.AddCitation
    (1,"Amegic is published under \\cite{Krauss:2001iv}.");
}

Amegic::Amegic(): 
  ME_Generator_Base("Amegic"), p_mmodel(NULL), p_amodel(NULL), p_cluster(NULL)
{
  DrawLogo(msg->Info());
  p_testmoms=NULL;
  p_gen=this;
}

Amegic::~Amegic() 
{
  My_In_File::CloseDB(rpa->gen.Variable("SHERPA_CPP_PATH")+"/Process/Amegic/");
  if (p_cluster) delete p_cluster;
  delete p_amodel;
}
 
bool Amegic::Initialize(const std::string &path,const std::string &file,
			MODEL::Model_Base *const model,
			BEAM::Beam_Spectra_Handler *const beamhandler,
			PDF::ISR_Handler *const isrhandler)
{
  if (dynamic_cast<UFO::UFO_Model*>(MODEL::s_model))
    THROW(fatal_error, "AMEGIC can only be used in built-in models. Please use Comix for UFO models.");
  p_mmodel=model;
  p_amodel = new Amegic_Model(model);
  m_path=path;
  m_file=file;
  p_int->SetBeam(beamhandler);
  p_int->SetISR(isrhandler);
  Data_Reader read(" ",";","#","=");
  read.AddWordSeparator("\t");
  read.SetInputPath(m_path);
  read.SetInputFile(m_file);
  SetPSMasses(&read);
  double helpd;
  if (!read.ReadFromFile(helpd,"DIPOLE_AMIN")) helpd=Max(rpa->gen.Accu(),1.0e-8);
  else msg_Info()<<METHOD<<"(): Set dipole \\alpha_{cut} "<<helpd<<".\n";
  rpa->gen.SetVariable("DIPOLE_AMIN",ToString(helpd));
  if (!read.ReadFromFile(helpd,"DIPOLE_ALPHA")) helpd=1.0;
  else msg_Info()<<METHOD<<"(): Set dipole \\alpha_{max} "<<helpd<<".\n";
  rpa->gen.SetVariable("DIPOLE_ALPHA",ToString(helpd));
  if (!read.ReadFromFile(helpd,"DIPOLE_ALPHA_FF")) helpd=0.0;
  else msg_Info()<<METHOD<<"(): Set FF dipole \\alpha_{max} "<<helpd<<".\n";
  rpa->gen.SetVariable("DIPOLE_ALPHA_FF",ToString(helpd));
  if (!read.ReadFromFile(helpd,"DIPOLE_ALPHA_FI")) helpd=0.0;
  else msg_Info()<<METHOD<<"(): Set FI dipole \\alpha_{max} "<<helpd<<".\n";
  rpa->gen.SetVariable("DIPOLE_ALPHA_FI",ToString(helpd));
  if (!read.ReadFromFile(helpd,"DIPOLE_ALPHA_IF")) helpd=0.0;
  else msg_Info()<<METHOD<<"(): Set IF dipole \\alpha_{max} "<<helpd<<".\n";
  rpa->gen.SetVariable("DIPOLE_ALPHA_IF",ToString(helpd));
  if (!read.ReadFromFile(helpd,"DIPOLE_ALPHA_II")) helpd=0.0;
  else msg_Info()<<METHOD<<"(): Set II dipole \\alpha_{max} "<<helpd<<".\n";
  rpa->gen.SetVariable("DIPOLE_ALPHA_II",ToString(helpd));
  if (!read.ReadFromFile(helpd,"DIPOLE_KAPPA")) helpd=2.0/3.0;
  else msg_Info()<<METHOD<<"(): Set dipole \\kappa="<<helpd<<"\n.";
  rpa->gen.SetVariable("DIPOLE_KAPPA",ToString(helpd));
  int helpi;
  if (!read.ReadFromFile(helpi,"DIPOLE_NF_GSPLIT"))
    helpi=Flavour(kf_jet).Size()/2;
  else msg_Info()<<METHOD<<"(): Set dipole N_f="<<helpi<<"\n.";
  rpa->gen.SetVariable("DIPOLE_NF_GSPLIT",ToString(helpi));
  if (!read.ReadFromFile(helpd,"DIPOLE_KT2MAX")) helpd=sqr(rpa->gen.Ecms());
  else msg_Info()<<METHOD<<"(): Set dipole \\k_{T,max}^2 "<<helpd<<".\n";
  rpa->gen.SetVariable("DIPOLE_KT2MAX",ToString(helpd));
  rpa->gen.SetVariable("NLO_SMEAR_THRESHOLD",
		       ToString(read.GetValue("NLO_SMEAR_THRESHOLD",0.0)));
  rpa->gen.SetVariable("NLO_SMEAR_POWER",
		       ToString(read.GetValue("NLO_SMEAR_POWER",0.5)));
  int ossub=read.GetValue<int>("OS_SUB",0);
  if (ossub==1) msg_Info()<<"Set on shell subtraction on. "<<std::endl;
  rpa->gen.SetVariable("OS_SUB",ToString(ossub));
  int sort=read.GetValue<int>("AMEGIC_SORT_LOPROCESS",1);
  rpa->gen.SetVariable("AMEGIC_SORT_LOPROCESS",ToString(sort));
  int libcheck=read.GetValue<int>("ME_LIBCHECK",0);
  rpa->gen.SetVariable("ME_LIBCHECK",ToString(libcheck));
  int cvp=read.GetValue<int>("AMEGIC_CUT_MASSIVE_VECTOR_PROPAGATORS",1);
  rpa->gen.SetVariable("AMEGIC_CUT_MASSIVE_VECTOR_PROPAGATORS",ToString(cvp));
  double alpha=read.GetValue<double>("AMEGIC_TCHANNEL_ALPHA",0.9);
  rpa->gen.SetVariable("AMEGIC_TCHANNEL_ALPHA",ToString(alpha));
  double salpha=read.GetValue<double>("AMEGIC_SCHANNEL_ALPHA",0.75);
  rpa->gen.SetVariable("AMEGIC_SCHANNEL_ALPHA",ToString(salpha));
  double eps=read.GetValue<double>("AMEGIC_CHANNEL_EPSILON",0.0);
  rpa->gen.SetVariable("AMEGIC_CHANNEL_EPSILON",ToString(eps));
  int gauge(read.GetValue<int>("AMEGIC_DEFAULT_GAUGE",1));
  AMEGIC::Process_Base::SetGauge(gauge);
  if (gauge!=10) msg_Info()<<METHOD<<"(): Set gauge "<<gauge<<"."<<std::endl;
  s_partcommit=read.GetValue<int>("AMEGIC_PARTIAL_COMMIT",0);
  MakeDir(rpa->gen.Variable("SHERPA_CPP_PATH")+"/Process",true);
  My_In_File::OpenDB(rpa->gen.Variable("SHERPA_CPP_PATH")+"/Process/Amegic/");
  return true;
}

PHASIC::Process_Base *Amegic::InitializeProcess(const PHASIC::Process_Info &pi,
                                                bool add)
{
  if (pi.m_fi.m_nloewtype!=PHASIC::nlo_type::lo) return NULL;
  PHASIC::Process_Base *newxs(NULL);
  size_t nis(pi.m_ii.NExternal()), nfs(pi.m_fi.NExternal());
  std::string name(PHASIC::Process_Base::GenerateName(pi.m_ii,pi.m_fi));
  Topology top(nis+nfs);
  bool oneisgroup(pi.m_ii.IsGroup()||pi.m_fi.IsGroup());
  if (oneisgroup) {
    newxs = new AMEGIC::Process_Group();
    newxs->SetGenerator(this);
    newxs->Init(pi,p_int->Beam(),p_int->ISR());
    if (!newxs->Get<AMEGIC::Process_Group>()->
	InitAmplitude(p_amodel,&top)) {
      msg_Debugging()<<METHOD<<"(): Init failed for '"
		     <<newxs->Name()<<"'\n";
      delete newxs;
      return NULL;
    }
    if (!s_partcommit)
      My_In_File::ExecDB(rpa->gen.Variable("SHERPA_CPP_PATH")+"/Process/Amegic/","begin");
    if (!newxs->Get<AMEGIC::Process_Group>()->ConstructProcesses()) {
      if (!s_partcommit)
	My_In_File::ExecDB(rpa->gen.Variable("SHERPA_CPP_PATH")+"/Process/Amegic/","commit");
      msg_Debugging()<<METHOD<<"(): Construct failed for '"
		     <<newxs->Name()<<"'\n";
      delete newxs;
      return NULL;
    }
    if (!s_partcommit)
      My_In_File::ExecDB(rpa->gen.Variable("SHERPA_CPP_PATH")+"/Process/Amegic/","commit");
    newxs->Get<AMEGIC::Process_Group>()->WriteMappingFile();
    msg_Tracking()<<"Initialized '"<<newxs->Name()<<"'\n";
    if (msg_LevelIsTracking()) newxs->Get<AMEGIC::Process_Group>()->PrintProcessSummary();
  }
  else {
    newxs = GetProcess(pi);
    if (!newxs) return NULL;
    newxs->SetGenerator(this);
    newxs->Init(pi,p_int->Beam(),p_int->ISR());
    p_testmoms = new Vec4D[newxs->NIn()+newxs->NOut()];
    if (!p_pinfo) {
      p_pinfo = Translate(pi);
      m_nin = newxs->NIn();
      m_flavs.clear();
      for (size_t i=0;i<m_nin;i++) 
	m_flavs.push_back(newxs->Flavours()[i]);
    }
    Phase_Space_Handler::TestPoint(p_testmoms,&newxs->Info(),this);
    Vec4D sum;
    Poincare lab(Vec4D(sqrt(10.0),0.0,0.0,1.0));
    msg_Debugging()<<"After boost:\n";
    for (size_t i(0);i<nis+nfs;++i) {
      lab.Boost(p_testmoms[i]);
      sum+=i<m_nin?-p_testmoms[i]:p_testmoms[i];
      msg_Debugging()<<"  p["<<i<<"] = "<<p_testmoms[i]<<"\n";
    }
    msg_Debugging()<<"} -> sum = "<<sum<<"\n";
    Poincare rot(Vec4D::ZVEC,Vec4D(sqrt(14.0),1.0,2.0,3.0));
    msg_Debugging()<<"After rotation {\n";
    for (size_t i(0);i<nis+nfs;++i) {
      rot.Rotate(p_testmoms[i]);
      sum+=i<m_nin?-p_testmoms[i]:p_testmoms[i];
      msg_Debugging()<<"  p["<<i<<"] = "<<p_testmoms[i]<<"\n";
    }
    msg_Debugging()<<"} -> sum = "<<sum<<"\n";
    newxs->Get<AMEGIC::Process_Base>()->SetTestMoms(p_testmoms);
    newxs->Get<AMEGIC::Process_Base>()->SetPrintGraphs(pi.m_gpath);
    My_In_File::ExecDB(rpa->gen.Variable("SHERPA_CPP_PATH")+"/Process/Amegic/","begin");
    if (!newxs->Get<AMEGIC::Process_Base>()->
	InitAmplitude(p_amodel,&top,m_umprocs,m_errprocs)) {
      My_In_File::ExecDB(rpa->gen.Variable("SHERPA_CPP_PATH")+"/Process/Amegic/","commit");
      msg_Debugging()<<METHOD<<"(): Init failed for '"
		     <<newxs->Name()<<"'\n";
      delete newxs;
      return NULL;
    }
    My_In_File::ExecDB(rpa->gen.Variable("SHERPA_CPP_PATH")+"/Process/Amegic/","commit");
  }
  if (add) Add(newxs);
  else m_rsprocs.push_back(newxs);
  newxs->SetGenerator(this);
  return newxs;
}

int Amegic::PerformTests()
{
  int tests(Process_Group::PerformTests());
  if (NewLibs()) THROW(normal_exit,"New libraries created. Please compile.");
  for (size_t i(0);i<m_rsprocs.size();++i) 
    if (m_rsprocs[i]->Get<AMEGIC::Amegic_Base>()->NewLibs())
      THROW(normal_exit,"New libraries created. Please compile.");
  Minimize();
  return tests;
}

bool Amegic::NewLibraries()
{
  if (NewLibs()) return true;
  for (size_t i(0);i<m_rsprocs.size();++i)
    if (m_rsprocs[i]->Get<AMEGIC::Amegic_Base>()->NewLibs()) return true;
  return false;
}

void Amegic::SetClusterDefinitions(PDF::Cluster_Definitions_Base *const defs)
{
  if (p_cluster==NULL) p_cluster = new Cluster_Algorithm(this);
  p_cluster->SetClusterDefinitions(defs);
}

Cluster_Amplitude *Amegic::ClusterConfiguration
(PHASIC::Process_Base *const proc,const Vec4D_Vector &p,
 const size_t &mode)
{
  p_cluster->Cluster(proc->Get<AMEGIC::Process_Base>(),mode);
  return p_cluster->Amplitude();
}

DECLARE_GETTER(Amegic,"Amegic",ME_Generator_Base,ME_Generator_Key);

ME_Generator_Base *ATOOLS::Getter
<ME_Generator_Base,ME_Generator_Key,Amegic>::
operator()(const ME_Generator_Key &key) const
{
  return new Amegic();
}

void ATOOLS::Getter<ME_Generator_Base,ME_Generator_Key,Amegic>::
PrintInfo(std::ostream &str,const size_t width) const
{ 
  str<<"The AMEGIC++ ME generator"; 
}

