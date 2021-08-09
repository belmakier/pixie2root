#include <iostream>
#include <vector>
#include <thread>

#include <TROOT.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TH1.h>
#include <TFile.h>

int main(int argc, char **argv) {

  std::string filename(argv[1]);
  std::cout << filename << std::endl;
  TFile *file = TFile::Open(filename.c_str());
  TTree *tree = (TTree*)file->Get("RawTree");
  file->ls();
  tree->Print();
  TTreeReader reader("RawTree", file);
  
  TTreeReaderValue<UInt_t> energy(reader, "0.2.0.eventEnergy");
  

  std::cout << "entering loop" << std::endl;
  while (reader.Next()) {
    std::cout << "here" << std::endl;
  }

  std::cout << "sorted" << std::endl;

  file->Close();
  std::cout << std::endl;
}
