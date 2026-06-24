import tkinter as tk
from tkinter import scrolledtext
import serial
import struct
import time

PORT = '/dev/cu.usbserial-10'
BAUDRATE = 115200

#Host Commands
CMD_GET_ACCEL = 0x01
CMD_SET_LED   = 0x02
CMD_LOG_START = 0x10
CMD_LOG_STOP  = 0x11
CMD_LOG_DUMP  = 0x12

#STM32 Response Commands
CMD_RESP_ACCEL     = 0x81
CMD_RESP_LOG_ENTRY = 0x82
CMD_RESP_ACK       = 0xFF
CMD_RESP_ERROR     = 0xFE

class STM32ControllerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("STM32 Data Logger & Control Panel")
        self.root.geometry("650x550")
        
        self.root.grid_rowconfigure(1, weight=1)
        self.root.grid_columnconfigure(0, weight=1)

        self.setup_ui()

        try:
            self.ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
            self.log_message(f"[SYSTEM] Connected to {PORT} successfully.\n")
        except serial.SerialException as e:
            self.ser = None
            self.log_message(f"[ERROR] Failed to connect to {PORT}.\nDetails: {e}\n")

    def setup_ui(self):
        #Control Panel Frame
        control_frame = tk.LabelFrame(self.root, text=" Device Controls ", padx=10, pady=10)
        control_frame.grid(row=0, column=0, padx=15, pady=10, sticky="ew")

        #Hardware Control
        btn_led_on = tk.Button(control_frame, text="LED ON", width=12, bg="#4CAF50", fg="black",
                               command=lambda: self.send_command(CMD_SET_LED, [0x01]))
        btn_led_on.grid(row=0, column=0, padx=5, pady=5)

        btn_led_off = tk.Button(control_frame, text="LED OFF", width=12, bg="#F44336", fg="black",
                                command=lambda: self.send_command(CMD_SET_LED, [0x00]))
        btn_led_off.grid(row=0, column=1, padx=5, pady=5)

        btn_accel = tk.Button(control_frame, text="Get Accel Data", width=15, bg="#2196F3", fg="black",
                              command=lambda: self.send_command(CMD_GET_ACCEL, []))
        btn_accel.grid(row=0, column=2, padx=5, pady=5)

        #Flash Logger Control
        btn_log_start = tk.Button(control_frame, text="Log Start", width=12, bg="#FF9800", fg="black",
                                  command=lambda: self.send_command(CMD_LOG_START, []))
        btn_log_start.grid(row=1, column=0, padx=5, pady=5)

        btn_log_stop = tk.Button(control_frame, text="Log Stop", width=12, bg="#9E9E9E", fg="black",
                                 command=lambda: self.send_command(CMD_LOG_STOP, []))
        btn_log_stop.grid(row=1, column=1, padx=5, pady=5)

        btn_log_dump = tk.Button(control_frame, text="Dump Flash", width=15, bg="#9C27B0", fg="black",
                                 command=lambda: self.send_command(CMD_LOG_DUMP, []))
        btn_log_dump.grid(row=1, column=2, padx=5, pady=5)

        #Console Output Frame
        console_frame = tk.LabelFrame(self.root, text=" System Log ", padx=10, pady=10)
        console_frame.grid(row=1, column=0, padx=15, pady=5, sticky="nsew")

        self.log_area = scrolledtext.ScrolledText(console_frame, wrap=tk.WORD, font=("Courier", 12), state='disabled', bg="#1E1E1E", fg="#00FF00")
        self.log_area.pack(fill=tk.BOTH, expand=True)

    def log_message(self, message):
        self.log_area.config(state='normal')
        self.log_area.insert(tk.END, message + "\n")
        self.log_area.see(tk.END)
        self.log_area.config(state='disabled')
        self.root.update()

    def calculate_checksum(self, cmd, length, data):
        cs = cmd ^ length
        for b in data:
            cs ^= b
        return cs

    def send_command(self, cmd, data):
        if self.ser is None or not self.ser.is_open:
            self.log_message("[ERROR] Serial port is closed.")
            return

        length = len(data)
        checksum = self.calculate_checksum(cmd, length, data)
        packet = bytearray([0xAA, cmd, length] + data + [checksum])
        
        cmd_names = {0x01: "GET_ACCEL", 0x02: "SET_LED", 0x10: "LOG_START", 0x11: "LOG_STOP", 0x12: "LOG_DUMP"}
        cmd_str = cmd_names.get(cmd, f"0x{cmd:02X}")
        self.log_message(f"\n[TX] Command sent: {cmd_str}")
        
        self.ser.write(packet)
        
        time.sleep(0.5 if cmd == CMD_LOG_DUMP else 0.1)
        
        if self.ser.in_waiting > 0:
            buffer = self.ser.read(self.ser.in_waiting)
            self.parse_buffer(buffer)
        else:
            self.log_message("[TIMEOUT] No response from device.")

    def parse_buffer(self, buffer):
        """Parse multiple packets from the continuous byte stream"""
        idx = 0
        while idx < len(buffer) - 3:
            if buffer[idx] == 0xAA:
                cmd = buffer[idx+1]
                length = buffer[idx+2]
                packet_total_length = 4 + length
                
                if idx + packet_total_length <= len(buffer):
                    data = buffer[idx+3 : idx+3+length]
                    self.process_packet(cmd, data)
                    idx += packet_total_length
                else:
                    break
            else:
                idx += 1

    def process_packet(self, cmd, data):
        """Execute logic based on parsed packet CMD"""
        if cmd == CMD_RESP_ACK:
            if len(data) == 1:
                self.log_message(f"  └── [ACK] Action for command 0x{data[0]:02X} completed.")
            else:
                self.log_message("  └── [ACK] Action completed successfully.")
                
        elif cmd == CMD_RESP_ERROR:
            self.log_message(f"  └── [ERROR] Device reported error code: 0x{data[0]:02X}")
            
        elif cmd == CMD_RESP_ACCEL:
            if len(data) == 6:
                ax, ay, az = struct.unpack('>hhh', data)
                self.log_message(f"  └── [DATA] Accel: X={ax:5d}, Y={ay:5d}, Z={az:5d}")
                
        elif cmd == CMD_RESP_LOG_ENTRY:
            if len(data) == 10:
                ts, ax, ay, az = struct.unpack('>Ihhh', data)
                self.log_message(f"  └── [RECORD] T:{ts}ms | X={ax:5d}, Y={ay:5d}, Z={az:5d}")

    def on_closing(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = STM32ControllerApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()