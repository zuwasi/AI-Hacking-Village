#!/usr/bin/env python3
"""
ECU Programming Tool - GUI Application
Communicates with the C ECU simulator via stdin/stdout
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
import subprocess
import threading
import queue
import os
import sys

class ECUProgrammerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Motorcycle ECU Programming Tool")
        self.root.geometry("900x650")
        
        # Configure default fonts
        default_font = ('Arial', 11)
        self.root.option_add('*Font', default_font)
        
        self.process = None
        self.current_vin = ""
        self.current_ecu_type = ""
        self.allowed_maps = []
        self.output_queue = queue.Queue()
        self.waiting_for_license = False
        self.current_flashed_map = None
        self.pending_map = None
        self.flash_state = False
        self.flash_timer = None
        
        self.setup_ui()
        
    def setup_ui(self):
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(3, weight=1)
        
        info_frame = ttk.LabelFrame(main_frame, text="Vehicle Information", padding="10")
        info_frame.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=5)
        
        self.vin_label = ttk.Label(info_frame, text="VIN: Not connected", font=('Arial', 12, 'bold'))
        self.vin_label.grid(row=0, column=0, sticky=tk.W, pady=2)
        
        self.ecu_label = ttk.Label(info_frame, text="ECU Type: Unknown", font=('Arial', 12, 'bold'))
        self.ecu_label.grid(row=1, column=0, sticky=tk.W, pady=2)
        
        self.connect_btn = ttk.Button(info_frame, text="Connect and Identify Motorcycle", 
                                       command=self.identify_motorcycle)
        self.connect_btn.grid(row=2, column=0, pady=5)
        
        map_frame = ttk.LabelFrame(main_frame, text="Map Selection", padding="10")
        map_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Label(map_frame, text="Select Map:").grid(row=0, column=0, sticky=tk.W)
        
        self.map_var = tk.StringVar()
        self.map_combo = ttk.Combobox(map_frame, textvariable=self.map_var, state="readonly")
        self.map_combo.grid(row=0, column=1, padx=5, sticky=(tk.W, tk.E))
        map_frame.columnconfigure(1, weight=1)
        
        self.flash_btn = ttk.Button(map_frame, text="Flash Selected Map", 
                                     command=self.flash_map, state="disabled")
        self.flash_btn.grid(row=1, column=0, columnspan=2, pady=5)
        
        self.warning_label = ttk.Label(map_frame, text="", foreground="red", font=("Arial", 12, "bold"))
        self.warning_label.grid(row=2, column=0, columnspan=2, pady=5)
        
        verify_frame = ttk.LabelFrame(main_frame, text="Verification", padding="10")
        verify_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=5)
        
        self.verify_btn = ttk.Button(verify_frame, text="Verify Flash Process", 
                                      command=self.verify_flash, state="disabled")
        self.verify_btn.grid(row=0, column=0, pady=5)
        
        self.verify_result_label = ttk.Label(verify_frame, text="", font=("Arial", 11))
        self.verify_result_label.grid(row=1, column=0, pady=2)
        
        log_frame = ttk.LabelFrame(main_frame, text="Log", padding="10")
        log_frame.grid(row=3, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)
        
        self.log_text = tk.Text(log_frame, height=15, wrap=tk.WORD, font=('Consolas', 10))
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        scrollbar = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.log_text.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.log_text.configure(yscrollcommand=scrollbar.set)
        
        self.start_ecu_process()
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        
    def log(self, message):
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.see(tk.END)
        
    def start_ecu_process(self):
        ecu_exe = os.path.join(os.path.dirname(os.path.dirname(__file__)), "src", "ecu_sim.exe")
        
        if not os.path.exists(ecu_exe):
            self.log(f"ERROR: ECU simulator not found at {ecu_exe}")
            self.log("Please build the C application first using build.bat")
            return
        
        try:
            self.process = subprocess.Popen(
                [ecu_exe],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            
            self.reader_thread = threading.Thread(target=self.read_output, daemon=True)
            self.reader_thread.start()
            
            self.log("ECU simulator started successfully")
            
        except Exception as e:
            self.log(f"ERROR: Failed to start ECU simulator: {e}")
            
    def read_output(self):
        while self.process and self.process.poll() is None:
            line = self.process.stdout.readline()
            if line:
                line = line.strip()
                if line:
                    self.output_queue.put(line)
                    self.root.after(10, self.process_output)
                    
    def process_output(self):
        try:
            while True:
                line = self.output_queue.get_nowait()
                
                if line == "ECU_SIM_READY":
                    self.log("ECU Simulator Ready")
                    
                elif line.startswith("VIN:"):
                    self.parse_identify_response(line)
                    
                elif line.startswith("PROMPT:"):
                    prompt_text = line.replace("PROMPT:", "").strip()
                    self.handle_license_prompt(prompt_text)
                    
                elif line.startswith("SUCCESS:"):
                    self.log(line)
                    if self.pending_map:
                        self.current_flashed_map = self.pending_map
                        self.pending_map = None
                    self.warning_label.config(text="")
                    
                elif line.startswith("ERROR:"):
                    self.log(line)
                    self.pending_map = None
                    self.warning_label.config(text="")
                    
                elif line.startswith("WARNING:"):
                    self.log(line)
                    
                else:
                    self.log(line)
                    
        except queue.Empty:
            pass
            
    def parse_identify_response(self, response):
        parts = response.split(", ")
        
        vin_part = parts[0].split(": ")[1]
        self.current_vin = vin_part
        
        ecu_part = parts[1].split(": ")[1]
        self.current_ecu_type = ecu_part
        
        maps_part = parts[2].split(": ")[1]
        self.allowed_maps = maps_part.split(",")
        
        self.vin_label.config(text=f"VIN: {self.current_vin}")
        
        # Set ECU type with color coding
        self.ecu_label.config(text=f"ECU Type: {self.current_ecu_type}")
        
        # Stop any previous flashing
        if self.flash_timer:
            self.root.after_cancel(self.flash_timer)
            self.flash_timer = None
        
        if self.current_ecu_type == "RACE":
            # Start flashing red for RACE ECU
            self.start_flashing()
        else:
            # Solid green for ROAD ECU
            self.ecu_label.config(foreground="green")
        
        all_maps = [
            "R_A (Road A - basic, 54HP)",
            "R_B (Road B - intermediate, 54HP)",
            "R_C (Road C - advanced, 54HP)",
            "X_A (Race A, 65HP)",
            "X_B (Race B, 77HP)",
            "X_C (Race C, 100HP)"
        ]
        
        self.map_combo['values'] = all_maps
        if all_maps:
            self.map_combo.current(0)
        
        self.flash_btn.config(state="normal")
        self.verify_btn.config(state="normal")
        self.current_flashed_map = None
        self.verify_result_label.config(text="", foreground="black")
        self.log(f"Identified: VIN={self.current_vin}, ECU={self.current_ecu_type}, Allowed={','.join(self.allowed_maps)}")
        
    def identify_motorcycle(self):
        if self.process and self.process.poll() is None:
            self.send_command("IDENTIFY")
        else:
            self.log("ERROR: ECU simulator not running")
            
    def flash_map(self):
        if not self.current_vin:
            messagebox.showerror("Error", "Please identify a motorcycle first")
            return
            
        selected = self.map_var.get()
        if not selected:
            messagebox.showerror("Error", "Please select a map")
            return
        
        map_id = selected.split(" ")[0]
        
        # Check if trying to flash illegal map to ROAD ECU
        if self.current_ecu_type == "ROAD" and map_id.startswith("X_"):
            self.warning_label.config(
                text="⚠️ WARNING: You are trying to flash an ILLEGAL RACE MAP to a ROAD ECU! ⚠️",
                foreground="red"
            )
            self.log("⚠️ REGULATORY WARNING: Attempting to flash illegal race map to ROAD ECU")
        else:
            self.warning_label.config(text="", foreground="black")
        
        self.pending_map = map_id
        self.log(f"Initiating flash for map: {map_id}")
        self.send_command(f"FLASH_MAP {map_id}")
        
    def handle_license_prompt(self, prompt_text):
        self.root.after(100, lambda: self.show_license_dialog(prompt_text))
        
    def show_license_dialog(self, prompt_text):
        vin_input = simpledialog.askstring("VIN Verification Required", 
                                           prompt_text + f"\n\nCurrent VIN: {self.current_vin}",
                                           parent=self.root)
        
        if vin_input is not None:
            self.send_command(vin_input)
        else:
            self.log("Flash operation cancelled by user")
            
    def send_command(self, command):
        if self.process and self.process.poll() is None:
            try:
                self.process.stdin.write(command + "\n")
                self.process.stdin.flush()
            except Exception as e:
                self.log(f"ERROR: Failed to send command: {e}")
        else:
            self.log("ERROR: ECU simulator not running")
            
    def verify_flash(self):
        if not self.current_vin:
            messagebox.showwarning("Verification", "No vehicle identified. Please connect first.")
            return
        
        if not self.current_flashed_map:
            self.verify_result_label.config(text="ℹ️ No map has been flashed yet.", foreground="blue")
            messagebox.showinfo("Verification", "No map has been flashed to this ECU yet.")
            return
        
        is_race_map = self.current_flashed_map.startswith("X_")
        is_road_ecu = self.current_ecu_type == "ROAD"
        
        if is_road_ecu and is_race_map:
            self.verify_result_label.config(
                text=f"❌ VIOLATION DETECTED: ROAD ECU has RACE map ({self.current_flashed_map})!",
                foreground="red"
            )
            
            result = messagebox.askquestion(
                "⚠️ REGULATORY VIOLATION DETECTED",
                f"CRITICAL SAFETY VIOLATION:\n\n"
                f"ECU Type: ROAD (Max 54 HP legal limit)\n"
                f"Flashed Map: {self.current_flashed_map} (RACE map, >54 HP)\n\n"
                f"This configuration violates street-legal regulations!\n"
                f"The motorcycle may produce illegal horsepower levels.\n\n"
                f"Do you want to IGNORE this violation and continue?",
                icon="warning"
            )
            
            if result == "yes":
                self.log("⚠️ USER CHOSE TO IGNORE REGULATORY VIOLATION")
                messagebox.showwarning(
                    "Violation Acknowledged",
                    "You have acknowledged the regulatory violation.\n"
                    "This vehicle is no longer street-legal."
                )
            else:
                self.log("User acknowledged violation - suggested corrective action")
                messagebox.showinfo(
                    "Corrective Action Recommended",
                    "Please flash an appropriate ROAD map (R_A, R_B, or R_C)\n"
                    "to restore street-legal compliance."
                )
        else:
            self.verify_result_label.config(
                text=f"✓ Configuration valid: {self.current_ecu_type} ECU with {self.current_flashed_map} map",
                foreground="green"
            )
            messagebox.showinfo(
                "Verification Passed",
                f"ECU Type: {self.current_ecu_type}\n"
                f"Flashed Map: {self.current_flashed_map}\n\n"
                f"✓ Configuration is compliant."
            )
    
    def start_flashing(self):
        """Start flashing the ECU label red for RACE ECUs"""
        self.flash_state = not self.flash_state
        if self.flash_state:
            self.ecu_label.config(foreground="red")
        else:
            self.ecu_label.config(foreground="dark red")
        
        # Schedule next flash toggle (500ms interval)
        self.flash_timer = self.root.after(500, self.start_flashing)
    
    def on_closing(self):
        # Stop flashing timer if running
        if self.flash_timer:
            self.root.after_cancel(self.flash_timer)
            
        if self.process and self.process.poll() is None:
            try:
                self.send_command("QUIT")
                self.process.wait(timeout=2)
            except:
                self.log("Force terminating ECU process...")
                self.process.terminate()
                try:
                    self.process.wait(timeout=1)
                except:
                    self.process.kill()
        self.root.destroy()

def main():
    root = tk.Tk()
    app = ECUProgrammerGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
