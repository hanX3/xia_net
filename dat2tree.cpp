#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

using namespace::std;

void get_hit_parten_info(string s, Bool_t &pu);

//
void dat2tree(int run_num)
{
  ifstream fi;
  fi.open(TString::Format("../data/run%04d.dat",run_num).Data());
  if(!fi){
    cout << "can not open " << TString::Format("../data/run%04d.dat",run_num).Data() << endl;
    return;
  }

  std::string line;
  stringstream ss;

  //line1
  std::getline(fi, line);
  cout << line << endl;

  //line2
  line.clear();
  std::getline(fi, line);
  ss.str(line);

  cout << ss.str() << endl;
  string a;
  getline(ss, a, ',');
  string run_type;
  getline(ss, run_type, ',');
  //cout << "run_type " << run_type << endl;
  if(!ss.str().compare("0x501")){
    cout << "wrong run type" << endl;
    return;
  }
  /*
  string t1, t2;
  getline(ss, t1, ',');
  getline(ss, t2, ',');
  std::time_t t = std::stoi(t2);
  std::tm *time_info = std::localtime(&t);
  char buffer[256];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", time_info);
  cout << buffer << endl;
  */

  TFile *fo =  new TFile(TString::Format("./rootfile/run%04d_%s.root",run_num,run_type.c_str()).Data(), "recreate");

  //line3
  line.clear();
  std::getline(fi, line);

  //real data
  Short_t ch;
  Bool_t pile_up;
  UShort_t adc;
  Long64_t ts;

  TTree *tr = new TTree("tr", TString::Format("run_type_%s",run_type.c_str()).Data());
  tr->Branch("ch", &ch, "ch/S");
  tr->Branch("pile_up", &pile_up, "pile_up/O");
  tr->Branch("adc", &adc, "adc/s");
  tr->Branch("ts", &ts, "ts/L");

  Long64_t n;
  string str_no, str_ch, str_hit, str_time_h, str_time_l, str_adc;
  line.clear();
  ss.str("");

  while(std::getline(fi, line)){
    ss.str(line);
    ss.clear();
    getline(ss, str_no, ',');
    getline(ss, str_ch, ',');
    getline(ss, str_hit, ',');
    getline(ss, str_time_h, ',');
    getline(ss, str_time_l, ',');
    getline(ss, str_adc, '\n');

    n = (Long64_t)std::stoll(str_no);
    if(n%10000==0){
      cout << "\r" << n;
      cout << flush;
    }
    ch = (Short_t)std::stoi(str_ch);
    get_hit_parten_info(str_hit, pile_up);
    adc = (UShort_t)std::stoi(str_adc);
    ts = (std::stoll(str_time_h)*0x100000000) + (Long64_t)std::stoll(str_time_l);
    
    /*
    cout << "str_no " << str_no << endl;
    cout << "str_ch " << str_ch << endl;
    cout << "str_hit " << str_hit << endl;
    cout << "str_time_h " << str_time_h << endl;
    cout << "str_time_l " << str_time_l << endl;
    cout << "str_adc " << str_adc << endl;
    cout << "ch " << ch << endl;
    cout << "pile_up " << pile_up << endl;
    cout << "adc " << adc << endl;
    cout << "ts " << ts << endl;
    */

    tr->Fill();

    //
    line.clear();
    str_no.clear();
    str_ch.clear();
    str_hit.clear();
    str_time_h.clear();
    str_time_l.clear();
    str_adc.clear();
  }
  cout << endl;
  
  fo->cd();
  tr->Write();
  fo->Close();

  fi.close();
}

//
void get_hit_parten_info(string s, Bool_t &pu)
{
  const static unsigned int k_mask_pile_up =  0x00040000;
  const static unsigned int k_shift_pile_up =  17;

  int hit = std::stoi(s, nullptr, 16);
  pu = (hit&k_mask_pile_up)>>k_shift_pile_up;
}

//
int main(int argc, char *argv[])
{
  if(argc != 2){
    std::cout << "need parameter" << std::endl;
    std::cout << "like: dat2tree 33" << std::endl;
    return -1;
  }

  int run = atoi(argv[1]);
  dat2tree(run);

  return 0;
}
