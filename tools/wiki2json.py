import json
from os import path
from argparse import ArgumentParser
from base64 import b64encode
import lxml.etree


parser = ArgumentParser()
parser.add_argument("-f", "--filename",
                    help="file name")

args = parser.parse_args()

filename = args.filename

# Strips the newline character
with open(filename) as infile:
    title = False
    text = False
    for line in infile:
        if (line.find("<title>") != -1):
            print(line.strip())
            title = True
        if (line.find("<text") != -1):
            text = True
        if (line.find("text>") != -1):
            title = False
            text = False
        if (title == True and text == True):
            print(line.strip())
