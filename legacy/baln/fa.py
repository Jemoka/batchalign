# Pathing facilities
import pathlib

# I/O and stl
import os
from posixpath import expanduser
import random # for testing

# Import pathing utilities
import glob
from shutil import move
from typing import ContextManager
from typing_extensions import dataclass_transform

# XML facilities
import xml.etree.ElementTree as ET

# Also, include regex
import re

# Import a progress bar
from tqdm import tqdm

# Argument parser
import argparse

# defaultdict
from collections import defaultdict

from baln.eaf import eafinject
from baln.dp import *

# language code enum
import enum

# silence OMP
os.environ["KMP_WARNINGS"] = "FALSE" 
os.environ["PYTHONWARNINGS"] = "ignore"

# import utilities
from .utils import *
from .eaf import *

# import biopython error type
from Bio import BiopythonDeprecationWarning

# ignore the pairwisealigner warning
import warnings
# with warnings.catch_warnings():
#     warnings.simplefilter("ignore", BiopythonDeprecationWarning)
#     # mfa
#     from montreal_forced_aligner.command_line.align import run_align_corpus
#     from montreal_forced_aligner.command_line.g2p import run_g2p
#     from montreal_forced_aligner.command_line.validate import run_validate_corpus
from montreal_forced_aligner.models import ModelManager
    
warnings.filterwarnings("ignore")

# Oneliner of directory-based glob and replace
globase = lambda path, statement: glob.glob(os.path.join(path, statement))
repath_file = lambda file_path, new_dir: os.path.join(new_dir, pathlib.Path(file_path).name)
bullet = lambda start,end: f" {int(round(start*1000))}_{int(round(end*1000))}" # start and end should be float seconds

# Get the default path of UnitCLAN installation
# from the path of the current file
CURRENT_PATH=pathlib.Path(__file__).parent.resolve()
CLAN_PATH=resolve_clan()
ATTRIBS_PATH=os.path.join(CURRENT_PATH, "./attribs.cut")

DISFLULENCY_CODE = re.compile("\[.*?\]")

# utility to filter out unwanted codes
def clean_codes(string):
    string = re.sub(r".*\]", "", string).strip()
    string = re.sub(r"\[.*", "", string).strip()
    string = re.sub(r"<|>", "", string).strip()
    string = re.sub(r"&=\w+", "", string).strip()
    string = re.sub(r"\(\.+\)", "", string).strip()
    string = string.replace("„", "").replace("‡", "")
    string = re.sub(r".:\+", "", string).strip()

    return string

class G2P_MODEL(enum.Enum):
    en = "english_us_arpa"
    es = "spanish_spain_mfa"
    nl = "NO_GEN!dutch_cv"
    zh = "NO_GEN!mandarin_china_mfa" # NO_GEN! prefix used for *dictionaries* not generated dictionaries

class ACOUSTIC_MODEL(enum.Enum):
    en = "english_us_arpa"
    es = "spanish_mfa"
    zh = "mandarin_mfa"
    nl = "dutch_cv"

# default MFA settings
def make_config_base():
    commands = argparse.Namespace()
    # we want to clean each time
    commands.clean = True
    # pathing
    commands.temporary_directory = ""
    commands.config_path = ""
    commands.speaker_characters = "0"

    return commands
    

# import textgrid
from .opt.textgrid.textgrid import Interval, TextGrid, IntervalTier


def align_directory_mfa(directory, data_dir, model=None, dictionary=None, beam=10, lang="en"):
    """Given a directory of .wav and .lab, align them

    Arguments:
        directory (string): string directory filled with .wav and .lab files with same name
        data_dir (string): output directory for textgrids
        [model (str)]: model to use
        [dictionary (str)]: dictionary to use
        [beam (int)]: beam width for initial MFA alignment
        [lang (str)]: 2-letter language code

    Returns:
        none
    """

    # get language model name
    acoustic_model = ACOUSTIC_MODEL[lang.lower()].value
    g2p_model = G2P_MODEL[lang.lower()].value
    no_gen = False

    if len(g2p_model.split("!")) == 1:
        g2p_model = g2p_model
    elif g2p_model.split("!")[0].strip() == "NO_GEN":
        g2p_model = g2p_model.split("!")[1].strip()
        no_gen = True


    defaultmodel = lambda x:os.path.join("~","mfa_data",x)
    defaultfolder = os.path.join("~","mfa_data")
    # make the path
    os.makedirs(os.path.expanduser(defaultfolder), exist_ok=True)

    # if models are not downloaded (signified by the note)
    if not os.path.isfile(os.path.expanduser(defaultmodel(f".mfamodels21__{acoustic_model}"))):
        # create model manager
        manager = ModelManager()
        print("Downloading missing models...")
        # download english models
        if not no_gen:
            manager.download_model("g2p", g2p_model, True)
        manager.download_model("acoustic", acoustic_model, True)
        manager.download_model("dictionary", g2p_model, True)
        # create note file
        with open(os.path.expanduser(defaultmodel(f".mfamodels21__{acoustic_model}")), "w") as df:
            df.write("")

    # define model
    if not model:
        acoustic_model = acoustic_model
    else:
        acoustic_model = model # TODO

    # define dictionary path
    if not dictionary:
        dictionary = os.path.join(directory, 'dictionary.txt')

    # generate dictionary if needed
    if not no_gen and not os.path.exists(dictionary):
        # run mfa
        os.system(f"mfa g2p {directory} {g2p_model} {dictionary} --clean")
    if no_gen:
        dictionary = g2p_model

    # run alignment
    os.system(f"mfa align {directory} {dictionary} {acoustic_model} {data_dir} --beam {beam} --retry_beam {beam*4} --clean")
   
# Parse a TextGrid file for word tier
def parse_textgrid_long(file_path):
    """Parse a TextGrid file for the word tier

    Arguments:
        file_path (string): name of the TextGrid file

    Returns:
        a list of (((word, (start_time, end_time))... x number_words),
                   ((phoneme, (start_time, end_time))... x number_phones))
    """

    # Load files
    text_grid = TextGrid.fromFile(file_path)

    # Get tiers
    word = [i for i in text_grid if "words" in i.name]
    pho = [i for i in text_grid if "phones" in i.name]

    # Convert pho and word tiers
    word_tiers = [[(i.mark, (i.minTime, i.maxTime)) for i in j] for j in word]
    phone_tiers = [[(i.mark, (i.minTime, i.maxTime)) for i in j] for j in pho]

    # flatten word tiers and sort 
    word_tiers = sorted(sum(word_tiers, []), key=lambda x:x[1][0])
    phone_tiers = sorted(sum(phone_tiers, []), key=lambda x:x[1][0])

    # Skip all blanks (to reinsert spaces)
    word_tiers = list(filter(lambda x:x[0]!="", word_tiers))
    phone_tiers = list(filter(lambda x:x[0]!="", phone_tiers))

    # return the result
    return (word_tiers, phone_tiers)

def parse_textgrid_short(file_path):
    """Parse a TextGrid file for the word tier

    Arguments:
        file_path (string): name of the TextGrid file

    Returns:
        a list of ((word, (start_time, end_time))... x number_words)
    """

    # Open the file
    with open(file_path, "r") as df:
        content = df.readlines()

    # Clean up newlines and spaces
    content = [i.strip() for i in content]
    # Find the IntervalTier line
    enumerated_line = enumerate(content)
    enumerated_line = list(filter(lambda x:x[1]=='"IntervalTier"', enumerated_line))
    # The word tier is the second IntervalTier
    word_tier = enumerated_line[1][0]
    # We then cut the content to the word tier (skipping the top label and line count)
    content = content[word_tier+5:]
    # Then, we iterate through three-line chunks and append to the wordlist
    wordlist_alignments = []
    # For each of the wordlist
    for i in range(0, len(content), 3):
        # We skip any spaces
        if content[i+2] != '"sp"':
            # And append the words in paris
            wordlist_alignments.append((content[i+2][1:-1], (float(content[i]), float(content[i+1]))))

    return wordlist_alignments

# Align the alignments!
def transcript_word_alignment(elan, alignments, alignment_form="long", aggressive=False, debug=False):
    """Align the output of parse_transcript and parse_textgrid_* together

    Arguments:
        elan (string): path of elan files
        alignments (string): path of parse_alignments
        alignment_form (string): long or short (MFA or P2FA textGrid)
        aggressive (bool): whether or not to DP
        debug (bool): debug print mode

    Returns:
        {"alignments": ((start, end)...*num_lines), "terms": [bulleted results: string...*num_utterances], "raw": [(word/pause: (start, end)/None)]}
    """

    transcript = elan2transcript(elan)["transcript"]
    tiers = elan2transcript(elan)["tiers"]

    # Get alignment form
    if alignment_form == "long":
        aligned_words, _ = parse_textgrid_long(alignments)
    elif alignment_form == "short":
        aligned_words = parse_textgrid_short(alignments)
    else:
        raise Exception(f"unknown alignment_form {alignment_form}")

    # cleaned transcirpt
    unaligned_words = []
    unaligned_tiers = []
    sentence_starts = []
    sentence_ends = []

    # a lookup hashmap of cleaned_word: index in unaligned_words
    # used to quicken solving each subproblem in the DP below
    lookup_dict = defaultdict(list)
    
    # we perform cleaning and tokenization of the transcript
    # to create an unaligned word plate. This is used fro
    # matching against the aligned word

    # index becase we'll be doing a lot of shuffling
    i = 0

    # For each sentence
    for tier, current_sentence in zip(tiers, transcript):
        # remove extra delimiters
        current_sentence = re.sub(r"\$\w+", "", current_sentence).strip()
        current_sentence = current_sentence.replace("+","+ ")
        current_sentence = current_sentence.replace("$","$ ")
        current_sentence = current_sentence.replace("-","- ")
        current_sentence = current_sentence.replace("↫","↫ ")
        current_sentence = re.sub(r"&=\w+?:\w+", "", current_sentence).strip()
        current_sentence = re.sub(r"\[.*?\]", "", current_sentence).strip()
        current_sentence = re.sub(r'\"[^/]', "", current_sentence).strip()

        # split results
        splits = current_sentence.split(" ")

        # append boundaries
        sentence_starts.append(i)

        # we generate a cleaned and uncleaned match list
        for word in splits:
            cleaned_word = word.lower().replace("[/]","") # because of weird spacing inclusions
            cleaned_word = re.sub(r"&\*\w+: ?\w+", '', cleaned_word)
            cleaned_word = cleaned_word.replace("(","").replace(")","")
            cleaned_word = cleaned_word.replace("[","").replace("]","")
            cleaned_word = cleaned_word.replace("<","").replace(">","")
            cleaned_word = cleaned_word.replace("“","").replace("”","")
            cleaned_word = cleaned_word.replace(",","").replace("!","")
            cleaned_word = cleaned_word.replace("?","").replace(".","")
            cleaned_word = cleaned_word.replace("&=","").replace("&-","")
            cleaned_word = cleaned_word.replace("+","").replace("&","")
            cleaned_word = cleaned_word.replace(":","").replace("^","")
            cleaned_word = cleaned_word.replace("$","").replace("\"","")
            cleaned_word = cleaned_word.replace("&*","").replace("∬","")
            cleaned_word = cleaned_word.replace("-","").replace("≠","")
            cleaned_word = cleaned_word.replace(":","").replace("↑","")
            cleaned_word = cleaned_word.replace("↓","").replace("↑","")
            cleaned_word = cleaned_word.replace("⇗","").replace("↗","")
            cleaned_word = cleaned_word.replace("→","").replace("↘","")
            cleaned_word = cleaned_word.replace("⇘","").replace("∞","")
            cleaned_word = cleaned_word.replace("≋","").replace("≡","")
            cleaned_word = cleaned_word.replace("∙","").replace("⌈","")
            cleaned_word = cleaned_word.replace("⌉","").replace("⌊","")
            cleaned_word = cleaned_word.replace("⌋","").replace("∆","")
            cleaned_word = cleaned_word.replace("∇","").replace("*","")
            cleaned_word = cleaned_word.replace("??","").replace("°","")
            cleaned_word = cleaned_word.replace("◉","").replace("▁","")
            cleaned_word = cleaned_word.replace("▔","").replace("☺","")
            cleaned_word = cleaned_word.replace("♋","").replace("Ϋ","")
            cleaned_word = cleaned_word.replace("∲","").replace("§","")
            cleaned_word = cleaned_word.replace("∾","").replace("↻","")
            cleaned_word = cleaned_word.replace("Ἡ","").replace("„","")
            cleaned_word = cleaned_word.replace("‡","").replace("ạ","")
            cleaned_word = cleaned_word.replace("ʰ","").replace("ā","")
            cleaned_word = cleaned_word.replace("ʔ","").replace("ʕ","")
            cleaned_word = cleaned_word.replace("š","").replace("ˈ","")
            cleaned_word = cleaned_word.replace("ˌ","").replace("‹","")
            cleaned_word = cleaned_word.replace("›","").replace("〔","")
            cleaned_word = cleaned_word.replace("〕","").replace("//","")
            cleaned_word = re.sub(r"@.", '', cleaned_word)
            cleaned_word = re.sub(r"&.", '', cleaned_word)
            # cleaned_word = re.sub(r"↫(.*)↫", r'', cleaned_word)
            # cleaned_word = re.sub(r"↫", r'', cleaned_word)
            cleaned_word = cleaned_word.strip()

            if debug:
                print(word, cleaned_word)

            # append the cleaned and uncleaned versions of words 
            unaligned_words.append((word, cleaned_word, i))
            lookup_dict[cleaned_word].append(i)
            unaligned_tiers.append(tier)
            i += 1

        # append boundaries
        sentence_ends.append(i)

    # freeze the lookup dictionary
    lookup_dict = dict(lookup_dict)

    # conform boundaries
    sentence_boundaries = list([list(range(i,j)) for i,j in zip(sentence_starts, sentence_ends)])

    # serialize aligned words and backplate
    align_payload = [PayloadTarget(key=i[0].lower(), payload=i[1])
                     for i in aligned_words]
    align_reference = [ReferenceTarget(key=i[1].lower(), payload=i[0])
                       for i in unaligned_words]

    # align!
    alignment = align(align_payload, align_reference) 

    # generate backplated alignments
    backplated_alignments = []

    # for each result, we throw away extra things in the FA, and include
    # only the elements with a match or in the reference (extra FA words
    # we ignore)
    for result in alignment:
        if type(result) == Match:
            backplated_alignments.append((result.reference_payload, result.payload))
        elif type(result) == Extra and result.extra_type == ExtraType.REFERENCE:
            backplated_alignments.append((result.payload, None))

    # tack on numerical ID
    backplated_alignments = [(i,a,b) for i,(a,b) in enumerate(backplated_alignments)]

    # sort and reincorporate backplated alignments
    # backplated_alignments = sorted(backplated_alignments+to_reinsert, key=lambda i:i[0])

    # set the begin
    j = 0

    # for each tier, perform timestamp correction to prevent self-overlap talk
    # which MFA sometimes generates, as well as fix the issues with sudden unbulleted
    # words in the input

    # check if we need backtracking
    # (i.e. "we corrected an error, backpropegate")
    backtracking = False

    # begin a loop to scan and correct errors
    while j < len(backplated_alignments):
    # while False:

        # get the results 
        indx, word, interval = backplated_alignments[j]
        tier = unaligned_tiers[indx]

        if debug:
            print(indx, word, interval)

        # if we are at an unalignable word, we move on
        # if we are backtracking, skip over anything that's
        # not part of the tier to be backtracked
        if not interval and not backtracking:
            j += 1
            continue
        elif backtracking and ((not interval) or (tier != backtracking)) and j!= 0:
            j -= 1 
            continue
        # if we are backtracking, and we can't do anything
        elif backtracking and ((not interval) or (tier != backtracking)) and j == 0:
            j += 1
            backtracking = False
            continue

        # reset backtracking
        backtracking = False

        # if there is an interval, we unpack it and ensure
        # that a few things is the case
        start,end = interval

        # get the first remaining indexable elements
        rem = None
        remword = None
        i = indx+1

        # go through in a while loop until we found
        # the next indexable element
        while i<len(backplated_alignments) and ((not rem) or (unaligned_tiers[i] != tier)):
            rem = backplated_alignments[i][2]
            remword = backplated_alignments[i]
            i += 1

        # if end is after the next starting, set the end
        # to be the next starting
        if rem and end > rem[0]:
            if debug:
                print("BACK1", end, remword)
            end = rem[0]
            backtracking = tier # backprop to correct prev. errors

        # if end is smaller than start, conform start to be
        # a bit before end
        if start > end:
            if debug:
                print("BACK2", start, end)
            start = max(end, 0.0001)-0.0001
            backtracking = tier # backprop to correct prev. errors

        # if we are unalignable, give up. Otherwise, don't give up
        if abs(start-end) < 0.001:
            backplated_alignments[indx] = (indx, word, None)
        else:
            backplated_alignments[indx] = (indx, word, (start, end))

        # if don't need backprop, continue
        if backtracking:
            j -= 1
        # if we do need backtracking
        else:
            j += 1 

    # conform the sentences 
    backplated_alignments_sentences = [[backplated_alignments[j] for j in i] for i in sentence_boundaries]

    # generate final results
    results = [[(i[1], i[2]) for i in j] for j in backplated_alignments_sentences]

    # extract alignments
    results_filtered = [list(filter(lambda x:x[1], i)) for i in results]
    # if there is nothing to align, just produce (0,0)
    alignments = [(i[0][1][0], i[-1][1][1]) if len(i) > 0 else (0,0) for i in results_filtered]

    # recreate results
    bulleted_results = []
    # Convert bulleted results
    for sentence in results:
        # bullet the sentence
        sentence_bulleted = []
        # set indx = 0
        indx = 0
        # for each term
        while indx < len(sentence):
            # load the value
            i = sentence[indx]
            if i[1]:
                # appending the current word and chorresponding bullet
                sentence_bulleted.append(clean_codes(i[0]) + bullet(i[1][0], i[1][1]))
            else:
                # remove, conservatively, all codes
                # if it has something
                res = clean_codes(i[0])
                if res != "":
                    sentence_bulleted.append(res)
            # increment
            indx += 1

        # replace sentence
        sentence = " ".join(sentence_bulleted).strip()

        # while len(sentence.strip()) > 0 and sentence.strip()[0] == "+":
            # setence = sentence[1:]

        ### Final Santy Checks and Shape Conformations ###
        # if you can insert proper logic, do so. If not, use
        # this area as the last-ditch preprocessing step to
        # make spontaneous final transcript adjustments happen
        
        # remove extra delimiters, as a final sanity check
        sentence = re.sub(r"^(\+ ?)+(.*)", r"\2", sentence)
        sentence = sentence.replace("+ ","+")
        sentence = sentence.replace("_ ","_")
        sentence = sentence.replace("- ","-")
        sentence = sentence.replace(" >",">")
        sentence = sentence.replace("< ","<")
        sentence = sentence.replace("$ ","$")
        sentence = sentence.replace("& ","&")
        sentence = sentence.replace("↫ ","↫")
        # however, double < should have a space between
        sentence = sentence.replace("<<","< <")
        sentence = sentence.replace("+<","")
        sentence = sentence.replace("[<]"," [<] ")
        sentence = sentence.replace("  "," ") # remove double spcaes

        sentence = re.sub(r"^\+", "", sentence)


        # concat and append to bulleted results
        bulleted_results.append(sentence.strip())


    # Return the fimal alignments
    return {"alignments": alignments, "terms": bulleted_results,
            "raw": results, "tiers": tiers}

def do_align(in_dir, out_dir, data_directory="data", model=None, dictionary=None, beam=10, prealigned=False, clean=True, align=True, aggressive=False, lang="en"):
    """Align a whole directory of .cha files

    Attributes:
        in_directory (string): the directory containing .cha and .wav/.mp3 file
        out_directory (string): the directory for the output files
        data_directory (string): the subdirectory (rel. to out_directory) which the misc.
                                 outputs go
        [model (str)]: the model to use
        [dictionary (str)]: dictionary to use
        [beam (int)]: beam width for initial MFA alignment
        [prealigned (bool)]: whether to start with preexisting alignments from the originial CHA files
        [clean (bool)]: whether to clean up, used for debugging
        [align (bool)]: whether to actually align, used for debugging
        [aggressive (bool)]: whether to use DP
        [lang (str)]: language of the sample

    Returns:
        none
    """

    # Define the data_dir
    DATA_DIR = os.path.join(out_dir, data_directory)

    # Make the data directory if needed
    if not os.path.exists(DATA_DIR):
        os.mkdir(DATA_DIR)

    # is_video flag for CHAT generation
    is_video = False

    ### PREPATORY OPS ###
    # convert all mp3s to wavs
    wavs = globase(in_dir, "*.wav")
    # if there are no wavs, convert
    if len(wavs) == 0 and len(globase(in_dir, "*.mp4")) > 0:
        mp42wav(in_dir)
        # indeed, is video!
        is_video = True
    elif len(wavs) == 0: 
        mp32wav(in_dir)

    # Generate elan elan elan elan
    chat2elan(in_dir)

    # generate utterance and word-level alignments
    elans = globase(in_dir, "*.eaf")

    if prealigned:
        # convert all chats to TextGrids
        chat2praat(in_dir)
    else:
        # convert all chats to transcripts
        chat2transcript(in_dir)

    # NOTE FOR FUTURE EDITORS
    # TextGrid != textGrid. P2FA and MFA generate different results
    # in terms of capitalization.

    # if we are to align
    if align:
        # Align the files
        align_directory_mfa(in_dir, DATA_DIR, beam=beam, model=model, dictionary=dictionary, lang=lang)

    # find textgrid files
    alignments = globase(DATA_DIR, "*.TextGrid")

    # store aligned files
    aligned_results = []

    # zip the results and dump into new eafs
    print("Done with MFA! Generating final word-level alignments and bullets...")
    for alignment in tqdm(sorted(alignments)):
        # Find the relative elan file
        elan = repath_file(alignment, in_dir).replace("TextGrid", "eaf").replace("textGrid", "eaf")
        # Align the alignment results
        # MFA TextGrids are long form
        print(f"Processing {alignment.replace('.TextGrid', '')}...")
        aligned_result = transcript_word_alignment(elan, alignment, alignment_form="long", aggressive=aggressive)

        aligned_results.append((alignment.replace('.TextGrid', ''), aligned_result))

        # Calculate the path to the old and new eaf
        old_eaf_path = os.path.join(in_dir,
                                    pathlib.Path(elan).name)
        new_eaf_path = os.path.join(out_dir,
                                    pathlib.Path(elan).name)
        # Dump the aligned result into the new eaf
        eafinject(old_eaf_path, new_eaf_path, aligned_result)
        # convert the aligned eafs back into chat
        elan2chat__single(new_eaf_path, is_video)

    ### CLEANUP OPS ###

    if clean:
        cleanup(in_dir, out_dir, data_directory)

    return aligned_results

