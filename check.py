""""
check.py
Checks the output of batchalign for xwor tier and main-tier alignment
"""

# os utilities
import os
# pathing
import pathlib
# regex
import re
# glob
import glob

# Oneliner of directory-based glob and replace
globase = lambda path, statement: glob.glob(os.path.join(path, statement))
repath_file = lambda file_path, new_dir: os.path.join(new_dir, pathlib.Path(file_path).name)

# path of CLAN
CLAN_PATH=""

# file to check
CHECKDIR="/Users/houliu/Documents/Projects/talkbank-alignment/EllisWeismer/30ec/TD/v2/out/"

# get output
files = globase(CHECKDIR, "*.cha")

# for each file
for checkfile in files:

    # run flo on the file
    CMD = f"{os.path.join(CLAN_PATH, 'flo +t%xwor')} {checkfile} >/dev/null 2>&1"
    # run!
    os.system(CMD)

    # path to result
    result_path = checkfile.replace("cha", "flo.cex")

    # read in the resulting file
    with open(result_path, "r") as df:
        # get alignment result
        data = df.readlines()

    # delete result file
    os.remove(result_path)

    # conform result with tab-seperated beginnings
    result = []
    # for each value
    for value in data:
        # if the value continues from the last line
        if value[0] == "\t":
            # pop the result
            res = result.pop()
            # append
            res = res.strip("\n") + " " + value[1:]
            # put back
            result.append(res)
        else:
            # just append typical value
            result.append(value)

    # new the result
    result = [re.sub(" \\x15\d*_\d*\\x15>", ">", i) for i in result] # bullets
    result = [re.sub("\\x15.*?\\x15 ?", "", i) for i in result] # bullets
    result = [re.sub("\(\.+\)", "", i) for i in result] # pause marks (we remove)
    result = [re.sub(".*?\\t", "", i) for i in result] # tabs
    result = [re.sub("\.", "", i) for i in result] # doduble spaces
    result = [re.sub("  ", " ", i).strip() for i in result] # doduble spaces

    # get paired results
    result_paired = []

    # pair up results
    for i in range(0, len(result)-3, 3):
        # append paired result
        result_paired.append((result[i], result[i+2]))

    # check for equivalence and report error
    errors = []

    # tabulate errors
    for indx,(i,j) in enumerate(result_paired):
        if i != j:
            errors.append(indx)

    # print results
    print()
    print(f"check.py output on {os.path.basename(checkfile)}")
    print("-----------------------------\n")

    # for the error
    for item in errors:
        print(f"Main tier ({item*3}): {result_paired[item][0]}")
        print(f"%xwor tier ({item*3+2}): {result_paired[item][1]}\n")
    # if no errors, print
    if len(errors) == 0:
        print("No inconsistencies found!\n")


