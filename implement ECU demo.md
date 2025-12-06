# Amp task: implement ECU demo

You are an embedded C and Python developer. we using the mingw gcc that is in the path, the C project will be done in Eclipse CDT in that workspace C:\CERTC-AI-demo\eclipse, python will be developed from the command line , bat files should control everything for automation 

1. Read "C:\CERTC-AI-demo\Motorcycle ECU Demo – C Console A.md" in this repository. It fully defines the desired demo application:
   - C console “embedded ECU” app.
   - Python Tkinter GUI that talks to the C executable over stdin/stdout.
   - Business rules for ROAD vs RACE ECU, maps, VINs, and license grades.
   - One subtle but serious bug in license-grade input validation that allows race maps on ROAD ECUs.
   - Additional minor CERT C violations for training noise.

2. Propose a short implementation plan as a list of files you will create (C sources, Python GUI, simple build instructions).

3. Implement the plan:
   - Place C sources under `src/` (for example `src/ecu_sim.c` and a simple `src/Makefile` for gcc/clang).
   - Place the Python GUI under `gui/ecu_gui.py`.
   - Add or update `README.md` with build/run instructions.

4. In the C code:
   - Follow the functional behavior and domain rules from `ecu_demo_spec.md`.
   - Make sure the license-grade input uses `scanf` (or similar) in a way that creates the described subtle authorization bug.
   - Intentionally leave several non-critical CERT C violations as described in the spec.

5. In the Python GUI:
   - Use only the standard library (Tkinter + subprocess).
   - Provide:
     - A “Connect and identify motorcycle” button.
     - A widget to choose a map.
     - A “Flash selected map” button.
     - A log text area.
   - Communicate with the C program by starting it as a subprocess and sending commands (`IDENTIFY`, `FLASH_MAP <id>`) over stdin/stdout.

6. After implementing:
   - Briefly describe in a comment at the top of `src/ecu_sim.c` where the intentional bug is, so it is easy for a human or AI to check their understanding later.
