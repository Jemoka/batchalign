import os
from baln.utils import *


def stm_to_chat_string(f):
    """Convert stm file to CHAT string

    Parameters
    ----------
    f : str
        Path to the file we are convert.

    Returns
    -------
    str
        A string in the CHAT format
    """

    with open(f, 'r') as df:
        lines = [i.strip() for i in df.readlines()]

    # the metdata/text is split by the <NA> token on each line
    metadata, transcript = zip(*[i.split("<NA>") for i in lines])

    # and now, we want to get rid of <unk> untranscribed tokens from the output
    # as we don't care about those. We want to clean out the text as well by stripping.
    # extra spaces
    transcript = [i.replace("<unk>", "").strip() for i in transcript]

    # the metadata is space deliminated
    metadata = [i.strip().split(" ") for i in metadata]
    # each row should now look like [corpus, ??, corpus, start_time_sec_str, end_time_sec_str]

    # specifically, we want to parcel out the corpus ID (each row is the same)
    corpus_name = metadata[0][0]

    # and calculate the start and end mili seconds from the string and start and end
    time_boundaries = [(int(float(i[-2])*1000), int(float(i[-1])*1000))
                    for i in metadata]

    # TODO we are assuming there's only one participant (given its TED talks that)
    # we are processing. So, we generate the CHAT-spec header as follows:
    header = [
        "@UTF8",
        "@Begin",
        "@Languages:\teng", # TODO are there non-English ted talks?
        "@Participants:\tPAR0 Participant",
        f"@ID:\teng|{corpus_name}|PAR0|||||Participant|||",
        f"@Media:\t{corpus_name}, audio"
    ]
    # and the body is just stiching the transcript together with time bullets 
    # recall that we must add a . to the end of each line to end utterances
    main = [f"*PAR0:\t{text} . {time[0]}_{time[1]}"
            for text,time in zip(transcript, time_boundaries)]
    # and generate the full CHAT transcript
    chat = "\n".join(header+main+["@End"])

    return chat

def sph2cha_dir(in_dir, out_dir):
    """Generate fake CHAT files from *SINGLE-SPEAKER* sph files

    Parameters
    ----------
    in_dir : str
        The input directory with which to recieve the file.
    out_dir : str
        The directory to dump the generated CHAT files.
    """

    # convert all the audio to wavs
    sph2wav(in_dir)

    # and move them all to the output folder
    wavs = globase(in_dir, "*.wav")
    [os.rename(i, repath_file(i, out_dir)) for i in wavs]

    # find all the stm transcripts
    stms = globase(in_dir, "*.stm")
    # and convert them all to CHAT conforming strings
    chat_strs = [stm_to_chat_string(i) for i in stms]

    # and write each of them to the output
    for f, chat_str in zip(stms, chat_strs):
        # calculate the new path
        f = repath_file(f, out_dir).replace(".stm", ".cha")

        with open(f, 'w') as df:
            df.write(chat_str)

