#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>

using namespace::std;

//
struct file_header{
  uint16_t blk_size;
  uint16_t mod_num;
  uint16_t run_format;
  uint16_t chan_head_len;
  uint16_t coin_pat;
  uint16_t coin_win;
  uint16_t max_comb_event_len;
  uint16_t board_version;
  uint16_t event_length0;
  uint16_t event_length1;
  uint16_t event_length2;
  uint16_t event_length3;
  uint16_t serial_number;
  uint16_t unused[19];
};

//
struct channel_header{
  uint16_t evt_pattern;
  uint16_t evt_info;
  uint16_t num_trace_block;
  uint16_t num_trace_blk_prev;
  uint16_t trig_time_lo;
  uint16_t trig_time_mi;
  uint16_t trig_time_hi;
  uint16_t trig_time_x;
  uint16_t adc;
  uint16_t chan_no;
  uint16_t psa[6];
  uint16_t unused[16];
};

//
int get_file_header_info(FILE *f, file_header &fh);
int get_channel_header_info(FILE *f, channel_header &ch);
int get_trace(FILE *f, vector<UShort_t> &v, int l);
int get_hit_parten_info(uint16_t hp1, uint16_t hp2, Bool_t &pu);


//
void x400tree(int run_num)
{
  FILE *fi = fopen(TString::Format("../data/run%04d.b00",run_num).Data(), "rb");
  if(!fi){
    cout << "can not open " << TString::Format("../data/run%04d.b00",run_num).Data() << endl;
    return;
  }

  fseek(fi, 0, SEEK_END);
  Long64_t file_size = ftell(fi);
  cout << "file_size " << file_size << " bytes" << endl;
  fseek(fi, 0, SEEK_SET);
  /*
  uint16_t buff; // read 16 bit data once
  cout << fread(&buff, sizeof(buff), 1, fi) << endl; 
  cout << buff << endl;
  */
  file_header fi_h;
  if(get_file_header_info(fi,fi_h) == -1){
    cout << "read file header wrong" << endl;
    return;
  }

  /*
  cout << "file header blk_size " << fi_h.blk_size << endl;
  cout << "file header mod_num " << fi_h.mod_num << endl;
  cout << "file header run_format " << fi_h.run_format << endl;
  cout << "file header chan_head_len " << fi_h.chan_head_len << endl;
  cout << "file header coin_pat " << fi_h.coin_pat << endl;
  cout << "file header coin_win " << fi_h.coin_win << endl;
  cout << "file header max_comb_event_len " << fi_h.max_comb_event_len << endl;
  cout << "file header board_version " << fi_h.board_version << endl;
  cout << "file header event_length0 " << fi_h.event_length0 << endl;
  cout << "file header event_length1 " << fi_h.event_length1 << endl;
  cout << "file header event_length2 " << fi_h.event_length2 << endl;
  cout << "file header event_length3 " << fi_h.event_length3 << endl;
  cout << "file header serial_number " << fi_h.serial_number << endl;
  */

  //
  if(fi_h.run_format != 0x400){
    cout << "only suitbale for runn type " << std::hex << fi_h.run_format << endl;
    return ;
  }
  stringstream ss_rt;
  ss_rt << std::hex << fi_h.run_format;
  TFile *fo =  new TFile(TString::Format("./rootfile/run%04d_0x%s.root",run_num,ss_rt.str().c_str()).Data(), "recreate");
  cout << TString::Format("./rootfile/run%04d_0x%s.root",run_num,ss_rt.str().c_str()).Data() << endl;
  TTree *tr = new TTree("tr", TString::Format("run_format_%d",fi_h.run_format).Data());

  Short_t ch;
  Bool_t pile_up;
  UShort_t adc;
  Long64_t ts;
  vector<UShort_t> v_trace;

  tr->Branch("ch", &ch, "ch/S");
  tr->Branch("pile_up", &pile_up, "pile_up/O");
  tr->Branch("adc", &adc, "adc/s");
  tr->Branch("ts", &ts, "ts/L");
  tr->Branch("v_trace", &v_trace);

  Long64_t n = 0;
  channel_header ch_h;
  while(1){
    if(feof(fi)){
      cout << "end of file" << endl;
      break;
    }

    if(n%100000==0){
      cout << "\r" << n;
      cout << std::flush;
    }
    ch = 0;
    pile_up = 0;
    adc = 0;
    ts = 0;
    v_trace.clear();
    memset(&ch_h, 0, sizeof(ch_h));

    if(get_channel_header_info(fi,ch_h) == -1){
      cout << "read channel header wrong" << endl;
      return;
    }

    /*
    cout << "channel header evt_pattern " << ch_h.evt_pattern << endl;
    cout << "channel header evt_info " << ch_h.evt_info << endl;
    cout << "channel header num_trace_block " << ch_h.num_trace_block << endl;
    cout << "channel header num_trace_blk_prev " << ch_h.num_trace_blk_prev << endl;
    cout << "channel header trig_time_lo " << ch_h.trig_time_lo << endl;
    cout << "channel header trig_time_mi " << ch_h.trig_time_mi << endl;
    cout << "channel header trig_time_hi " << ch_h.trig_time_hi << endl;
    cout << "channel header trig_time_x " << ch_h.trig_time_x << endl;
    cout << "channel header adc " << ch_h.adc << endl;
    cout << "channel header chan_no " << ch_h.chan_no << endl;
    for(int i=0;i<6;i++){
      cout << "channel header psa " << i << " " << ch_h.psa[i] << endl;
    }
    */

    if(ch_h.num_trace_block==0){
      cout << endl;
      cout << ftell(fi) << "/" << file_size; 
      break;
    }
    if(get_trace(fi,v_trace,fi_h.blk_size*ch_h.num_trace_block) == -1){
      cout << "get trace wrong " << endl;
      return;
    }

    ch = ch_h.chan_no;
    get_hit_parten_info(ch_h.evt_pattern, ch_h.evt_info, pile_up);
    adc = ch_h.adc;
    ts = ((Long64_t)ch_h.trig_time_x)*0x1000000000000 + ((Long64_t)ch_h.trig_time_hi)*0x100000000 + ((Long64_t)ch_h.trig_time_mi)*0x10000 + ((Long64_t)ch_h.trig_time_lo);

    tr->Fill();

    n++;
  }
  cout << endl;

  fo->cd();
  tr->Write();
  fo->Close();

  fclose(fi);
}

//
int get_file_header_info(FILE *f, file_header &header)
{
  if(fread(&header.blk_size,sizeof(header.blk_size),1,f)<0){
    cout << "read file header blk_size wrong" << endl;
    return -1;
  }

  if(fread(&header.mod_num,sizeof(header.mod_num),1,f)<0){
    cout << "read file header mod_num wrong" << endl;
    return -1;
  }

  if(fread(&header.run_format,sizeof(header.run_format),1,f)<0){
    cout << "read file header run_format wrong" << endl;
    return -1;
  }

  if(fread(&header.chan_head_len,sizeof(header.chan_head_len),1,f)<0){
    cout << "read file header chan_head_len wrong" << endl;
    return -1;
  }

  if(fread(&header.coin_pat,sizeof(header.coin_pat),1,f)<0){
    cout << "read file header coin_pat wrong" << endl;
    return -1;
  }

  if(fread(&header.coin_win,sizeof(header.coin_win),1,f)<0){
    cout << "read file header coin_win wrong" << endl;
    return -1;
  }

  if(fread(&header.coin_win,sizeof(header.coin_win),1,f)<0){
    cout << "read file header max_comb_event_len wrong" << endl;
    return -1;
  }

  if(fread(&header.max_comb_event_len,sizeof(header.max_comb_event_len),1,f)<0){
    cout << "read file header max_comb_event_len wrong" << endl;
    return -1;
  }
  
  if(fread(&header.event_length0,sizeof(header.event_length0),1,f)<0){
    cout << "read file header event_length0 wrong" << endl;
    return -1;
  }

  if(fread(&header.event_length1,sizeof(header.event_length1),1,f)<0){
    cout << "read file header event_length1 wrong" << endl;
    return -1;
  }

  if(fread(&header.event_length2,sizeof(header.event_length2),1,f)<0){
    cout << "read file header event_length2 wrong" << endl;
    return -1;
  }

  if(fread(&header.event_length3,sizeof(header.event_length3),1,f)<0){
    cout << "read file header event_length3 wrong" << endl;
    return -1;
  }

  if(fread(&header.serial_number,sizeof(header.serial_number),1,f)<0){
    cout << "read file header serial_number wrong" << endl;
    return -1;
  }

  if(fread(&header.unused,sizeof(header.unused[0]),19,f)<0){
    cout << "read file header unused wrong" << endl;
    return -1;
  }

  return 0;
}

//
int get_channel_header_info(FILE *f, channel_header &header)
{
  if(fread(&header.evt_pattern,sizeof(header.evt_pattern),1,f)<0){
    cout << "read channel header evt_pattern wrong" << endl;
    return -1;
  }

  if(fread(&header.evt_info,sizeof(header.evt_info),1,f)<0){
    cout << "read channel header evt_info wrong" << endl;
    return -1;
  }
  
  if(fread(&header.num_trace_block,sizeof(header.num_trace_block),1,f)<0){
    cout << "read channel header num_trace_block wrong" << endl;
    return -1;
  }

  if(fread(&header.num_trace_blk_prev,sizeof(header.num_trace_blk_prev),1,f)<0){
    cout << "read channel header num_trace_blk_prev wrong" << endl;
    return -1;
  }

  if(fread(&header.trig_time_lo,sizeof(header.trig_time_lo),1,f)<0){
    cout << "read channel header trig_time_lo wrong" << endl;
    return -1;
  }

  if(fread(&header.trig_time_mi,sizeof(header.trig_time_mi),1,f)<0){
    cout << "read channel header trig_time_mi wrong" << endl;
    return -1;
  }

  if(fread(&header.trig_time_hi,sizeof(header.trig_time_hi),1,f)<0){
    cout << "read channel header trig_time_hi wrong" << endl;
    return -1;
  }

  if(fread(&header.trig_time_x,sizeof(header.trig_time_x),1,f)<0){
    cout << "read channel header trig_time_x wrong" << endl;
    return -1;
  }

  if(fread(&header.adc,sizeof(header.adc),1,f)<0){
    cout << "read file header adc wrong" << endl;
    return -1;
  }

  if(fread(&header.chan_no,sizeof(header.chan_no),1,f)<0){
    cout << "read file header chan_no wrong" << endl;
    return -1;
  }

  if(fread(&header.psa,sizeof(header.psa[0]),6,f)<0){
    cout << "read channel header psa wrong" << endl;
    return -1;
  }
  
  if(fread(&header.unused,sizeof(header.unused[0]),16,f)<0){
    cout << "read channel header unused wrong" << endl;
    return -1;
  }

  return 0;
}

//
int get_trace(FILE *f, vector<UShort_t> &v, int l)
{
  v.clear();

  UShort_t d;
  for(int i=0;i<l;i++){
    if(fread(&d,sizeof(UShort_t),1,f)<0){
      cout << "read trace wrong " << endl;
      return -1;
    }else{
      v.push_back(d);
    }
  }

  return 0;
}

//
int get_hit_parten_info(uint16_t hp1, uint16_t hp2, Bool_t &pu)
{
  UInt_t hp = (UInt_t)hp2*0x10000 + hp1;

  const static unsigned int k_mask_pile_up =  0x00040000;
  const static unsigned int k_shift_pile_up =  17;

  pu = (hp&k_mask_pile_up)>>k_shift_pile_up;

  return 0;
}

//
int main(int argc, char *argv[])
{
  if(argc != 2){
    std::cout << "need parameter" << std::endl;
    std::cout << "like: bin2tree 33" << std::endl;
    return -1;
  }

  int run = atoi(argv[1]);
  x400tree(run);

  return 0;
}

