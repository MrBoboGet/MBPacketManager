![Logo](Logo_Small.png)
# MBPacketManager

MBPacketManager is a generic packet manager, with a builtin build system for C/C++. Other builtin packet types include Vim extensions and Bash extensions. 

# Features

## Powerful packet specification

At the core of MBPacketManager is the vectorization of commands and the ability to efficiently operate on a specific set of packets. 

### Examples

```bash
# Compiles the packets MBUtility MBDoc and MBPacketManager
# with Debug as the compile configuration
mbpm compile MBUtility MBDoc MBPacketManager -c:Debug

# Compile all of the dependancies for MBRadio, in topological order
mbpm compile MBRadio --dependancies

# Update the local installation with all of the user packets, that 
# depend on MBParsing
mbpm upload MBParsing --dependants --local

# Compile all the locally installed packets, without the attribute NonMBBuild
mbpm compile --allinstalled -na:NonMBBuild
```


All of these packet specifications are also packet type agnostic, meaning that any new packet type can use this common functionality. 

## C/C++ build systems

[MBBuild](https://mrboboget.github.io/MBPacketManager/MBBuild/index.html) unlike other build systems that aim to encompass lot's of use cases and special cases, aims for the opposite: Build specifications should be minimal and aim to be as standardized as possible. 

Building C/C++ therefore only support a very limited set of configurations for every target: What kind, executable or library, what sources it contains, and which language and standard the sources are written in. The whole build can also specify which other packets it depends on. 

Compile flags and which compiler to use is specified in a separate config file, which means that compatible compile flags are up to the user to decide, and forces targets to not depend on non-standard behavior. 

MBBuild by default uses gcc to inspect the sources with `gcc -MM -MF - `, with the added flags for the current compile configuration to determine which headers a source file currently depends on. 

### Examples

A typical build file, named `MBSourceInfo.json` by default, might look like this. 

```json
{
    "Dependancies":["MBUtility"],
    "Language":"C++",
    "Standard":"C++17",
    "Targets":
    {
        "helloworld":
        {
            "OutputName":"helloworld",
            "TargetType":"Executable",
            "Sources":
            {
               "/main.cpp" 
            }
        }
    }
}
```


The file specifying the compile flags can look like this: 

```json
{
    "LanguageConfigs":
    {
        "CompileConfigurations":
        {
            "Debug":
            {
                "Toolchain":"gcc",
                "CompileFlags":["-g", "-Wall"],
                "LinkFlags":[]
            },
            "Release":
            {
                "Toolchain":"gcc",
                "CompileFlags":["-O2","-Wall"],
                "LinkFlags":[]
            }
        }
    }
}
```


This allows for compilation with for example the command, assuming that you are in the same directory as `MBSourceInfo.json`, `mbpm compile ./ -c:Debug`. It also means that, as the compile flags are separate from the build specification, that configurations for specific tasks like code coverage are easily reused. 

## Extensibility

MBPacketManager is designed to allow it to be extended with new types of packets. Extension can add commands that are vectorized over a packet specification, or specializing a specific command to do something different for this specific packet type when it's encountered. 

MBBuild is for example implemented by specializing `mbpm compile` to execute the build process for packets of type "C" or "C++". Packet types that aren't overriden by any extensions are skipped, allowing for modular extension of the capabilities without breaking existing configurations. 

Some commands like `mbpm upload` however, can specify that they operate on the whole set of packet specified, granting generic functionality. 

## Incremental updates and uploads

A core part of the reason for the development of MBPacketManager was to allow moving code between different computers and compile it in a generic way. 

Builtin is a file transfer protocol that only downloads and uploads the files that are changed between different indexes. It also enables the ability to download individual files/directories from repositories programatically. A client aswell as a generic server implementation is provided, allowing for easy integration with various backends, and one such backend can be found at [MBWebsite](https://github.com/MrBoboGet/MBWeb/MBWebsite). 

# Documentation

Complete documentation for MBPacketManager can be found at [here](https://mrboboget.github.io/MBPacketManager/index.html). It includes specification for the various configuration files used, as well as a complete description of the command line interface. 

# Compilation

The code is compiled using MBPacketManager itself. The buildprocess is very simple however, the content of the dependencies has to be in a directory with the same name as it, and on the include path. Then the libraries for each dependency has to be linked. 

# License

This code is licensed under CC-zero, and the only external dependency [cryptopp](https://github.com/weidai11/cryptopp) is also in the public domain. 

