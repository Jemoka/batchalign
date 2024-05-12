# XML facilities
from posixpath import isfile
import xml.etree.ElementTree as ET

# Also, stdlib
import re, os

# glob
import glob

# import cleanup
from .utils import resolve_clan, change_media

# import pathlib
import pathlib

CLAN_PATH=resolve_clan()

# Oneliner of directory-based glob and replace
globase = lambda path, statement: glob.glob(os.path.join(path, statement))
repath_file = lambda file_path, new_dir: os.path.join(new_dir, pathlib.Path(file_path).name)
bullet = lambda start,end: f" {int(round(start*1000))}_{int(round(end*1000))}" # start and end should be float seconds

# indent an XML
def indent(elem, level=0):
    """Indent an XML

    Attributes:
        element (ET.ElementTree.root): element tree root
        level (int): which level to start aligning from

    Citation:
    https://stackoverflow.com/a/33956544/5684313

    Returns void
    """

    i = "\n" + level*"  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            indent(elem, level+1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i

def eafaddsubtier(root, content, tier_name, tier_shortname):
    """Add a SubElement tier

    Attributes:
        root (ET.ElementTree): element tree to add to
        content (dict): {"a1": "this is the content under annotation a1", ...}
        tier_name (str): wor
        tier_shortname (str): xw
    """

    # get the tiers
    tiers = root[2:]

    # Create the xword annotation ID index
    id_indx = 0

    # lower
    tier_name = tier_name.lower()
    tier_shortname = tier_shortname.lower()

    # replace content with the bulleted version
    # For each tier 
    for tier in tiers:
        # get the name of the tier
        tier_id = tier.attrib.get("TIER_ID","")

        # create new contentd tier
        content_tier = ET.SubElement(root, "TIER")
        content_tier.set("TIER_ID", f"{tier_name}@{tier_id}")
        content_tier.set("PARTICIPANT", tier_id)
        content_tier.set("LINGUISTIC_TYPE_REF", "dependency")
        content_tier.set("DEFAULT_LOCALE", "us")
        content_tier.set("PARENT_REF", tier_id)

        # we delete all previous content/wor tiers
        if tier_name in tier_id and tier_id[0] != "x":
            root.remove(tier)
            continue

        # we ignore anything that's a "@S*" tier
        # because those are metadata
        if "@" in tier_id:
            continue

        # For each annotation
        for annotation in tier:
            # get annotation
            annotation = annotation[0]
            # get ID
            annot_id = annotation.attrib.get("ANNOTATION_ID", "0")

            # get desired content
            body = content.get(annot_id)
            # if we don't have any content for the line, then ignore the line
            if not body:
                continue
            # if our content is literally just the utterance delimiter, continue
            if body.strip() in [".", "!", "?"]:
                continue

            # append annotation line
            # create two element
            content_annot = ET.SubElement(content_tier, 'ANNOTATION')
            content_annot_cont = ET.SubElement(content_annot, 'REF_ANNOTATION')

            # adding annotation metadata
            content_annot_cont.set("ANNOTATION_ID", f"{tier_shortname}{id_indx}")
            content_annot_cont.set("ANNOTATION_REF", annot_id)

            # and add content
            content_word_cont = ET.SubElement(content_annot_cont, "ANNOTATION_VALUE")

            # with the bulleted content
            content_word_cont.text = body

            # update index
            id_indx += 1

def eafalign(root, annotations, alignments=None, bullets=None):
    """inject MFA alignments into EAF (%wor tier)

    Attributes:
        root (ET.ElementTree): element tree
        annotations (tuple(time, TIER, id)): tier information extracted from input
        alignments (dict): raw, processed MFA output dictionary from fa.transcript*()

    Returns:
        None

    Side effects:
        Modify root
    """

    if alignments:
        # create a lookup dict of xwor tier outputs
        terms_flattened = list(zip([i[-1] for i in annotations], alignments["terms"]))
        terms_dict = dict(terms_flattened)

        # inject the xwor result as a subtier to the root
        eafaddsubtier(root, terms_dict, "wor", "xw")

        # set alignment back to what the rest of the function would expect
        alignments = alignments["alignments"]
    else:
        alignments = bullets

    # Remove the old time slot IDs
    root[1].clear()

    # For each of the resulting annotations, dump the time values
    # Remember that time values dumped in milliseconds so we
    # multiply the result by 1000 and round to int
    for annotation, alignment in zip(annotations, alignments):
        # Create the beginning time slot
        element_start = ET.Element("TIME_SLOT")
        # Set the time slot ID to the correct beginning
        element_start.set("TIME_SLOT_ID", f"ts{annotation[0][0]}")

        # Create the beginning time slot
        element_end = ET.Element("TIME_SLOT")
        # Set the time slot ID to the correct beginning
        element_end.set("TIME_SLOT_ID", f"ts{annotation[0][1]}")

        # if alignment is not None, set time value
        if alignment:
            # Set the time slot ID to the correct end
            element_start.set("TIME_VALUE", f"{int(alignment[0]*1000)}")
            # Set the time slot ID to the correct end
            element_end.set("TIME_VALUE", f"{int(alignment[1]*1000)}")

        # Appending the element
        root[1].append(element_start)
        root[1].append(element_end)

def eafud(root, annotations, morphodata):
    """inject parsed UD results into EAF (%mor and %gra tier)

    Attributes:
        root (ET.ElementTree): element tree
        annotations (tuple(time, TIER, id)): tier information extracted from input
        morphodata (list): raw, processed Stanza output dictionary from ud

    Returns:
        None

    Side effects:
        Modify root
    """

    # slice of gra and mor tiers
    mor, gra = zip(*morphodata)

    # get indicies of morphology data that is just xxx or yyy
    # mor should not output them
    lines_to_filter = []
    for indx, i in enumerate(mor):
        # this is becasue our morphology tagger outptus
        # just the end punctuation if nothing parsed is
        # there
        if i in ['.', '!', '?']:
            lines_to_filter.append(indx)
    # we do this here instead of below because we want to make
    # sure that the zipped annotations are correct (i.e. we have
    # bijective matching of mor lines to annotations in EAF)

    # create a lookup dict of xwor tier outputs
    morpho_flattened = list(zip([i[-1] for i in annotations], mor))
    # remove the lines to filter
    morpho_flattened = [i for j, i in enumerate(morpho_flattened) if j not in lines_to_filter]
    morpho_dict = dict(morpho_flattened)

    grapho_flattened = list(zip([i[-1] for i in annotations], gra))
    # remove the lines to filter
    grapho_flattened = [i for j, i in enumerate(grapho_flattened) if j not in lines_to_filter]
    grapho_dict = dict(grapho_flattened)

    # add the result into the elementtree
    eafaddsubtier(root, morpho_dict, "mor", "mr")
    eafaddsubtier(root, grapho_dict, "gra", "gr")

def eafinject(file_path, output_path, alignments=None, morphodata=None, bullets=None):
    """get an unaligned eaf file to be aligned

    Attributes:
        file_path (string): file path of the .eaf file
        output_path (string): output path of the aligned .eaf file
        alignments (list or dict): output of transcript word alignment
        morphodata (dict): raw, processed Stanza output dictionary from ud

    Returns:
        None
    """

    # Load the file
    parser = ET.XMLParser(encoding="utf-8")
    tree = ET.parse(file_path, parser=parser)
    root =  tree.getroot()
    tiers = root[2:]

    # create an array for annotations
    annotations = []

    # For each tier 
    for tier in tiers:
        # get the name of the tier
        tier_id = tier.attrib.get("TIER_ID","")

        # we ignore anything that's a "@S*" tier
        # because those are metadata
        if "@" in tier_id:
            continue

        # For each annotation
        for annotation in tier:

            # Parse timeslot ID
            try: 
                timeslot_id_1 = int(annotation[0].attrib.get("TIME_SLOT_REF1", "0")[2:])
                timeslot_id_2 = int(annotation[0].attrib.get("TIME_SLOT_REF2", "0")[2:])
            except ValueError:
                print(file_path)

            # and append the metadata + transcript to the annotations
            annotations.append(((timeslot_id_1,timeslot_id_2), # (time, time)
                                tier_id, # PAR
                                annotation[0].attrib.get("ANNOTATION_ID", "0"))) # axx

    # we will sort annotations by timeslot ref
    annotations.sort(key=lambda x:int(x[-1][1:]))

    # if we are injecting the EAF with alignments
    if alignments:
        eafalign(root, annotations, alignments)
    if bullets:
        eafalign(root, annotations, bullets=bullets)
    if morphodata:
        eafud(root, annotations, morphodata)

    # Indent the tree
    indent(root)
    # And write tit to file
    tree.write(output_path, encoding="unicode", xml_declaration=True)

def elan2transcript(file_path):
    """convert an eaf XML file to tiered transcripts

    Attributes:
        file_path (string): file path of the .eaf file

    Returns:
        a dictionary of shape {"transcript":transcript, "tiers":tiers},
        with two arrays with transcript information.
    """

    # Load the file
    parser = ET.XMLParser(encoding="utf-8")
    tree = ET.parse(file_path, parser=parser)
    root =  tree.getroot()
    tiers = root[2:]

    # create an array for annotations
    annotations = []

    # For each tier 
    for tier in tiers:
        # get the name of the tier
        tier_id = tier.attrib.get("TIER_ID","")

        # we ignore anything that's a "@S*" tier
        # because those are metadata
        if "@" in tier_id:
            continue

        # For each annotation
        for annotation in tier:
            # Get the text of the transcript
            transcript = annotation[0][0].text
            # and append and sort by the time slot references
            try: 
                timeslot_id = int(annotation[0].attrib.get("TIME_SLOT_REF1", "0")[2:])
            except ValueError:
                print(file_path)

            # and append the metadata + transcript to the annotations
            annotations.append((timeslot_id, tier_id, transcript))

    # we will sort annotations by timeslot ref
    annotations.sort(key=lambda x:x[0])

    # we will seperate the tuples after sorting
    _, tiers, transcript = zip(*annotations)

    return {"transcript":transcript, "tiers":tiers}


# chat2elan a whole path
def chat2elan(directory, skip_bullets=True):
    """Convert a folder of CLAN .cha files to corresponding ELAN XMLs

    Note: this function STRIPS EXISTING TIME CODES (though,
          nondestructively)

    Attributes:
        directory (string): the string directory in which .cha is in
        [skip_bullets] (bool): whether or not to clean out old bullets

    Returns:
        None
    """
   
    # get all files in that directory
    files = globase(directory, "*.cha")
    # get the name of cleaned files
    files_cleaned = [i.replace(".cha", ".new.cha") for i in files]

    # then, clean out all bullets
    for in_file, out_file in zip(files, files_cleaned):
        # read the in file
        with open(in_file, "r") as df:
            in_file_content = df.read()

        # perform cleaning and write
        if skip_bullets:
            cleaned_content = re.sub("\x15.*?\x15", "", in_file_content)
        else:
            cleaned_content = in_file_content.strip()

        # write
        with open(out_file, "w") as df:
            df.write(cleaned_content)
            
    # chat2elan it!
    if skip_bullets:
        CMD = f"{os.path.join(CLAN_PATH, 'chat2elan')} +c +e.wav {' '.join(files_cleaned)} "
    else:
        CMD = f"{os.path.join(CLAN_PATH, 'chat2elan')} +c +s +e.wav {' '.join(files_cleaned)} "

    # run!
    os.system(CMD)
    # delete any error logs
    for f in globase(directory, "*.err.cex"):
        os.remove(f)
    # delete intermediary
    for f in globase(directory, "*.new.cha"):
        os.remove(f)
    # and rename the files
    for f in globase(directory, "*.new.c2elan.eaf"):
        os.rename(f, f.replace(".new.c2elan.eaf", ".eaf"))


# chat2elan a whole path
def elan2chat(directory, video=False):
    """Convert a folder of CLAN .eaf files to corresponding CHATs
    files:
        directory (string): the string directory in which .elans are in
        [video] (bool): replace @Media tier annotation from audio => video
    Returns:
        None
    """
   
    # get all files in that directory
    files = globase(directory, "*.eaf")
    # process each file
    for fl in files:
        # elan2chatit!
        CMD = f"{os.path.join(CLAN_PATH, 'elan2chat +c ')} {fl} "
        # run!
        os.system(CMD)
    # delete any error logs
    for f in globase(directory, "*.err.cex"):
        os.remove(f)
    # and rename the files
    for f in globase(directory, "*.elan.cha"):
        os.rename(f, f.replace(".elan.cha", ".cha"))

        # change media type if needed
        if video:
            change_media(f.replace(".elan.cha", ".cha"), "video")

# chat2elan a whole path
def elan2chat__single(fl, video=False):
    """Convert a folder of CLAN .eaf files to corresponding CHATs
    files:
        f (string): the string directory in that points to one eaf
        [video] (bool): replace @Media tier annotation from audio => video
    Returns:
        None
    """
   
    # elan2chatit!
    CMD = f"{os.path.join(CLAN_PATH, 'elan2chat +c ')} {fl} "
    # run!
    os.system(CMD)
    # find error file and remove it
    error_file = fl.replace(".eaf", ".err.cex")
    if os.path.isfile(error_file):
        os.remove(error_file)
    # process out file
    out_file = fl.replace(".eaf", ".elan.cha")
    # and rename the files
    if os.path.isfile(out_file):
        os.rename(out_file, fl.replace(".eaf",".cha"))

        # change media type if needed
        if video:
            change_media(fl.replace(".eaf",".cha"), "video")

