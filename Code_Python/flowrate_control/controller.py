import subprocess
import tkinter as tk
from tkinter import messagebox
from pathlib import Path
from time import sleep
from mfc import MFC


def check_serial_port():
    """Attempt to automatically identify the serial port of the MFC
    (eg. /dev/ttyUSB0).
    Implementation based on https://unix.stackexchange.com/a/144735
    """
    for usb_path in Path('/sys/bus/usb/devices/').glob('usb*'):
        for dev_path in usb_path.rglob('dev'):
            dev_name = subprocess.run(
                ['udevadm', 'info', '-q', 'name', '-p', dev_path.parent],
                stdout=subprocess.PIPE).stdout.decode().strip()
            if dev_name.startswith('bus/'):
                # not actual devices -- controllers and hubs etc
                continue
            prop_out = subprocess.run(
                ['udevadm', 'info', '-q', 'property', '-p', dev_path.parent],
                stdout=subprocess.PIPE).stdout.decode().strip()
            if 'Voegtlin_Instruments' in prop_out:
                return f'/dev/{dev_name}'
    return 'None'


class AirflowControl(tk.Tk):
    """Minimal GUI for controlling air flow rate for ball suspension.
    """
    def __init__(self):
        super().__init__()
        self.mfc = None

        self.port_var = tk.StringVar()
        self.addr_var = tk.StringVar()
        self.curr_rate = tk.DoubleVar()
        self.tgt_rate = tk.DoubleVar()
        default_port = check_serial_port()
        if default_port is not None:
            self.port_var.set(default_port)
        self.addr_var.set('1')

        self.title('MFC Controller')
        w_main_frame = tk.Frame(self)
        w_main_frame.pack(padx=10, pady=5, fill=tk.X, expand=10)

        # Device setup frame
        w_setup_frame = tk.LabelFrame(w_main_frame, text='Device Setup')
        w_port_label = tk.Label(w_setup_frame, text='Serial Port')
        w_port_label.grid(row=0, column=0)
        w_port_entry = tk.Entry(w_setup_frame, width=20,
                                textvariable=self.port_var)
        w_port_entry.grid(row=0, column=1, columnspan=2)
        w_addr_label = tk.Label(w_setup_frame, text='Address')
        w_addr_label.grid(row=2, column=0)
        w_addr_entry = tk.Entry(w_setup_frame, width=10,
                                textvariable=self.addr_var)
        w_addr_entry.grid(row=2, column=1)
        w_setup_button = tk.Button(w_setup_frame, text='Connect',
                                   command=self.connect_mfc)
        w_setup_button.grid(row=2, column=2)
        w_setup_frame.pack(pady=5, fill=tk.X)
        
        # Flow rate control frame
        w_ctrl_frame = tk.LabelFrame(w_main_frame, text='Flow Rate Control')
        w_curr_rate_label = tk.Label(w_ctrl_frame, text='Current Rate')
        w_curr_rate_label.grid(row=0, column=0)
        w_curr_rate_val = tk.Label(w_ctrl_frame, textvariable=self.curr_rate)
        w_curr_rate_val.grid(row=0, column=1, columnspan=3)
        w_tgt_rate_label = tk.Label(w_ctrl_frame, text='Target Rate')
        w_tgt_rate_label.grid(row=1, column=0)
        w_tgt_rate_entry = tk.Entry(w_ctrl_frame, width=10, state='disabled',
                                    textvariable=self.tgt_rate)
        w_tgt_rate_entry.grid(row=1, column=1)
        w_set_rate_button = tk.Button(w_ctrl_frame, text='Set',
                                      command=self.set_rate, state='disabled')
        w_set_rate_button.grid(row=1, column=2)
        w_off_button = tk.Button(w_ctrl_frame, text='Off',
                                 command=self.turn_off, state='disabled')
        w_off_button.grid(row=1, column=3)
        w_ctrl_frame.pack(pady=5, fill=tk.X)

        self.widgets = {'tgt_rate': w_tgt_rate_entry,
                        'set_button': w_set_rate_button,
                        'off_button': w_off_button}

    
    def connect_mfc(self):
        port = str(self.port_var.get())
        try:
            addr = int(self.addr_var.get())
        except ValueError:
            messagebox.showerror(title='Invalid Input',
                                 message='Address must be an integer')
            return
        print(f'Connecting to MFC on port {port}, addr {addr}')
        try:
            self.mfc = MFC(port, addr)
            for w in self.widgets.values():
                w['state'] = 'normal'
            self.update_rate_event()
        except Exception as err:
            messagebox.showerror(title='MFC Connection Error',
                                 message=str(err))
    
    def set_rate(self):
        try:
            tgt_rate = float(self.tgt_rate.get())
        except ValueError:
            messagebox.showerror(title='Invalid Input',
                                 message='Flow rate must be a number')
            return
        try:
            print(f'Setting flow rate to {"%.2f" % tgt_rate}')
            self.mfc.set_rate(tgt_rate, settling_time=0)
        except Exception as err:
            messagebox.showerror(title='MFC Error', message=str(err))
    
    def turn_off(self):
        try:
            print(f'Setting flow rate to 0')
            self.mfc.set_rate(0, settling_time=0)
        except Exception as err:
            messagebox.showerror(title='MFC Error', message=str(err))

    def update_rate_event(self):
        try:
            self.curr_rate.set(self.mfc.get_rate())
            self.after(500, self.update_rate_event)
        except Exception as err:
            messagebox.showerror(title='MFC Error', message=str(err))

if __name__ == '__main__':
    app = AirflowControl()
    app.mainloop()
