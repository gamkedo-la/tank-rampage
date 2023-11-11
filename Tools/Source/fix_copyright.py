import sys
import os
import re

CopyrightProperty = "CopyrightNotice"
CopyrightIniFile = os.path.join("config","DefaultGame.ini")
SourceDir = "source"

IncludeFiles = [".*\.h", ".*\.cpp", ".*\.cs"]

def getNotice(rootDir):
    configFile = os.path.join(rootDir, CopyrightIniFile)
    keyPrefix = CopyrightProperty + "="

    with open(configFile, "r") as file:
        for line in file:
            line = line.strip()
            if line.startswith(keyPrefix):
                value = line[len(keyPrefix):].strip()
                return value
    return None

def formatNoticeLines(notice):
    noticeLines = notice.splitlines()
    for i, line in enumerate(noticeLines):
        noticeLines[i] = "// " + line
    return noticeLines

def checkCopyright(file, notice):
    noticeLines = formatNoticeLines(notice)
    numNoticeLines = len(noticeLines)

    with open(file, "r") as file:
        noticeLineNumber = 0
        while(line := file.readline()):
            if line.rstrip("\n") != noticeLines[noticeLineNumber]:
                return False
            noticeLineNumber += 1
            if noticeLineNumber == numNoticeLines:
                return True
    return False

def replaceCopyright(file, notice):
    noticeLines = formatNoticeLines(notice)
    lines = []

    with open(file, "r") as f:
        while(line := f.readline()):
            lines.append(line.rstrip("\n"))

    # remove until non-comment line
    firstNonComment = len(lines)
    for i, line in enumerate(lines):
        if(not line.lstrip().startswith("//")):
            firstNonComment = i
            break

    if firstNonComment == len(lines):
        lines.clear()
    else:
        del lines[:firstNonComment]
    
    # Be sure there is a blank line between copyright section          
    if(not lines or (len(lines[0]) > 0 and not lines[0].isspace())):
        noticeLines.append('')     

    lines = noticeLines + lines

    with open(file, "w") as f:
        for line in lines:
            f.write(line)
            f.write("\n")
  
def replaceInSourceFiles(rootDir, notice):
    count = 0
    dir = os.path.join(rootDir, SourceDir)
    for dirpath, subdirs, files in os.walk(dir):
        for file in files:
            if (any(re.match(includeRegex, file) for includeRegex in IncludeFiles)):
                filepath = os.path.join(dirpath, file)
                if not checkCopyright(filepath, notice): 
                    print("Updating " + filepath)
                    replaceCopyright(filepath, notice)
                    count += 1
    return count

######################### MAIN SCRIPT ##################################

rootDir = os.getcwd()

print("Replacing copyright notices in " + rootDir + "...")
print()

notice = getNotice(rootDir)
if not notice:
    print("Unable to get copyright notice from config in " + os.path.join(rootDir, CopyrightIniFile))
    sys.exit(1)

count = replaceInSourceFiles(rootDir, notice)

print()
print("Completed successfully: " + str(count) + " files updated.")
