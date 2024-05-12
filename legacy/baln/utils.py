# import os utilties
import os
# and glob
import glob
# pathlib
import pathlib
# regular expressions
import re


# resolve CLAN
def resolve_clan():
    # resolve for where CLAN is
    if os.path.isfile("./flo"):
        return "."
    else:
        return ""

CLAN_PATH=resolve_clan()

# Oneliner of directory-based glob and replace
globase = lambda path, statement: glob.glob(os.path.join(path, statement))
repath_file = lambda file_path, new_dir: os.path.join(new_dir, pathlib.Path(file_path).name)

def check_media_link(f):
    """Remove bullets from a file

    Attributes:
        f (string): file path

    Returns:
        none, used for side effects
    """

    # get abspath of file
    f = os.path.abspath(f)
    # open the file and strip bullets
    with open(f, 'r') as df:
        new_string = df.read()
    # remove heritage labels, if needed
    new_string = new_string.replace("@Options:\theritage", "@Options:\tmulti")
    # remove bad comment labeling (which in an artifact of media links generated with EAF)
    new_string = re.sub(r"%:([\w\t ]+)\n%mor:.+?\n%gra:.+?\n", r"@Comment:\1\n", new_string)
    new_string = re.sub(r"%mor:.+?\n%:([\w\t ]+)\n%gra:.+?\n", r"@Comment:\1\n", new_string)
    new_string = re.sub(r"%mor:.+?\n%gra:.+?\n%:([\w\t ]+)\n", r"@Comment:\1\n", new_string)
    new_string = re.sub(r"%:([\w\t ]+?)\n\*", r"@Comment:\1\n*", new_string)
    # check for unlinked media line
    if re.search(r"@Media:	, audio", new_string): # this is the sign of no media line
        # remove media line
        new_string = re.sub(r'@Media.*\n', '', new_string)
        new_string = re.sub(r'\d+_\d+', '', new_string)
    else:
        # check for -1, and fix it
        new_string = re.sub(r'-1_', '0_', new_string)
        new_string = re.sub(r'0_199', '', new_string)
        new_string = re.sub(r'0_79', '', new_string)
        new_string = re.sub(r'0_4', '', new_string)

        new_string = re.sub(r'•-1_', '•0_', new_string)
        new_string = re.sub(r'•0_199•', '', new_string)
        new_string = re.sub(r'•0_79•', '', new_string)
        new_string = re.sub(r'•0_4•', '', new_string)

        if re.search(r"@Media:	\w+, ?(audio)|(video)", new_string) and ("" not in new_string) and ("•" not in new_string): # this is the sign of no actual media line
            new_string = re.sub(r"@Media:	(\w+), audio", r"@Media:	\1, audio, unlinked", new_string)
            new_string = re.sub(r"@Media:	(\w+), video", r"@Media:	\1, video, unlinked", new_string)


    # open the file and write content
    with open(f, 'w') as df:
        # write
        df.write(new_string.replace("\n\t\n","\n"))

def strip_bullets(f):
    """Remove bullets from a file

    Attributes:
        f (string): file path

    Returns:
        none, used for side effects
    """

    # get abspath of file
    f = os.path.abspath(f)
    # open the file and strip bullets
    with open(f, 'r') as df:
        file_content = df.read()
        # remove all bullet symbols
        new_string = re.sub(r'\d+_\d+', '', file_content)
    # now, we also nee to add unlinked to the @Media line
    # as we just got rid of the bullets;
    # first we look for the media line
    media_line_match = re.search(r"@Media.*", new_string)
    if media_line_match and media_line_match[0].split(" ")[-1] != 'unlinked':
        media_line_new = media_line_match[0].strip() # clone the string
        # replace and add `unlinked` if needed
        media_line_new += ", unlinked"
        # replace!
        new_string = new_string.replace(media_line_match[0], media_line_new)
    # open the file and write content
    with open(f, 'w') as df:
        # write
        df.write(new_string)

def change_media(f, format="video"):
    """Change the media header to say a different format

    Attributes:
        f (string): file path

    Returns:
        none, used for side effects
    """

    # get abspath of file
    f = os.path.abspath(f)
    # open the file and strip bullets
    with open(f, 'r') as df:
        file_content = df.read()
        # remove all bullet symbols
        new_string = re.sub(r'@Media:(.*), .*', r"@Media:\1, "+format, file_content)
    # open the file and write content
    with open(f, 'w') as df:
        # write
        df.write(new_string)

def fix_transcript(f, lowcase="caps.cut"):
    """Fix transcript by adding any needed annotations, retracings, etc.

    Attributes:
        f (string): file path
        lowcase (string): lowcase file, if None is used then it will not be ran

    Returns:
        none, used for side effects
    """

    # get dir
    dir_path = os.path.dirname(os.path.realpath(__file__))
    # get abspath of file
    f = os.path.abspath(f)

    ###########################################
    ## Fix disfluencies and join group words ##
    ###########################################

    DISFLUENCY_FILE = os.path.abspath(os.path.join(dir_path, "disfluencies.cut"))
    REP_JOIN_FILE = os.path.abspath(os.path.join(dir_path, "rep-join.cut"))
    # run disfluency replacement
    CMD = f"{os.path.join(CLAN_PATH, 'chstring')} +c{DISFLUENCY_FILE} {f} "
    os.system(CMD)
    # move old file to backup
    os.rename(f, f.replace(".cha", ".old.cha"))
    # rename new file
    os.rename(f.replace(".cha", ".chstr.cex"), f)
    # run rep join replacement
    CMD = f"{os.path.join(CLAN_PATH, 'chstring')} +c{REP_JOIN_FILE} {f} "
    os.system(CMD)
    # delete old file
    os.remove(f)
    # rename new file
    os.rename(f.replace(".cha", ".chstr.cex"), f)


    ###############
    ## Retracing ##
    ###############

    # run 
    CMD = f"retrace +c {f} "
    os.system(CMD)
    # delete old file
    os.remove(f)
    # rename new file
    os.rename(f.replace(".cha", ".retrace.cex"), f)


    #################
    ## Lowercasing ##
    #################

    if lowcase:
        CAPS_FILE = os.path.abspath(os.path.join(dir_path, lowcase))
        # save/change workdir for lowcase
        workdir = os.getcwd()
        # change it to the output
        os.chdir(pathlib.Path(f).parent.absolute())
        # run 
        CMD = f"lowcase +d1 +i{CAPS_FILE} {os.path.basename(f)} "
        os.system(CMD)
        # change it back to the output
        os.chdir(workdir)
        # delete old file
        os.remove(f)
        os.remove(f.replace(".cha", ".old.cha"))
        # rename new file
        os.rename(f.replace(".cha", ".lowcas.cex"), f)

def cleanup(in_directory, out_directory, data_directory="data"):
    """Clean up alignment results so that workspace is clear

    Attributes:
        in_directory (string): the input directory containing files
        out_directory (string): the output directory containing possible output files
        data_directory (string): a subdirectory which the misc. outputs go

    Returns:
        none
    """

    # Define the data_dir
    DATA_DIR = os.path.join(out_directory, data_directory)

    # move all the lab files 
    tgfiles = globase(in_directory, "*.textGrid")
    # Rename each one
    for f in tgfiles:
        # we don't actually want to repath UPPERCASE TextGrid
        # unfortunately windows is not smart enough to ignore
        # it in the glob.
        if "TextGrid" in f:
            continue
        os.rename(f, repath_file(f, DATA_DIR)) 

    # move all the lab files 
    tgfiles = globase(in_directory, "*.TextGrid")
    # Rename each one
    for f in tgfiles:
        os.rename(f, repath_file(f.replace(".TextGrid", ".orig.textGrid"), DATA_DIR)) 

    # move all the lab files 
    labfiles = globase(in_directory, "*.lab")
    # Rename each one
    for f in labfiles:
        os.rename(f, repath_file(f, DATA_DIR)) 

    # move all the wav files 
    wavfiles = globase(in_directory, "*.orig.wav")
    # Rename each one
    for f in wavfiles:
        os.rename(f, repath_file(f, DATA_DIR)) 

    # move all the rest of wav files 
    mp3files = globase(in_directory, "*.mp3")
    # Rename each one
    for f in mp3files:
        os.rename(f, repath_file(f, DATA_DIR)) 

    # move all the rest of mp4 files 
    mp4files = globase(in_directory, "*.mp4")
    # Rename each one
    for f in mp4files:
        os.rename(f, repath_file(f, DATA_DIR)) 

    # move all the orig.cha files from repathing
    chafiles = globase(in_directory, "*.orig.cha")
    # Rename each one
    for f in chafiles:
        os.rename(f, repath_file(f, DATA_DIR)) 

    # move all the old.cha files from repathing
    chafiles = globase(in_directory, "*.old.cha")
    # Rename each one
    for f in chafiles:
        os.rename(f, repath_file(f, DATA_DIR)) 

    # delete all the old.cha files from output
    chafiles = globase(out_directory, "*.old.cha")
    # Rename each one
    for f in chafiles:
        os.rename(f, repath_file(f, DATA_DIR).replace(".old.cha", ".unfix.cha")) 

    # clean up the elans
    elanfiles = globase(out_directory, "*.eaf")
    # Rename each one
    for f in elanfiles:
        os.rename(f, repath_file(f, DATA_DIR)) 

    # Clean up the dictionary, if exists
    dict_path = os.path.join(in_directory, "dictionary.txt")
    # Move it to the data dir too
    if os.path.exists(dict_path):
        os.rename(dict_path, repath_file(dict_path, DATA_DIR))

    # Cleaning up
    # Removing all the eaf files generated
    for eaf_file in globase(in_directory, "*.eaf"):
        os.remove(eaf_file)
    for eaf_file in globase(out_directory, "*.eaf"):
        os.remove(eaf_file)

    # Removing all the transcript files generated
    for eaf_file in globase(in_directory, "*.txt"):
        os.remove(eaf_file)

# fixbullets a whole path
def fixbullets(directory):
    """Fix a whole folder's bullets

    files:
        directory (string): the string directory in which .chas are in

    Returns:
        None
    """
   
    # get all files in that directory
    files = globase(directory, "*.cha")
    # process each file
    for fl in files:
        # elan2chatit!
        CMD = f"{os.path.join(CLAN_PATH, 'fixbullets ')} {fl} "
        # run!
        os.system(CMD)
    # delete any error logs
    for f in globase(directory, "*.err.cex"):
        os.remove(f)
    # delete any preexisting chat files to old
    for f in globase(directory, "*.cha"):
        os.rename(f, f.replace(".cha", ".old.cha"))
    # and rename the files
    for f in globase(directory, "*.fxblts.cex"):
        os.rename(f, f.replace(".fxblts.cex", ".cha"))

# chat2transcript a whole path
def chat2transcript(directory, mor=False):
    """Generate transcripts for a whole directory

    Arguments:
        directory (string): string directory filled with chat files
        mor (bool): generate mor-like output

    Returns:
        none
    """

    # then, finding all the cha files
    files = globase(directory, "*.cha")

    # use flo to convert chat files to text
    if mor:
        CMD = f"{os.path.join(CLAN_PATH, 'flo +d +cm +a +t*')} {' '.join(files)} "
    else:
        CMD = f"{os.path.join(CLAN_PATH, 'flo +d +ca +t*')} {' '.join(files)} "

    # run!
    os.system(CMD)

    # and rename the files to lab files, which are essentially the same thing
    for f in globase(directory, "*.flo.cex"):
        os.rename(f, f.replace(".flo.cex", ".lab"))


# chat2praat, then clean up tiers, for a whole path
def chat2praat(directory):
    """Convert a whole directory to praat files

    Arguments:
        directory (string) : string directory filled with chat

    Returns:
        none
    """
    # then, finding all the cha files
    files = globase(directory, "*.cha")

    # use flo to convert chat files to text
    CMD = f"{os.path.join(CLAN_PATH, 'chat2praat +e.wav +c -t% -t@')} {' '.join(files)} "
    # run!
    os.system(CMD)

    # then, finding all the praat files
    praats = globase(directory, "*.c2praat.textGrid")

    for praat in praats:
        # load the textgrid
       os.rename(praat, praat.replace("c2praat.textGrid", "TextGrid"))

    # delete any error logs
    for f in globase(directory, "*.err.cex"):
        os.remove(f)


# optionally convert mp3 to wav files
def mp32wav(directory):
    """Generate wav files from mp3

    Arguments:
        directory (string): string directory filled with chat files

    Returns:
        none
    """

    # then, finding all the elan files
    mp3s = globase(directory, "*.mp3")

    # convert each file
    for f in mp3s:
        os.system(f"ffmpeg -i {f} {f.replace('mp3','wav')} -c copy")

# optionally convert sph to wav files
def sph2wav(directory):
    """Generate wav files from mp3

    Arguments:
        directory (string): string directory filled with chat files

    Returns:
        none
    """

    # then, finding all the elan files
    sphs = globase(directory, "*.sph")

    # convert each file
    for f in sphs:
        os.system(f"sox {f} {f.replace('sph','wav')}")


# optionally convert mp4 to wav files
def mp42wav(directory):
    """Generate wav files from mp4

    Arguments:
        directory (string): string directory filled with chat files

    Returns:
        none
    """

    # then, finding all the elan files
    mp4s = globase(directory, "*.mp4")

    # convert each file
    for f in mp4s:
        os.system(f"ffmpeg -i {f} {f.replace('mp4','wav')} -c copy")

def wavconformation(directory):
    """Reconform wav files

    Arguments:
        directory (string): string directory filled with chat files

    Returns:
        none
    """

    # then, finding all the elan files
    wavs = globase(directory, "*.wav")

    # convert each file
    for f in wavs:
        # Conforming the wav
        os.system(f"ffmpeg -i {f} temp.wav")
        # move the original
        os.remove(f)
        # and move the new back
        os.rename("temp.wav", f)

# read all chat files
def read_flo(f):
    """Utility to read a single flo file

    Arguments:
        f (str): the file to read

    Returns:
        list[str] a string of results
    """

    # open and read file
    with open(f, 'r') as df:
        # read!
        lines = df.readlines()

    # coallate results
    results = []

    # process lines for tab-deliminated run-on lines
    for line in lines:
        # if we have a tab
        if line[0] == '\t':
            # take away the tab, append, and put back in results
            results.append(results.pop()+" "+line.strip())
        # otherwise, just append
        else:
            results.append(line.strip())

    # return results
    return results

def read_chat(f):
    # get lines in the file
    lines = read_flo(f)

    # split by tier and crop info
    lines = [i.split("\t") for i in lines]

    # chop off end delimiters in main tier
    lines = [[i[0], i[1][:-2]] if i[0][0]=='*' else i for i in lines]

    # get tiers seperated

    # array for header tiers
    header_tiers = []
    # pop out header tiers
    while lines[0][0][0] == '@':
        header_tiers.append(lines.pop(0))

    # array for main tiers
    main_tiers = []
    # pop out main tiers
    while lines[0][0][0] == '*':
        main_tiers.append(lines.pop(0))

    # and set the rest to closing
    closing_tiers = lines

    # process main tiers
    main_tiers_processed = []

    # for every line, process
    for line in main_tiers:
        # split the line
        line_split = line[1].split(" ")
        # pair them up
        line_paired = [(line_split[i],
                        line_split[i+1])
                    for i in range(0,len(line_split), 2)]
        # and fix the right end
        bullet_extract = lambda x: [int(i)
                                    for i in
                                    re.sub(r".(\d+)_(\d+).", r"\1|\2", x).split('|')]
        line_extracted = [[i[0], bullet_extract(i[1])] for i in line_paired]

        # append results
        main_tiers_processed.append((line[0], line_extracted))

    return header_tiers, main_tiers_processed, closing_tiers

