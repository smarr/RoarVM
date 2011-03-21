# Example script for generating the sources and build config on Windows platform
# pass the configuration class name as argument
# The $SQUEAKVM env var should be set to path to VM binary
# The generator.image should be located in ../build directory.
#
# generate.cmd CogMsWindowsConfig
#

rmdir /S /Q ..\src
cd ..\build || exit
del /Q PharoDebug.log

echo %1 generateWithSources. Smalltalk snapshot: false andQuit: true. > ./script.st

%SQUEAKVM% -headless generator.image script.st

