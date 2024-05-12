from pathlib import Path
import subprocess
import whisper
import os

import click

def process_file(file, output):
    """Run OpenAI Whisper

    Parameters
    ----------
    file : str
        The file to process.
    output : str
        The directory for which output can be placed.
    """
    
    print("Loading model...")
    # load the input model
    model = whisper.load_model("large-v2")

    print("Perfroming transcription...")
    # perform transcription
    result = model.transcribe(file)

    print("Writing results...")
    # write results to file
    res = []

    # create the segments
    for i in result["segments"]:
        res.append((i["start"], i["end"], i["text"]))

    # and write them to file
    with open(os.path.join(output, Path(file).stem+".cut"), 'w') as df:
        df.write("\n".join([f"{i[0]:.2f}-{i[1]:.2f}:\t{i[2].strip()}" for i in res]))

    print("Done...")

##### The rest of this file contains terminal CLI management code, unimportant to Whisper ####
@click.command()
@click.argument('input_file', type=click.Path(exists=True,
                                         dir_okay=False,
                                         file_okay=True))
@click.argument('output_path', type=click.Path(exists=True,
                                               dir_okay=True,
                                               file_okay=False))
def run(input_file, output_path):
    """Runs OpenAI Whisper on INPUT_FILE and place the transcript into OUTPUT_PATH"""

    process_file(input_file, output_path)


if __name__=="__main__":
    run()

    
