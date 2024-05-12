""""
tokenize.py
Parses output from AWS ASR for the front of the pipeline
"""

# import uuid
from posixpath import isfile
import uuid
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
# default dictionaries
from collections import defaultdict
# utility string punctuations
import string
# import temporary file
import tempfile
# tarball handler
import tarfile
from urllib import request

# json utilities
import json

# import time
import time

# import nltk
from nltk import sent_tokenize
# handwavy tokenization
from nltk.tokenize import word_tokenize
from nltk import RegexpParser, pos_tag

# progress bar
from tqdm import tqdm

# ui tools
from tkinter import *
from tkinter import ttk
from tkinter.scrolledtext import ScrolledText

# import the tokenization engine
from .utokengine import UtteranceEngine
from .utils import fix_transcript

# enums for provider
from enum import Enum

class ASRProvider(Enum):
    REV = 1
    WHISPER = 2

MODEL_PATH="https://dl.dropboxusercontent.com/s/4qhixi742955p35/model.tar.gz?dl=0"
MODEL="flowing-salad-6"

# silence huggingface
os.environ["TOKENIZERS_PARALLELISM"] = "FALSE"

# Oneliner of directory-based glob and replace
globase = lambda path, statement: glob.glob(os.path.join(path, statement))

# default model and directory paths
defaultmodel = lambda x:os.path.join("~","mfa_data",x)
defaultfolder = os.path.join("~","mfa_data")

# get text GUI
def interactive_edit(name, string):
    # create a new UI
    root = Tk(screenName=f"Interactive Fix")
    root.title(f"Interactively Fixing {name}")
    # create the top frame
    top = ttk.Frame(root, padding=5)
    top.grid()
    # label
    ttk.Label(top, text=f"Fixing {name}  ", font='Helvetica 14 bold').grid(column=0, row=0)
    ttk.Label(top, text="Use *** to seperate speakers, newlines to seperate utterances.").grid(column=1, row=0)
    # insert a textbox and place it
    text_box = ScrolledText(root, font='TkFixedFont 16', height=40, width=100, borderwidth=20, highlightthickness=0)
    text_box.grid(column=0, row=1)
    # and insert our fixit string
    def settext():
        text_box.delete(1.0, "end")
        text_box.insert("end", string)
    # create a finishing function
    settext()
    # create the bottom frame
    bottom = ttk.Frame(root, padding=5)
    bottom.grid()
    # don't stop until we do
    # this is a CHEAP MAINLOOP, but its used
    # to control when the text element can
    # be destroyed (i.e. not before reading)
    def stopit():
        # hide
        root.withdraw()
        # leave!
        root.quit()
    # save the current contents to a file
    def savepoint(auto=False):
        # draft text
        text = text_box.get("1.0","end-1c")
        # basepath
        basepath = os.path.join("~", ".ba-checkpoints")
        # make the checkpoints directory
        pathlib.Path(os.path.expanduser(basepath)).mkdir(parents=True, exist_ok=True)
        # save
        with open(os.path.expanduser(os.path.join(basepath, f"{f'{name}_{uuid.uuid4()}.auto' if auto else 'm'}.cut")), 'w') as df:
            df.write(text.strip())
    # read the current contents from a file
    def readpoint():
        # make the checkpoints directory
        pathlib.Path(os.path.expanduser(basepath)).mkdir(parents=True, exist_ok=True)
        # save
        with open(os.path.expanduser(os.path.join(basepath, "m.cut")), 'w+') as df:
            text = df.read().strip()
        text_box.delete(1.0, "end")
        text_box.insert("end", text)
    # create the buttons
    ttk.Button(bottom, text="Reset", command=settext).grid(column=0, row=0,
                                                           padx=20, pady=5)
    ttk.Button(bottom, text="Restore Checkpoint", command=readpoint).grid(column=1, row=0,
                                                                       padx=20, pady=5)
    ttk.Button(bottom, text="Save Checkpoint", command=savepoint).grid(column=2, row=0,
                                                                    padx=20, pady=5)
    ttk.Button(bottom, text="Submit", command=stopit).grid(column=3, row=0,
                                                           padx=20, pady=5)
    # mainloop
    root.mainloop()
    savepoint(True)
    final_text = text_box.get("1.0","end-1c")
    root.destroy()

    # return the new contents of the textbox
    return final_text.strip()

# process a file
corpus = None

# process asr output jsons 
def process_asr_output(data, name=None, interactive=False, noprompt=False):
    global corpus
    
    """Process JSON Data

    Attributes:
        data (dict): JSON data in the Rev.AI spec
        [name] (str): name of the audio, to add to @Media tier
        [interactive] (bool): whether to support interactive fixes
        [noprompt] (bool): whether or not to prompt nothing
    """

    # create array for total collected utterance
    utterance_col = []

    # for each utterance
    for utterance in data["monologues"]:
        # get a list of words
        words = utterance["elements"]
        # coallate words (not punct) into the shape we expect
        # which is ['word', [start_ms, end_ms]]. Yes, this would
        # involve multiplying by 1000 to s => ms
        words = [[i["value"], [round(i["ts"]*1000),
                                round(i["end_ts"]*1000)]] # the shape
                for i in words # for each word
                    if i["type"] == "text" and
                    not re.match(r'<.*>', i["value"])] # if its text
        # final words
        final_words = []
        # go through the words, if there is a space, split time in n parts
        for word, (i,o) in words:
            # split the word
            word_parts = word.split(" ")
            # if we only have one part, we don't interpolate
            if len(word_parts) == 1:
                final_words.append([word, [i,o]])
                continue
            # otherwise, we interpolate the itme
            cur = i
            div = (o-i)//len(word_parts)
            # for each part, add the start and end
            for part in word_parts:
                final_words.append([part.strip(), [cur, cur+div]])
                cur += div
        # if the final words is > 300, split into n parts
        if len(final_words) > 300:
            # for each group, append
            for i in range(0, len(final_words), 300):
                # concatenate with speaker tier and append to final collection
                # not unless the crop is empty
                if len(final_words[i:i+300]) > 0:
                    utterance_col.append((utterance["speaker"], final_words[i:i+300]))
        else:
            # concatenate with speaker tier and append to final collection
            if len(final_words) > 0:
                utterance_col.append((utterance["speaker"], final_words))

    # get a list of speaker IDs
    speaker_ids = {i[0] for i in utterance_col} # this is a set!
    speakers = {}

    # generate the speaker information, or
    # otherwise fill it out

    if noprompt:
        # generate info
        for speaker in speaker_ids:
            # add to list
            speakers[speaker] = {"name": f"Participant", "tier": f"PAR{speaker}"}
        # corpus name
        corpus = "corpus_name"
    else:
        speakers_filtered = []
        # filter 2 statements for each speaker needed
        # and then push
        for speaker in speaker_ids:
            speakers_filtered.append(sorted(list(filter(lambda x:x[0]==speaker, utterance_col)), key=lambda x:len(x[1]))[-2:])

        # prompt hello
        print("Welcome to interactive speaker identification!")
        print(f"You are working with sample {name}.\n")

        # print out samples from speaker
        for indx, speaker in enumerate(speaker_ids):
            print(f"\033[1mSpeaker {speaker}\033[0m")
            try: 
                print("\n".join(["start: %02d:%02d; "%divmod(i[1][0][1][0]//1000, 60)+" ".join([j[0] for j in i[1]]) for i in speakers_filtered[indx]]))
            except:
                breakpoint()
            print()

        # prompt for info
        for speaker in speaker_ids:
            # get info and handle error correction
            # get speaker info
            speaker_name = input(f"Please enter the role of speaker {speaker} (i.e. Participant): ").strip()
            # if we don't have first capital and spelt correctly
            while not (speaker_name.strip() != "" and speaker_name[0].isupper() and "*" not in speaker_name):
                print("Invalid response. Please follow the formatting example provided.")
                speaker_name = input(f"Please enter the role of speaker {speaker} (i.e. Participant): ").strip()
            # get tier info
            speaker_tier = input(f"Please enter the 3-letter ID tier code of speaker {speaker} (i.e. PAR): ")
            # if we don't have first capital and spelt correctly
            while not (speaker_name != "" and speaker_tier.isupper() and "*" not in speaker_name):
                print("Invalid response. Please follow the formatting example provided.")
                speaker_tier = input(f"Please enter the 3-letter ID tier code of speaker {speaker} (i.e. PAR): ").strip()
            # add to list
            speakers[speaker] = {"name": speaker_name, "tier": speaker_tier}
            print()
        # Name of the corpus
        if not corpus:
            corpus = input("Corpus name not found; please enter corpus name: ")

    # go through the list and reshape the main tier
    utterance_col = [['*'+speakers[i[0]]["tier"]+":",
                    i[1]] for i in utterance_col]

    # create the @Participants tier
    participants_tier = ", ".join(set([v["tier"]+f" {v['name']}"
                                       for k,v in speakers.items()]))
    participants_tier = ["@Participants:", participants_tier]

    # create the @ID tiers and @ID tier string
    id_tiers = set([f"eng|{corpus}|{v['tier']}|||||{v['name']}|||" for k,v in speakers.items()])
    id_tiers = [["@ID:",i] for i in id_tiers]

    # finally, create media tier
    media_tier = ["@Media:", f"{name}, audio"]

    # assemble final header and footer
    header = [["@Begin"],
                ["@Languages:", "eng"],
                participants_tier,
                *id_tiers,
                media_tier]
    footer = [["@End"]]

    # return result
    return header, utterance_col, footer

# global realignment function (for rev.ai)
def asr__rev_json(f):
    """Process a RevAI text file as input

    Attributes:
        f (str): file to process
    """

    # open the file and read its lines
    with open(f, 'r') as df:
        data = json.load(df)

    # process and return
    return data

# sent an audio file to ASR, and then process the resulting JSON
def asr__rev_wav(f, key=None, lang="en", num_speakers=None):
    """Perform ASR on an .wav file and return Rev.AI JSON

    Arguments:
    f (str): the .wav file path to process
    key (str): the Rev.AI key
    lang (str): language
    """
    # late import for backwards capatibility
    from rev_ai import apiclient, JobStatus

    if lang == "zh":
        lang = "cmn"

    POSTPROCESSOR_LANGS = ['en', 'es', 'fr', 'en/es']

    # create client
    client = apiclient.RevAiAPIClient(key)

    print(f"Uploading '{pathlib.Path(f).stem}'...")
    # we will send the file for processing
    job = client.submit_job_local_file(f,
                                       metadata=f"batchalign_{pathlib.Path(f).stem}",
                                       language=lang,
                                       skip_postprocessing=True if lang in POSTPROCESSOR_LANGS else False,
                                       speakers_count=num_speakers)

    # we will wait
    status = client.get_job_details(job.id).status

    # we will wait until it finishes
    print(f"Rev.AI is transcribing '{pathlib.Path(f).stem}'...")

    # check status
    while status == JobStatus.IN_PROGRESS:
        # sleep
        time.sleep(30)
        # check again
        status = client.get_job_details(job.id).status

    # if we failed, report failure and give up
    if status == JobStatus.FAILED:
        # get error
        err = client.get_job_details(job.id).failure_detail
        # print error
        raise Exception(f"Job '{pathlib.Path(f).stem}' failed! {err}.")

    # and now, we extract result
    transcript_json = client.get_transcript_json(job.id)

    # finally, we run processing and return the resutls
    return transcript_json

# global realignment function
def retokenize(infile, outfile, utterance_engine, interactive=False, provider=ASRProvider.REV, lang="en", noprompt=False, **kwargs):
    """Function to retokenize an entire chat file

    Attributes:
        infile (str): in .cha or json file
        outfile (str): out, retokenized out file
        utterance_engine (UtteranceEngine): trained utterance engine instance
        provider (ASRProvider): which asr provider to use
        [lang] (str): language
        [interactive] (bool): whether to enable midway user fixes to label data/train
        [noprompt] (bool): whether or not to provide no prompt
        **kwargs: keyword parameters 

    Used for output side effects
    """
    # store ASR output by performing ASR using the indicated method
    asr = None

    if provider == ASRProvider.REV:
        key = kwargs.get("key")
        num_speakers = kwargs.get("speakers")
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

        # if its json, use the Rev json processor
        if pathlib.Path(infile).suffix == ".json":
            asr = asr__rev_json(infile)
        # if its a .wav file, use the .wav processor
        elif pathlib.Path(infile).suffix == ".wav":
            asr = asr__rev_wav(infile, key=key, lang=lang, num_speakers=num_speakers)
    elif provider == ASRProvider.WHISPER:
        whisper = kwargs["whisper"]
        audio, segments = whisper.load(infile, kwargs["speakers"])
        asr = whisper(audio.all(), segments)

    # then, process the ASR
    header, main, closing = process_asr_output(asr, pathlib.Path(infile).stem, noprompt=noprompt)
        
    # chunk the data with our model
    chunked_passages = []

    # for each line, create chunks
    for line in tqdm(main):
        # if the speaker line contains nothing, don't do anything
        if len(line[1]) == 0:
            continue
        # extract words and bullets
        words, bullets = zip(*line[1])
        # build a giant string for passage
        passage = " ".join(words)
        # we will now use UtteranceEngine to redo
        # utterance tokenization
        # chunk the passage into utterances
        if lang == "en":
            chunked_passage = utterance_engine(passage)
        else:
            chunked_passage = [i.replace("...", ".") for i in sent_tokenize(passage)]
        # remove the end delimiters (again!) because
        # all we case about here are the split utterances
        # we will add "." later
        chunked_passage = (line[0], [re.sub(r'([.?!])', r'', i)
                             for i in chunked_passage])
        # append it to the chunked_passages
        chunked_passages.append(chunked_passage)

    # if we need to the interactive fixit program,
    # run the routine
    if interactive and (interactive != 'h'):
        # create the output string
        fixit_string = "\n\n***\n".join([(i[0]+"\n\n")+ "\n\n".join(i[1]) 
                                        for i in chunked_passages])

        # let the user edit the fixit string
        edited_text = [(i.split("\n\n")[0], [j.strip() for j in i.split("\n\n")[1:]])
                   for i in interactive_edit(pathlib.Path(infile).stem,
                                             fixit_string).split("\n\n***\n")]

        # set the chunked passage back
        chunked_passages = edited_text

    # collect retokenized lines
    realigned_lines = []
    
    # calculate the bullets
    bullets = [j[1] for i in main for j in i[1]]
    words = [j[0] for i in main for j in i[1]]
    bullet_tally = 0

    # for each line, perform analysis and append
    for speaker, chunked_passage in chunked_passages:
        # check if we have no content, if so, give up
        if len(chunked_passage) == 0:
            continue

        # split the passage align to realign with the bullets
        # that was previously provided. we do a len() here
        # because all we really need is the index at which
        # the first and last bullet of the line exists
        chunked_passage_split = [len(i.split(' ')) for i in chunked_passage]
        # tally up to create zipped result
        chunked_passage_split = [bullet_tally]+[sum(chunked_passage_split[:i+1])+bullet_tally
                                for i in range(len(chunked_passage_split))]
        # set the new index of the bullet
        bullet_tally = chunked_passage_split[-1]
        # and now, pair one-off-shift groups: this is the IDs of the start
        # and stop times we look for each utterance's new bullet
        shifts = list(zip(chunked_passage_split, chunked_passage_split[1:]))

        # finally, lookup the actual bullet values
        new_bullets = [[bullets[i[0]][0], bullets[min(i[1], len(bullets)-1)-1][1]] for i in shifts]

        # we will stringify it into new bullets
        new_bullets_str = [f'{i[0]}_{i[1]}' for i in new_bullets]

        # and finally, we can construct the final line
        # speaker, tab, sentence, end of sentence mark, end bullet
        realigned_lines = realigned_lines + [speaker+'\t'+i[0]+' . '+i[1]
                                            for i in zip(chunked_passage, new_bullets_str)]

    # realign header and closing
    realigned_header = ['\t'.join(i).strip() for i in header]
    realigned_closing = ['\t'.join(i).strip() for i in closing]

    # new chat file
    new_chat = ["@UTF8"]+realigned_header+realigned_lines+realigned_closing

    # and finally, synthesize
    with open(outfile, 'w') as df:
        # write!
        df.writelines([i+'\n' for i in new_chat])

    # and fix any errors
    # for most cases, don't use locase
    # TODO until more lowcase files are avilable
    if lang != "en":
        fix_transcript(outfile, None)
    else:
        fix_transcript(outfile)

    return asr, header, main, closing


def retokenize_directory(in_directory, model_path=os.path.join("~","mfa_data","model"), interactive=False, lang="en", noprompt=False, **kwargs):
    """Retokenize the directory, or read Rev.ai JSON files and generate .cha

    Attributes:
        in_directory (str): input directory containing files
        model_path (str): path to a BertTokenizer and BertModelForTokenClassification
                          trained on the segmentation task.
        [interactive] (bool): whether to run the interactive routine
        [lang] (str): language
        [noprompt] (bool): whether or not to provide no prompt

    Returns:
        None, used for .cha file generation side effects.
    """

    # check if model exists. If not, download it if needed.
    if not model_path and not os.path.exists(os.path.expanduser(defaultmodel("model"))):
        print("Getting segmentation model...")
        # make the path
        os.makedirs(os.path.expanduser(defaultfolder), exist_ok=True)
        # download the tarball
        model_data = request.urlopen(MODEL_PATH)
        # put it down
        model_zip = os.path.expanduser(defaultmodel("model.tar.gz"))
        with open(model_zip, 'wb') as df:
            df.write(model_data.read())
        # open it
        tar = tarfile.open(model_zip, "r:gz")
        tar.extractall(os.path.expanduser(defaultfolder))
        tar.close()
        # and then rename it
        basepath = os.path.expanduser(defaultfolder)
        os.rename(os.path.join(basepath, MODEL), os.path.join(basepath, "model"))

    # if a None was explicitly passed in
    if not model_path:
        model_path = os.path.join("~","mfa_data","model")

    # find all the JSON files
    files = globase(in_directory, "*.json")
    # if we can't find any JSON files, we have to go run
    # ASR
    if len(files) == 0:
        # scan for wav files
        files = globase(in_directory, "*.wav")
        # if we don't have wav files, find mp3/4 and convert
        if len(files) == 0:
            # convert again
            files = globase(in_directory, "*.mp3")
            # if we don't have mp3, its prox mp4
            if len(files) == 0:
                files = globase(in_directory, "*.mp4")
            # convert!
            for f in files:
                # find the files
                suf = pathlib.Path(f).suffix
                CMD = f"ffmpeg -i {f} {f.replace(suf, '.wav')} -c copy"
                # run!
                os.system(CMD)
            # convert again
            files = globase(in_directory, "*.wav")
    # so, we will convert the input directory to
    # seed an utterance engine to use
    E = UtteranceEngine(os.path.expanduser(model_path))

    if kwargs["whisper"]:
        language = ""
        model = None
        if lang == "en":
            language = "english"
            model = "talkbank/CHATWhisper-en-large-v1"
        elif lang == "es":
            language = "spanish"
        elif lang == "it":
            language = "italian"
        elif lang == "de":
            language = "german"
        elif lang == "pt":
            language = "portuguese"
        elif lang == "fr":
            language = "french"
        elif lang == "ca":
            language = "catalan"
        elif lang == "nl":
            language = "dutch"
        else:
            raise ValueError(f"Batchalign does not recognize the language code you provided; however, there's a good chance that it just hasn't been added to be recogonized and is actually supported by Whisper. Please reach out. Language code supplied: {lang}")

        if model == None:
            model = "openai/whisper-large-v2"

        from .asrengine import ASREngine
        engine = ASREngine(model, language=language)

        kwargs["whisper"] = engine
        kwargs["provider"] = ASRProvider.WHISPER

        if not kwargs.get("speakers"):
            raise ValueError("For Whisper-based ASR, you must provide a number of speakers with --num_speakers or -n flag.")

    # we will then perform the retokenization
    for f in files:
        # retokenize the file!
        retokenize(f, f.replace(pathlib.Path(f).suffix, ".cha"), E, interactive, lang=lang, noprompt=noprompt, **kwargs)


    # remove converted files
    converted = globase(in_directory, "*_converted*")
    for i in converted:
        os.remove(i)


