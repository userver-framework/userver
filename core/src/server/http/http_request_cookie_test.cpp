#include <userver/utest/utest.hpp>

#include <unordered_map>

#include <fmt/format.h>

#include <userver/server/http/http_request.hpp>

#include <server/http/http_request_parser.hpp>

#include "create_parser_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct CookiesData {
  std::string name;
  std::string data;
  std::unordered_map<std::string, std::string> expected;
};

class HttpRequestCookies : public ::testing::TestWithParam<CookiesData> {};

std::string PrintCookiesDataTestName(
    const ::testing::TestParamInfo<CookiesData>& data) {
  std::string res = data.param.name;
  if (res.empty()) res = "_empty_";
  return res;
}

}  // namespace

INSTANTIATE_UTEST_SUITE_P(
    /**/, HttpRequestCookies,
    ::testing::Values(CookiesData{"empty", "", {}},
                      CookiesData{"lower_only", "a=b", {{"a", "b"}}},
                      CookiesData{"upper_only", "A=B", {{"A", "B"}}},
                      CookiesData{
                          "mixed", "a=B; A=b", {{"a", "B"}, {"A", "b"}}}),
    PrintCookiesDataTestName);

UTEST_P(HttpRequestCookies, Test) {
  const auto& param = GetParam();
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&param,
       &parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            dynamic_cast<server::http::HttpRequestImpl&>(*request);
        const server::http::HttpRequest http_request(http_request_impl);
        EXPECT_EQ(http_request.CookieCount(), param.expected.size());
        for (const auto& [name, value] : param.expected) {
          EXPECT_TRUE(http_request.HasCookie(name));
          EXPECT_EQ(http_request.GetCookie(name), value);
        }
        size_t names_count = 0;
        for (const auto& name : http_request.GetCookieNames()) {
          ++names_count;
          EXPECT_TRUE(param.expected.find(name) != param.expected.end());
        }
        EXPECT_EQ(names_count, param.expected.size());
      });

  const auto request = "GET / HTTP/1.1\r\nCookie: " + param.data + "\r\n\r\n";
  parser.Parse(request.data(), request.size());
  EXPECT_EQ(parsed, true);
}

UTEST(HttpRequestCookiesHashDos, HashDos) {
  bool parsed = false;
  auto parser = server::CreateTestParser(
      [&parsed](std::shared_ptr<server::request::RequestBase>&& request) {
        parsed = true;
        auto& http_request_impl =
            dynamic_cast<server::http::HttpRequestImpl&>(*request);
        const server::http::HttpRequest http_request(http_request_impl);

        EXPECT_EQ(http_request.CookieCount(), 3589);
      });

  std::string headers;
  headers +=
      "\r\nCookie: "
      "aabqy=1;aadrs=1;aadsc=1;aaipi=1;aaqzo=1;aasec=1;abibr=1;abmcl=1;abmgp=1;"
      "abxhz=1;acifj=1;adcfg=1;adcsm=1;adedj=1;adfbd=1;adjwo=1;adklt=1;adlhs=1;"
      "adopu=1;adqca=1;adubo=1;aduea=1;adxmv=1;adztq=1;aeeyr=1;aeiso=1;aeltg=1;"
      "aexns=1;aeyxz=1;afbde=1;afcqu=1;afjbp=1;afvcb=1;afxtd=1;afziz=1;afzwd=1;"
      "agejq=1;agggd=1;agmve=1;agqbg=1;agqej=1;agyoq=1;ahaoz=1;ahdya=1;ahefu=1;"
      "aheib=1;ahfxj=1;ahpij=1;ahruc=1;aiciu=1;aicwm=1;aifmd=1;aigin=1;aiilo=1;"
      "aiirq=1;aisil=1;aithf=1;aiyav=1;ajbuc=1;ajfta=1;ajojx=1;ajoua=1;ajowk=1;"
      "ajpzu=1;akidl=1;akomp=1;akxdj=1;aldkc=1;aliew=1;alihf=1;alvmm=1;amajg=1;"
      "ambgt=1;ambky=1;amcxv=1;amdbp=1;amgxc=1;amiof=1;amqiv=1;amxhz=1;aneny=1;"
      "anjbh=1;anllp=1;anvsr=1;aobxk=1;aocgr=1;aochi=1;aogia=1;aohlz=1;aonpx=1;"
      "aostq=1;aotct=1;apawf=1;apdnd=1;aplow=1;apqur=1;apvjo=1;apydk=1;aqepk=1;"
      "aqfgg=1;aqghp=1;arlay=1;arnxl=1;aruvk=1;arvdj=1;aryun=1;asbuf=1;aslvx=1;"
      "asmkj=1;asppx=1;aspzc=1;asrfv=1;assmh=1;asutl=1;atajx=1;atalo=1;atmrm=1;"
      "atoou=1;atpkq=1;atrpw=1;atwfd=1;atwlg=1;aueyp=1;aumvu=1;aumxd=1;auopi=1;"
      "avfty=1;avifw=1;avnkj=1;avsbs=1;avxaj=1;avybh=1;avzmm=1;awael=1;awcij=1;"
      "awlao=1;awlpq=1;awtzy=1;awxce=1;axisv=1;axkna=1;axlam=1;axlwa=1;axqmm=1;"
      "axsfb=1;axtnx=1;axuiv=1;ayiuo=1;ayked=1;aylzk=1;aysjf=1;aytcw=1;azacb=1;"
      "azbet=1;azbyt=1;azhun=1;aztdw=1;azxtr=1;badox=1;badwt=1;bajau=1;banch=1;"
      "batma=1;bavym=1;bbcoy=1;bbfus=1;bbibi=1;bbipn=1;bblbx=1;bbnym=1;bbqds=1;"
      "bbrfl=1;bbyiv=1;bcdfl=1;bckck=1;bcpsx=1;bcqsi=1;bcsdb=1;bdcks=1;bddon=1;"
      "bdgrh=1;bdlzi=1;bdpnc=1;bdraf=1;benmq=1;beqmj=1;beqrz=1;bevbe=1;bfggu=1;"
      "bfkvk=1;bfyhz=1;bfymo=1;bgdcf=1;bgdvf=1;bgfib=1;bgnrx=1;bgqua=1;bgyqg=1;"
      "bgzhg=1;bhghi=1;bhiui=1;bhmwh=1;bhnta=1;bhtff=1;bicks=1;bidom=1;bihwg=1;"
      "bixma=1;bjiju=1;bjkxp=1;bjmgv=1;bjybw=1;bjzus=1;bkaio=1;bkccd=1;bkeip=1;"
      "bklun=1;bkvyv=1;bkybc=1;blgim=1;blhbq=1;bljvq=1;blnkc=1;blnrm=1;blpee=1;"
      "bmcjb=1;bmepc=1;bmknv=1;bmnyc=1;bmork=1;bmpjl=1;bmtrf=1;bnhqh=1;bnojs=1;"
      "bnwny=1;bnykb=1;bnyqy=1;bnzts=1;boanw=1;bocju=1;boenw=1;bofwc=1;bogke=1;"
      "bohbt=1;bohnq=1;boniu=1;boxkr=1;bpbyq=1;bpcch=1;bpkku=1;bqaah=1;bqmgz=1;"
      "bqthp=1;bqtxz=1;bqvej=1;bqvpz=1;bqxuq=1;brqvj=1;brtoc=1;brvuh=1;brxhz=1;"
      "bsblw=1;bscog=1;bsexf=1;bsffh=1;bskil=1;bskkm=1;bsklu=1;bssdz=1;bsyui=1;"
      "bszaf=1;btcvk=1;btfoy=1;btgyb=1;btijj=1;btjnw=1;btlrq=1;btqpb=1;btvpy=1;"
      "bubor=1;bucds=1;bucgc=1;bugah=1;buhld=1;bumez=1;butwz=1;bvaus=1;bvfqu=1;"
      "bvitw=1;bvjuz=1;bvmme=1;bvoyw=1;bvpse=1;bvuux=1;bvxzk=1;bvydp=1;bvynn=1;"
      "bwjtv=1;bwkmg=1;bwkwp=1";
  headers +=
      "\r\ncOokie: "
      "bwmik=1;bwnjn=1;bwpus=1;bwrfl=1;bwryn=1;bwvms=1;bwweb=1;bwwvj=1;bwyud=1;"
      "bxclx=1;bxiof=1;bxivu=1;bxmap=1;bxmop=1;bxttv=1;bxxvz=1;bybdy=1;bycoo=1;"
      "bylyu=1;byyug=1;bzauz=1;bzedp=1;bzmuk=1;bzrcl=1;bzsks=1;bzsoi=1;bztkt=1;"
      "bzuoy=1;bzyrl=1;caaik=1;cacwf=1;cafvi=1;cagei=1;cagme=1;capex=1;catrz=1;"
      "cbbcs=1;cbdrl=1;cbete=1;cbgon=1;cbhqa=1;cbkgk=1;cbpbt=1;ccacn=1;ccfod=1;"
      "ccjni=1;ccken=1;cclkq=1;ccowj=1;ccrxh=1;ccvev=1;cczhp=1;cdatf=1;cdfeo=1;"
      "cdtwg=1;cejhd=1;cepyd=1;cevge=1;cevwr=1;cfdrv=1;cfgyq=1;cfmps=1;cfoyi=1;"
      "cfpkg=1;cfqsg=1;cfrni=1;cfusc=1;cgdhi=1;cgmiv=1;cgnkg=1;cgnni=1;cgrhi=1;"
      "cgwfx=1;chceb=1;chilx=1;chjdt=1;chjvi=1;chtfz=1;chwga=1;chwou=1;chxbg=1;"
      "cibqb=1;cibxf=1;cidlo=1;ciirh=1;citnw=1;cixoz=1;cjaoy=1;cjfzm=1;cjiqf=1;"
      "cjjzg=1;cjlsm=1;cjxpt=1;ckchz=1;ckcih=1;ckjhq=1;ckmnu=1;ckplw=1;ckqgz=1;"
      "ckxci=1;cldvt=1;clvdo=1;clwcz=1;cmcyk=1;cmezr=1;cmiqf=1;cmmir=1;cmnuw=1;"
      "cmpjx=1;cmrlc=1;cmxld=1;cnmpu=1;cnnzv=1;cnqup=1;cnrve=1;cnyky=1;cohun=1;"
      "cojpx=1;coort=1;cosdv=1;coswz=1;cpgxv=1;cphwr=1;cpmpi=1;cpvmi=1;cqhkc=1;"
      "cqshv=1;crbdu=1;crhtm=1;crkiz=1;csdsc=1;cslbm=1;csodz=1;cspzb=1;cssny=1;"
      "ctokl=1;ctoph=1;ctrmz=1;ctzch=1;cuizk=1;cuksr=1;cuoda=1;cvmki=1;cvpec=1;"
      "cvqpv=1;cvtdg=1;cvzkj=1;cwcog=1;cwewj=1;cwjcj=1;cwmuc=1;cwnyl=1;cwoyj=1;"
      "cwpbn=1;cwqpb=1;cwqye=1;cwtsy=1;cwumm=1;cwwbx=1;cxbcg=1;cxdrk=1;cxgzy=1;"
      "cxsnv=1;cxxqb=1;cxykn=1;cydht=1;cygzs=1;cyhaf=1;cyrax=1;cyskb=1;czilh=1;"
      "czlvh=1;czmye=1;czqpb=1;czrnj=1;czszp=1;czupf=1;czuzd=1;czzad=1;dabdo=1;"
      "dacfh=1;daeaq=1;daenz=1;dapoo=1;dbbji=1;dbkmj=1;dblkn=1;dblmx=1;dbmxh=1;"
      "dcaah=1;dccfr=1;dcihu=1;dcrls=1;dczpl=1;ddivi=1;ddjqb=1;ddqua=1;ddswa=1;"
      "ddtad=1;ddyrd=1;debxi=1;deckq=1;dehlw=1;deikg=1;delwl=1;deoqr=1;deoyv=1;"
      "deubn=1;deuoe=1;dfqgx=1;dfqoz=1;dfruz=1;dftex=1;dgabo=1;dgmkz=1;dgnrf=1;"
      "dhrjv=1;dhtaj=1;dijlt=1;dinkn=1;diyjf=1;djfax=1;djlki=1;djvxm=1;djwbq=1;"
      "djxuc=1;djyfe=1;dkaoe=1;dkpwd=1;dkuzr=1;dkvfz=1;dkyie=1;dldys=1;dlfle=1;"
      "dlilx=1;dlslw=1;dlxab=1;dmgeh=1;dmlht=1;dmuno=1;dmvss=1;dmxfk=1;dmymr=1;"
      "dncfj=1;dnhjx=1;dnmbq=1;dnohq=1;dnwyj=1;dnxsx=1;dofkx=1;dortq=1;douco=1;"
      "dowko=1;doyje=1;dpaah=1;dpeen=1;dpfct=1;dphgt=1;dqmgi=1;dqomh=1;dqreg=1;"
      "dqyej=1;dqynx=1;dqzjb=1;dragr=1;drcqr=1;drgxs=1;driut=1;drocf=1;drrtx=1;"
      "drvth=1;drysn=1;dsdhh=1;dstvs=1;dswwt=1;dszdm=1;dszne=1;dtdga=1;dtgak=1;"
      "dtgsw=1;dtkxb=1;dtmdd=1;dtmwz=1;dtqul=1;duaeb=1;dubtj=1;duegf=1;dugfi=1;"
      "dugkj=1;dulyp=1;durrn=1;duwfo=1;dvdjw=1;dvgbk=1;dvglb=1;dvktr=1;dvngo=1;"
      "dvqdj=1;dvtny=1;dvugx=1";
  headers +=
      "\r\ncoOkie: "
      "dvvgz=1;dvxen=1;dwhwc=1;dwlng=1;dwqrw=1;dwvrd=1;dwyzh=1;dxbic=1;dxmff=1;"
      "dxnep=1;dxqjf=1;dxvfu=1;dxybc=1;dyaux=1;dyawa=1;dycal=1;dyfmc=1;dyhmd=1;"
      "dymio=1;dyoyp=1;dyspp=1;dyttc=1;dyufy=1;dzbvp=1;dzfbu=1;dzftz=1;dzimj=1;"
      "dzmvp=1;dzrjr=1;dztrg=1;eaila=1;eaiqu=1;earzh=1;easew=1;eawsr=1;ebcur=1;"
      "ebftn=1;ebhyc=1;ebqvv=1;ebtkz=1;ebttw=1;ebvkw=1;eciyk=1;eclig=1;edaeh=1;"
      "edcmh=1;edfkn=1;edgvy=1;edivc=1;edjuk=1;edolm=1;edpmp=1;edrjd=1;edtgv=1;"
      "edvlg=1;edywc=1;eenqc=1;eeobc=1;eeodt=1;eeodu=1;eeswe=1;eevgu=1;eevoh=1;"
      "eexdj=1;eeykj=1;eeyvm=1;efans=1;efate=1;efeaw=1;efham=1;eflnr=1;eflot=1;"
      "eftbl=1;efwzc=1;efzkf=1;egmym=1;egoob=1;egryv=1;egzjl=1;ehjvi=1;ehkcz=1;"
      "ehkji=1;ehxlu=1;ehxnw=1;ehyrn=1;ehzly=1;eiiwq=1;eikxm=1;einiq=1;eiobt=1;"
      "eiqwb=1;ejeyz=1;ejmwn=1;ejnzn=1;ejrkf=1;ejybb=1;ekfkv=1;ekgry=1;ekhih=1;"
      "ekiko=1;eklqp=1;eklyd=1;ekmhr=1;ekokh=1;ekots=1;ekozx=1;ekrar=1;elawg=1;"
      "elhit=1;elksy=1;elnsc=1;emelh=1;emhxu=1;emizj=1;emvdl=1;enazp=1;engbg=1;"
      "enhip=1;ennml=1;entbz=1;entjy=1;enzvd=1;eohln=1;eojdy=1;eojfr=1;eoygi=1;"
      "epbar=1;epdyj=1;epdza=1;epeiz=1;epgne=1;epsup=1;eqacu=1;eqcge=1;eqevo=1;"
      "eqpfr=1;eqzly=1;eragh=1;ercus=1;erjap=1;erneh=1;erxwt=1;erydr=1;erzgf=1;"
      "esahq=1;esbkv=1;eshmx=1;esqnk=1;esrex=1;etben=1;eteyf=1;etozy=1;euaez=1;"
      "euage=1;euddq=1;euean=1;euijc=1;eutsl=1;euxiu=1;euzyj=1;evaav=1;evojz=1;"
      "evucz=1;evvrn=1;evwtz=1;evyba=1;ewfeh=1;ewpti=1;exeiy=1;exihx=1;exlot=1;"
      "exony=1;expvl=1;exsbr=1;exwey=1;exxft=1;exzak=1;exzhg=1;eylcr=1;ezavj=1;"
      "ezbir=1;ezdzt=1;ezeum=1;ezlzr=1;ezubz=1;ezvlk=1;ezvvl=1;fahlj=1;fajle=1;"
      "falrb=1;falrj=1;famez=1;fanem=1;fbgbu=1;fbhmr=1;fbkrq=1;fblje=1;fbutc=1;"
      "fbuzh=1;fcgkn=1;fcrgk=1;fdasl=1;fdfez=1;fdifs=1;fdjqs=1;fdoir=1;fdovj=1;"
      "fdqfd=1;fdure=1;fdylv=1;fdzdi=1;fdzig=1;febbz=1;fefim=1;feoku=1;fepkh=1;"
      "feuux=1;fexpd=1;ffade=1;ffaqn=1;ffcth=1;ffhvp=1;fflpc=1;ffosh=1;ffoyr=1;"
      "ffqph=1;ffseb=1;ffsru=1;ffump=1;fgacy=1;fgaib=1;fgbfy=1;fggac=1;fgizh=1;"
      "fglsq=1;fguxh=1;fgwht=1;fgwlh=1;fgytb=1;fhcnw=1;fhdfb=1;fhgri=1;fhhqx=1;"
      "fhhub=1;fhnux=1;fhplq=1;fhutq=1;fhykg=1;fibdv=1;fibst=1;figsf=1;fihei=1;"
      "fihmf=1;fihuw=1;fiizm=1;filin=1;fiqcj=1;fizxl=1;fjafk=1;fjbrs=1;fjfyx=1;"
      "fjgwz=1;fjild=1;fjivg=1;fjknp=1;fjpdh=1;fjtjx=1;fkgxo=1;fkist=1;fklak=1;"
      "fkpfn=1;fkryk=1;fktde=1;fkxfe=1;fkxhz=1;fkyib=1;flexo=1;flfcv=1;flhiu=1;"
      "flrcr=1;flwnv=1;fmaul=1;fmhcu=1;fmjuk=1;fmliv=1;fmpje=1;fmsgy=1;fmxwo=1;"
      "fngac=1;fnjgf=1;fnjow=1;fnlkd=1;fobyg=1;foscn=1;fotld=1;foxub=1;fpdxr=1;"
      "fpecn=1;fpgog=1;fpmda=1";
  headers +=
      "\r\ncooKie: "
      "fppyf=1;fprot=1;fptty=1;fpttz=1;fpxkb=1;fqbvb=1;fqcwx=1;fqdjs=1;fqdnx=1;"
      "fqikr=1;fqsww=1;fqzmp=1;frbbz=1;freky=1;frfui=1;frjwg=1;frnqn=1;frqbm=1;"
      "frwoc=1;frxgm=1;frzeh=1;frznv=1;fsiln=1;fsqed=1;ftble=1;ftewq=1;ftfjw=1;"
      "ftkty=1;ftkvv=1;ftlve=1;ftlvv=1;ftwjk=1;ftyko=1;fuaas=1;fublo=1;fucht=1;"
      "fuczg=1;funlt=1;funwz=1;fusrn=1;futbu=1;futvn=1;fuujo=1;fuyff=1;fvduy=1;"
      "fvieq=1;fvrvi=1;fvsfz=1;fvvvp=1;fvxjd=1;fwbfg=1;fwfmi=1;fxids=1;fxmtm=1;"
      "fxsop=1;fxvrd=1;fyava=1;fyazc=1;fycce=1;fyfzd=1;fyovc=1;fyput=1;fzbxr=1;"
      "fzpux=1;fztlc=1;fztul=1;fzvhr=1;gaavq=1;gapuc=1;garcm=1;gards=1;gawfr=1;"
      "gbbzt=1;gbhpq=1;gbiis=1;gbjdx=1;gbkxq=1;gblop=1;gbmnh=1;gbqcj=1;gbril=1;"
      "gbxcc=1;gckmz=1;gcrsg=1;gctak=1;gctjk=1;gczpi=1;gdmin=1;gdpdf=1;gdwxg=1;"
      "gesdq=1;geuae=1;geydf=1;gfcza=1;gggce=1;ggish=1;gglfl=1;ghdki=1;ghffd=1;"
      "ghgbq=1;ghgcp=1;ghgji=1;ghjpb=1;ghzfj=1;giayj=1;gibby=1;giftv=1;gihpg=1;"
      "gihuo=1;gimbp=1;girbg=1;giufj=1;gjcew=1;gjcwa=1;gjlzx=1;gkemq=1;gkeyg=1;"
      "gkfbx=1;gkiqt=1;gkjsy=1;gkupr=1;gkuzf=1;gkvuv=1;gkyix=1;glbub=1;glinp=1;"
      "glvqw=1;glzam=1;gnbzr=1;gnikv=1;gnkjs=1;gnkoy=1;gnmmx=1;gnorw=1;gnqgy=1;"
      "gnuuj=1;gnuzs=1;gobne=1;goewt=1;gojei=1;golhs=1;golkw=1;gonti=1;goslk=1;"
      "gotfr=1;gotof=1;goxek=1;gpipb=1;gpnye=1;gppfs=1;gpqbh=1;gpuhr=1;gpybd=1;"
      "gqffn=1;gqnuf=1;gqnwa=1;gqonq=1;gqosg=1;gqxga=1;gqygs=1;gqyvd=1;grfqb=1;"
      "grksj=1;grnze=1;gscbc=1;gslzg=1;gsmxc=1;gsqgr=1;gsrxq=1;gswua=1;gszjw=1;"
      "gtjwq=1;gtkfu=1;gtnpv=1;gtqve=1;gtqxk=1;gtxvy=1;gucen=1;gufww=1;gugeb=1;"
      "gugjj=1;guloe=1;gumet=1;gvmne=1;gvrma=1;gwggp=1;gwobv=1;gwwsn=1;gxezy=1;"
      "gxkrt=1;gxlao=1;gxwue=1;gylmg=1;gymuv=1;gypee=1;gzcmp=1;gzeaf=1;gzhyh=1;"
      "gzloy=1;gzopv=1;gzvsj=1;gzxmg=1;gzxri=1;hajqa=1;hario=1;hashv=1;hbahn=1;"
      "hbdqc=1;hbdqz=1;hblva=1;hbokl=1;hbtos=1;hbzul=1;hcdcq=1;hchdw=1;hcnwu=1;"
      "hcxnq=1;hczld=1;hdejy=1;hdmez=1;hdqct=1;hdrix=1;hdsjm=1;hdxbt=1;hedsh=1;"
      "hehpp=1;hejby=1;hejfg=1;heyip=1;hfclc=1;hfeoa=1;hfpac=1;hfspv=1;hgkxr=1;"
      "hgpxf=1;hgtkj=1;hgwsd=1;hhbyi=1;hhcbo=1;hheyb=1;hhmpp=1;hhslb=1;hhzwx=1;"
      "higxz=1;hihea=1;hijbr=1;hjcub=1;hjeix=1;hjhxh=1;hjijy=1;hjjzv=1;hjklb=1;"
      "hjltq=1;hjpro=1;hjtgx=1;hjyhz=1;hkcwr=1;hkenb=1;hkxjt=1;hlgys=1;hliva=1;"
      "hlnrl=1;hloth=1;hlpea=1;hlptf=1;hmczx=1;hmfqh=1;hmmoc=1;hmqnn=1;hmtpd=1;"
      "hmugs=1;hnbia=1;hnhqo=1;hnoak=1;hnqhf=1;hnqor=1;hntvj=1;hnvdx=1;hoaea=1;"
      "hobdf=1;hokdj=1;hopml=1;houbx=1;howeu=1;hpgna=1;hpish=1;hpqai=1;hqbbi=1;"
      "hqdeb=1;hqfce=1;hqitd=1;hqwff=1;hrbfn=1;hrcve=1;hrxaz=1;hrzgf=1;hsbsa=1;"
      "hsdiy=1;hstwx=1;hsurb=1";
  headers +=
      "\r\ncookIe: "
      "htcua=1;htdqr=1;htlkr=1;htxwx=1;huckw=1;huhzk=1;hunsy=1;huobb=1;huoht=1;"
      "hupjc=1;hupqy=1;hvapq=1;hvduu=1;hvoox=1;hvtsy=1;hwawo=1;hwbuo=1;hwerg=1;"
      "hwfna=1;hwhwb=1;hxcaa=1;hxefu=1;hxika=1;hxiyd=1;hxkmf=1;hxruc=1;hxuzr=1;"
      "hxwke=1;hybrz=1;hywso=1;hzefz=1;hzejh=1;hzfcl=1;hzhds=1;hzirs=1;hzkuy=1;"
      "hzxgu=1;iabai=1;iarnq=1;iawdm=1;iaztf=1;ibail=1;ibjls=1;ibjxm=1;ibkga=1;"
      "iboay=1;ibvml=1;ibvpu=1;icbet=1;icgyv=1;ickjn=1;icrer=1;idctv=1;ideni=1;"
      "idhao=1;idiev=1;idqnn=1;idwat=1;ieajw=1;iejtn=1;iewpj=1;ieyvx=1;iffxc=1;"
      "ifiyj=1;ifmif=1;ifrid=1;ifzus=1;igkru=1;igoqk=1;igsyw=1;igtxd=1;ihhwq=1;"
      "ihkmk=1;ihphr=1;ihtlx=1;ihyme=1;iihrg=1;iiime=1;iijaj=1;iikxd=1;iimxq=1;"
      "iinog=1;iiwmp=1;iiwxk=1;iiyxh=1;iizwb=1;ijctu=1;ijimr=1;ijrkj=1;ijumm=1;"
      "ikexa=1;ikfwl=1;ikgdt=1;ikhdn=1;iknfx=1;ikric=1;iktwd=1;ikxxh=1;ikyci=1;"
      "ilfyp=1;ilmcd=1;ilvvo=1;imadi=1;imfwf=1;imgmc=1;imlnh=1;imubo=1;imvyz=1;"
      "imzud=1;incqb=1;indfw=1;inmck=1;ioamm=1;iobzc=1;iofqr=1;iogmc=1;iohej=1;"
      "iorlo=1;ioswk=1;ipnfb=1;ipsus=1;iptgs=1;ipvjc=1;ipwtq=1;ipxey=1;iqhlw=1;"
      "iqwfy=1;iraqq=1;ircpw=1;irdch=1;irkpm=1;irmpy=1;islyj=1;issga=1;isuot=1;"
      "itngq=1;itnvh=1;ituho=1;ituzd=1;itwvc=1;iuekk=1;iulya=1;iupji=1;iuuyt=1;"
      "iuybt=1;ivcwh=1;iveos=1;ivjnz=1;ivogw=1;iwbet=1;iwgpk=1;iwirb=1;iwjuo=1;"
      "iwkwx=1;iwnsi=1;iwpcw=1;iwuol=1;ixfvy=1;ixigt=1;ixlpj=1;ixpkk=1;ixrgl=1;"
      "ixuwe=1;iyvpz=1;izasm=1;izilu=1;iznsh=1;izpsj=1;izrsa=1;izwdj=1;jaljb=1;"
      "jaqrz=1;jblmc=1;jbpir=1;jbyym=1;jccdg=1;jcjod=1;jckxg=1;jcnoj=1;jdgvm=1;"
      "jdioq=1;jdkae=1;jdnxc=1;jdoev=1;jdpfc=1;jdwut=1;jedmh=1;jeeza=1;jegai=1;"
      "jejnr=1;jekwv=1;jeluo=1;jepbu=1;jeqki=1;jeskb=1;jetec=1;jeujw=1;jewql=1;"
      "jfagf=1;jfcyd=1;jfizx=1;jfpfc=1;jfqve=1;jftjn=1;jfuxq=1;jfxcj=1;jfxzv=1;"
      "jgcbl=1;jglgl=1;jgxcm=1;jhbqh=1;jhcjx=1;jhhlg=1;jhmkw=1;jhnyb=1;jhrin=1;"
      "jhsnd=1;jifbo=1;jiwtr=1;jjccb=1;jjdap=1;jjdef=1;jjnxu=1;jjoom=1;jjovl=1;"
      "jjpiw=1;jjvbz=1;jjznz=1;jkipq=1;jkobe=1;jlaly=1;jldou=1;jlprx=1;jlqzf=1;"
      "jlsgt=1;jlyay=1;jlzjm=1;jmath=1;jmgbx=1;jmixs=1;jmjjw=1;jmmco=1;jnagv=1;"
      "jnhzc=1;jnkgg=1;jnlms=1;jnspn=1;jnyly=1;jojkh=1;jotgf=1;jovgm=1;jovnw=1;"
      "jozmz=1;jpkgz=1;jpoec=1;jptyd=1;jpvln=1;jpxpb=1;jpyxh=1;jqaqa=1;jqbfn=1;"
      "jqbup=1;jqewi=1;jqhlu=1;jqvsm=1;jqygg=1;jqzzm=1;jrdvl=1;jrimv=1;jrjbs=1;"
      "jrpiy=1;jrpte=1;jrqxy=1;jsbnx=1;jsiqm=1;jsjpr=1;jslav=1;jslrd=1;jsnqu=1;"
      "jsnvh=1;jtbng=1;jtdka=1;jthjb=1;jthpn=1;jthzw=1;jtnbt=1;jtood=1;jtqgo=1;"
      "jtsvl=1;jtvnu=1;jtwwe=1;juasl=1;julei=1;junuc=1;jurjd=1;juuoy=1;juyjg=1;"
      "jvbby=1;jvcvm=1;jvdms=1";
  headers +=
      "\r\ncookiE: "
      "jvqcq=1;jvqej=1;jwbrr=1;jwera=1;jwftq=1;jwgae=1;jwgfi=1;jwkss=1;jwlaz=1;"
      "jwsih=1;jwvto=1;jxbgw=1;jxxjd=1;jxyir=1;jybwr=1;jykdr=1;jykqh=1;jynsm=1;"
      "jyqpr=1;jyros=1;jytzh=1;jywbl=1;jzduq=1;jzejw=1;jzibw=1;jzlfu=1;jznip=1;"
      "jzpht=1;jzyyk=1;kaetl=1;karnh=1;kaybt=1;kbkde=1;kbpfc=1;kbqjs=1;kbrdx=1;"
      "kbsxd=1;kbtvh=1;kbxnp=1;kbzkg=1;kcajt=1;kcdqj=1;kcgbn=1;kcilz=1;kcqyq=1;"
      "kctum=1;kcwpd=1;kdmvc=1;kefzx=1;keige=1;keihz=1;kekzj=1;kelvy=1;keppb=1;"
      "kercu=1;kexjo=1;kfjvw=1;kfnkh=1;kfowq=1;kghxy=1;kgkyr=1;kgoll=1;kgorz=1;"
      "kgrti=1;kgudp=1;kgxdq=1;kgzyv=1;khcgq=1;khcxn=1;kheoa=1;khlsh=1;kiaqh=1;"
      "kibem=1;kiibo=1;kiknq=1;kimzi=1;kisjn=1;kiuik=1;kjcfe=1;kjhbn=1;kjkro=1;"
      "kjstd=1;kjvpx=1;kkfsf=1;kkfye=1;kkzcz=1;klduh=1;klepm=1;kloov=1;klpxo=1;"
      "klrlt=1;klsvd=1;klvyw=1;kmbxp=1;kmkjh=1;kmkmr=1;kmlde=1;kmqom=1;kmugf=1;"
      "kmxzj=1;kngsa=1;knjjg=1;knpru=1;knugy=1;koprh=1;kpagp=1;kpctr=1;kphcf=1;"
      "kpiaa=1;kpihw=1;kpjvd=1;kprgb=1;kptuv=1;kpvzl=1;kqbio=1;kqcwa=1;kqecz=1;"
      "kqobm=1;kqofy=1;kqxph=1;krbwa=1;krege=1;krget=1;krhgr=1;krlwb=1;krmes=1;"
      "krpew=1;krrij=1;krshj=1;krxkl=1;krzln=1;ksbai=1;ksbuw=1;ksfdr=1;ksfpr=1;"
      "kslac=1;kssqp=1;kstuw=1;kswnn=1;kthur=1;ktjch=1;kuixl=1;kuvhz=1;kuyhq=1;"
      "kvdzj=1;kveaq=1;kvehv=1;kvhcz=1;kvkue=1;kvmmw=1;kvouw=1;kvyho=1;kvytz=1;"
      "kvyxc=1;kwchq=1;kwdox=1;kwjri=1;kwort=1;kwurp=1;kwwnk=1;kxfgc=1;kxgpq=1;"
      "kxlgp=1;kxqqi=1;kxtxm=1;kxubc=1;kxvja=1;kymgl=1;kyrxe=1;kytcz=1;kyxpf=1;"
      "kyycn=1;kyzua=1;kyzxt=1;kzatg=1;kzsvh=1;kztby=1;kzufy=1;lapfe=1;lapmx=1;"
      "laulk=1;laxbs=1;lbkik=1;lbnah=1;lbppp=1;lbyiy=1;lbywj=1;lccgs=1;lceqa=1;"
      "lcfhc=1;lchei=1;lckpy=1;lcoec=1;lcqaq=1;lcvyx=1;lczcq=1;lddjy=1;ldfcs=1;"
      "ldgur=1;ldlrp=1;ldmxs=1;ldqne=1;ldrmz=1;ldvci=1;ldwgz=1;ldwmw=1;ldzhq=1;"
      "leakp=1;lebjc=1;lepge=1;lewmo=1;lffyz=1;lfmmq=1;lfonf=1;lfovl=1;lfpcy=1;"
      "lfpqh=1;lfqpq=1;lfsdy=1;lfvoa=1;lgbue=1;lgcrj=1;lgfpg=1;lgqgb=1;lgrsc=1;"
      "lguej=1;lgwwm=1;lhctt=1;lhkhd=1;lhkyj=1;libnt=1;likpq=1;likxn=1;limuk=1;"
      "liovg=1;lirnk=1;ljnqw=1;ljubw=1;ljxna=1;lkfub=1;lkgwh=1;lkhrl=1;lkios=1;"
      "lkjqv=1;lkmvw=1;lkqyg=1;lkrhr=1;lkthe=1;lkvrf=1;llaik=1;llczl=1;llgnf=1;"
      "llmke=1;lmiqm=1;lmlkw=1;lmlzw=1;lmqvz=1;lndut=1;lnguy=1;lnoia=1;lnpjg=1;"
      "lnqcj=1;lnzjp=1;locig=1;lojzi=1;lomhs=1;loqud=1;loukc=1;lovcx=1;loxzo=1;"
      "lpfnf=1;lpfya=1;lpodl=1;lpqwc=1;lprdn=1;lpwqy=1;lpzvg=1;lqbaj=1;lqdko=1;"
      "lqmxi=1;lqval=1;lqwnt=1;lrdsa=1;lrjty=1;lrkbh=1;lrvpi=1;lrwnc=1;lrzav=1;"
      "lsdkw=1;lslhi=1;lsouv=1;lspnd=1;ltaym=1;ltcic=1;ltkjb=1;luajp=1;lunze=1;"
      "lvayi=1;lvbbw=1;lvelz=1";
  headers +=
      "\r\nCOokie: "
      "lvhlx=1;lvpna=1;lvsiw=1;lvtkn=1;lwofq=1;lwrms=1;lwrwk=1;lwtdo=1;lxavs=1;"
      "lxbkr=1;lxndy=1;lxtcz=1;lxyoa=1;lydle=1;lyhru=1;lyhsf=1;lylnm=1;lytdk=1;"
      "lytkf=1;lytwu=1;lyuuj=1;lyvld=1;lywhw=1;lzkli=1;lzrgv=1;lztav=1;lzufv=1;"
      "lzvio=1;lzxqz=1;lzyqz=1;maglo=1;maqut=1;maveq=1;mavpi=1;mayld=1;mazje=1;"
      "mbbsm=1;mbdjf=1;mbeop=1;mbfdt=1;mbowf=1;mboxy=1;mbpbs=1;mbqkk=1;mbxem=1;"
      "mcapi=1;mcbky=1;mccql=1;mcfof=1;mcgsp=1;mckop=1;mcwnk=1;mddlr=1;mdjgo=1;"
      "mdkkx=1;mdkuq=1;mdlxv=1;mdmsf=1;mdplq=1;mdrzo=1;mdxru=1;mdxzn=1;mdyoo=1;"
      "mebhg=1;meebx=1;meeml=1;mejtb=1;mekqu=1;mekxp=1;melne=1;memvy=1;menmt=1;"
      "mepnn=1;mesto=1;mevrj=1;mexks=1;mffwm=1;mffyd=1;mfgcj=1;mfilt=1;mfjal=1;"
      "mfkrb=1;mftbm=1;mgege=1;mgozg=1;mgpgl=1;mgwet=1;mgzex=1;mgzhk=1;mhegl=1;"
      "mhfwt=1;mhfxj=1;mhhsd=1;mhjbn=1;mhlap=1;mhsnm=1;mhthn=1;mhtjg=1;mihjd=1;"
      "mihwc=1;miisk=1;miyjp=1;mjirl=1;mjixh=1;mjmur=1;mjxkj=1;mjylu=1;mjysr=1;"
      "mjzjd=1;mkgog=1;mkhcb=1;mkhpr=1;mkizg=1;mkpih=1;mktho=1;mlaxw=1;mlngq=1;"
      "mlowe=1;mlrfh=1;mltqd=1;mlwcg=1;mmgww=1;mmhcz=1;mmjmh=1;mmnee=1;mmqgu=1;"
      "mmtyl=1;mmukf=1;mmxrx=1;mmyjt=1;mnanu=1;mnbcg=1;mnjmw=1;mnkko=1;mnlkc=1;"
      "mnlvw=1;mnmuh=1;mnvgr=1;mnzch=1;mobph=1;moojg=1;mookh=1;moxzb=1;mphka=1;"
      "mpjrk=1;mpobn=1;mpoeq=1;mpymi=1;mpzqw=1;mqitg=1;mqjvg=1;mqnjp=1;mragz=1;"
      "mrdmw=1;mrhea=1;mrlhh=1;mrmdj=1;mrnpq=1;mrpvu=1;mrqij=1;msbqy=1;msnop=1;"
      "msoip=1;msxum=1;msyio=1;mtaqw=1;mubua=1;mucgf=1;mupfw=1;muqfr=1;muuwo=1;"
      "muvpx=1;mvcfb=1;mvlwv=1;mvmgc=1;mvmpc=1;mvuat=1;mvvpp=1;mvwse=1;mwayw=1;"
      "mwbia=1;mwexf=1;mwipj=1;mwipm=1;mwldk=1;mwlfp=1;mworg=1;mwqci=1;mxcpo=1;"
      "mxwhh=1;mxzeu=1;myatu=1;mycwi=1;mydgx=1;myedp=1;myerk=1;myffz=1;myfpj=1;"
      "myjes=1;mylpz=1;myuky=1;myxol=1;myyha=1;mzcfs=1;mzdpk=1;mzjoi=1;mzqpw=1;"
      "mzqyx=1;mzrcz=1;mzssr=1;mzwdg=1;mzxcv=1;naayf=1;naein=1;nagmn=1;nalsv=1;"
      "naorm=1;naosx=1;narnk=1;naube=1;navxw=1;nbbni=1;nbbnj=1;nbenm=1;nbgxa=1;"
      "nbhnj=1;nbzeq=1;ncbqn=1;nceog=1;ncfqp=1;nckkw=1;ndpdt=1;ndzgo=1;necsh=1;"
      "neexv=1;nespx=1;netdz=1;nettn=1;newgg=1;newmf=1;nexxo=1;nffpw=1;nfnxb=1;"
      "nfuhq=1;nfydc=1;ngajz=1;ngaun=1;ngfte=1;nggge=1;nglyd=1;ngmcp=1;ngwxc=1;"
      "nhbma=1;nhivf=1;nhkgo=1;nhqrb=1;nhumo=1;nhvtr=1;nhzhg=1;nicoh=1;nitur=1;"
      "njgcq=1;njkag=1;njmik=1;njmmi=1;nkeov=1;nkllx=1;nklza=1;nksfz=1;nktqe=1;"
      "nkzkd=1;nlddx=1;nldpw=1;nlhyw=1;nlzxz=1;nmfww=1;nmhwe=1;nmiei=1;nmlzj=1;"
      "nmopp=1;nnbul=1;nngtt=1;nnhkr=1;nnhxv=1;nnmaj=1;nnmky=1;nnmvv=1;nnpsf=1;"
      "nnslx=1;nnwgp=1;noawq=1;nohtd=1;noqdh=1;notse=1;nplol=1;npowp=1;npulh=1;"
      "npuos=1;npvmk=1;npxtc=1";
  headers +=
      "\r\nCoOkie: "
      "nqlcc=1;nqnvy=1;nrjie=1;nrsdi=1;nryji=1;nssvx=1;ntatg=1;ntipa=1;ntmul=1;"
      "ntmwf=1;ntpcs=1;nttgq=1;nttwx=1;ntxct=1;ntybi=1;nubeh=1;nugoi=1;nupdu=1;"
      "nuqia=1;nuvvz=1;nvedu=1;nvhsp=1;nvhyd=1;nvkdb=1;nvkkc=1;nvlsn=1;nvotn=1;"
      "nvotz=1;nvpas=1;nwiwg=1;nwktu=1;nwmrx=1;nwvov=1;nxltv=1;nxnmr=1;nygzx=1;"
      "nyjzu=1;nyphp=1;nytyo=1;nywug=1;nziuc=1;nzljo=1;nzpyq=1;oatmp=1;oavun=1;"
      "oaxqh=1;obgii=1;obgot=1;objjf=1;objwl=1;obsus=1;obwcr=1;obxvn=1;occyt=1;"
      "ochqs=1;ochyr=1;ocijb=1;ocmxs=1;ocsma=1;odapg=1;oddpi=1;odike=1;odtwq=1;"
      "odzkx=1;odznu=1;oeaof=1;oedgk=1;oepgl=1;ofcbk=1;ofghq=1;ofgrq=1;oflcs=1;"
      "ofxyt=1;ofzpm=1;ogfvw=1;ogmys=1;ognmq=1;ogojb=1;ogqnp=1;ogqre=1;ogurp=1;"
      "ohbuk=1;ohfbk=1;ohfoo=1;ohkut=1;ohtxh=1;ohwbq=1;ohzek=1;oiauo=1;oilvo=1;"
      "oindc=1;oiqtn=1;ojgxx=1;ojhqg=1;ojnlu=1;ojuek=1;okhts=1;okitp=1;okker=1;"
      "okpuo=1;okroi=1;oktgr=1;okwzd=1;olduz=1;olfux=1;olmpy=1;olnht=1;oloaq=1;"
      "ololj=1;olwdb=1;olyuy=1;omifr=1;omsan=1;omtnh=1;omxla=1;omzyh=1;ondrl=1;"
      "onhat=1;onwok=1;oodpk=1;oohwc=1;oolhf=1;oolip=1;oomjv=1;oosgd=1;oosmr=1;"
      "oowio=1;opcli=1;opcwp=1;opitc=1;opmhr=1;opwwl=1;opwyi=1;opzzk=1;oqjfo=1;"
      "oqqza=1;oqrlt=1;oqyow=1;oqzbn=1;orbgg=1;ordhe=1;orikd=1;orlfl=1;orvoe=1;"
      "osdku=1;osrdh=1;ostwq=1;otfly=1;otgko=1;otmdq=1;otyst=1;ouals=1;oucig=1;"
      "ounny=1;ousbs=1;ovgth=1;ovhvz=1;ovncr=1;ovuev=1;ovuie=1;ovwzy=1;owiut=1;"
      "owrvv=1;owuuq=1;owwtw=1;oxcdm=1;oxeri=1;oxnud=1;oxorq=1;oxzdo=1;oyhhz=1;"
      "oyiip=1;oyujc=1;ozbgl=1;oznoa=1;oznqd=1;ozvlz=1;ozvyb=1;ozwze=1;paacq=1;"
      "pabwf=1;paclt=1;pahvo=1;pamsi=1;pazdo=1;pazov=1;pbckt=1;pbjpk=1;pbjsi=1;"
      "pbkkn=1;pbomi=1;pbrbw=1;pccqt=1;pcgbj=1;pcqxc=1;pdcga=1;pdgba=1;pdpqp=1;"
      "pdymn=1;peeay=1;peeve=1;peqjq=1;peviy=1;pevzs=1;peyen=1;pezef=1;pfkyn=1;"
      "pflwv=1;pfocw=1;pfxka=1;pghyp=1;pgiwp=1;pgjwq=1;pgpwk=1;phvcj=1;phvui=1;"
      "phwio=1;phynj=1;phzgj=1;pifvk=1;pihgs=1;pixcc=1;piyio=1;pizrh=1;pjsey=1;"
      "pjzng=1;pkawz=1;pkfqi=1;pknen=1;pksgm=1;pksjh=1;pkxhn=1;pkxig=1;pkyaa=1;"
      "plblo=1;pldnj=1;pldsx=1;plgrl=1;plhay=1;pljac=1;plosn=1;pltby=1;pltff=1;"
      "plxor=1;plzki=1;pmdhj=1;pmeqm=1;pmfvs=1;pminh=1;pmkco=1;pmowy=1;pmprk=1;"
      "pmuab=1;pnhkc=1;pnjjw=1;pnkhe=1;pnldd=1;pnthd=1;pntkd=1;pnxhh=1;poalt=1;"
      "pokit=1;pombm=1;poowc=1;porqm=1;posjd=1;powzv=1;pozjn=1;ppbzy=1;ppeij=1;"
      "ppgah=1;ppsob=1;ppurp=1;ppxgp=1;ppxjr=1;ppzus=1;pqnoq=1;pqqql=1;pquqy=1;"
      "pqxtt=1;prdxg=1;prlrc=1;prpio=1;pscvf=1;pshll=1;psrdu=1;psukg=1;psvgu=1;"
      "ptkle=1;ptlxf=1;ptmmh=1;ptpri=1;ptqdf=1;ptqyp=1;ptwtm=1;puawi=1;puhpb=1;"
      "puhvu=1;pujgq=1;pujtc=1";
  headers +=
      "\r\nCooKie: "
      "pumxj=1;punfb=1;puvan=1;puzug=1;pvnhr=1;pvqob=1;pvsjh=1;pwdvd=1;pwmrp=1;"
      "pwsug=1;pwtru=1;pwuiv=1;pwwzd=1;pwzvq=1;pxmxe=1;pxyao=1;pylln=1;pyqvv=1;"
      "pyrci=1;pyvkf=1;pyxox=1;pyynu=1;pzakx=1;pzlei=1;pzoqu=1;pzrfl=1;qacgo=1;"
      "qafwd=1;qajzc=1;qaqxw=1;qarrx=1;qbbfx=1;qbcbx=1;qbjvb=1;qbnov=1;qboba=1;"
      "qbwcs=1;qcann=1;qcart=1;qccay=1;qcicc=1;qclnj=1;qctkf=1;qdael=1;qdemp=1;"
      "qdjoy=1;qdphe=1;qdspv=1;qedyi=1;qeeeb=1;qefmk=1;qenlo=1;qeokh=1;qeppw=1;"
      "qesyk=1;qevtx=1;qfbcg=1;qfcuo=1;qfmjr=1;qfxos=1;qgfhc=1;qghhj=1;qgjfk=1;"
      "qgkma=1;qgktl=1;qgvsh=1;qhbtt=1;qhesf=1;qhnqi=1;qhrhm=1;qhtjg=1;qhvsk=1;"
      "qhxct=1;qibrt=1;qibuz=1;qinyc=1;qirxu=1;qisch=1;qivab=1;qizdl=1;qjabg=1;"
      "qjiit=1;qjoki=1;qjqxn=1;qklps=1;qkmqk=1;qksyq=1;qkxjx=1;qkzeh=1;qlfyo=1;"
      "qllns=1;qluhb=1;qlvfz=1;qlygc=1;qmjak=1;qmjqh=1;qmqup=1;qnblw=1;qncik=1;"
      "qnopv=1;qnoyo=1;qnpuq=1;qnqhs=1;qnuol=1;qnyak=1;qofqc=1;qokzu=1;qozar=1;"
      "qozil=1;qpfnp=1;qpgwe=1;qpiyh=1;qpjqj=1;qplvq=1;qpoxn=1;qppqc=1;qpyao=1;"
      "qqbpg=1;qqhbw=1;qqrja=1;qrgpg=1;qrjdi=1;qrqbe=1;qrtqq=1;qscbd=1;qsekw=1;"
      "qshwh=1;qslhg=1;qsrbs=1;qsurh=1;qswdr=1;qsxwj=1;qtacl=1;qtarc=1;qthhj=1;"
      "qtpbv=1;qtvfk=1;qtwak=1;qucvu=1;qumuc=1;quoas=1;quucp=1;quxus=1;qvfgl=1;"
      "qviea=1;qvlcp=1;qvmbv=1;qvmok=1;qvpoe=1;qvxws=1;qwclr=1;qwdho=1;qwoqx=1;"
      "qwqys=1;qwtwm=1;qwwuu=1;qwxgf=1;qwykq=1;qxcsl=1;qxehu=1;qxfht=1;qxfra=1;"
      "qxlon=1;qxoga=1;qxoif=1;qxova=1;qxwsx=1;qxxhm=1;qxzfa=1;qzbko=1;qzcnm=1;"
      "qzkgg=1;qzphh=1;qzpqo=1;qzqef=1;qzteb=1;qzzgg=1;radxo=1;raofb=1;raosu=1;"
      "rarpg=1;raywv=1;rbcyd=1;rbpnn=1;rbzrr=1;rctje=1;rcykd=1;rdbmi=1;rdgbt=1;"
      "rdntk=1;rdxwx=1;reold=1;repya=1;reqpt=1;rfiql=1;rfrkc=1;rfsff=1;rfslu=1;"
      "rfsok=1;rfupz=1;rfxgn=1;rgcte=1;rgomo=1;rgshd=1;rgvzi=1;rhfbp=1;rhpzt=1;"
      "rhwyd=1;rhxkz=1;rhxtv=1;rhzzl=1;riacg=1;riaep=1;rifnx=1;rihqv=1;riocx=1;"
      "rismy=1;rjjcs=1;rjjil=1;rjkkb=1;rjvzi=1;rjycz=1;rjyff=1;rkbxm=1;rkgvb=1;"
      "rkqbo=1;rktwg=1;rkuoq=1;rlawg=1;rlbqf=1;rlcea=1;rlicv=1;rlmso=1;rlpcb=1;"
      "rlrjk=1;rmgjs=1;rmile=1;rmled=1;rnerx=1;rneyc=1;rnfbv=1;rnftk=1;rnkyc=1;"
      "rnlko=1;rnquu=1;rnrvm=1;rntyz=1;rnwxq=1;rojue=1;roobd=1;roofk=1;rovyd=1;"
      "rpbyw=1;rpcny=1;rpcuj=1;rpdia=1;rpifo=1;rpjeq=1;rppmz=1;rpvmh=1;rqavd=1;"
      "rqeok=1;rqfxw=1;rqfxx=1;rqkyt=1;rqtbp=1;rquee=1;rqvxe=1;rqxdu=1;rraer=1;"
      "rrcge=1;rrcoh=1;rrdxp=1;rrebm=1;rrjsb=1;rrkoh=1;rrlft=1;rrrvr=1;rsare=1;"
      "rsbym=1;rseto=1;rshda=1;rsilo=1;rsreq=1;rssis=1;rsywc=1;rsztk=1;rtblk=1;"
      "rtffx=1;rtpfh=1;rtqhe=1;rtrex=1;rtsyf=1;rtwfw=1;rubdb=1;rubxj=1;rucad=1;"
      "rufto=1;ruies=1;ruiwn=1";
  headers +=
      "\r\nCookIe: "
      "rusrk=1;ruwqw=1;ruxmm=1;rvdqe=1;rvedz=1;rvpzb=1;rvtht=1;rvuol=1;rvxjy=1;"
      "rwgxh=1;rwjue=1;rwoai=1;rwpks=1;rwrel=1;rwtkr=1;rxbda=1;rxkkl=1;rxrvi=1;"
      "rxwsc=1;rxzje=1;ryekh=1;ryggv=1;ryjlr=1;rylne=1;ryrin=1;ryyoc=1;rzcvf=1;"
      "rzglh=1;rzluz=1;rzqog=1;rzscq=1;rzvhf=1;rzwfj=1;rzxbd=1;sabtj=1;saeef=1;"
      "sayme=1;sazwy=1;sbfci=1;sbgfk=1;sbkem=1;sbmrw=1;sbokb=1;sbrgx=1;sbvar=1;"
      "schno=1;sclsp=1;scmpk=1;scxpz=1;sczen=1;sdcfj=1;sdjvk=1;sdmvr=1;sdraw=1;"
      "seeiw=1;seext=1;sefwq=1;seyyr=1;sfbrf=1;sfgiw=1;sfhtb=1;sfkeb=1;sfmcf=1;"
      "sfmow=1;sfodl=1;sfpiq=1;sfuoz=1;sfwxh=1;sfzub=1;sgdez=1;sgszj=1;sgvkz=1;"
      "sgzjm=1;sgzvh=1;shbxr=1;shoqa=1;shphi=1;shqzs=1;sicvb=1;sidkr=1;simhs=1;"
      "simqx=1;sirvy=1;siuzd=1;sivxj=1;siwic=1;siwiw=1;sizbu=1;sjhet=1;sjjcn=1;"
      "sjlpa=1;sjpxl=1;sjrtw=1;sjwmw=1;sjzna=1;sjztq=1;skbio=1;skcpo=1;skhoc=1;"
      "skltn=1;skurr=1;skwku=1;slash=1;slhgu=1;slkkz=1;slkmk=1;slnje=1;smkis=1;"
      "smrfo=1;smriv=1;smuxr=1;snfxh=1;snkid=1;snlmk=1;snqsa=1;snttn=1;soeay=1;"
      "soqey=1;sorok=1;sovyj=1;spcto=1;speop=1;spgpl=1;spmbn=1;spssu=1;spuuz=1;"
      "sqajb=1;sqbjg=1;sqcac=1;sqkay=1;sqolg=1;sqseq=1;sqxae=1;sqyyz=1;srdhy=1;"
      "srxcr=1;srxxf=1;sryce=1;sryzg=1;srznn=1;sseas=1;sslpa=1;ssqtl=1;ssudo=1;"
      "ssyyc=1;stbks=1;sthbj=1;stjvm=1;stspb=1;stzac=1;stzvc=1;subpa=1;suvhz=1;"
      "suzix=1;suzxh=1;svuve=1;svuvl=1;svwoy=1;swhkk=1;swjbt=1;swkql=1;swqtt=1;"
      "swqxg=1;swrfm=1;swuwb=1;sxcop=1;sxdzo=1;sxepg=1;sxgkf=1;sxneb=1;sxsoj=1;"
      "sxtgg=1;sxtno=1;sxwxw=1;sxyfp=1;sxzfe=1;sxzoh=1;syeyv=1;syini=1;sykaa=1;"
      "systv=1;syzfz=1;szjpx=1;szwwa=1;szxij=1;szzix=1;tagws=1;taies=1;tammk=1;"
      "taohj=1;taorv=1;tasam=1;taupq=1;tbygc=1;tcaok=1;tcgcp=1;tcoua=1;tcscf=1;"
      "tcslk=1;tcvur=1;tcwsl=1;tdmnx=1;tdmtm=1;tdpxh=1;tdrys=1;tdvyh=1;tectu=1;"
      "tedwj=1;tektt=1;tepiq=1;teqso=1;terdl=1;tetfk=1;texku=1;texxa=1;tezdr=1;"
      "tfaud=1;tfjvw=1;tfkyh=1;tflfh=1;tfqfu=1;tfsbv=1;tfulv=1;tfxvd=1;tgdpc=1;"
      "tgguo=1;tgiaa=1;tgslo=1;tgxvg=1;thoib=1;thuia=1;thwqr=1;thzad=1;tidfl=1;"
      "tigcd=1;tipiy=1;titxg=1;tiwfl=1;tizvf=1;tjfob=1;tjjee=1;tjvpv=1;tjwna=1;"
      "tjzgy=1;tkban=1;tkera=1;tkfmm=1;tkfmu=1;tkhos=1;tkhug=1;tktpk=1;tkvws=1;"
      "tkvxm=1;tkyeh=1;tlbzm=1;tlhjd=1;tllek=1;tlmlh=1;tlztd=1;tmhgj=1;tmqpb=1;"
      "tmvfx=1;tmxzq=1;tnayh=1;tnbpd=1;tnoan=1;tnrpe=1;tnscc=1;tntgb=1;tnuzo=1;"
      "tocqk=1;toikn=1;toqzj=1;tperd=1;tpglq=1;tpiqp=1;tpqfc=1;tpscs=1;tpssj=1;"
      "tpwqd=1;tqfib=1;tqlgw=1;tqmnw=1;tqnrz=1;trbyp=1;trndo=1;trpjq=1;trrtz=1;"
      "trwfy=1;trwxq=1;tryio=1;tsaby=1;tsbho=1;tsbrr=1;tsfuy=1;tshaj=1;tsizx=1;"
      "tszwq=1;tthad=1;tuhiv=1";
  headers +=
      "\r\nCookiE: "
      "tuhwi=1;tuitr=1;tukxw=1;tusdw=1;tuuhm=1;tuzaf=1;tvezx=1;tvgoj=1;tvkcm=1;"
      "tvrej=1;tvtuz=1;tvwyd=1;twazy=1;twdlb=1;twdli=1;twima=1;twwvv=1;twzre=1;"
      "txarp=1;txmjb=1;txwyc=1;tykrp=1;tyqqn=1;tywip=1;tzimw=1;tzlvv=1;tzpvy=1;"
      "tzxuu=1;uabwm=1;uacfj=1;uaiwh=1;uaoao=1;uaouf=1;uaqmz=1;uatah=1;ubcmj=1;"
      "ubgay=1;ubmrs=1;ubqal=1;ubwkh=1;ubzlc=1;uccfv=1;uccyf=1;uckhy=1;uclfx=1;"
      "ucmqo=1;ucnqp=1;udpkr=1;udtqd=1;udxdh=1;uefrx=1;uehdp=1;uehga=1;uehtc=1;"
      "uekce=1;uelfe=1;ueqay=1;ueqem=1;ufaee=1;ufalj=1;ufhyv=1;ufmzo=1;ufpxw=1;"
      "ufxrq=1;ugngu=1;ugopt=1;uhakc=1;uhchl=1;uhejx=1;uhfaf=1;uhior=1;uhqkb=1;"
      "uhsgd=1;uhuxe=1;uhuyh=1;uhxfl=1;uiajs=1;uibys=1;uilyh=1;uipuv=1;ujdxm=1;"
      "ujoqa=1;ujpnx=1;ujpry=1;ujyzy=1;ujzvo=1;ukbmv=1;ukbwg=1;ukihc=1;ukkqf=1;"
      "uklpc=1;ukxgv=1;ukzrl=1;uldxv=1;ulhbb=1;ulifc=1;ulloe=1;ulocy=1;ulsjk=1;"
      "umdgd=1;umhpr=1;umlir=1;umske=1;umvkg=1;unafr=1;unkwh=1;unpwt=1;unujf=1;"
      "unvav=1;unvlw=1;uodfd=1;uojxc=1;uokgd=1;uolma=1;uowgs=1;upgid=1;upvkl=1;"
      "uqbpn=1;uqbsn=1;uqdio=1;uqfjx=1;uqhsx=1;uqihk=1;uqjep=1;uqjit=1;uqqqd=1;"
      "uqscj=1;urgfd=1;urlji=1;urpgo=1;urrgv=1;uruas=1;urxhy=1;usbkz=1;uspqw=1;"
      "ustgy=1;usudu=1;uswyd=1;utcfd=1;utqjh=1;utrmz=1;utvrn=1;utxff=1;uucgq=1;"
      "uuhao=1;uumur=1;uunhq=1;uutir=1;uvccb=1;uvdwy=1;uvesd=1;uvetu=1;uvisc=1;"
      "uvjbs=1;uvoep=1;uvpbq=1;uvves=1;uvzhi=1;uwjed=1;uwpws=1;uwrku=1;uwtzs=1;"
      "uxcta=1;uxjza=1;uxjzm=1;uxpyx=1;uycdn=1;uycru=1;uydkv=1;uydrt=1;uyskz=1;"
      "uytot=1;uzkxb=1;uzzwg=1;vafvi=1;vanms=1;vaoai=1;vavdj=1;vbfku=1;vbksc=1;"
      "vbncq=1;vbrra=1;vbzjo=1;vchiy=1;vcmow=1;vcqtm=1;vcxax=1;vcynd=1;vdflc=1;"
      "vdjtn=1;vdyvq=1;vedmk=1;vegxe=1;veniv=1;vepdf=1;vesby=1;veshj=1;vewto=1;"
      "vffvc=1;vfhsu=1;vfifv=1;vfjum=1;vfrmk=1;vfyac=1;vgbbl=1;vgczb=1;vgrhy=1;"
      "vgswz=1;vgwnk=1;vgxci=1;vgyem=1;vgykq=1;vgzle=1;vgzno=1;vhavc=1;vhlcf=1;"
      "vhoin=1;vhqqc=1;vhwiu=1;vhxss=1;viwet=1;vizep=1;vjarj=1;vjhzm=1;vjjin=1;"
      "vkcuw=1;vkgjk=1;vkizi=1;vkkoh=1;vktqq=1;vkwar=1;vlcws=1;vlfcz=1;vlpxy=1;"
      "vlrqm=1;vltee=1;vluzw=1;vlxin=1;vmimo=1;vmotn=1;vngfp=1;vnibs=1;vnjza=1;"
      "vnval=1;vnvbu=1;vocjc=1;vocyz=1;vodiv=1;vofjg=1;vogek=1;vohbu=1;vomeg=1;"
      "vomum=1;votib=1;voudz=1;voufk=1;voxaj=1;vpjko=1;vpmmd=1;vpoms=1;vppuk=1;"
      "vqfsg=1;vqimn=1;vqkqo=1;vqstw=1;vqtvb=1;vqtzy=1;vqwxe=1;vqyfx=1;vqyrt=1;"
      "vrffm=1;vrilz=1;vrmhh=1;vsbtl=1;vsdrd=1;vsesp=1;vsltk=1;vsqjn=1;vsrxd=1;"
      "vsseb=1;vsxut=1;vtkww=1;vttox=1;vtuoq=1;vtxei=1;vuhno=1;vuhpy=1;vumiv=1;"
      "vuoqm=1;vvckc=1;vvgee=1;vvjlp=1;vvqlb=1;vvvva=1;vwhjy=1;vwoxe=1;vwsya=1;"
      "vwtpl=1;vxbqx=1;vxktw=1";
  headers +=
      "\r\ncOOkie: "
      "vxrbq=1;vxrca=1;vxrvi=1;vydkn=1;vynty=1;vyqee=1;vywey=1;vzejg=1;vzkjq=1;"
      "vzoqe=1;vzrhc=1;wabfz=1;wadaz=1;wafzj=1;waitp=1;wamdl=1;wbgel=1;wbgrz=1;"
      "wbkos=1;wbooa=1;wboyp=1;wbuee=1;wbvmv=1;wbvok=1;wbxbc=1;wcgch=1;wcjhh=1;"
      "wcxug=1;wdajv=1;wdhgg=1;wdjqy=1;wdrpi=1;wdtrh=1;webed=1;wekog=1;welsg=1;"
      "weowk=1;wevqh=1;weytc=1;wfglx=1;wfhil=1;wfkys=1;wflzz=1;wfnkc=1;wfpnm=1;"
      "wfzuj=1;wgdvv=1;wgffw=1;wgqbm=1;wgwom=1;wgxng=1;whbkp=1;whbro=1;whetj=1;"
      "whfzd=1;whlvd=1;whovq=1;whpbe=1;whvev=1;wihsh=1;wijix=1;wimto=1;winoa=1;"
      "wiylq=1;wizdl=1;wjsgu=1;wjxhk=1;wjxwn=1;wkbee=1;wkdak=1;wkgxm=1;wkjvx=1;"
      "wkqhr=1;wkqsb=1;wkthm=1;wkuse=1;wlacf=1;wlcvh=1;wldhz=1;wlgdy=1;wlqhg=1;"
      "wmhnt=1;wmiar=1;wmjic=1;wmjkb=1;wmnfh=1;wmznx=1;wnbvj=1;wndlo=1;wngja=1;"
      "wnial=1;wnjin=1;wnsky=1;wofvg=1;wolfv=1;wooox=1;worvx=1;wpfgq=1;wpfru=1;"
      "wpgdg=1;wphsi=1;wpnyn=1;wpudd=1;wqruf=1;wqyrq=1;wrcla=1;wrdbf=1;wrnut=1;"
      "wsbyg=1;wsejz=1;wsjxe=1;wskzs=1;wsnxs=1;wsobb=1;wsqhh=1;wsthf=1;wsxyr=1;"
      "wsykr=1;wtkcu=1;wtkfh=1;wubag=1;wubvs=1;wucjk=1;wuhpz=1;wuold=1;wuqjg=1;"
      "wurlt=1;wurrl=1;wuylo=1;wvaqf=1;wvbkz=1;wvcfs=1;wvchd=1;wvfta=1;wvloz=1;"
      "wvpfl=1;wvxvv=1;wvzlc=1;wwbcm=1;wwdsn=1;wwqzt=1;wwzgb=1;wxfpy=1;wxonc=1;"
      "wxsxc=1;wxukz=1;wycgp=1;wyhet=1;wzcgi=1;wzegi=1;wzfdr=1;wznho=1;xabvf=1;"
      "xahuh=1;xaqqt=1;xbaqx=1;xbgqd=1;xbgxq=1;xbxfc=1;xcbgk=1;xcfby=1;xcfnd=1;"
      "xcjqf=1;xcnlp=1;xcqfm=1;xctna=1;xdcee=1;xdesl=1;xdpgi=1;xdxqd=1;xdydt=1;"
      "xejjd=1;xerbp=1;xeryl=1;xeujs=1;xewgy=1;xfagi=1;xfeyk=1;xfgsa=1;xfoew=1;"
      "xfxom=1;xgaom=1;xgapc=1;xgbqa=1;xgfjx=1;xgfuo=1;xgiud=1;xgolz=1;xgrrn=1;"
      "xhksy=1;xhlad=1;xhlox=1;xhlqo=1;xhwai=1;xhxvb=1;xiadu=1;xiaie=1;xietg=1;"
      "xiibg=1;xijjk=1;ximzf=1;xiose=1;xiryt=1;xiujj=1;xivhx=1;xixgw=1;xixip=1;"
      "xjara=1;xjbsx=1;xjdxs=1;xjkep=1;xjlob=1;xjntf=1;xjpry=1;xjrfe=1;xjujw=1;"
      "xkcau=1;xknbr=1;xkndl=1;xkzyn=1;xldyt=1;xlign=1;xlinf=1;xllsa=1;xllvb=1;"
      "xlqal=1;xlvio=1;xmcoz=1;xmgfy=1;xmsxb=1;xmwcr=1;xndir=1;xnlsv=1;xnwlo=1;"
      "xoaru=1;xodgg=1;xoeqt=1;xofwa=1;xolry=1;xomey=1;xomfo=1;xowyu=1;xpbgp=1;"
      "xpiwl=1;xpyhv=1;xqekk=1;xqkgd=1;xqkkr=1;xqpzk=1;xqyio=1;xqynr=1;xrjay=1;"
      "xrjon=1;xrugq=1;xryom=1;xrzal=1;xscec=1;xscxu=1;xsepi=1;xsios=1;xsjob=1;"
      "xsrxn=1;xsxqu=1;xtbgm=1;xtddh=1;xtjmn=1;xtlwh=1;xtodb=1;xtpdm=1;xtwvv=1;"
      "xtyoe=1;xuhxy=1;xujlw=1;xulwg=1;xusnb=1;xusrj=1;xutli=1;xvfht=1;xvkyv=1;"
      "xvrln=1;xvxra=1;xwhft=1;xwhna=1;xwinc=1;xwlcc=1;xwqxv=1;xwuso=1;xxdjm=1;"
      "xxiqw=1;xxspp=1;xxvds=1;xxvok=1;xxxgp=1;xxyvn=1;xygxb=1;xynnh=1;xyske=1;"
      "xytuz=1;xyvlx=1;xzexi=1";

  const auto request = "GET / HTTP/1.1" + headers + "\r\n\r\n";

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < 100; i++) {
    parser.Parse(request.data(), request.size());
  }
  EXPECT_EQ(parsed, true);
  auto stop = std::chrono::steady_clock::now();

  // 1 second for "good" code, 100 seconds for "bad" code
  // use 10 sec limit as a geometric mean to differentiate these cases
  const auto kLimitTime = std::chrono::seconds(10);
  EXPECT_LE(stop - start, kLimitTime);
}

USERVER_NAMESPACE_END
