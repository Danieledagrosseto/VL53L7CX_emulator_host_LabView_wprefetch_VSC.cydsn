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

## LabVIEW JSON commands

Hardware startup and interrupt-driven UART reception (`RxIsr`) are unchanged.
After the existing startup delay, the host scans 7-bit I2C addresses
`0x03..0x77` and saves the detected addresses without sending UART output.
The main loop waits for `Isnewcommand`; the old numeric menu is no longer
available. Existing sensor/example functions remain in the project.

Send one JSON object per line, terminated by CRLF (`\r\n`). LF-only lines are
also accepted. Allow startup to finish (including its 2-second delay and scan)
before sending the first command. LabVIEW must wait for each complete response
before sending another command; there is no command queue. Each JSON payload
may contain at most 255 bytes, excluding the line terminator.

The currently supported request is:

```text
{"type":"configuration","command":"scan_i2c","id":1}\r\n
```

Each request rescans the bus, replaces the saved addresses, and returns:

```text
{"type":"configuration","command":"scan_i2c","id":1,"status":"ok","addresses":[82,224]}\r\n
```

Addresses are ascending decimal **8-bit write addresses** (7-bit address shifted
left once): `82` corresponds to `0x29`, and `224` corresponds to `0x70`.
An empty bus returns `"status":"ok","addresses":[]`. An I2C failure other than
an address NACK returns `i2c_scan_failed`, clears the saved address count, and
does not return a partial address list. Startup scan failures remain silent;
a subsequent scan command retries the bus and reports any failure.

`type` and `command` are case-sensitive strings. `id` must be a nonnegative
integer no greater than `9007199254740991` (the exact-integer limit of the JSON
library's double representation). Responses echo these fields when valid;
missing/invalid fields are `null`. All responses are compact JSON followed by
CRLF, with no menu, scan table, or diagnostic text.

Errors have `"status":"error"` and an `error` string, for example:

```text
{"type":"configuration","command":"scan_i2c","id":null,"status":"error","error":"invalid_id"}\r\n
```

Error codes are `invalid_json`, `invalid_request`, `invalid_id`,
`unsupported_command`, `command_too_long`, `invalid_character` (embedded NUL),
`uart_receive_error`, `i2c_scan_failed`, and `out_of_memory`.
Unparseable or UART-damaged requests use `null` for all three correlation fields.
Allocation failure also uses these `null` fields. Overlength or damaged input
is drained through LF so the next request can be received normally.

### Host regression tests

With a Windows GCC installation, run these commands from the project folder.
The tests compile the actual application code with mocked hardware and the
existing cJSON implementation; no board is needed:

```powershell
$testExe = Join-Path $env:TEMP 'vl53l7cx-json-uart-tests.exe'
gcc -std=gnu11 -I .\tests .\tests\json_uart_test.c .\cJSON.c -lm -o $testExe
if ($LASTEXITCODE -eq 0) { & $testExe }
Remove-Item -LiteralPath $testExe -ErrorAction SilentlyContinue
```

Coverage includes silent startup, exact response framing, rescanning, empty
and fully populated buses, bus errors, ID validation/echoing, maximum line
length, overflow/UART error recovery, pending-buffer protection, and response
allocation failures.

## Git

The repository is local to this project directory. Application sources, Creator
project/design/workspace files, VS Code configuration, and `Generated_Source`
are intended to be versioned. Generated sources are retained for immediate
IntelliSense and to preserve any component user-section code.

[.gitignore](.gitignore) excludes toolchain output, temporary generation folders,
reports, logs, backups, and Creator's per-user preferences. In particular, it
does not exclude the generated `config.hex` required by the project.
No remote repository is configured automatically.
