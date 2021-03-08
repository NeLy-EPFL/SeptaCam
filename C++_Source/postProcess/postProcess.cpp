#include <stdio.h>
#include <string> 
#include <iostream> 
#include <fstream>
#include <string>

#include "../NIR_Imaging.h"

using namespace std;

int main()
{
  string working_dir = "/data/temp---01-05-2018---03-11-08";
  cameras_save(working_dir);
  return 0;
}
