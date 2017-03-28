/*
 * Program: data.cpp
 *        Developed by J. Lighthall Jul-Dec 2016
 *
 * Description: Runs evt2root on the files listed in runs.lst. User has option of deleting the 
 * .evt files after conversion. The commands run for each conversion are listed in rootinput.txt.
 * The location of the data files (input) and ROOT files (output) are specified in the code. The
 * use of this code will generate a seperate ROOT file for each run.
 *
 * Requires:
 *        runs.lst
 *        rootinput.txt
 *
 * Output:
 *        evt_files.list
 */

//C and C++ libraries
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>

using namespace std;

int main()
{
  int list[99];
  const char* fname="runs.lst"; //name of file with list of run numbers
  
  ifstream infile(fname);
  printf("Reading list file \"%s\"\n",fname);
  printf(" The following runs will be converted\n");
  int i=0;
  while (infile >> list[i]) {
    printf("  %d\n",list[i]);
    i++;
  }

  char ans;
  bool del=false;
  cout << " Do you want to delete the .evt files after conversion? (y/n)" << endl << "  ";
  cin >> ans;
  if (ans=='y')
    del=true;
      
  for(int j=0;j<i;j++)
    {
      std::ofstream outfile("evt_files.list"); //name of file referenced in evt2root_NSCL11.C
      string str0 = "/data0/lighthall/data/"; //location of .evt files
      outfile << "Output ROOT file: /data0/lighthall/root/run" << list[j] << ".root" << endl;
      outfile << "Data directory: "<< str0 << endl;
      outfile << list[j] << endl;
      system("root -l < rootinput.txt"); //name of file with ROOT command
      
      string str1 = "rm -vf ";
      str1+=str0;
      str1+="run-";
      string str2 = "-00.evt";
      string str;
      stringstream num;
      num << list[j];
      str=num.str();
      string str3 =  str1+str+str2;
      if(del)
	system(str3.c_str()); //use this line to delete the .evt files after conversion
    }
}
