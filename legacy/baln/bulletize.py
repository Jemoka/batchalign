"""
bulletize.py
To make bullets.... WHat more can you want?
"""

from .utils import *
from .retokenize import *
from .eaf import *
from .dp import align, PayloadTarget, ReferenceTarget, Match, ExtraType

import nltk
from nltk import word_tokenize

from collections import defaultdict

from pathlib import Path

def bulletize_file(audio, chat, elan, out_dir, lang="en", is_video=False, **kwargs):
    # calculate the output path
    elan_out = repath_file(elan, out_dir)

    # read the lines
    # open label file
    with open(chat, 'r') as df:
        # so, for some reason, .lab outputs some lines
        # truncated with a \t in the begining. so we
        # replace out the \n\t with just a space
        data = df.read()
        labels = data.replace('\n\t',' ').strip().split('\n')
        labels = [i.replace(".", "").replace(",", "").replace("!", "").replace("?", "")
                for i in labels]
        labels = [i.split("\t")[-1].strip() for i in filter(lambda x:x[0] == "*", labels)]

    # split into utterances
    split_utterances = []
    for utterance in labels:
        try: 
            split_passage = word_tokenize(utterance)
        except LookupError:
            # we are missing punkt
            nltk.download('punkt')
            # perform tokenization
            split_passage = word_tokenize(utterance)

        split_utterances.append(split_passage)

    # store ASR output by performing ASR using the indicated method
    # def tmp:
    infile = audio

    key = kwargs.get("key")
    # find key; if non-existant, ask for it.
    if key == None:
        # find keyfile
        with open(os.path.expanduser(defaultmodel("rev_key")), "a+") as df:
            # seek beginning
            df.seek(0,0)
            # get contents
            cont = df.read()
            # if no content, prompt
            if cont == "":
                cont = input("Enter your Rev.AI API key: ")
                df.write(cont.strip())
            # anyways, its just the keyfile
            key = cont.strip()

    asr = None

    # if its json, use the Rev json processor
    if pathlib.Path(infile).suffix == ".json":
        asr = asr__rev_json(infile)
    # if its a .wav file, use the .wav processor
    elif pathlib.Path(infile).suffix == ".wav":
        asr = asr__rev_wav(infile, key=key, lang=lang)


    # get all the words
    timed_words = []

    for speaker in asr["monologues"]:
        for word in speaker["elements"]:
            if word["type"] == "text":
                timed_words.append(word)

    # and sort it for alignment
    timed_words = list(sorted(timed_words, key=lambda x:x.get("ts", 0)))

    # We finally, then take those two things and create alignment plates
    references = []
    payloads = []

    for indx, utterance in enumerate(split_utterances):
        for word in utterance:
            references.append(ReferenceTarget(word.strip(), indx))
    for word in timed_words:
        payloads.append(PayloadTarget(word["value"], (word["ts"], word["end_ts"])))

    # AND DP!!!
    alignments = align(payloads, references)

    # for each match, we append it to the list for the correct utterances
    dd = defaultdict(list)
    for i in alignments:
        if type(i) == Match:
            dd[i.reference_payload].append(i.payload)
        elif i.extra_type == ExtraType.REFERENCE:
            dd[i.payload].append(None)
    dd = [i[1] for i in sorted(list(dict(dd).items()), key=lambda x:x[0])]

    # and compute the most likely timescodes for each utterance
    likely_timecodes = []
    last_known_timecode = 0

    # we keep track of this variable called "last_known_timecode"
    # as a "default" timecode we put when we have NO idea what to put
    # for the timecode. It should be kept to the "last" timecode
    # that we have read; i.e. we will then be able to start the next utterance
    # there at a minimum.
    for i, utterance in enumerate(dd):
        start = last_known_timecode
        end = None

        # if we have an immediate start, we can confidently say
        # that that is the real start of the utterance
        if utterance[0]:
            start = utterance[0][0]
            last_known_timecode = start

        # otherwise, we can just leave start as the last_known_timecode
        # which is just when the previous utterance ended

        # we now try to figure out the ending
        # first let's hope there's just an end 
        if utterance[-1]:
            end = utterance[-1][1]
            last_known_timecode = end
        else:
            # we first set the last known timecode to the
            # last possible word 
            filtered_words = list(filter(lambda x:x, utterance))
            if len(filtered_words) > 0:
                last_known_timecode = filtered_words[-1][1]

            # if there is no end, we search forward to get a
            # "temp" timecode for this one.
            cursor = i+1
            while not end and cursor < len(dd):
                filtered_words = list(filter(lambda x:x, dd[cursor]))
                if len(filtered_words) > 0:
                    end = filtered_words[0][0]
                    break
                cursor += 1

            # if we STILL have nothing, we set it to the last
            # known timecode + *ARBITURARY* 5 seconds to "guess"
            # what the possible time is
            if not end:
                last_known_timecode += 5
                end = last_known_timecode

        # either way, we append
        likely_timecodes.append((start, end))

    eafinject(elan, elan_out, bullets=likely_timecodes)
    elan2chat__single(elan_out, is_video)

def bulletize_path(in_dir, out_dir, data_dir="data", lang="en",
                   clean=True, aggressive=None, **kwargs):
    """Bulletize the an input file

    Parameters
    ----------
    in_dir : str
        the input directory to bulletize
    out_dir : str
        the output directory to bulletize
    data_dir : str
        where the data files should be sweeped to
    lang : str
        the language to process
    clean : str
        whether to sweep files
    aggressive : None
        Legacy useless option. Pass None. Used for CLI stability.

    kwargs: key (rev ai key)
    """
    
    # Define the data_dir
    DATA_DIR = os.path.join(out_dir, data_dir)

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
    chat2elan(in_dir, True)

    # generate utterance and word-level alignments
    elans = globase(in_dir, "*.eaf")

    # and run the thing
    for file in elans:
        print("Processing", Path(file).stem)

        bulletize_file(file.replace("eaf", "wav"),
                       file.replace("eaf", "cha"),
                       file, out_dir, lang,
                       is_video,
                       **kwargs)

    if clean:
        cleanup(in_dir, out_dir, data_dir)

    print("Rebulletize Done!")
    print("NOTE: the @Media tier is not nessesarily set as the file wasn't")
    print("linked before. You should go and check @Media tier now.")

# audio = "../../talkbank-alignment/bulletize/input/per60.wav"
# chat = "../../talkbank-alignment/bulletize/input/per60.cha"
# elan = "../../talkbank-alignment/bulletize/input/per60.eaf"

# in_dir = "../../talkbank-alignment/bulletize/input"
# out_dir = "../../talkbank-alignment/bulletize/output"

