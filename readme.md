# evt2root
This program converts files generated by the DAQ (`.evt` files) to file read by ROOT (`.root` files).

Description: Takes `.evt` files from a list and converts them to ROOT format. 
The file `evt_files.lst` specifies the location and name of the ouput ROOT file on the first line. 
Only one ROOT file is generated. The second line is the directory where all the data files are located.
The list of run numbers is the remainder of the file. 
This script also needs a library where the detectors classes are defined.

## Version - `evt2root_NSCL11_mADC.C`
Version used by Florida State University, generalized to include unpacking of Mesytec ADC modules.
It takes all types of modules in the same way and unpacks it.
This is the current version for use with SPS data.

## Requirements
The following files are required for evt2root to run.
* `2016_detclass.h`
* `VM_BaseClass.cpp`
* `VM_Module.cpp`
* `evt_files.lst`

## Execution
To run evt2root, enter the following commands.
```
root -l
.L VM_BaseClass.cpp+
.L VM_Module.cpp+
.x evt2root_NSCL11.C+
```
This list of commands is executed by the file `rootinput.C` and can be executed as follows.
```
root -l -q rootinput.C
```
### Manual mode
Concatonate several `.evt` files into one `.root` file. To run, edit the file `evt_files.lst` and pass `rootinput.txt` to ROOT. The file `evt_files.lst` should be of the form
```
Output ROOT file: <output_dir>/filename.root
Data directory: <input_dir>
0001
0002
0003
0004
```
with the list of `.evt` included. In this example, files 0001--0004 are read in from the input directory and output to filename.root in the output directory. The data from *all four* files will be combined together into a *single* output file.

### Batch mode
Convert several `.evt` files to individulally. The file `data.cpp` allows the user to convert a list of files in batch mode. The use of this code will generate a seperate ROOT file for each run.
The location of the data files (input) and ROOT files (output) are specified in the code. Any modifications require re-compiling the program with the following command.
```
g++ data.cpp -o data.out

```

Once up to date, the file is run with the following  command.
```
./data.out

```
The program runs evt2root individually on the files listed in `runs.lst`. The user has option of deleting the `.evt` files after conversion. For each run listed in `runs.lst`, a new version of the file `evt_file.lst` is generated. The commands run for each individual conversion are listed in `rootinput.txt`.
 
The run-to-run changes in `runs.lst` and `evt_file.lst` are excuded by `.gitignore`. To force the updated files to be saved to the repository, use the command `git add -f foo.bar`.

#### Requires:
* `data.cpp`
* `runs.lst`
* `rootinput.C`
 
#### Output:
* `evt_files.lst`

## Data structure
The SPS detectors are read out in the following manner.
### ADC
The signals read out by CAEN ADCs and fill the `ADC.` branches of the `DataTree.`

### TDC
Not currently used. Data will fill the `TDC.` branches of the `DataTree`.
The `CAENHit` detector class is used.