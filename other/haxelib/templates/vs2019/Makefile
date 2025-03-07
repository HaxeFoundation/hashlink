MS_BUILD?="C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/MSBuild/Current/Bin/MSBuild.exe"
PLATFORM?=x64
CONFIG?=Release

all: build

build:
	$(MS_BUILD) ::name::.sln -t:::name:: -nologo -verbosity:minimal -property:Configuration=$(CONFIG) -property:Platform=$(PLATFORM)
