import os
import sys
import json

PROJECT_NAME = "Flicker"
RESOURCES_DIR = "Tools/ModuleGenerator/Resources/"

########## Functions ########################

def createModuleDirectories(moduleName):
	basePath = "Source/" + moduleName
	os.makedirs(basePath + "/Private", exist_ok = True )
	os.makedirs(basePath + "/Public", exist_ok = True )


def createModuleFiles(moduleName):
	writeTemplateFile("Module.Build.cs", "", moduleName)
	writeTemplateFile("ModuleLogging.h", "Private/", moduleName)
	writeTemplateFile("ModuleModule.cpp", "Private/", moduleName)

def writeTemplateFile(templateFileName, subDirectory, moduleName):
	outputFileName = templateFileName.replace("Module", moduleName, 1)

	inputFilePath = RESOURCES_DIR + templateFileName
	outputFilePath = "Source/" + moduleName + "/" + subDirectory + outputFileName

	with open(inputFilePath, "r") as inputFile: 
		inputFileContents = inputFile.read()

	outputFileContents = inputFileContents.replace("%ModuleName%", moduleName)

	with open(outputFilePath, "w") as outputFile:
		outputFile.write(outputFileContents)

def updateProjectFile(moduleName):

	projectFile = PROJECT_NAME + ".uproject"
	with open(projectFile, "r") as inputFile: 
		projectData = json.load(inputFile)

	projectData["Modules"].append({
			"Name": moduleName,
			"Type": "Runtime",
			"LoadingPhase": "Default",
			"AdditionalDependencies": [
				"Engine"
			]
	})

	outputFileContents = json.dumps(projectData, indent=4)

	with open(projectFile, 'w') as outputFile: 
		outputFile.write(outputFileContents)

########## main ########################

if len(sys.argv) != 2:
	print("Usage: [ModuleName]")
	sys.exit(1)

moduleName = sys.argv[1]

print("Creating module " + moduleName)

createModuleDirectories(moduleName)
createModuleFiles(moduleName)
updateProjectFile(moduleName)
