import zipfile
import sys
import os
import re

# Exclude these staging output manifest files
# See https://forums.unrealengine.com/t/what-is-manifest-nonufsfiles-win64-for/135806
# TODO: In future can also read the Manifest_DebugFiles_Win64.txt to see which debug files to exclude
includeExclude = ["Manifest_.*\.txt", ".*\.pdb",".*\.tps",".*\.ttf"]

def zipdir(ziph, path, exclusions = None, inclusions = None):
    # Do not include the folder being zipped in the archive itself
    # If we want to do this we could use parent path below for zip_root instead of input path
    # zip_root = os.path.join(path, '..')
    zip_root = path
    for root, dirs, files in os.walk(path):
        for file in files:
            if (inclusions and any(re.match(includeRegex, file) for includeRegex in inclusions)) or (exclusions and not any(re.match(excludeRegex, file) for excludeRegex in exclusions)):
                file_path = os.path.join(root, file)
                zip_path = os.path.relpath(file_path, zip_root)
                print("Writing " + zip_path)
                ziph.write(file_path, zip_path)


######################### MAIN SCRIPT ##################################

if len(sys.argv) != 4:
    print("Usage: [mode] [Source Directory] [Output Path]")
    sys.exit(1)

#d for debug symbols or #s for shipping
mode = sys.argv[1]
source_dir = sys.argv[2]
zip_file = sys.argv[3]
is_debug_mode = mode == "d"

print("Zipping " + source_dir + " to " + zip_file)

ziph = zipfile.ZipFile(zip_file, 'w', zipfile.ZIP_DEFLATED)

if is_debug_mode:
    zipdir(ziph, source_dir, inclusions = includeExclude)
else:
    zipdir(ziph, source_dir, exclusions = includeExclude)

ziph.close()

print()
print("Zip created successfully: " + zip_file)
