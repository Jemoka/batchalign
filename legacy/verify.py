""""
verify.py
Performs one-off human validation of aligned results from batchalign facilities
"""

# os utilities
import os
# pathing
import pathlib
# regex
import re
# glob
import glob
# argument
import argparse
# randomness
import random
# import csv
import csv

# numpy
import numpy as np

# import analyze tools
from analyze import mean_confidence_interval, nsyl

# arguments!
import argparse

# path to CLAN. Empty string means system.
CLAN_PATH=""

# Oneliner of directory-based glob and replace
globase = lambda path, statement: glob.glob(os.path.join(path, statement))
repath_file = lambda file_path, new_dir: os.path.join(new_dir, pathlib.Path(file_path).name)

# NOTE THAT THE INPUT DIRECTORY MUST ONLY CONTAIN .WAVS AND .CHA,
# NOTHING ELSE SHOULD BE INSIDE THE INPUT DIRECTORY!

# file to check
parser = argparse.ArgumentParser(description="check batchalign output manually, and write result")
parser.add_argument("in_dir", type=str, help='input directory with aligned .cha files and .wav audio')
parser.add_argument('-o', "--out_file", type=str, help='path to output csv; if missing default to in_dir/check.csv')
parser.add_argument("--rate", type=float, default=0.05, help='sampling rate to check per file')
parser.add_argument("--seed", type=int, default=7, help='randomness seed for reproduction')

# parse args
args = parser.parse_args()

# seed seed
random = random.Random(args.seed)
# the percent to sample from each file
CHECKRATE=args.rate
VERIFY = args.in_dir
# get out file, or generate if missing
if args.out_file:
    OUTFILE = args.out_file
else:
    OUTFILE = os.path.join(VERIFY, "check.csv")

# open the temp files for writing
matched_files = list(zip(sorted(globase(VERIFY, "*.cha")), sorted(globase(VERIFY, "*.wav"))))
# create global results array
global_results = []

input(f"""
Welcome to interactive alignment verification
----------------------------------------------
We are verifying the contents of {VERIFY}. 

There are {len(matched_files)} number of files, we are going to be listening
to {round(CHECKRATE*100)}% of the words in each file.

For every word prompted, you will have to tap *one* key to indicate
that you heard a (the full word) s (a part of the word) d (not the word.)
You can also tap f to repeat and c to go back.

There will be a progress bar on the left denoting
the status of *ONE file*. If there are multiple files
to verify, that bar will go from 0%-100% multiple times.

Tap enter when you are ready. """)


# function to getch
import os
import sys    
import termios
import fcntl

def getch():
    """Get a single character

    Source:
        https://stackoverflow.com/questions/510357/how-to-read-a-single-character-from-the-user
    """
    fd = sys.stdin.fileno()

    oldterm = termios.tcgetattr(fd)
    newattr = termios.tcgetattr(fd)
    newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
    termios.tcsetattr(fd, termios.TCSANOW, newattr)

    oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)

    try:        
        while 1:            
            try:
                c = sys.stdin.read(1)
                break
            except IOError: pass
    finally:
        termios.tcsetattr(fd, termios.TCSAFLUSH, oldterm)
        fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)

    return c

def getprinch(t):
    """Display message and get a single character

    Attributes:
        t (str): message to display
    """
    
    print(t, end='')
    sys.stdout.flush()
    action = getch()
    print()
    return action


# function to play sound
def playsound(f, start, end):
    """Function to play a sound with ffplay

    Arguments:
        f (str): file to play
        start (int): integer in seconds where to start
        end (int): integer in seconds where to end
    """

    # 200ms per CLAN specs too
    # also to adjust for ffplay small-interval bug

    CMD = f"ffplay '{f}' -vn -t {max((end-start), 200)}ms -ss {start}ms -nodisp -autoexit  &> /dev/null &disown"
    os.system(CMD)

# check a file
def check(checkfile, checksound, checkrate=0.1):
    """Check a file and return to results

    Arguments:
        checkfile (str): path to CHAT file to check
        checksound (str): path to sound file to check
        [checkrate[ (float): percentage to check, default 0.1

    Returns:
        list [[file, word, start (ms), end (ms), '0' or '1' (STRING)for fail/success]...x words]
    """
    
    print(f"\nWe are now working on {pathlib.Path(checkfile).stem}")

    verify_results = []

    # run flo on the file
    CMD = f"{os.path.join(CLAN_PATH, 'flo +t%xwor +t%wor -t*')} '{checkfile}' >/dev/null 2>&1"
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
    result = [re.sub(r"\x15(\d*)_(\d*)\x15", r"|pause|\1_\2|pause|", i) for i in result] # bullets
    result = [re.sub("\(\.+\)", "", i) for i in result] # pause marks (we remove)
    result = [re.sub("\.", "", i) for i in result] # doduble spaces
    result = [re.sub("  ", " ", i).strip() for i in result] # doduble spaces
    result = [re.sub("\[.*?\]", "", i).strip() for i in result] # doduble spaces
    result = [[j.strip().replace("  ", " ")
            for j in re.sub(r"(.*?)\t(.*)", r"\1±\2", i).split('±')]
            for i in result] # tabs

    # extract pause info
    wordinfo = []

    # calculate final results, which skips any in-between tiers
    # isolate *PAR speaking time only. We do this by tallying
    # a running offset which removes any time between two *PAR
    # tiers, which may include an *INV tier
    for tier, res in result:
        lasttoken = None
        # split tokens
        for token in res.split(" "):
            # if pause, calculate pause
            if token != "" and token[0] == "|":
                # split pause 
                res = token.split("_")
                # get pause values
                res = [int(re.sub("\+.*", "", i.replace("|pause|>", "").replace("|pause|", "")))
                        for i in token.split("_")]
                    # append result
                wordinfo.append((lasttoken, res[0], res[1], nsyl(lasttoken)))

            # save last token
            lasttoken = token.replace("+","").replace("<","").replace(">","")

    # calculate number to sample
    number_to_sample = int(len(wordinfo)*checkrate)
    # sample equal parts of monosyllabic and multisyllabic
    monosyllabic_samples = list(filter(lambda x:x[3] == 1, wordinfo))
    multisyllabic_samples = list(filter(lambda x:x[3] > 1, wordinfo))
    # actual do sampling
    monosyllabic_samples = random.sample(monosyllabic_samples,
                                         number_to_sample)
    multisyllabic_samples = random.sample(multisyllabic_samples,
                                          number_to_sample)
    # add and shuffle
    samples = monosyllabic_samples+multisyllabic_samples
    random.shuffle(samples)

    # for each sample, playback sampling
    indx = 0
    while indx < len(samples):
        # get sample
        sample = samples[indx]
        # get progress
        progress = str(round((indx/len(samples))*100))
        # play
        playsound(checksound, sample[1], sample[2])
        # ask
        t = '\'' # to support fstring '
        action = getprinch(f"{progress+'%':>4} {f'{t}{sample[0]}{t}':<15}(a)full/(s)part/(d)none/(f)repeat/(c)back: ")
        # keep asking if response invalid or is repeat
        while action == "" or action[0].lower() not in ['a', 's', 'd', 'c']:
            playsound(checksound, sample[1], sample[2])
            action = getprinch(f"{progress+'%':>4} {f'{t}{sample[0]}{t}':<15}(a)full/(s)part/(d)none/(f)repeat/(c)back: ")
        # and now, parse
        if action[0].lower() == 'a':
            verify_results.append([pathlib.Path(checkfile).stem, sample[0], sample[1], sample[2], "1FULL"])
        elif action[0].lower() == 's':
            verify_results.append([pathlib.Path(checkfile).stem, sample[0], sample[1], sample[2], "1PART"])
        elif action[0].lower() == 'd':
            verify_results.append([pathlib.Path(checkfile).stem, sample[0], sample[1], sample[2], "0NONE"])

        # move forward or back
        if action[0].lower() == 'c':
            verify_results.pop(-1)
            indx -= 1
        else:
            indx += 1

    # return results
    return verify_results

# for each result, run check
for transcript, audio in matched_files:
    # append
    global_results += check(transcript, audio, CHECKRATE)

# get confidence
mean, bottom, top = mean_confidence_interval(np.array([int(i[-1][0]) for i in  global_results]))
band = top - mean

# write
with open(OUTFILE, 'w') as df:
    # open writer and write results
    writer = csv.writer(df)
    writer.writerow(["file", "word", "start", "end", "correct"])
    writer.writerows(global_results)

print(f"""
Thanks. We have processed {len(matched_files)} files in {VERIFY}.

Within which, {(mean*100):.2f}%±{(band*100):.2f}% of words were correctly identified
at a confidence interval of 95% based on a single-variable t test.
""")

