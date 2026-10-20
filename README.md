# VL53L7CX emulator host: VS Code and PSoC Creator

This workspace edits the existing PSoC 5LP project (CY8C5888LTI-LP097) in
VS Code and builds it with PSoC Creator 4.4 and its bundled ARM GCC 5.4.1.
PSoC Creator remains responsible for schematic editing, pin assignments,
component generation, programming, and hardware debugging.

## Open and build

1. Install PSoC Creator 4.4 with the ARM GCC 5.4-2016-q2-update toolchain.
2. Open [the VS Code workspace](VL53L7CX_emulator_host_LabView_wprefetch_VSC.code-workspace)
   and install the recommended Microsoft C/C++ extension.
3. Press **Ctrl+Shift+B** to build Debug. Under **Terminal > Run Task**, select
   Build Release, Rebuild, Clean, or Open Workspace as needed.
4. Use Open Workspace to edit the schematic, pins, or components in Creator,
   or to program/debug the board. Build after changing the hardware design.

The [standalone Creator workspace](VL53L7CX_emulator_host_LabView_wprefetch_VSC.cywrk)
references only [this project](VL53L7CX_emulator_host_LabView_wprefetch_VSC.cyprj).
The existing parent workspace and project settings are not replaced.
Build output is written under `ARM_GCC_541\Debug` or `ARM_GCC_541\Release`;
the output filename is controlled by the existing Creator project settings.

The same build can be run from PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\.vscode\psoc-creator.ps1 -Action Build -Configuration Debug
```

Tasks default to
`C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator`.
For a nonstandard installation, set the `PSOC_CREATOR_HOME` environment variable
to the directory containing `bin\cyprjmgr.exe` and restart VS Code.
Also update `compilerPath` in [the IntelliSense configuration](.vscode/c_cpp_properties.json)
to that installation's `import\gnu\arm\5.4.1\bin\arm-none-eabi-gcc.exe`.
IntelliSense uses the Debug configuration and Cortex-M3 compiler flags.

Add/remove application source files through Creator as well as VS Code:
Creator builds the files registered in its project, not every file in the folder.
Do not manually edit generated files outside Creator's designated user sections.

## Git

The repository is local to this project directory. Application sources, Creator
project/design/workspace files, VS Code configuration, and `Generated_Source`
are intended to be versioned. Generated sources are retained for immediate
IntelliSense and to preserve any component user-section code.

[.gitignore](.gitignore) excludes toolchain output, temporary generation folders,
reports, logs, backups, and Creator's per-user preferences. In particular, it
does not exclude the generated `config.hex` required by the project.
No remote repository is configured automatically.
