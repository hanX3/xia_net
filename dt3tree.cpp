#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#define MaxN 4

using namespace::std;

//
void get_hit_parten_info(string s, Bool_t &pu);
Long64_t get_ts_evt(string s1, string s2); 

//
void dt3tree(int run_num)
{
  ifstream fi;
  fi.open(TString::Format("../data/run%04d.dt3",run_num).Data());
  if(!fi){
    cout << "can not open " << TString::Format("../data/run%04d.dt3",run_num).Data() << endl;
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
  if(!ss.str().compare("0x503")){
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
  Int_t n_hit;
  Short_t ch[MaxN];
  UShort_t adc[MaxN];
  Long64_t ts_local[MaxN];
  Bool_t pile_up;
  Long64_t ts_event;

  TTree *tr = new TTree("tr", TString::Format("run_type_%s",run_type.c_str()).Data());
  tr->Branch("n_hit", &n_hit, "n_hit/I");
  tr->Branch("ch", ch, "ch[n_hit]/S");
  tr->Branch("adc", adc, "adc[n_hit]/s");
  tr->Branch("ts_local", ts_local, "ts_local[n_hit]l/L");
  tr->Branch("pile_up", &pile_up, "pile_up/O");
  tr->Branch("ts_event", &ts_event, "ts_event/L");

  Long64_t n;
  string str_no, str_hit, str_time_evt_h, str_time_evt_l, str_pps_time, str_ts_local[4], str_adc[4];
  line.clear();
  ss.str("");

  while(std::getline(fi, line)){
    n_hit = 0;
    memset(ch, 0, sizeof(ch));
    memset(adc, 0, sizeof(adc));
    memset(ts_local, 0, sizeof(ts_local));
    pile_up = 0;
    ts_event = 0;

    ss.str(line);
    ss.clear();
    getline(ss, str_no, ',');
    getline(ss, str_hit, ',');
    getline(ss, str_time_evt_h, ',');
    getline(ss, str_time_evt_l, ',');
    getline(ss, str_pps_time, ',');
    for(int i=0;i<4;i++){
      getline(ss, str_ts_local[i], ',');
    }
    for(int i=0;i<4;i++){
      getline(ss, str_adc[i], ',');
    }

    /*
    cout << "str_no " << str_no << endl;
    cout << "str_hit " << str_hit << endl;
    cout << "str_time_evt_h " << str_time_evt_h << endl;
    cout << "str_time_evt_l " << str_time_evt_l << endl;
    for(int i=0;i<4;i++){
      cout << "str_ts_local " << str_ts_local[i] << endl;
      cout << "str_adc " << str_adc[i] << endl;
    }
    */

    for(int i=0;i<4;i++){
      if(std::stoi(str_adc[i])>0){
        ch[n_hit] = i;
        adc[n_hit] = std::stoi(str_adc[i]);
        ts_local[n_hit] = std::stoll(str_ts_local[i]);
        
        n_hit++;
      }
    }

    get_hit_parten_info(str_hit, pile_up);
    ts_event = get_ts_evt(str_time_evt_h, str_time_evt_l);

    /*
    std::cout << "n_hit " << n_hit << std::endl; 
    for(int i=0;i<n_hit;i++){
      std::cout << "ch " << ch[i] << std::endl;
      std::cout << "adc " << adc[i] << std::endl;
      std::cout << "ts_local " << ts_local[i] << std::endl;
    }
    std::cout << "pile_up " << pile_up << std::endl; 
    std::cout << "ts_event " << ts_event << std::endl; 
    */
    n = (Long64_t)std::stoll(str_no);
    if(n%10000==0){
      cout << "\r" << n;
      cout << flush;
    }
    

    tr->Fill();

    //
    line.clear();
    str_no.clear();
    str_hit.clear();
    str_time_evt_h.clear();
    str_time_evt_l.clear();
    str_pps_time.clear();
    for(int i=0;i<4;i++){
      str_ts_local[i].clear();
      str_adc[i].clear();
    }
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
Long64_t get_ts_evt(string s1, string s2)
{
  Long64_t th = std::stoll(s1);
  Long64_t tl = std::stoll(s2);

  return th*0x1000000+tl;
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
  dt3tree(run);

  return 0;
}
