# evt2root
This program converts files generated by ASICS (`.evt` files) to file read by ROOT (`.root` files).

Description: Takes `.evt` files from a list and converts them to ROOT format. 
The file `evt_files.list` specifies the location and name of the ouput ROOT
file on the first line. Only one ROOT file is generated. The second line is
the directory where all the data files are located. The list of run numbers
is the remainder of the file. This script also needs a library where the
detectors classes are defined.

## Versions
Two version of this code exist, depending on the electonics setup.
### `evt2root_NSCL11.C`
Version used by Texas A&M University. Adopted & tested for the NSCLDAQ11 version.

### `evt2root_NSCL11_mADC.C`
Version used by Florida State University, generalized to include Mesytec modules. 

## Requirements
*VM_BaseClass.cpp
*VM_Module.cpp
*SimpleInPipe.cpp
*evt_files.list


## Execution
To run evt2root, the following macros are loaded.
```
root -l
.x VM_BaseClass.cpp+
.L VM_Module.cpp+
.L SimpleInPipe.cpp+
.x evt2root_NSCL11.C+
```
This list of commands is contained within the file `rootinput.txt`.
```
root -l < rootinput.txt
```
### Manual mode
Concatonate several `.evt` files into one `.root` file. To run, edit the file `evt_files.list` and pass `rootinput.txt` to ROOT. The file `evt_files.list` should be of the form
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
The program runs evt2root individually on the files listed in `runs.lst`. The user has option of deleting the `.evt` files after conversion. The commands run for each conversion are listed in `rootinput.txt`.  
 
#### Requires:
*`runs.lst`
*`rootinput.txt`
 
#### Output:
>`evt_files.list`
