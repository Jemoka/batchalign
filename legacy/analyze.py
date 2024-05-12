""""
analyze.py
Analyze the results of verify.py
"""

from os import pread
import pathlib
import csv
import pandas as pd

# Stats
import numpy as np
import scipy.stats


# Utility functions
def mean_confidence_interval(data, confidence=0.95):
    """calculate confidence interval

    Attributes:
        data (array-like): data
        [confidence] (float): confidence band

    Returns:
        (mean, lower bound, upper bound)
    """
    a = 1.0 * np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * scipy.stats.t.ppf((1 + confidence) / 2., n-1)
    return m, m-h, m+h

# syllable calculation
def syllables(word):
    """manually count syllables

    Attributes:
        word (str): string word
    
    Source:
        stackoverflow.com/questions/14541303/count-the-number-of-syllables-in-a-word
    """
    
    count = 0
    vowels = 'aeiouy'
    word = word.lower()
    if word == '':
        print("Empty word found!")
        return 0
    if word[0] in vowels:
        count +=1
    for index in range(1,len(word)):
        if word[index] in vowels and word[index-1] not in vowels:
            count +=1
    if word.endswith('e'):
        count -= 1
    if word.endswith('le'):
        count += 1
    if count == 0:
        count += 1
    return count

# download our syllable dictionary
from nltk.corpus import cmudict
try:
    d = cmudict.dict()
except LookupError:
    import nltk
    nltk.download('cmudict')

def nsyl(word):
    try:
        return [len(list(y for y in x if y[-1].isdigit())) for x in d[word.lower()]][0]
    except KeyError:
        #if word not found in cmudict
        return syllables(word)

if __name__ == "__main__":
    import argparse

    # file to check
    parser = argparse.ArgumentParser(description="check output CSV of the verify.py results to calculate statistics")
    parser.add_argument("in_file", type=str, help='path to the CSV output of verify.py')

    # parse args
    args = parser.parse_args()

    # File to scan
    SCANFILE = args.in_file.strip()

    # open the scanfile    
    df = pd.read_csv(SCANFILE)
    # calculate syllables
    df["syllables"] = df.word.apply(nsyl)

    # get result percentage
    percentage_part = len(df[df.correct == "1PART"])/(len(df)-len(df[df.correct == "0NONE"]))

    # split the result
    multisyllabic = df[df.syllables > 1].correct.apply(lambda x:int(x[0]))
    monosyllabic = df[df.syllables == 1].correct.apply(lambda x:int(x[0]))

    # get the ranges for each segment
    mean, bot, top = mean_confidence_interval(df.correct.apply(lambda x:int(x[0])))
    multimean, multibot, multitop = mean_confidence_interval(multisyllabic)
    monomean, monobot, monotop = mean_confidence_interval(monosyllabic)

    multiband = multitop-multimean
    monoband = monotop-monomean
    band = top-mean

    print(f"""
    Thanks. We have processed {pathlib.Path(SCANFILE).stem}.

    Within which, {(multimean*100):.2f}%±{(multiband*100):.2f}% of multi-syllabic words were correctly identified
                  {(monomean*100):.2f}%±{(monoband*100):.2f}% of mono-syllabic words were correctly identified
                  {(mean*100):.2f}%±{(band*100):.2f}% of all words were correctly identified
    at a confidence interval of 95% based on a single-variable t test.

    Within sucesseses, {(percentage_part*100):.2f}% are partial.
    """)


