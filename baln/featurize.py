"""
featurize.py
Takes audio samples and featurize them into standard features
"""

# h5py data
import h5py

# pathing tools
import os
import pickle
from pathlib import Path

# data types
from abc import ABC, abstractstaticmethod
from enum import Enum
from collections import defaultdict

import numpy as np

# mfcc
from python_speech_features import mfcc
import scipy.io.wavfile as wav

# import utilities
from .utils import *

# demo = [[('beans', (24.38, 24.67)),
#          ('are', (24.67, 24.78)),
#          ('fun', (24.78, 25.22)),
#          ('.', None)],
#         [('thank', (25.29, 25.45)),
#          ('you', (25.45, 25.51)),
#          ('very', (25.51, 25.66)),
#          ('much', (25.66, 25.83)),
#          ('.', None)]]

class FProcessorAction(Enum):
    EXPERIMENT = 0
    TIER = 1
    UTTERANCE = 2
    TURN = 3

class FProcessor(ABC):
    produces_scalar = False

    @abstractstaticmethod
    def process(payload, active_region=None):
        pass

class FAudioProcessor(FProcessor):
    @abstractstaticmethod
    def process(audio_file:str, timestamps:list):
        pass

class FBulletProcessor(FProcessor):
    @abstractstaticmethod
    def process(bullet_array:list):
        pass

class Featurizer(object):

    def __init__(self):

        self.__experiment_processors = []
        self.__tier_processors = []
        self.__utterance_processors = []
        self.__turn_processors = []

    def register_processor(self, name, processor:FProcessor, 
                           processor_action:FProcessorAction):

        if processor_action == FProcessorAction.EXPERIMENT:
            self.__experiment_processors.append((name, processor))
        elif processor_action == FProcessorAction.TIER:
            self.__tier_processors.append((name, processor))
        elif processor_action == FProcessorAction.UTTERANCE:
            self.__utterance_processors.append((name, processor))
        elif processor_action == FProcessorAction.TURN:
            self.__turn_processors.append((name, processor))
        else:
            raise ValueError(f"Batchalign: processor action {processor_action} unknown!")

    @staticmethod
    def __process_with(processors, bullets, alignments, audio):
        """Process a set of data with a set of processors

        Attributes:
            processors ([FProcessor]): list of processors to use
            bullets ([[("word", (beg, end)) ...] ...]): list of utterances contining list of words
            audio (str): audio file path
        """

        processed = {"scalars": {},
                     "vectors": {}}
        
        for name, i in processors:
            scalarp = "scalars" if i.produces_scalar else "vectors"
            if issubclass(i, FAudioProcessor):
                processed[scalarp][name] = i.process(audio, alignments)
            elif issubclass(i, FBulletProcessor):
                processed[scalarp][name] = i.process(bullets)
            else:
                raise ValueError(f"Batchalign: processor type {type(i)} unknown!")

        return processed

    def process(self, data, audio):
        """Process a bullet dataset

        Attributes:
            data ({"alignments": ..., "raw": ..., "tiers": ...}): the raw alignment data
            audio (str): audio file path

        Returns:
            {"experiment": {"scalars":, ...}, "tier": {...}, "utterance": {..}}
        """

        # process experiment level
        experiment_processed = self.__process_with(self.__experiment_processors,
                                                data["raw"], data["alignments"],
                                                audio)

        # process tier level (sorry for the code quality)
        # get tier ids
        id_list = defaultdict(lambda : len(id_list))
        id_indx = defaultdict(list)
        for i, tier in enumerate(data["tiers"]):
            id_indx[id_list[tier]].append(i)
        id_list = {v:k for k,v in id_list.items()} # list of {id: tier} (i.e. {0: PAR, 1: INV})
        id_indx = dict(id_indx) # get the indicies for the tier {id: [ids ...]} (i.e. {0: [0,1,2,3, ...]})
        # slice out tiers and process
        tier_processed = {}
        for key, value in id_indx.items():
            tier = id_list[key]

            alignments = [data["alignments"][i] for i in value]
            raw = [data["raw"][i] for i in value]

            tier_processed[tier] = self.__process_with(self.__tier_processors,
                                                       raw, alignments,
                                                       audio)
        # process utterance level
        utterance_processed = []

        for raw, alignments in zip(data["raw"], data["alignments"]):
            result = self.__process_with(self.__utterance_processors,
                                        [raw], [alignments],
                                        audio)

            utterance_processed.append(result)

        # process turn level
        turns_processed = []

        current_turn = data["tiers"][0]
        raw_cache = []
        alignments_cache = []

        for raw, alignments, tier in zip(data["raw"], data["alignments"], data["tiers"]):
            if current_turn != tier:
                # process and append
                result = self.__process_with(self.__turn_processors,
                                             raw_cache, alignments_cache,
                                             audio)
                # add the tier info to the list
                result["scalars"]["speaker"] = current_turn
                turns_processed.append(result)

                current_turn = tier
                raw_cache = []
                alignments_cache = []

            # append current
            raw_cache.append(raw)
            alignments_cache.append(alignments)

        # if there's anything left, process the last one
        # process and append
        result = self.__process_with(self.__turn_processors,
                                        raw_cache, alignments_cache,
                                        audio)
        # add the tier info to the list
        result["scalars"]["speaker"] = current_turn
        turns_processed.append(result)

        return { "experiment": experiment_processed,
                 "tier": tier_processed,
                 "utterance": utterance_processed,
                 "turn": turns_processed }

###### Actual Processors ######

class MLU(FBulletProcessor):
    produces_scalar = True
    @staticmethod
    def process(bullet_array:list):
        # get number of utterances
        num_utterances = len(bullet_array)
        # TODO TODO BAD get the numer of morphemes 
        num_morphemes = [len(i) for i in bullet_array]

        return sum(num_morphemes)/num_utterances

class VoicedDuration(FBulletProcessor):
    produces_scalar = True
    @staticmethod
    def process(bullet_array:list):
        # sum the time when something's voiced
        return sum([sum(word[1][1]-word[1][0] for word in utt if word[1]) for utt in bullet_array])

class SilenceDuration(FBulletProcessor):
    # TODO how to compute this. Does this include inter-utterance?
    produces_scalar = True
    @staticmethod
    def process(bullet_array:list):
        # tally
        silence = 0
        # loop through each utterance to compute silences
        for utterance in bullet_array:
            # filter for no times
            bulleted_words_with_time = list(filter(lambda x:x[1], utterance))
            for i in range(len(bulleted_words_with_time)-2): # -2 to not read the last one
                cur_word = bulleted_words_with_time[i]
                next_word = bulleted_words_with_time[i+1]

                # compute silence: i.e. space between two words
                silence += next_word[1][0] - cur_word[1][1]

        return silence

class VoiceSilenceRatio(FBulletProcessor):
    produces_scalar = True
    @staticmethod
    def process(bullet_array:list):
        try:
            return VoicedDuration.process(bullet_array)/SilenceDuration.process(bullet_array)
        except ZeroDivisionError:
            return -100

class MFCC(FAudioProcessor):
    produces_scalar = False
    @staticmethod
    def process(audio_file:str, timestamps:list):
        (rate,sig) = wav.read(audio_file)

        # slice the timestamps to the desired ranges
        timestamps_sliced = [(int(i[0]*rate),
                            int(i[1]*rate)) for i in timestamps]
        timestamps_range = sum([list(range(start, end)) for start, end in timestamps_sliced], [])
        # slice the signal based on the ranges
        sig_sliced = sig[timestamps_range]

        mfcc_feat = mfcc(sig_sliced,rate)
        
        return mfcc_feat

###### Create Default Featurizer ######
featurizer = Featurizer()

# Register...
# experiment processors
featurizer.register_processor("voiced_duration", VoicedDuration, FProcessorAction.EXPERIMENT)
featurizer.register_processor("mfcc", MFCC, FProcessorAction.EXPERIMENT)
# tier processors
featurizer.register_processor("mlu", MLU, FProcessorAction.TIER)
featurizer.register_processor("voiced_duration", VoicedDuration, FProcessorAction.TIER)
featurizer.register_processor("silence_duration", SilenceDuration, FProcessorAction.TIER)
featurizer.register_processor("voice_silence_ratio", VoiceSilenceRatio, FProcessorAction.TIER)
featurizer.register_processor("mfcc", MFCC, FProcessorAction.TIER)
# turn processors
featurizer.register_processor("voiced_duration", VoicedDuration, FProcessorAction.TURN)
featurizer.register_processor("silence_duration", SilenceDuration, FProcessorAction.TURN)
featurizer.register_processor("voice_silence_ratio", VoiceSilenceRatio, FProcessorAction.TURN)
featurizer.register_processor("mfcc", MFCC, FProcessorAction.TURN)
# utterance processors
featurizer.register_processor("voiced_duration", VoicedDuration, FProcessorAction.UTTERANCE)
featurizer.register_processor("silence_duration", SilenceDuration, FProcessorAction.UTTERANCE)

###### Normal Commands ######
def store_to_group(group, feature_dict):
    """Store a part of a feature dictioray to HDF5 group

    Attributes:
        group (hdf5.Group): the hdf5 group to store data
        feature_dict ({"vectors": ..., "scalars": ...}): the data to encode

    Used for side effects.
    """
    # store experiment scalars
    for k,v in feature_dict["scalars"].items():
        group.attrs[k] = v

    # and the datasets, which should be numpy arrays
    for k,v in feature_dict["vectors"].items():
        group.create_dataset(k, data=v)

def featurize(alignment, in_dir, out_dir, data_dir="data", lang="en", clean=True):

    for filename, data in alignment:
        # experiment name
        name = Path(filename).stem
        # generate the paths to things
        audio = os.path.join(in_dir, f"{name}.wav")
        hdf5 = os.path.join(out_dir, f"{name}.hdf5")
        pkl = os.path.join(out_dir, f"{name}.pkl")
        chat = os.path.join(out_dir, f"{name}.cha")
        
        # featurize!
        features = featurizer.process(data, audio)

        # store the features
        f = h5py.File(hdf5, "w")
        f.attrs["num_utterances"] = len(data["raw"])

        # create the initial groups
        exp = f.create_group("experiment")
        tier = f.create_group("tier")
        utterance = f.create_group("utterance")
        turn = f.create_group("turn")

        # store experiment-wide results
        store_to_group(exp, features["experiment"])

        # store per-tier results
        for tier_name, value in features["tier"].items():
            tier_grp = tier.create_group(tier_name)
            store_to_group(tier_grp, value)

        # store per-utterance results
        for i, value in enumerate(features["utterance"]):
            utt_grp = utterance.create_group(str(i))
            store_to_group(utt_grp, value)

        # store per-utterance results
        for i, value in enumerate(features["turn"]):
            turn_grp = turn.create_group(str(i))
            store_to_group(turn_grp, value)


        f.close()

        # pickle the fetarues too
        with open(pkl, "wb") as f:
            pickle.dump(features, f)

        
