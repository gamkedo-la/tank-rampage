# Tank Rampage

## Info

Trello: Coming Soon!

Project uses Unreal 5.3 with a mix of C++ and Blueprints.

## Releasing

On main run the following with the new version:

```
./release_version 1.0.0.0
```

## Set up

### Unreal

Requires Unreal 5.3 installed from Epic Games.

### Python

If you want to use the convenience command line tools: `create_module`, `release_package`, `update_copyrights` then you need to have an accessible version of `Python 3+` installed on your machine.  These tools are totally optional.

- `create_module` : Skaffolding tool to quickly generate boilerplate files for a new game module
- `release_package`: Creates zip files with packaged build and unncessary files stripped out.  Separate zip for symbol files for debugging any crashes from deployed builds.
- `update_copyrights` : Replaces all the copyright notices at the top of C++ source files with the value configured in the project settings.

Requires Python 3+ for command line tools to generate new game modules.

## Creating a new game module

See this great [video](https://www.youtube.com/watch?v=DqqQ_wiWYOw&t=1s) from Ari explaining how game modules work

1. Run `./create_module [moduleName]` . Module names should be prefixed with `TR` to avoid any possible name clashes with stock Unreal module names
2. Regenerate the VS solution files by right clicking on the `uproject` file 
3. Add the module name in the `PrivateDependencyModuleNames` of the main game module to be sure it is built.
4. Make sure the base generated module builds in VS
5. Add the module name in the `PrivateDependencyModuleNames` or `PublicDependencyModuleNames` of the Build.cs file of other modules that need to reference this.  Use private unless the module has a public header that includes a header in the dependent module.
6. Be sure to use `[MODULENAME]_API` on any functions or classes or other symbols that need to be exported in the Public folder in order to be able to reference them from other modules.  If the function or variable is inline then the export declaration is not needed.
