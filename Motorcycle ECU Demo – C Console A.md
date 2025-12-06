# Motorcycle ECU Demo – C Console App + Python GUI

## 1. Goal

Create a small training/demo project that simulates a motorcycle ECU configuration tool.

The project must:

1. Consist of:
   - A C console application that behaves like an embedded ECU programming interface.
   - A simple Python GUI application that talks to the C executable (simulating a diagnostic/programming tool).

2. Implement realistic business rules about allowed horsepower (HP) for ROAD vs RACE motorcycles.

3. Contain **one important, subtle vulnerability**:
   - Due to incorrect validation of user input from `scanf` (or similar C input function), it becomes possible to flash illegal high-HP race maps into a ROAD ECU.
   - This bug should be logically plausible, not an obvious “if (cheat_code)” backdoor.

4. Contain several additional, minor CERT C rule violations for training purposes, without changing the main behavior of the application.

This project will be used to demonstrate how an agentic AI (Sourcegraph Amp) can:

- Read the C code, the static analysis report, and this specification.
- Identify which CERT C violations are actually critical for safety/regulatory compliance.
- Explain why the input-validation bug is more important than other reported violations.
- Optionally propose a patch that fixes the critical bug.

## 2. Domain model and business rules

### 2.1 ECU types and VINs

There are two ECU variants:

1. ROAD ECU:
   - Legal constraint: all flashable maps must be **≤ 54 HP**.
   - Intended for street-legal motorcycles.

2. RACE ECU:
   - No power limitation for track use.

The demo should use a small, fixed list of VINs (Vehicle Identification Numbers).

Example:

- `123456` → ROAD ECU, allowed maps A/B/C (see below).
- `234567` → ROAD ECU, allowed maps A/B only.
- `345678` → RACE ECU, allowed maps A/B/C.

The exact values and mappings can be hard-coded in the C program.

When the GUI clicks “Connect and identify motorcycle”, the C app should:

1. Pick one VIN deterministically or randomly from the list.
2. Return:
   - The VIN.
   - The ECU type (ROAD or RACE).
   - The list of maps nominally allowed for this VIN.

### 2.2 Ignition maps and horsepower

Define six ignition maps:

ROAD maps (legal on ROAD and RACE):
- Map `R_A`: “Road A – basic”, ≤ 54 HP.
- Map `R_B`: “Road B – intermediate”, ≤ 54 HP.
- Map `R_C`: “Road C – advanced”, ≤ 54 HP.

RACE maps (illegal on ROAD, legal on RACE):
- Map `X_A`: “Race A – 65 HP”.
- Map `X_B`: “Race B – 77 HP”.
- Map `X_C`: “Race C – 100 HP”.

For ROAD ECUs:
- Only R_* maps are legal.
- Any X_* map is a regulatory violation.

For RACE ECUs:
- Any map (R_* or X_*) is allowed.

### 2.3 Technician license grades

There are three license grades:

- Grade 1 (basic).
- Grade 2 (intermediate).
- Grade 3 (advanced).

The mapping between VIN and minimum license grade can be simple:

- Basic ROAD VIN: min grade 1.
- Sportier ROAD VIN: min grade 2.
- RACE VIN: min grade 3.

The exact mapping is less important than the rule that license grade must be validated correctly.

## 3. C console application requirements

### 3.1 General structure

Implement the C app as a single console program, for example `ecu_sim.c`, built as a standard executable.

The C app should:

1. Start, initialize its internal VIN list and map information.
2. Wait for simple commands from stdin (one command per line).
3. Print responses to stdout.

Commands can be plain text, for example:

- `IDENTIFY`
- `GET_ALLOWED_MAPS`
- `FLASH_MAP <map_id>`
- `QUIT`

It is acceptable to keep the protocol very simple because the focus is on the logic and CERT C rules.

### 3.2 Identification flow

When the C app receives `IDENTIFY`:

1. Select a VIN from the fixed list.
   - For demo purposes, random selection is preferred, but deterministic selection is acceptable.
2. Determine ECU type (ROAD or RACE) and store it internally.
3. Print a response line, for example:
   - `VIN: 123456, ECU: ROAD, allowed maps: R_A,R_B,R_C`

### 3.3 Map flashing flow (including the bug)

When the C app receives `FLASH_MAP <map_id>`:

1. If there is no current VIN/ECU identified, print an error.
2. Ask the user on stdin to re-enter their technician license grade, as an integer.
   - Example prompt: `Enter technician license grade (1=basic, 2=intermediate, 3=advanced):`

3. Read this value using a C input function that parses integers, such as:
   - `scanf("%d", &license_grade);`

4. The code must implement the following **intended** logic:

   a. Validate the result of the input operation and the range:
      - Conversion must succeed.
      - `license_grade` must be in [1, 3].
   b. If conversion fails or value is out of range, the flash operation must be rejected.
   c. For ROAD ECUs:
      - Only R_* maps are allowed.
      - License grade must meet or exceed the VIN’s minimum license requirement.
   d. For RACE ECUs:
      - R_* and X_* maps are allowed if license grade is high enough.

5. However, the implementation must contain a **subtle vulnerability**:

   - The return value of `scanf` (or equivalent) is ignored.
   - `license_grade` is not initialized before it is passed to `scanf`.
   - If the user enters malformed input (for example “2.5”, “A3”, or an empty line), `scanf("%d", &license_grade)` returns 0 and does not modify `license_grade`.
   - The subsequent authorization logic uses the stale or uninitialized `license_grade` value.

   One acceptable pattern for the bug:

   - There is a global variable `int license_grade = 3;` that is intended only for some internal test mode.
   - Before reading from stdin, the code does not reset `license_grade`.
   - If `scanf` fails, `license_grade` remains 3 and the code interprets it as “highest license”.
   - The authorization logic then allows X_* maps even for ROAD ECUs.

   This must create a situation where:

   - For a ROAD ECU VIN, entering malformed input for the license check causes the program to accept and flash a race map (e.g. `X_B` or `X_C`).
   - The program prints a warning or note, but still proceeds with the flash.

   The bug should naturally trigger a set of CERT C violations, such as:
   - Ignoring return value from `scanf` (ERR33/ERR34 style).
   - Using an uninitialized or stale variable (EXP33).
   - Insufficient validation of values passed to library functions (MEM04/MEM07).
   Exact rule IDs are not strictly required, but the code should be written in a way that a static analyzer configured for CERT C will report multiple findings around the license input and authorization logic.

### 3.4 Additional minor CERT C violations (noise)

Intentionally introduce several **non-critical** CERT C violations to serve as noise in the static analysis report. Examples:

- Use `strcpy` for copying constant strings into fixed-size buffers that are large enough.
- Omit `const` on parameters that are not modified.
- Ignore the return value of `printf` in places where failure is not meaningful for this demo.
- Have a couple of global variables that could be local.

These should not affect the main functional behavior and should not create additional serious vulnerabilities beyond the license check bug described above.

## 4. Python GUI requirements

Implement a small Python GUI application, for example `ecu_gui.py`.

Constraints:

1. Use only the Python standard library if possible.
   - Tkinter is preferred for the GUI so it can run without external dependencies.

2. The GUI should have:
   - A button: “Connect and identify motorcycle”.
   - A list or dropdown showing available maps for the identified VIN.
   - A button: “Flash selected map”.
   - A text area or log view that displays messages from the C app.

3. Communication method:

   Any simple IPC mechanism is acceptable, for example:

   - Start the C executable as a subprocess using `subprocess.Popen`, and exchange commands via stdin/stdout.
   - The GUI sends text commands (`IDENTIFY`, `FLASH_MAP <map_id>`) to the C process and reads its responses.

4. Interaction flow:

   a. When the user clicks “Connect and identify motorcycle”:
      - Start the C process (if not already running).
      - Send `IDENTIFY`.
      - Parse the response to display VIN, ECU type, and allowed maps.

   b. When the user selects a map and clicks “Flash selected map”:
      - Send the `FLASH_MAP` command with the chosen map id.
      - When the C app prompts for technician license grade, pop up a dialog box to ask the user to type the grade.
      - Relay the user’s input to the C app.
      - Display the result (success, warning, or error) in the log.

5. For demo purposes, it is acceptable for the GUI to be single-threaded and to block while waiting for responses.

## 5. Repository layout

The resulting project should have a simple layout, for example:

- `ecu_demo_spec.md` (this file).
- `src/ecu_sim.c` (C console app).
- `src/Makefile` or a simple build script for the C app.
- `gui/ecu_gui.py` (Python GUI).
- `README.md` explaining how to build and run the demo.

## 6. Intended use with static analysis and Amp

This project will be compiled and analyzed with a static analysis tool configured for CERT C.

Later, a Sourcegraph Amp agent will be instructed to:

1. Read:
   - The C source files.
   - The static analysis report (XML or HTML).
   - This `ecu_demo_spec.md` and `README.md`.

2. Identify:
   - Which CERT C violations represent a real safety/regulatory risk for ROAD ECUs.
   - Specifically, the input-validation bug in the technician license check that allows RACE maps (65/77/100 HP) on ROAD VINs.

3. Explain:
   - Why that bug is more critical than other CERT C violations.
   - Which code paths and input sequences trigger the bug.

4. Optionally, propose:
   - A patch that correctly validates the result of `scanf` and the license grade range.
   - A small set of tests or usage scenarios that confirm the fix.

The code should be written clearly enough that an AI agent can understand the control flow and data flow, but not so trivial that the vulnerability is obvious at first glance.
